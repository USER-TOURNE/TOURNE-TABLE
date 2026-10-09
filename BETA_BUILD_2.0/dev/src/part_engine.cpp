// ---- Audio engine: capture, analysis and what the renderer reads -----------------
//
// 1.5 ran two threads to get audio onto the screen. A capture thread woke on
// every WASAPI event, about 100 times a second while anything played, and a
// pacing thread woke once per frame to post the render tick. Every wake is a
// core leaving a sleep state, which is the cost this mod has always been built
// around, so 2.0 merges them: the pacing thread also owns the loopback client,
// and once per frame it drains whatever audio has arrived, analyses it, and
// posts the tick. Audio and video share one wake.
//
// That only works because loopback is drained by polling rather than by the
// event, so the client is opened with a 500 ms buffer (1.5 asked for 20 ms,
// which was fine for an event every 10 ms and is not for a frame every 33 ms
// at a 30 FPS target). The event is still registered; it is what the thread
// waits on while idle, because WASAPI only signals it when audio arrives.
//
// Idle has three steps now:
//   playing       one wake per frame, as above.
//   trickle       Pause When Silent reached. Loopback: the audio event wakes
//                 the thread, which only drains the packet unless it is
//                 louder than the audible level (then it analyses and draws
//                 at once, so waking up costs no latency); otherwise one
//                 analysis + render tick every 250 ms. An app holding a silent
//                 stream open signals the event 100 times a second, and those
//                 wakes now cost a drain each instead of a full frame. An
//                 input device signals every device period whatever it hears,
//                 so for one the thread just polls every 250 ms.
//   deep idle     5 s after that, loopback only and only with a working peak
//                 meter: the stream is stopped and the endpoint's peak meter
//                 is read 4 times a second instead. A running capture stream
//                 registers an audio power request, which can hold the PC out
//                 of sleep; a stopped one doesn't. A meter reading above the
//                 audible level (the same -70 dBFS the engine uses, Input Gain
//                 included) only restarts the stream and goes back to trickle
//                 for a 1 s look; the engine's own test then decides whether
//                 it is playing. An input device stays in trickle: once its
//                 stream is stopped, its peak meter can read 0 for good.

enum class VizIdleState { Playing, Trickle, Deep };

// The one "is anything playing" level, -70 dBFS on the mono mix after Input
// Gain. The engine needs this and a bar above 2% to call audio audible; deep
// idle wakes on the endpoint meter crossing it. The meter reads the loudest
// channel before Input Gain, and |mono mix| <= the loudest channel, so meter
// x gain is never below what the engine sees: nothing the engine would call
// audible can sleep through deep idle.
constexpr float kVizAudibleLin = 0.000316f;

// IAudioMeterInformation (endpointvolume.h), declared here because the mingw
// headers only forward-declare it. Methods in vtable order, from the Windows
// SDK; only GetPeakValue is called.
struct IVizAudioMeter : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE GetPeakValue(float* pfPeak) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetMeteringChannelCount(UINT* pnChannelCount) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetChannelsPeakValues(UINT32 u32ChannelCount, float* afPeakValues) = 0;
    virtual HRESULT STDMETHODCALLTYPE QueryHardwareSupport(DWORD* pdwHardwareSupportMask) = 0;
};
static const IID kIID_IAudioMeterInformation = {
    0xC02216F6, 0x8C67, 0x4B5B, {0x9D, 0x00, 0xD0, 0x08, 0xE7, 0x3E, 0x00, 0x64}};

// Precision bands, written by the engine thread once per frame, read by the
// UI thread once per drawn frame. Same seqlock pattern as the 7 classic bands.
struct VizBandFrame {
    int count = 0;
    float level[VIZ_BARS_MAX] = {};  // display height 0..1, after ballistics
    float zone[3] = {};              // summed level per EQ zone (low / mid / high)
    float zoneCount[3] = {};         // bands in each zone, for averages (Terminal meters)
};
std::atomic<uint32_t> g_bandFrameSeq{0};
VizBandFrame g_bandFrame;

void PublishBandFrame(const VizBandFrame& f) {
    uint32_t seq = g_bandFrameSeq.load(std::memory_order_relaxed);
    g_bandFrameSeq.store(seq + 1, std::memory_order_release);
    std::atomic_thread_fence(std::memory_order_release);
    g_bandFrame.count = f.count;
    memcpy(g_bandFrame.level, f.level, sizeof(float) * (size_t)std::clamp(f.count, 0, VIZ_BARS_MAX));
    memcpy(g_bandFrame.zone, f.zone, sizeof(f.zone));
    memcpy(g_bandFrame.zoneCount, f.zoneCount, sizeof(f.zoneCount));
    g_bandFrameSeq.store(seq + 2, std::memory_order_release);
}

void ReadBandFrame(VizBandFrame& dst) {
    for (;;) {
        uint32_t seq1 = g_bandFrameSeq.load(std::memory_order_acquire);
        if (seq1 & 1) continue;
        dst.count = std::clamp(g_bandFrame.count, 0, VIZ_BARS_MAX);
        memcpy(dst.level, g_bandFrame.level, sizeof(float) * (size_t)dst.count);
        memcpy(dst.zone, g_bandFrame.zone, sizeof(dst.zone));
        memcpy(dst.zoneCount, g_bandFrame.zoneCount, sizeof(dst.zoneCount));
        std::atomic_thread_fence(std::memory_order_acquire);
        if (seq1 == g_bandFrameSeq.load(std::memory_order_relaxed)) return;
    }
}

// What the engine thread needs from the settings, copied as one value on the
// UI thread (LoadSettings) so the engine never reads half of a settings change.
struct VizEngineConfig {
    bool precision = true;
    VizWorkload workload = VizWorkload::Hybrid;
    ttdsp::SpectrumEngine::Config spec;
    ttdsp::Ballistics ball;
    ttdsp::DisplayMap disp;
    float sensDb = 0.f;
    bool autoGain = false;
    float autoGainMaxDb = 12.f;
    VizChannel channel = VizChannel::Mix;
    bool wantDominant = false;
    bool wantLoudness = false;
    bool wantTruePeak = false;     // Loudness (full) readout: the 4x oversampler
    bool wantCorrelation = false;  // Loudness (full) readout or the Goniometer
    bool wantGonio = false;
    bool beat = false;
    bool loudnessResetOnTrack = true;
};
std::mutex g_engineCfgMutex;
VizEngineConfig g_engineCfg;
std::atomic<int> g_engineCfgGen{1};

// GPU workload feed: the band layout (changes rarely) and the newest
// unwindowed block of each tier (changes every frame). The UI thread uploads
// both; the GPU does the rest.
struct VizGpuFeed {
    std::mutex m;
    int layoutSerial = 0;
    int n = 0;
    double rate[3] = {48000, 12000, 3000};
    bool used[3] = {true, false, false};
    double peakCal = 0.0;
    std::vector<ttdsp::SpectrumEngine::BandMap> maps;
    std::vector<ttdsp::Band> bands;
    std::vector<float> window;
    std::vector<float> blocks;  // 3 x n
    unsigned dirty = 0;         // tiers with a new block the UI hasn't taken
} g_gpuFeed;

// Loudness, true peak and correlation, for the readout.
struct VizMeterValues {
    double momentary = -HUGE_VAL, shortTerm = -HUGE_VAL, integrated = -HUGE_VAL;
    double truePeak = -HUGE_VAL, correlation = 0.0;
    bool valid = false;
};
std::mutex g_meterMutex;
VizMeterValues g_meters;

// Goniometer: the newest stereo samples as (side, mid) pairs.
std::mutex g_gonioMutex;
std::vector<float> g_gonioXY;
std::atomic<uint32_t> g_gonioSerial{0};

// Engine thread control.
std::atomic<bool> g_captureWanted{false};
std::atomic<bool> g_audioOpen{false};
HANDLE g_engineWake = nullptr;    // auto-reset: settings, pause, resume, unload
HANDLE g_engineClosed = nullptr;  // manual-reset: set while the stream is closed
std::atomic<int> g_idleState{(int)VizIdleState::Playing};

// Opens the chosen audio source (see VizResolveAudioDevice): an output
// device by loopback, as 1.5 always did with the default one, or an input
// device as a plain recording stream. Same buffer and outputs as before: the
// channel mask for loudness weighting, and the device itself for the
// deep-idle peak meter. *fellBack says the chosen device wasn't there, or was
// there but couldn't be opened (*openFailed: typically a DAW holding it in
// exclusive mode); either way the default output stands in for it.
static bool VizOpenAudioClientOn(IMMDevice* pDev, bool loopback, ComPtr<IAudioClient>& pClient,
                                 ComPtr<IAudioCaptureClient>& pCapture, UINT32& sampleRate, UINT32& channels,
                                 bool& isFloat, DWORD& channelMask, HANDLE hEvent) {
    ComPtr<IAudioClient> pC;
    if (FAILED(pDev->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)pC.GetAddressOf())))
        return false;

    WAVEFORMATEX* pwfx = nullptr;
    pC->GetMixFormat(&pwfx);
    if (!pwfx) return false;

    UINT32 sr = pwfx->nSamplesPerSec, ch = pwfx->nChannels;
    bool fl = (pwfx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) ||
              (pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
               reinterpret_cast<WAVEFORMATEXTENSIBLE*>(pwfx)->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT);
    DWORD mask = (pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
                     ? reinterpret_cast<WAVEFORMATEXTENSIBLE*>(pwfx)->dwChannelMask
                     : 0;

    // 500 ms, in 100 ns units.
    HRESULT hr = pC->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                (loopback ? AUDCLNT_STREAMFLAGS_LOOPBACK : 0u) | AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
                                5000000, 0, pwfx, nullptr);
    CoTaskMemFree(pwfx);
    if (FAILED(hr)) return false;

    if (hEvent) pC->SetEventHandle(hEvent);

    ComPtr<IAudioCaptureClient> pCap;
    if (FAILED(pC->GetService(__uuidof(IAudioCaptureClient), (void**)pCap.GetAddressOf()))) return false;

    if (FAILED(pC->Start())) return false;

    sampleRate = sr;
    channels = ch;
    isFloat = fl;
    channelMask = mask;
    pClient = pC;
    pCapture = pCap;
    return true;
}

bool VizInitAudioClient(IMMDeviceEnumerator* pEnum, ComPtr<IMMDevice>& pDevOut,
                        ComPtr<IAudioClient>& pClient, ComPtr<IAudioCaptureClient>& pCapture,
                        UINT32& sampleRate, UINT32& channels, bool& isFloat, DWORD& channelMask,
                        HANDLE hEvent, bool* loopbackOut, bool* fellBack, bool* openFailed) {
    pClient.Reset();
    pCapture.Reset();
    pDevOut.Reset();
    *openFailed = false;

    bool loopback = true;
    std::wstring key = VizAudioSourceKey();
    ComPtr<IMMDevice> pDev = VizResolveAudioDevice(pEnum, key, &loopback, fellBack);
    if (!pDev) return false;
    if (VizOpenAudioClientOn(pDev.Get(), loopback, pClient, pCapture, sampleRate, channels, isFloat, channelMask,
                             hEvent)) {
        *loopbackOut = loopback;
        pDevOut = pDev;
        return true;
    }
    // The chosen device is there but won't open. This used to retry the
    // same device every 500 ms for as long as it stayed busy, showing
    // nothing; now the default output stands in (once per attempt), and the
    // caller says so and checks back on the chosen one now and then.
    if (*fellBack || key.empty() || key == L"default_output") return false;
    ComPtr<IMMDevice> def;
    if (FAILED(pEnum->GetDefaultAudioEndpoint(eRender, eConsole, &def)) || !def) return false;
    if (!VizOpenAudioClientOn(def.Get(), true, pClient, pCapture, sampleRate, channels, isFloat, channelMask,
                              hEvent))
        return false;
    Wh_Log(L"[Audio] %s would not open; using the default output instead", VizDeviceName(pDev.Get()).c_str());
    *fellBack = true;
    *openFailed = true;
    *loopbackOut = true;
    pDevOut = def;
    return true;
}

// Whether the chosen source can be opened now, without disturbing the stream
// that is standing in for it: a shared-mode Initialize on a client that is
// released straight away (it is never started, so nothing is captured).
static bool VizProbeAudioSource(IMMDeviceEnumerator* pEnum) {
    bool loopback = true, fellBack = false;
    ComPtr<IMMDevice> d = VizResolveAudioDevice(pEnum, VizAudioSourceKey(), &loopback, &fellBack);
    if (!d || fellBack) return false;
    ComPtr<IAudioClient> c;
    if (FAILED(d->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)c.GetAddressOf()))) return false;
    WAVEFORMATEX* pwfx = nullptr;
    c->GetMixFormat(&pwfx);
    if (!pwfx) return false;
    HRESULT hr = c->Initialize(AUDCLNT_SHAREMODE_SHARED, loopback ? AUDCLNT_STREAMFLAGS_LOOPBACK : 0u, 5000000, 0,
                               pwfx, nullptr);
    CoTaskMemFree(pwfx);
    return SUCCEEDED(hr);
}

class VizEngine {
public:
    // ---- Thread lifetime --------------------------------------------------------
    void ThreadInit() {
        BuildVizSeeds();
        if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                    __uuidof(IMMDeviceEnumerator), (void**)enum_.GetAddressOf()))) {
            enum_.Reset();
        }
        if (enum_) {
            notify_ = new VizEndpointNotificationClient();
            notifyRegistered_ = SUCCEEDED(enum_->RegisterEndpointNotificationCallback(notify_));
        }
        audioEvent_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        ring_.assign(RING_CAP, 0.f);
    }
    void ThreadExit() {
        Close();
        if (enum_ && notify_ && notifyRegistered_) enum_->UnregisterEndpointNotificationCallback(notify_);
        if (notify_) notify_->Release();
        notify_ = nullptr;
        ttnpu::EndSession(npuSession_);
        enum_.Reset();
        if (audioEvent_) CloseHandle(audioEvent_);
        audioEvent_ = nullptr;
    }
    HANDLE AudioEvent() const { return audioEvent_; }
    bool IsOpen() const { return (bool)client_; }
    bool IsStopped() const { return stopped_; }
    // Loopback (an output device) or a recording stream (an input device),
    // as opened. The idle ladder treats the two differently.
    bool IsLoopback() const { return loopback_; }
    // A peak meter that answered its last read. Deep idle needs one.
    bool MeterOk() const { return meter_ && meterOk_; }

    // Opens the stream if it isn't, or reopens it after a device change. A
    // source that won't open is retried after 0.5 s, then 1, 2, 4 ... up to
    // 30 s, and straight away again after any device change. While the
    // default output is standing in for a chosen device that wouldn't open,
    // the chosen one is probed every 30 s and taken back when it's free.
    void EnsureOpen() {
        bool changed = g_deviceChanged.exchange(false, std::memory_order_relaxed);
        ULONGLONG now = GetTickCount64();
        if (changed) retryMs_ = kRetryMinMs;
        if (client_ && !changed) {
            if (!openFallback_ || now - lastProbe_ < kProbeMs) return;
            lastProbe_ = now;
            if (!VizProbeAudioSource(enum_.Get())) return;
            Wh_Log(L"[Audio] the chosen source opens again; switching back to it");
            retryMs_ = kRetryMinMs;
            lastReinit_ = 0;
        }
        if (now - lastReinit_ < retryMs_) {
            if (changed) g_deviceChanged.store(true, std::memory_order_relaxed);
            return;
        }
        lastReinit_ = now;
        Close();
        if (!enum_) return;
        UINT32 sr = 48000, ch = 2;
        bool fl = true;
        DWORD mask = 0;
        bool loopback = true, fellBack = false, openFailed = false;
        std::wstring source = VizAudioSourceKey();
        if (VizInitAudioClient(enum_.Get(), device_, client_, capture_, sr, ch, fl, mask, audioEvent_, &loopback,
                               &fellBack, &openFailed)) {
            sampleRate_ = sr;
            channels_ = ch;
            isFloat_ = fl;
            channelMask_ = mask;
            loopback_ = loopback;
            stopped_ = false;
            retryMs_ = kRetryMinMs;
            openFallback_ = openFailed;
            lastProbe_ = now;
            meter_.Reset();
            device_->Activate(kIID_IAudioMeterInformation, CLSCTX_ALL, nullptr, (void**)meter_.GetAddressOf());
            float probe = 0.f;
            meterOk_ = meter_ && SUCCEEDED(meter_->GetPeakValue(&probe));
            ResetAnalysis();
            configuredGen_ = 0;  // sample rate may have changed: rebuild the precision engine
            g_audioOpen.store(true);
            if (g_engineClosed) ResetEvent(g_engineClosed);
            Wh_Log(L"[Audio] %s open (%s): %u Hz, %u ch, %s", loopback ? L"loopback" : L"capture",
                   VizDeviceName(device_.Get()).c_str(), sr, ch, fl ? L"float" : L"int16");
            // Said once per source: a missing device is worth one heads-up,
            // not one per reconnect attempt.
            if (fellBack && source != warnedSource_) {
                warnedSource_ = source;
                if (openFailed)
                    VizPostAudioNotice(L"The chosen audio source (" + VizAudioSourceLabel(source) +
                                       L") is connected but couldn't be opened (another app may be using it in "
                                       L"exclusive mode), so the visualizer is listening to the default output "
                                       L"until it can.");
                else
                    VizPostAudioNotice(L"The chosen audio source (" + VizAudioSourceLabel(source) +
                                       L") isn't connected or enabled, so the visualizer is listening to the "
                                       L"default output until it comes back.");
            } else if (!fellBack) {
                warnedSource_.clear();
            }
            g_audioOnFallback.store(fellBack, std::memory_order_relaxed);
        } else {
            ULONGLONG was = retryMs_;
            retryMs_ = std::min<ULONGLONG>(retryMs_ * 2, kRetryMaxMs);
            if (retryMs_ != was) Wh_Log(L"[Audio] no audio source could be opened; next try in %llu ms", retryMs_);
        }
    }

    void Close() {
        if (client_) client_->Stop();
        capture_.Reset();
        client_.Reset();
        meter_.Reset();
        meterOk_ = false;
        device_.Reset();
        stopped_ = false;
        ResetAnalysis();
        g_audioOpen.store(false);
        if (g_engineClosed) SetEvent(g_engineClosed);
    }

    // Deep idle: stop the stream, keep the client (restarting it is instant).
    // Loopback only: an input device's meter may read nothing once our
    // stream is stopped, which would strand it in deep idle.
    void StopStream() {
        if (client_ && !stopped_ && loopback_) {
            client_->Stop();
            stopped_ = true;
            Wh_Log(L"[Idle] deep idle: loopback stopped, watching the peak meter");
        }
    }
    void StartStream() {
        if (client_ && stopped_) {
            client_->Start();
            stopped_ = false;
            lastPacketQpc_ = 0;
            Wh_Log(L"[Idle] loopback restarted");
        }
    }
    // The endpoint's own peak meter, 0..1 (the loudest channel, before Input
    // Gain). Reading it costs one COM call and needs no stream of ours to be
    // running. False when there is no meter or the read failed; the caller
    // then must not trust silence (and MeterOk() turns false, so deep idle
    // isn't entered again on this stream).
    bool ReadMeter(float* peak) {
        *peak = 0.f;
        if (!meter_) return false;
        HRESULT hr = meter_->GetPeakValue(peak);
        meterOk_ = SUCCEEDED(hr);
        return meterOk_;
    }

    // ---- Per frame ------------------------------------------------------------------
    // Drains whatever has arrived into the analysis inputs (rings, meters)
    // without analysing it. Cheap: what trickle does on each audio event.
    // Returns true when the audio since the last Frame() got loud enough to
    // be worth looking at now: above the audible level and 6 dB above what
    // the last frame saw, so a steady noise floor above -70 dBFS (a hum on a
    // virtual cable, an app's dither) doesn't turn every event into a frame,
    // while music starting still does at once.
    bool Pump() {
        SyncConfig();
        if (client_ && !stopped_) pendingGot_ += Drain();
        float trigger = std::max(kVizAudibleLin, 2.f * lastFramePeak_);
        return pendingGot_ > 0 && blockPeak_ > trigger;
    }

    // Drains the stream, runs the analysis the settings ask for, and
    // publishes everything the renderer reads. dt is the time since the last
    // call, which the ballistics and the classic silence decay are scaled by.
    // Returns false when nothing the renderer reads has changed (settled
    // digital silence), so an idle caller can skip the render tick.
    bool Frame(double dt) {
        Pump();
        dt = std::clamp(dt, 0.0005, 0.25);
        int got = pendingGot_;
        bool changed = PublishScope();
        if (cfg_.precision) changed |= PrecisionFrame(dt, got);
        else {
            ClassicFrame(dt, got);
            changed = true;
        }
        if (cfg_.wantLoudness || cfg_.wantGonio) changed |= PublishMeters();
        if (cfg_.wantGonio && got > 0) {
            PublishGonio();
            changed = true;
        }
        lastFramePeak_ = blockPeak_;
        blockPeak_ = 0.f;
        pendingGot_ = 0;
        return changed;
    }

private:
    static constexpr int RING_CAP = VIZ_FFT_SIZE_MAX * 4;
    static constexpr ULONGLONG kRetryMinMs = 500, kRetryMaxMs = 30000, kProbeMs = 30000;

    void ResetAnalysis() {
        std::fill(ring_.begin(), ring_.end(), 0.f);
        ringHead_ = ringCount_ = 0;
        for (int b = 0; b < VIZ_NUM_BANDS; b++) bandEnv_[b] = 0.f;
        PublishBands(bandEnv_);
        currentFftSize_ = 0;
        std::fill(std::begin(bandLevel_), std::end(bandLevel_), 0.f);
        VizBandFrame empty;
        PublishBandFrame(empty);
        lastPacketQpc_ = 0;
        blockPeak_ = lastFramePeak_ = 0.f;
        pendingGot_ = 0;
        zeroRun_ = 0;
        silentFinal_ = settled_ = gpuSilentFlushed_ = false;
    }

    // Picks up a settings change. The precision engine also has to be rebuilt
    // when the sample rate changes, which EnsureOpen signals by clearing the
    // generation it last built for.
    void SyncConfig() {
        int gen = g_engineCfgGen.load(std::memory_order_acquire);
        if (gen == configuredGen_) return;
        configuredGen_ = gen;
        {
            std::lock_guard<std::mutex> lock(g_engineCfgMutex);
            cfg_ = g_engineCfg;
        }
        cfg_.spec.sampleRate = (int)sampleRate_;
        cfg_.spec.maxBands = VIZ_BARS_MAX;
        if (cfg_.precision) {
            spec_.Configure(cfg_.spec);
            const auto& bands = spec_.Bands();
            for (size_t b = 0; b < bands.size() && b < (size_t)VIZ_BARS_MAX; b++) {
                double fc = bands[b].fc;
                bandZone_[b] = (fc < 300.0) ? 0 : (fc < 2500.0) ? 1 : 2;
                bandBass_[b] = fc < 150.0;
            }
            std::fill(std::begin(bandLevel_), std::end(bandLevel_), 0.f);
            agcDb_ = 0.f;
            loudEnvDb_ = -200.f;
            if (cfg_.workload == VizWorkload::Gpu) {
                std::lock_guard<std::mutex> lock(g_gpuFeed.m);
                g_gpuFeed.layoutSerial++;
                g_gpuFeed.n = spec_.Cfg().fftSize;
                for (int t = 0; t < 3; t++) {
                    g_gpuFeed.rate[t] = spec_.TierRate(t);
                    g_gpuFeed.used[t] = spec_.TierUsed(t);
                }
                g_gpuFeed.peakCal = spec_.PeakCal();
                g_gpuFeed.maps = spec_.Maps();
                g_gpuFeed.bands = bands;
                g_gpuFeed.window.assign(spec_.Window(), spec_.Window() + g_gpuFeed.n);
                g_gpuFeed.blocks.assign((size_t)3 * g_gpuFeed.n, 0.f);
                g_gpuFeed.dirty = 7;
            }
        }
        // Loudness weights from the channel mask: BS.1770 counts the
        // surrounds 1.41 times, leaves LFE out, and treats everything else as
        // a front channel.
        float w[ttdsp::LoudnessMeter::kMaxCh];
        for (UINT32 c = 0, bit = 0; c < (UINT32)ttdsp::LoudnessMeter::kMaxCh; c++) {
            w[c] = 1.f;
            if (!channelMask_) continue;
            while (bit < 32 && !(channelMask_ & (1u << bit))) bit++;
            DWORD sp = (bit < 32) ? (1u << bit) : 0;
            bit++;
            if (sp == SPEAKER_LOW_FREQUENCY) w[c] = 0.f;
            else if (sp == SPEAKER_BACK_LEFT || sp == SPEAKER_BACK_RIGHT || sp == SPEAKER_SIDE_LEFT ||
                     sp == SPEAKER_SIDE_RIGHT)
                w[c] = 1.41f;
        }
        loudness_.Configure((int)sampleRate_, (int)std::min<UINT32>(channels_, ttdsp::LoudnessMeter::kMaxCh), w);
        loudness_.SetTruePeak(cfg_.wantTruePeak);
        correlation_.Configure((int)sampleRate_, 300.0);
        // Digital silence (see PrecisionFrame): zeros enough to fill the
        // deepest tier's whole ring (2 N at fs / 16) plus every decimator's
        // history, after which every input the analysis can see is zero.
        {
            int deepest = 0;
            if (cfg_.precision) deepest = spec_.TierUsed(2) ? 2 : spec_.TierUsed(1) ? 1 : 0;
            silentNeeded_ = ((long long)2 * std::max(256, spec_.Cfg().fftSize) << (2 * deepest)) + 1024;
            zeroRun_ = 0;
            silentFinal_ = settled_ = gpuSilentFlushed_ = false;
        }
        lastMeters_.valid = false;  // publish the meters on the next frame whatever they read
        lastTrackTick_ = g_nowPlayingChangedTick.load(std::memory_order_relaxed);
    }

    // ---- Drain ----------------------------------------------------------------------
    // Returns the number of frames that arrived (silent packets included).
    int Drain() {
        UINT32 packetSize = 0;
        HRESULT hr = capture_->GetNextPacketSize(&packetSize);
        if (hr == AUDCLNT_E_DEVICE_INVALIDATED) {
            g_deviceChanged.store(true, std::memory_order_relaxed);
            return 0;
        }
        if (FAILED(hr)) return 0;

        // Input Gain is folded into the mixdown, as in 1.5, so it reaches the
        // scope, the bars and the meters alike.
        float inputGainLin = (g_settings.inputGainDb == 0.0f) ? 1.0f : powf(10.f, g_settings.inputGainDb / 20.f);
        UINT32 ch = std::max<UINT32>(1, channels_);
        float monoScale = inputGainLin / (float)ch;
        int total = 0;  // blockPeak_ accumulates until Frame() takes it

        while (packetSize > 0) {
            BYTE* pData = nullptr;
            UINT32 numFrames = 0;
            DWORD flags = 0;
            HRESULT hrBuf = capture_->GetBuffer(&pData, &numFrames, &flags, nullptr, nullptr);
            if (hrBuf == AUDCLNT_E_DEVICE_INVALIDATED) {
                g_deviceChanged.store(true, std::memory_order_relaxed);
                break;
            }
            if (FAILED(hrBuf)) break;
            bool silent = (flags & AUDCLNT_BUFFERFLAGS_SILENT) || !pData;
            if (numFrames > 0) {
                total += (int)numFrames;
                ConvertPacket(silent ? nullptr : pData, numFrames, ch, inputGainLin, monoScale);
            }
            capture_->ReleaseBuffer(numFrames);
            hr = capture_->GetNextPacketSize(&packetSize);
            if (hr == AUDCLNT_E_DEVICE_INVALIDATED) {
                g_deviceChanged.store(true, std::memory_order_relaxed);
                break;
            }
            if (FAILED(hr)) break;
        }
        LARGE_INTEGER q;
        QueryPerformanceCounter(&q);
        if (total > 0) lastPacketQpc_ = q.QuadPart;
        return total;
    }

    // One packet: the classic mono ring (scope and classic bands), the
    // precision engine's chosen channel, loudness, correlation, goniometer.
    // A silent packet (pData null) is real silence of known length: it is fed
    // to the precision engine and the meters as zeros, so they decay on time,
    // while the classic ring, as in 1.5, is left alone. Each consumer only
    // gets work done for it when it is in use: the interleaved copy for
    // loudness, L/R correlation for the full loudness readout or the
    // Goniometer, the stereo history for the Goniometer.
    void ConvertPacket(const BYTE* pData, UINT32 frames, UINT32 ch, float gain, float monoScale) {
        const bool prec = cfg_.precision;
        const bool loud = cfg_.wantLoudness;
        const bool corr = cfg_.wantCorrelation;
        const bool gonio = cfg_.wantGonio;
        if (prec) mono_.resize(frames);
        if (loud) inter_.resize((size_t)frames * ch);
        if (gonio) stereo_.resize((size_t)frames * 2);
        if (!pData) {
            if (prec) {
                // Once the analysis holds nothing but zeros, more zeros change
                // none of its state, so they needn't be pushed.
                if (zeroRun_ < silentNeeded_) {
                    std::fill(mono_.begin(), mono_.end(), 0.f);
                    spec_.Push(mono_.data(), (int)frames);
                }
                zeroRun_ += frames;
            }
            if (loud) {
                std::fill(inter_.begin(), inter_.end(), 0.f);
                loudness_.Process(inter_.data(), (int)frames, (int)ch);
            }
            if (corr) correlation_.PushSilence((int)frames);
            if (gonio) {
                std::fill(stereo_.begin(), stereo_.end(), 0.f);
                AppendGonio(frames);
            }
            return;
        }
        const float* f32 = reinterpret_cast<const float*>(pData);
        const INT16* i16 = reinterpret_cast<const INT16*>(pData);
        int lastNonZero = -1;
        for (UINT32 f = 0; f < frames; f++) {
            float l = 0.f, r = 0.f, sum = 0.f;
            for (UINT32 c = 0; c < ch; c++) {
                float v = isFloat_ ? f32[f * ch + c] : i16[f * ch + c] / 32768.f;
                sum += v;
                if (loud) inter_[(size_t)f * ch + c] = v * gain;
                if (c == 0) l = v;
                if (c == 1) r = v;
            }
            if (ch == 1) r = l;
            float mono = sum * monoScale;
            ring_[ringHead_] = mono;
            ringHead_ = (ringHead_ + 1) % RING_CAP;
            if (ringCount_ < RING_CAP) ringCount_++;
            blockPeak_ = std::max(blockPeak_, fabsf(mono));
            l *= gain;
            r *= gain;
            if (prec) {
                float v;
                switch (cfg_.channel) {
                    case VizChannel::Left: v = l; break;
                    case VizChannel::Right: v = r; break;
                    case VizChannel::Mid: v = 0.5f * (l + r); break;
                    case VizChannel::Side: v = 0.5f * (l - r); break;
                    default: v = mono; break;
                }
                mono_[f] = v;
                if (v != 0.f) lastNonZero = (int)f;
            }
            if (corr) correlation_.Push(l, r);
            if (gonio) {
                stereo_[2 * f] = l;
                stereo_[2 * f + 1] = r;
            }
        }
        // One clock read per packet (1.5 read it per sample).
        if (frames > 0) {
            lastAudioTick_ = GetTickCount64();
            scopeFlatPublished_ = false;
        }
        if (prec) {
            if (lastNonZero >= 0) {
                if (zeroRun_ >= silentNeeded_) wakeFromSilence_ = true;
                zeroRun_ = (long long)frames - 1 - lastNonZero;
            } else {
                zeroRun_ += frames;
            }
            spec_.Push(mono_.data(), (int)frames);
        }
        if (loud) loudness_.Process(inter_.data(), (int)frames, (int)ch);
        if (gonio) AppendGonio(frames);
    }

    void AppendGonio(UINT32 frames) {
        // Keep the newest 2048 stereo frames.
        const size_t keep = 2048;
        gonioPending_.insert(gonioPending_.end(), stereo_.begin(), stereo_.begin() + (size_t)frames * 2);
        if (gonioPending_.size() > keep * 2)
            gonioPending_.erase(gonioPending_.begin(), gonioPending_.end() - keep * 2);
    }

    // ---- Oscilloscope trace (1.5, unchanged apart from where it lives) ---------------
    // Returns whether a new trace was published.
    bool PublishScope() {
        if (g_settings.shape != VizShape::Oscilloscope) return false;
        static constexpr double SCOPE_HOLD_MS = 60.0;
        static constexpr double SCOPE_FADE_MS = 140.0;
        double sinceAudioMs = (double)(GetTickCount64() - lastAudioTick_);
        float staleFade = 1.0f;
        if (sinceAudioMs > SCOPE_HOLD_MS)
            staleFade = std::max(0.0f, 1.0f - (float)((sinceAudioMs - SCOPE_HOLD_MS) / SCOPE_FADE_MS));
        if (staleFade <= 0.0f && scopeFlatPublished_) return false;

        auto ringAt = [&](int i) -> float {
            int m = i % RING_CAP;
            if (m < 0) m += RING_CAP;
            return ring_[m];
        };
        int windowMs = std::clamp(g_settings.oscilloscopeWindowMs, 5, 250);
        int span = (int)((double)sampleRate_ * (double)windowMs / 1000.0);
        span = std::clamp(span, VIZ_WAVE_SAMPLES, RING_CAP / 2);
        // Trigger on the nearest rising zero crossing before the window's
        // natural start, so the trace holds still (see the 1.4 notes).
        int start = ringHead_ - span;
        int searchSpan = std::min(span / 2, RING_CAP - span - 1);
        for (int k = 1; k <= searchSpan; k++) {
            if (ringAt(start - k - 1) <= 0.f && ringAt(start - k) > 0.f) {
                start -= k;
                break;
            }
        }
        float waveSnap[VIZ_WAVE_SAMPLES];
        float gain = cfg_.precision ? 1.0f : agcGain_;
        for (int w = 0; w < VIZ_WAVE_SAMPLES; w++) {
            int from = start + (int)((long long)w * span / VIZ_WAVE_SAMPLES);
            int to = start + (int)((long long)(w + 1) * span / VIZ_WAVE_SAMPLES);
            if (to <= from) to = from + 1;
            float peak = 0.f;
            for (int s = from; s < to; s++) {
                float v = ringAt(s);
                if (fabsf(v) > fabsf(peak)) peak = v;
            }
            waveSnap[w] = std::clamp(peak * gain * staleFade, -1.0f, 1.0f);
        }
        PublishWaveform(waveSnap);
        scopeFlatPublished_ = (staleFade <= 0.0f);
        return true;
    }

    // ---- Classic engine (1.4 / 1.5 numbers, unchanged) -----------------------------------
    void ClassicFrame(double dt, int got) {
        static constexpr float GRAVITY[VIZ_NUM_BANDS] = {0.018f, 0.020f, 0.022f, 0.025f,
                                                         0.030f, 0.036f, 0.042f};
        int wanted = (g_settings.fftSize == 1024 || g_settings.fftSize == 2048 || g_settings.fftSize == 4096 ||
                      g_settings.fftSize == 8192)
                         ? g_settings.fftSize
                         : 1024;
        if (wanted != currentFftSize_) {
            currentFftSize_ = wanted;
            BuildHannWindow(wanted);
            BuildTwiddleFactors(wanted);
            re_.assign(wanted, 0.f);
            im_.assign(wanted, 0.f);
            ringHead_ = ringCount_ = 0;
            for (int b = 0; b < VIZ_NUM_BANDS; b++) bandEnv_[b] = 0.f;
            PublishBands(bandEnv_);
            BuildLogBins(sampleRate_, wanted);
        }
        if (got == 0) {
            // 1.5 applied this once per capture wake, which with no audio was
            // every 20 ms. Wakes are per frame now, so the step is scaled to
            // the same 20 ms to keep the fall speed exactly as it was.
            float k = (float)(dt / 0.020);
            for (int b = 0; b < VIZ_NUM_BANDS; b++) bandEnv_[b] = std::max(0.f, bandEnv_[b] - GRAVITY[b] * k);
            PublishBands(bandEnv_);
            return;
        }

        static constexpr float AGC_TARGET = 0.85f, AGC_ATTACK = 0.35f, AGC_RELEASE = 0.012f,
                               AGC_GLIDE = 0.04f, AGC_FLOOR = 0.010f, AGC_IDLE_AUDIBLE = 0.030f;
        while (ringCount_ >= currentFftSize_) {
            int readStart = (ringHead_ - ringCount_ + RING_CAP) % RING_CAP;
            for (int i = 0; i < currentFftSize_; i++) {
                re_[i] = ring_[(readStart + i) % RING_CAP] * g_hannWindow[i];
                im_[i] = 0.f;
            }
            ringCount_ -= currentFftSize_ / 2;
            bool analysedOnNpu = cfg_.workload == VizWorkload::Npu &&
                                 ttnpu::Analyze(npuSession_, currentFftSize_, re_.data(), re_, im_);
            if (!analysedOnNpu) VizFFT(re_, im_);

            float t_sens = g_settings.sensitivity / 100.0f;
            float sliderGain = (t_sens <= 1.0f) ? 0.25f + t_sens * t_sens * 2.75f : 3.0f + (t_sens - 1.0f) * 4.0f;
            auto eq = GetVizEQMultipliers(g_settings.eq);
            static constexpr float BAND_SENSITIVITY[VIZ_NUM_BANDS] = {0.30f, 0.22f, 0.12f, 0.06f,
                                                                       0.030f, 0.018f, 0.010f};
            float rawBand[VIZ_NUM_BANDS];
            float blockPeak = 0.f;
            for (int b = 0; b < VIZ_NUM_BANDS; b++) {
                int bStart = g_logBinStart[b];
                int bEnd = g_logBinStart[b + 1];
                if (bEnd <= bStart) bEnd = bStart + 1;
                float sumSq = 0.f;
                int count = 0;
                for (int k = bStart; k < bEnd; k++) {
                    sumSq += re_[k] * re_[k] + im_[k] * im_[k];
                    count++;
                }
                float rms = (count > 0) ? sqrtf(sumSq / (float)count) : 0.f;
                float eqM = (VIZ_BAND_EQ_ZONE[b] == 0) ? eq.low : (VIZ_BAND_EQ_ZONE[b] == 1) ? eq.mid : eq.high;
                float rawGained = (rms / (currentFftSize_ * 0.5f)) / BAND_SENSITIVITY[b] * sliderGain * eqM;
                rawBand[b] = std::max(0.f, rawGained);
                blockPeak = std::max(blockPeak, rawBand[b]);
            }
            bool blockAudible = (blockPeak >= AGC_FLOOR);
            bool blockIdleAudible = (blockPeak >= AGC_IDLE_AUDIBLE);
            if (g_settings.autoGain) {
                if (blockAudible) {
                    float k = (blockPeak > agcPeakEnv_) ? AGC_ATTACK : AGC_RELEASE;
                    agcPeakEnv_ += (blockPeak - agcPeakEnv_) * k;
                    float maxBoost = powf(10.f, g_settings.autoGainMaxDb / 20.f);
                    float want = (agcPeakEnv_ > 1e-6f) ? (AGC_TARGET / agcPeakEnv_) : maxBoost;
                    want = std::clamp(want, 1.f, maxBoost);
                    agcGain_ += (want - agcGain_) * AGC_GLIDE;
                }
            } else {
                agcPeakEnv_ = 0.f;
                agcGain_ = 1.f;
            }
            float maxMag = 0.f;
            for (int b = 0; b < VIZ_NUM_BANDS; b++) {
                float rawGained = rawBand[b] * agcGain_;
                float mag;
                switch (g_settings.sensitivityCurve) {
                    case VizSensitivityCurve::Exponential: mag = 1.f - expf(-rawGained); break;
                    case VizSensitivityCurve::Power: mag = std::min(1.f, powf(rawGained, 0.6f)); break;
                    case VizSensitivityCurve::Knee:
                    default: {
                        constexpr float knee = 0.7f;
                        mag = (rawGained <= knee) ? rawGained
                                                  : knee + (1.f - knee) * tanhf((rawGained - knee) / (1.f - knee));
                        break;
                    }
                }
                mag = std::max(0.f, std::min(1.f, mag));
                bandEnv_[b] = (mag >= bandEnv_[b]) ? mag : std::max(0.f, bandEnv_[b] - GRAVITY[b]);
                maxMag = std::max(maxMag, bandEnv_[b]);
            }
            PublishBands(bandEnv_);
            if (maxMag > 0.03f && (!g_settings.autoGain || blockIdleAudible))
                g_lastAudibleTickMs.store(GetTickCount64(), std::memory_order_relaxed);
            if (g_settings.beatFlashEnabled) {
                if (bandEnv_[0] - prevBassEnv_ > 0.12f) g_beatPulse.store(1.0f, std::memory_order_relaxed);
                prevBassEnv_ = bandEnv_[0];
            }
            if (g_settings.peakFreqEnabled) {
                int nyquistBin = currentFftSize_ / 2;
                int loBin = std::max(1, (int)(20.f * currentFftSize_ / (float)sampleRate_));
                int hiBin = std::min(nyquistBin - 1, (int)(20000.f * currentFftSize_ / (float)sampleRate_));
                int bestBin = -1;
                float bestMag = 0.f;
                for (int k = loBin; k <= hiBin; k++) {
                    float mag = re_[k] * re_[k] + im_[k] * im_[k];
                    if (mag > bestMag) {
                        bestMag = mag;
                        bestBin = k;
                    }
                }
                if (bestBin > 0 && sqrtf(bestMag) / (currentFftSize_ * 0.5f) > 0.02f)
                    g_dominantFreqHz.store((float)bestBin * (float)sampleRate_ / (float)currentFftSize_,
                                           std::memory_order_relaxed);
            }
        }
    }

    // ---- Precision engine --------------------------------------------------------------
    // Digital silence. Once every sample the analysis can see is an exact
    // zero (zeroRun_ >= silentNeeded_), one last forced analysis gives the
    // levels of pure zeros, and after that there is nothing for an FFT to
    // find: no FFTs run until a non-zero sample arrives. The bars keep their
    // ballistics until they are within 1e-4 of the floor (a tenth of a pixel
    // on a 1000 px bar), are then put exactly on it and published once, and
    // from there a frame does nothing at all and reports no change. The first
    // non-zero sample forces every tier to be analysed on the next frame, so
    // the bass tiers don't wait out a hop after the silence.
    // Returns whether anything the renderer reads was published.
    bool PrecisionFrame(double dt, int got) {
        LARGE_INTEGER q;
        QueryPerformanceCounter(&q);
        // Stream starved (nothing at all, not even silent packets, for more
        // than 40 ms): that is silence too, so feed its length in as zeros and
        // the spectrum decays on time instead of freezing on the last note.
        if (got == 0 && client_ && !stopped_) {
            double since = lastPacketQpc_ ? (double)(q.QuadPart - lastPacketQpc_) / (double)VizQpcFreq() : 1.0;
            if (since > 0.040) {
                int n = std::min((int)(dt * sampleRate_), cfg_.spec.fftSize);
                if (n > 0) {
                    if (zeroRun_ < silentNeeded_) {
                        zeros_.assign((size_t)n, 0.f);
                        spec_.Push(zeros_.data(), n);
                    }
                    zeroRun_ += n;
                }
            }
        }
        const bool silent = zeroRun_ >= silentNeeded_;
        const bool force = wakeFromSilence_;
        wakeFromSilence_ = false;
        if (!silent) silentFinal_ = settled_ = gpuSilentFlushed_ = false;

        if (cfg_.workload == VizWorkload::Gpu) {
            // The GPU does the analysis. Here: hand over fresh blocks (in
            // silence, the all-zero ones once and then nothing), and judge
            // silence from the samples themselves.
            if (!silent || !gpuSilentFlushed_) {
                bool take = force || silent;
                std::lock_guard<std::mutex> lock(g_gpuFeed.m);
                int n = g_gpuFeed.n;
                if (n > 0 && g_gpuFeed.blocks.size() >= (size_t)3 * n) {
                    for (int t = 0; t < 3; t++)
                        if (spec_.TakeTierBlock(t, &g_gpuFeed.blocks[(size_t)t * n], take)) g_gpuFeed.dirty |= 1u << t;
                    if (silent) gpuSilentFlushed_ = true;
                }
            }
            if (got > 0 && blockPeak_ > kVizAudibleLin)
                g_lastAudibleTickMs.store(GetTickCount64(), std::memory_order_relaxed);
            return true;  // the renderer runs the ballistics, so it always has work
        }
        if (silent && settled_) return false;

        auto npu = [this](const float* windowed, int n, float* re, float* im) -> bool {
            if (cfg_.workload != VizWorkload::Npu) return false;
            npuRe_.resize(n);
            npuIm_.resize(n);
            if (!ttnpu::Analyze(npuSession_, n, windowed, npuRe_, npuIm_)) return false;
            for (int k = 0; k < n / 2; k++) {
                re[k] = npuRe_[k];
                im[k] = npuIm_[k];
            }
            re[n / 2] = im[n / 2] = 0.f;  // the NPU model leaves out the Nyquist bin
            return true;
        };
        bool fresh = false;
        if (!silentFinal_) {
            fresh = spec_.Analyze(force || silent, npu);
            if (silent) silentFinal_ = true;
            if (fresh) {
                VizPerf(kPerfAnalyses);
                VizPerf(kPerfFfts, (uint32_t)spec_.LastFftCount());
            }
        }

        const int nb = std::min(spec_.NumBands(), VIZ_BARS_MAX);
        const float* db = spec_.LevelsDb();
        const float range = std::max(1.f, cfg_.disp.ceilDb - cfg_.disp.floorDb);
        ttdsp::DisplayMap disp = cfg_.disp;
        disp.gainDb = cfg_.sensDb + agcDb_;
        // Attack / release coefficients once per frame, not once per band.
        const ttdsp::BallisticsCoef bc = ttdsp::BallisticsCoefs(dt, cfg_.ball, range);
        float maxX = 0.f, rawMax = -300.f, bass = 0.f, maxLevel = 0.f;
        VizBandFrame& out = frame_;
        out.count = nb;
        out.zone[0] = out.zone[1] = out.zone[2] = 0.f;
        out.zoneCount[0] = out.zoneCount[1] = out.zoneCount[2] = 0.f;
        for (int b = 0; b < nb; b++) {
            float x = disp.ToNorm(db[b]);
            bandLevel_[b] = ttdsp::BallisticsApply(bandLevel_[b], x, bc);
            out.level[b] = bandLevel_[b];
            out.zone[bandZone_[b]] += bandLevel_[b];
            out.zoneCount[bandZone_[b]] += 1.f;
            maxX = std::max(maxX, x);
            maxLevel = std::max(maxLevel, bandLevel_[b]);
            rawMax = std::max(rawMax, db[b]);
            if (bandBass_[b]) bass = std::max(bass, x);
        }
        if (silent && silentFinal_ && maxLevel < 1e-4f) {
            for (int b = 0; b < nb; b++) bandLevel_[b] = out.level[b] = 0.f;
            out.zone[0] = out.zone[1] = out.zone[2] = 0.f;
            bassSlow_ = 0.f;
            settled_ = true;
            PublishBandFrame(out);
            return true;
        }

        // Silence, judged on both the samples and the drawn level, so neither
        // a quiet hiss nor a display range set very low can hold the mod awake.
        bool audible = got > 0 && blockPeak_ > kVizAudibleLin && maxX > 0.02f;
        if (audible) g_lastAudibleTickMs.store(GetTickCount64(), std::memory_order_relaxed);

        // Auto Gain: lift the loudest band toward 85% of the range. Boost
        // only, held through silence, smoothed over about 0.4 s.
        if (cfg_.autoGain) {
            if (audible) {
                float target = cfg_.disp.floorDb + 0.85f * range;
                float loud = rawMax + cfg_.sensDb;
                float k = 1.f - expf(-(float)dt / ((loud > loudEnvDb_) ? 0.03f : 2.0f));
                loudEnvDb_ = (loudEnvDb_ < -150.f) ? loud : loudEnvDb_ + (loud - loudEnvDb_) * k;
                float want = std::clamp(target - loudEnvDb_, 0.f, cfg_.autoGainMaxDb);
                agcDb_ += (want - agcDb_) * (1.f - expf(-(float)dt / 0.4f));
            }
        } else {
            agcDb_ = 0.f;
        }

        // Beat: bass pulling ahead of its own 60 ms average. A difference of
        // two envelopes rather than "rose since the last block", so it fires
        // the same way at any frame rate.
        bassSlow_ += (bass - bassSlow_) * (1.f - expf(-(float)dt / 0.06f));
        if (cfg_.beat && bass - bassSlow_ > 0.12f) g_beatPulse.store(1.0f, std::memory_order_relaxed);

        if (cfg_.wantDominant && fresh) {
            double hz = spec_.DominantHz(-70.0);
            if (hz > 0.0) g_dominantFreqHz.store((float)hz, std::memory_order_relaxed);
        }
        PublishBandFrame(out);
        return true;
    }

    // Returns whether any reading changed since the last publish.
    bool PublishMeters() {
        // A new track starts a new integrated measurement.
        ULONGLONG tick = g_nowPlayingChangedTick.load(std::memory_order_relaxed);
        if (cfg_.loudnessResetOnTrack && tick != lastTrackTick_) {
            lastTrackTick_ = tick;
            loudness_.Reset();
        }
        VizMeterValues v;
        v.momentary = loudness_.Momentary();
        v.shortTerm = loudness_.ShortTerm();
        v.integrated = loudness_.Integrated();
        v.truePeak = loudness_.TruePeakDb();
        v.correlation = correlation_.Value();
        v.valid = true;
        bool changed = !(v.momentary == lastMeters_.momentary && v.shortTerm == lastMeters_.shortTerm &&
                         v.integrated == lastMeters_.integrated && v.truePeak == lastMeters_.truePeak &&
                         v.correlation == lastMeters_.correlation && lastMeters_.valid);
        lastMeters_ = v;
        if (!changed) return false;
        std::lock_guard<std::mutex> lock(g_meterMutex);
        g_meters = v;
        return true;
    }

    void PublishGonio() {
        {
            std::lock_guard<std::mutex> lock(g_gonioMutex);
            size_t n = gonioPending_.size() / 2;
            g_gonioXY.resize(n * 2);
            // (side, mid) = ((L - R) / 2, (L + R) / 2): mono runs straight up.
            for (size_t i = 0; i < n; i++) {
                float l = gonioPending_[2 * i], r = gonioPending_[2 * i + 1];
                g_gonioXY[2 * i] = 0.5f * (l - r);
                g_gonioXY[2 * i + 1] = 0.5f * (l + r);
            }
        }
        gonioPending_.clear();
        g_gonioSerial.fetch_add(1, std::memory_order_release);
    }

    // WASAPI
    ComPtr<IMMDeviceEnumerator> enum_;
    VizEndpointNotificationClient* notify_ = nullptr;
    bool notifyRegistered_ = false;
    ComPtr<IMMDevice> device_;
    ComPtr<IAudioClient> client_;
    ComPtr<IAudioCaptureClient> capture_;
    ComPtr<IVizAudioMeter> meter_;
    HANDLE audioEvent_ = nullptr;
    UINT32 sampleRate_ = 48000, channels_ = 2;
    bool isFloat_ = true;
    DWORD channelMask_ = 0;
    bool stopped_ = false;
    ULONGLONG lastReinit_ = 0;
    std::wstring warnedSource_;  // the source last reported missing
    LONGLONG lastPacketQpc_ = 0;
    float blockPeak_ = 0.f;      // mono-mix peak since the last Frame(), after Input Gain
    float lastFramePeak_ = 0.f;  // the same, for the frame before
    int pendingGot_ = 0;         // frames drained (by Pump) since the last Frame()
    bool loopback_ = true;
    bool meterOk_ = false;
    ULONGLONG retryMs_ = kRetryMinMs;
    bool openFallback_ = false;  // the default output stands in for a source that wouldn't open
    ULONGLONG lastProbe_ = 0;

    // Shared by both engines: the classic mono ring also feeds the scope.
    std::vector<float> ring_;
    int ringHead_ = 0, ringCount_ = 0;
    ULONGLONG lastAudioTick_ = 0;
    bool scopeFlatPublished_ = false;

    // Classic
    std::vector<float> re_, im_;
    int currentFftSize_ = 0;
    float bandEnv_[VIZ_NUM_BANDS] = {};
    float agcPeakEnv_ = 0.f, agcGain_ = 1.f, prevBassEnv_ = 0.f;
    ttnpu::Session npuSession_;

    // Precision
    VizEngineConfig cfg_;
    int configuredGen_ = 0;
    ttdsp::SpectrumEngine spec_;
    float bandLevel_[VIZ_BARS_MAX] = {};
    unsigned char bandZone_[VIZ_BARS_MAX] = {};
    bool bandBass_[VIZ_BARS_MAX] = {};
    float agcDb_ = 0.f, loudEnvDb_ = -200.f, bassSlow_ = 0.f;
    VizBandFrame frame_;
    std::vector<float> mono_, zeros_, npuRe_, npuIm_;
    long long zeroRun_ = 0, silentNeeded_ = 1LL << 62;  // digital silence, see PrecisionFrame
    bool silentFinal_ = false, settled_ = false, gpuSilentFlushed_ = false, wakeFromSilence_ = false;

    // Meters
    std::vector<float> inter_, stereo_, gonioPending_;
    ttdsp::LoudnessMeter loudness_;
    ttdsp::Correlation correlation_;
    VizMeterValues lastMeters_;
    ULONGLONG lastTrackTick_ = 0;
};

// The capture "thread" is now a state of the engine thread. These keep their
// 1.5 names and meaning for the callers (pause on fullscreen, resume, unload):
// Start asks for the stream, Stop asks for it to be closed and waits until it
// is, then clears the bars, exactly as joining the old thread did.
void StartVizCaptureThread() {
    g_captureWanted.store(true);
    if (g_engineWake) SetEvent(g_engineWake);
}

void StopVizCaptureThread() {
    g_captureWanted.store(false);
    if (g_engineWake) SetEvent(g_engineWake);
    if (g_engineClosed && g_audioOpen.load()) WaitForSingleObject(g_engineClosed, 1500);
    float zeroBands[VIZ_NUM_BANDS] = {};
    PublishBands(zeroBands);
    VizBandFrame empty;
    PublishBandFrame(empty);
}
