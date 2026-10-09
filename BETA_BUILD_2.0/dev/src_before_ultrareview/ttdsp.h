// ---- Precision analysis core (Analysis Engine = Precision) ---------------------
//
// Everything in here is plain C++ with no Windows dependency, so it can be
// compiled and tested on its own against a reference implementation. The mod
// pastes it in verbatim.
//
// What it replaces, and why. The 1.4 analysis ran one complex FFT over real
// data, summed it into 7 fixed bands, and then every bar interpolated between
// those 7 numbers. Bar Count and FFT Size changed how many bars were drawn,
// not how much the bars knew. This core gives every bar its own frequency band
// with its own edges, measured straight from the spectrum:
//
//   * a real-input FFT (an N/2-point complex FFT plus a post-twiddle), which
//     is half the work of running a complex FFT on real data;
//   * three resolution tiers: the signal as captured, decimated by 4 and
//     decimated by 16, each analysed with the same FFT size. Bass bands come
//     from the long, finely resolved windows, treble from the short, fast ones,
//     so a 1/24-octave bar at 50 Hz is a real measurement instead of a smear;
//   * band layouts on log / linear / mel / Bark / ERB scales, IEC 61260-1
//     fractional-octave bands, or equal-tempered musical notes;
//   * A / C / Z weighting (IEC 61672-1) and a dB-per-octave tilt, added per
//     band in dB, which costs nothing;
//   * frame-rate-independent ballistics: every coefficient is 1 - exp(-dt/tau)
//     from the time that actually passed, so 60, 144 and 240 Hz all move the
//     bars at the same speed;
//   * BS.1770-5 loudness (momentary, short-term, gated integrated), 4x
//     oversampled true peak, and a stereo correlation meter.
//
// Cost, at FFT Size 2048 and 144 FPS: one 1024-point complex FFT per frame for
// the top tier, the lower tiers only when enough new decimated samples have
// arrived to be worth it (every 1.5 and 6 frames). Well under 1% of one core.
namespace ttdsp {

constexpr double kPi = 3.14159265358979323846;

// ---- Small helpers ----------------------------------------------------------

inline bool IsPow2(int n) { return n > 0 && (n & (n - 1)) == 0; }
inline int Log2i(int n) {
    int l = 0;
    while ((1 << l) < n) l++;
    return l;
}
inline float DbFromPower(double p) { return (float)(10.0 * log10(p > 1e-30 ? p : 1e-30)); }

// Newest-first audio history. Push is one store; Latest copies the newest n
// samples, oldest first, into a linear buffer for windowing.
class Ring {
public:
    void Init(int capacity) {
        buf_.assign((size_t)std::max(1, capacity), 0.f);
        head_ = 0;
        count_ = 0;
        total_ = 0;
    }
    void Clear() {
        std::fill(buf_.begin(), buf_.end(), 0.f);
        head_ = 0;
        count_ = 0;
    }
    void Push(float v) {
        buf_[head_] = v;
        head_ = (head_ + 1 == (int)buf_.size()) ? 0 : head_ + 1;
        if (count_ < (int)buf_.size()) count_++;
        total_++;
    }
    // Zero-padded at the front when fewer than n samples exist yet.
    void Latest(float* out, int n) const {
        int cap = (int)buf_.size();
        int have = std::min(n, count_);
        int pad = n - have;
        for (int i = 0; i < pad; i++) out[i] = 0.f;
        int start = head_ - have;
        if (start < 0) start += cap;
        for (int i = 0; i < have; i++) {
            out[pad + i] = buf_[start];
            if (++start == cap) start = 0;
        }
    }
    float At(int ageFromNewest) const {  // 0 = newest
        int cap = (int)buf_.size();
        int i = head_ - 1 - ageFromNewest;
        while (i < 0) i += cap;
        return buf_[i % cap];
    }
    int Capacity() const { return (int)buf_.size(); }
    int Count() const { return count_; }
    unsigned long long Total() const { return total_; }

private:
    std::vector<float> buf_;
    int head_ = 0, count_ = 0;
    unsigned long long total_ = 0;
};

// ---- Windows ------------------------------------------------------------------
//
// Periodic ("DFT-even") forms, which are the right ones for spectral analysis:
// the symmetric forms used for filter design put the window's last sample on
// top of the next period's first and widen the main lobe slightly. Figures are
// from F. J. Harris (1978).
//
//   Hann              -31.5 dB sidelobes, ENBW 1.50 bins   the general default
//   Hamming           -43 dB,             ENBW 1.36        narrower main lobe
//   Blackman-Harris   -92 dB,             ENBW 2.00        high dynamic range
//   Flat-top          scallop ~0.01 dB,   ENBW ~3.8        accurate tone levels
//   Rectangular       -13 dB,             ENBW 1.00        reference only
enum class WindowKind { Hann, Hamming, BlackmanHarris, FlatTop, Rectangular };

struct WindowSums {
    double sum = 0.0;    // sum of w[n]   = N x coherent gain
    double sumSq = 0.0;  // sum of w[n]^2 = N x incoherent (power) gain
};

inline WindowSums BuildWindow(WindowKind kind, int n, float* out) {
    double a[5] = {1, 0, 0, 0, 0};
    switch (kind) {
        case WindowKind::Hann: a[0] = 0.5; a[1] = 0.5; break;
        case WindowKind::Hamming: a[0] = 0.54; a[1] = 0.46; break;
        case WindowKind::BlackmanHarris:
            a[0] = 0.35875; a[1] = 0.48829; a[2] = 0.14128; a[3] = 0.01168;
            break;
        case WindowKind::FlatTop:
            a[0] = 0.21557895; a[1] = 0.41663158; a[2] = 0.277263158;
            a[3] = 0.083578947; a[4] = 0.006947368;
            break;
        default: break;
    }
    WindowSums s;
    for (int i = 0; i < n; i++) {
        double x = 2.0 * kPi * i / n;
        double w = a[0] - a[1] * cos(x) + a[2] * cos(2 * x) - a[3] * cos(3 * x) + a[4] * cos(4 * x);
        out[i] = (float)w;
        s.sum += w;
        s.sumSq += w * w;
    }
    return s;
}

// ---- Real-input FFT -------------------------------------------------------------
//
// N real samples are packed as N/2 complex values z[k] = x[2k] + i x[2k+1],
// transformed with an N/2-point radix-2 FFT, and unpacked with one post-twiddle
// pass:
//   X[k] = E[k] + W^k O[k],  E = (Z[k] + conj Z[m-k]) / 2,
//                            O = (Z[k] - conj Z[m-k]) / 2i,   W = e^(-2 pi i / N)
// Output is bins 0..N/2 inclusive.
class RealFft {
public:
    bool Init(int n) {
        if (!IsPow2(n) || n < 16) return false;
        n_ = n;
        m_ = n / 2;
        int lg = Log2i(m_);
        rev_.resize(m_);
        for (int i = 0; i < m_; i++) {
            int r = 0;
            for (int b = 0; b < lg; b++)
                if (i & (1 << b)) r |= 1 << (lg - 1 - b);
            rev_[i] = r;
        }
        twr_.resize(m_ / 2);
        twi_.resize(m_ / 2);
        for (int k = 0; k < m_ / 2; k++) {
            double a = -2.0 * kPi * k / m_;
            twr_[k] = (float)cos(a);
            twi_[k] = (float)sin(a);
        }
        pr_.resize(m_ + 1);
        pi_.resize(m_ + 1);
        for (int k = 0; k <= m_; k++) {
            double a = -2.0 * kPi * k / n_;
            pr_[k] = (float)cos(a);
            pi_[k] = (float)sin(a);
        }
        zr_.assign(m_, 0.f);
        zi_.assign(m_, 0.f);
        return true;
    }
    int Size() const { return n_; }

    void Forward(const float* in, float* re, float* im) {
        const int m = m_;
        for (int k = 0; k < m; k++) {
            int r = rev_[k];
            zr_[r] = in[2 * k];
            zi_[r] = in[2 * k + 1];
        }
        for (int len = 2; len <= m; len <<= 1) {
            int half = len >> 1;
            int stride = m / len;
            for (int i = 0; i < m; i += len) {
                for (int j = 0; j < half; j++) {
                    float wr = twr_[j * stride], wi = twi_[j * stride];
                    int a = i + j, b = a + half;
                    float vr = zr_[b] * wr - zi_[b] * wi;
                    float vi = zr_[b] * wi + zi_[b] * wr;
                    zr_[b] = zr_[a] - vr;
                    zi_[b] = zi_[a] - vi;
                    zr_[a] += vr;
                    zi_[a] += vi;
                }
            }
        }
        for (int k = 0; k <= m; k++) {
            int ka = (k == m) ? 0 : k;
            int kb = (k == 0) ? 0 : m - k;
            float ar = zr_[ka], ai = zi_[ka];
            float br = zr_[kb], bi = -zi_[kb];  // conj Z[m-k]
            float er = 0.5f * (ar + br), ei = 0.5f * (ai + bi);
            float dr = ar - br, di = ai - bi;
            float orr = 0.5f * di, oi = -0.5f * dr;  // (a - b) / 2i
            re[k] = er + pr_[k] * orr - pi_[k] * oi;
            im[k] = ei + pr_[k] * oi + pi_[k] * orr;
        }
    }

private:
    int n_ = 0, m_ = 0;
    std::vector<int> rev_;
    std::vector<float> twr_, twi_, pr_, pi_, zr_, zi_;
};

// ---- Half-band decimator (by 2) ---------------------------------------------------
//
// A half-band lowpass has every even-offset tap except the centre at zero, so
// only about a quarter of the taps cost a multiply, and it only has to be
// evaluated for every second input sample. Kaiser-windowed, 59 taps, beta 9:
// about 90 dB of stopband from 0.3 fs, passband flat to 0.2 fs. After the
// decimation that leaves the output clean up to 0.8 of its own Nyquist, which
// is exactly the band the tier selector below allows each tier to serve.
class HalfBand {
public:
    static constexpr int kTaps = 59;  // 4k + 3, so the outermost taps are non-zero
    static constexpr int kMid = kTaps / 2;

    HalfBand() {
        double beta = 9.0;
        auto bessel0 = [](double x) {
            double sum = 1.0, term = 1.0;
            for (int k = 1; k < 50; k++) {
                term *= (x / (2.0 * k)) * (x / (2.0 * k));
                sum += term;
                if (term < 1e-12 * sum) break;
            }
            return sum;
        };
        double denom = bessel0(beta);
        double gain = 0.0;
        for (int i = 0; i < kTaps; i++) {
            int n = i - kMid;
            double h = (n == 0) ? 0.5 : sin(kPi * n / 2.0) / (kPi * n);
            double r = (double)n / kMid;
            double w = bessel0(beta * sqrt(std::max(0.0, 1.0 - r * r))) / denom;
            h *= w;
            if (n != 0 && (n % 2) == 0) h = 0.0;
            coef_[i] = h;
            gain += h;
        }
        // Unity gain at DC exactly.
        for (int i = 0; i < kTaps; i++) coef_[i] /= gain;
        // Odd-offset taps, folded by symmetry: odd_[j] multiplies x[mid - (2j+1)] + x[mid + (2j+1)].
        for (int j = 0; j < kOdd; j++) odd_[j] = (float)coef_[kMid + 2 * j + 1];
        center_ = (float)coef_[kMid];
        Reset();
    }
    void Reset() {
        for (int i = 0; i < 2 * kTaps; i++) hist_[i] = 0.f;
        pos_ = 0;
        phase_ = 0;
    }
    // Returns true when an output sample was produced (every second call).
    bool Push(float x, float* out) {
        // Doubled history so the newest kTaps samples are always contiguous.
        hist_[pos_] = x;
        hist_[pos_ + kTaps] = x;
        pos_ = (pos_ + 1 == kTaps) ? 0 : pos_ + 1;
        phase_ ^= 1;
        if (phase_) return false;
        const float* h = hist_ + pos_;  // h[0] oldest ... h[kTaps-1] newest
        float acc = center_ * h[kMid];
        for (int j = 0; j < kOdd; j++) acc += odd_[j] * (h[kMid - (2 * j + 1)] + h[kMid + (2 * j + 1)]);
        *out = acc;
        return true;
    }
    double Coef(int i) const { return coef_[i]; }

private:
    static constexpr int kOdd = (kMid + 1) / 2;
    double coef_[kTaps];
    float odd_[kOdd];
    float center_ = 0.5f;
    float hist_[2 * kTaps];
    int pos_ = 0, phase_ = 0;
};

// ---- Band layouts -------------------------------------------------------------------

enum class BandLayout { Scale, Iec, Musical };
enum class FreqScale { Log, Linear, Mel, Bark, Erb };

struct Band {
    double f1 = 0, f2 = 0, fc = 0;  // edges and centre, Hz
};

inline double ScaleFwd(FreqScale s, double f) {
    switch (s) {
        case FreqScale::Linear: return f;
        case FreqScale::Mel: return 2595.0 * log10(1.0 + f / 700.0);
        case FreqScale::Bark: return 26.81 * f / (1960.0 + f) - 0.53;  // Traunmueller
        case FreqScale::Erb: return 21.4 * log10(1.0 + 0.00437 * f);  // Glasberg & Moore
        default: return log(std::max(f, 1e-3));
    }
}
inline double ScaleInv(FreqScale s, double u) {
    switch (s) {
        case FreqScale::Linear: return u;
        case FreqScale::Mel: return 700.0 * (pow(10.0, u / 2595.0) - 1.0);
        case FreqScale::Bark: return 1960.0 * (u + 0.53) / (26.28 - u);
        case FreqScale::Erb: return (pow(10.0, u / 21.4) - 1.0) / 0.00437;
        default: return exp(u);
    }
}

// n bands evenly spaced on the chosen scale between fmin and fmax. Each band's
// centre is the midpoint of its edges in the scale's own units, so on Log it is
// the geometric mean.
inline std::vector<Band> ScaleBands(FreqScale s, double fmin, double fmax, int n) {
    std::vector<Band> out;
    n = std::max(1, n);
    double u0 = ScaleFwd(s, fmin), u1 = ScaleFwd(s, fmax);
    for (int i = 0; i < n; i++) {
        double a = u0 + (u1 - u0) * i / n;
        double b = u0 + (u1 - u0) * (i + 1) / n;
        Band bd;
        bd.f1 = ScaleInv(s, a);
        bd.f2 = ScaleInv(s, b);
        bd.fc = ScaleInv(s, 0.5 * (a + b));
        out.push_back(bd);
    }
    return out;
}

// IEC 61260-1:2014 / ANSI S1.11, base-10 system. G = 10^(3/10), reference
// 1000 Hz, bandwidth designator b (1 = octave, 3 = third-octave, ...).
//   b odd:  fm = fr G^(x/b)
//   b even: fm = fr G^((2x+1)/(2b))
//   edges:  fm G^(-1/2b), fm G^(+1/2b)
// A band is kept when its exact midband frequency is within a quarter of a
// band of [fmin, fmax], so the band labelled "20 Hz" (exactly 19.95 Hz) is in
// a 20 Hz - 20 kHz range, the way any RTA shows it.
inline std::vector<Band> IecBands(int b, double fmin, double fmax) {
    std::vector<Band> out;
    b = std::max(1, b);
    const double G = pow(10.0, 0.3);
    const double tol = pow(G, 1.0 / (4.0 * b));
    for (int x = -10 * b; x <= 10 * b; x++) {
        double fm = (b % 2) ? 1000.0 * pow(G, (double)x / b) : 1000.0 * pow(G, (2.0 * x + 1.0) / (2.0 * b));
        if (fm < fmin / tol || fm > fmax * tol) continue;
        Band bd;
        bd.fc = fm;
        bd.f1 = fm * pow(G, -1.0 / (2.0 * b));
        bd.f2 = fm * pow(G, 1.0 / (2.0 * b));
        out.push_back(bd);
    }
    return out;
}

// Equal temperament: one band per note (stepsPerOctave 12) or per quarter tone
// (24), centred on fm = A4 x 2^(k / steps), edges half a step either side.
inline std::vector<Band> MusicalBands(int stepsPerOctave, double a4, double fmin, double fmax) {
    std::vector<Band> out;
    int steps = (stepsPerOctave >= 24) ? 24 : 12;
    double half = pow(2.0, 0.5 / steps);
    int kLo = (int)floor(steps * log2(fmin / a4)) - 1;
    int kHi = (int)ceil(steps * log2(fmax / a4)) + 1;
    for (int k = kLo; k <= kHi; k++) {
        double fm = a4 * pow(2.0, (double)k / steps);
        if (fm < fmin * 0.9999 || fm > fmax * 1.0001) continue;
        Band bd;
        bd.fc = fm;
        bd.f1 = fm / half;
        bd.f2 = fm * half;
        out.push_back(bd);
    }
    return out;
}

// ---- Weighting curves (IEC 61672-1), dB at f -------------------------------------
inline double AWeightDb(double f) {
    double f2 = f * f;
    double ra = (12194.0 * 12194.0 * f2 * f2) /
                ((f2 + 20.6 * 20.6) * sqrt((f2 + 107.7 * 107.7) * (f2 + 737.9 * 737.9)) *
                 (f2 + 12194.0 * 12194.0));
    return 20.0 * log10(std::max(ra, 1e-30)) + 2.00;
}
inline double CWeightDb(double f) {
    double f2 = f * f;
    double rc = (12194.0 * 12194.0 * f2) / ((f2 + 20.6 * 20.6) * (f2 + 12194.0 * 12194.0));
    return 20.0 * log10(std::max(rc, 1e-30)) + 0.06;
}

enum class Weighting { Z, A, C };
enum class Detector { Rms, Peak };
enum class LevelRef { ThirdOctave, Band };

// ---- Spectrum engine ----------------------------------------------------------------
//
// Owns the three tiers (rings, decimators, FFTs), the per-band bin maps and
// the per-band dB offsets. Analyze() turns the newest window of each tier into
// one dBFS figure per band.
//
// Level calibration. Both detectors read a full-scale sine as 0 dBFS:
//   RMS:  mean square from the bins, 2 sum|X|^2 / (N sum w^2), which is
//         Parseval's theorem for a windowed block, plus 3.01 dB so a sine's
//         0.5 mean square reads 0 dB. Correct for noise and tones alike, as
//         long as the band covers the tone's main lobe.
//   Peak: the largest bin, 2|X| / sum w. Exact for a tone on a bin centre,
//         low by the window's scallop loss between bins.
// With Level Reference = third-octave, RMS bands are scaled to the power a
// 1/3-octave band at the same centre would hold (+10 log10(BW13 / BW)). That
// makes pink noise read flat and makes bar heights independent of how many
// bars there are, which is what a 1/3-octave RTA shows.
class SpectrumEngine {
public:
    struct Config {
        int sampleRate = 48000;
        int fftSize = 2048;
        int maxTier = 2;  // 0 = single resolution, 1 = add /4, 2 = add /4 and /16
        WindowKind window = WindowKind::Hann;
        BandLayout layout = BandLayout::Scale;
        FreqScale scale = FreqScale::Log;
        int octaveFraction = 6;     // IEC b, or 12 / 24 for Musical
        double fmin = 20.0, fmax = 20000.0;
        double a4 = 440.0;
        int bars = 32;              // used by Scale layout
        int maxBands = 2048;
        Weighting weighting = Weighting::Z;
        double tiltDbPerOct = 0.0;  // pivot 1 kHz
        Detector detector = Detector::Rms;
        LevelRef levelRef = LevelRef::ThirdOctave;
        double zoneDb[3] = {0, 0, 0};  // EQ preset, low (<300 Hz) / mid / high (>2.5 kHz)
    };

    struct BandMap {
        int tier = 0;
        int k0 = 1, k1 = 1;      // inclusive bin range
        float w0 = 1.f, w1 = 1.f; // weight of the first / last bin (partial overlap)
        float offsetDb = 0.f;    // calibration + weighting + tilt + EQ + level reference
        float peakOffsetDb = 0.f;// same for the Peak detector (no bandwidth term)
    };

    bool Configure(const Config& c) {
        cfg_ = c;
        int n = c.fftSize;
        if (!IsPow2(n) || n < 256) n = 2048;
        cfg_.fftSize = n;
        cfg_.maxTier = std::clamp(c.maxTier, 0, 2);
        double nyq = 0.5 * cfg_.sampleRate;
        double fmax = std::min(cfg_.fmax, nyq * 0.98);
        double fmin = std::clamp(cfg_.fmin, 1.0, fmax * 0.5);

        // Bands.
        switch (cfg_.layout) {
            case BandLayout::Iec: bands_ = IecBands(cfg_.octaveFraction, fmin, fmax); break;
            case BandLayout::Musical:
                bands_ = MusicalBands(cfg_.octaveFraction, cfg_.a4, fmin, fmax);
                break;
            default: bands_ = ScaleBands(cfg_.scale, fmin, fmax, cfg_.bars); break;
        }
        if ((int)bands_.size() > cfg_.maxBands) bands_.resize(cfg_.maxBands);
        if (bands_.empty()) bands_ = ScaleBands(FreqScale::Log, fmin, fmax, 1);
        for (auto& b : bands_) {  // a band can't extend past what any tier can see
            b.f2 = std::min(b.f2, nyq * 0.98);
            b.f1 = std::min(b.f1, b.f2 * 0.999);
        }

        window_.resize(n);
        WindowSums ws = BuildWindow(cfg_.window, n, window_.data());
        fft_.Init(n);
        re_.assign(n / 2 + 1, 0.f);
        im_.assign(n / 2 + 1, 0.f);
        scratch_.assign(n, 0.f);
        for (int t = 0; t < 3; t++) {
            ring_[t].Init(n * 2);
            power_[t].assign(n / 2 + 1, 0.f);
            fresh_[t] = 0;
            valid_[t] = false;
        }
        for (auto& d : dec_) d.Reset();

        // Calibration constants (see the class comment).
        const double rmsCal = 10.0 * log10(2.0 / (n * ws.sumSq)) + 10.0 * log10(2.0);
        const double peakCal = 20.0 * log10(2.0 / ws.sum);
        // Hop per tier before its FFT is worth redoing: the top tier every
        // frame, the decimated ones once N/16 new samples have arrived.
        hop_[0] = 1;
        hop_[1] = hop_[2] = std::max(16, n / 16);

        maps_.assign(bands_.size(), BandMap());
        tierUsed_[0] = true;
        tierUsed_[1] = tierUsed_[2] = false;
        // Half-width of the window's main lobe, in bins. A tone spreads that
        // far either side of its bin, so a band has to be at least a full
        // lobe wide (both halves) before a tone in the next band over stops
        // leaking into it.
        double lobe = 2.0;
        switch (cfg_.window) {
            case WindowKind::BlackmanHarris: lobe = 4.0; break;
            case WindowKind::FlatTop: lobe = 5.0; break;
            case WindowKind::Rectangular: lobe = 1.0; break;
            default: break;
        }
        for (size_t i = 0; i < bands_.size(); i++) {
            const Band& b = bands_[i];
            BandMap& m = maps_[i];
            double bw = std::max(b.f2 - b.f1, 1e-6);
            // Tier: the least decimated (fastest) one whose main lobe fits
            // inside the band, as long as the band sits inside that tier's
            // clean passband. Falls back to the finest tier the passband
            // allows, which is the best resolution there is.
            int tier = 0;
            for (int t = 0; t <= cfg_.maxTier; t++) {
                double fs = TierRate(t);
                if (b.f2 > 0.4 * fs) break;
                tier = t;
                if (2.0 * lobe * fs / n <= bw) break;
            }
            m.tier = tier;
            tierUsed_[tier] = true;
            double df = TierRate(tier) / n;
            double x1 = b.f1 / df, x2 = b.f2 / df;  // edges in bins
            int k0 = (int)floor(x1 + 0.5), k1 = (int)floor(x2 + 0.5);
            k0 = std::clamp(k0, 1, n / 2);
            k1 = std::clamp(k1, k0, n / 2);
            m.k0 = k0;
            m.k1 = k1;
            if (k0 == k1) {
                m.w0 = (float)std::clamp(x2 - x1, 0.0, 1.0);
                m.w1 = m.w0;
            } else {
                m.w0 = (float)std::clamp((k0 + 0.5) - x1, 0.0, 1.0);
                m.w1 = (float)std::clamp(x2 - (k1 - 0.5), 0.0, 1.0);
            }
            double off = 0.0;
            if (cfg_.weighting == Weighting::A) off += AWeightDb(b.fc);
            else if (cfg_.weighting == Weighting::C) off += CWeightDb(b.fc);
            off += cfg_.tiltDbPerOct * log2(b.fc / 1000.0);
            int zone = (b.fc < 300.0) ? 0 : (b.fc < 2500.0) ? 1 : 2;
            off += cfg_.zoneDb[zone];
            double refTerm = 0.0;
            if (cfg_.levelRef == LevelRef::ThirdOctave) {
                double bw13 = b.fc * (pow(2.0, 1.0 / 6.0) - pow(2.0, -1.0 / 6.0));
                refTerm = 10.0 * log10(bw13 / bw);
            }
            m.offsetDb = (float)(rmsCal + off + refTerm);
            m.peakOffsetDb = (float)(peakCal + off);
        }
        levelsDb_.assign(bands_.size(), -200.f);
        configured_ = true;
        return true;
    }

    // Mono samples in. Feeds the top tier directly and the decimator cascade
    // for the others (only as deep as a band actually needs).
    void Push(const float* x, int count) {
        if (!configured_) return;
        int deepest = tierUsed_[2] ? 2 : tierUsed_[1] ? 1 : 0;
        for (int i = 0; i < count; i++) {
            float v = x[i];
            ring_[0].Push(v);
            fresh_[0]++;
            if (deepest < 1) continue;
            float a, b2;
            if (!dec_[0].Push(v, &a)) continue;
            if (!dec_[1].Push(a, &b2)) continue;
            ring_[1].Push(b2);
            fresh_[1]++;
            if (deepest < 2) continue;
            float c, d;
            if (!dec_[2].Push(b2, &c)) continue;
            if (!dec_[3].Push(c, &d)) continue;
            ring_[2].Push(d);
            fresh_[2]++;
        }
    }

    // Runs the FFT for each tier that has enough new data, then rebuilds the
    // band levels. Returns true if anything was recomputed.
    //
    // fftOverride lets the caller run the transform somewhere else (the NPU):
    // it gets the windowed block and must fill re/im with bins 0..N/2, or
    // return false to fall back to the CPU FFT.
    template <typename FftOverride>
    bool Analyze(bool force, FftOverride&& fftOverride) {
        if (!configured_) return false;
        const int n = cfg_.fftSize;
        bool any = false;
        for (int t = 0; t < 3; t++) {
            if (!tierUsed_[t]) continue;
            if (!force && fresh_[t] < hop_[t]) continue;
            fresh_[t] = 0;
            ring_[t].Latest(scratch_.data(), n);
            for (int i = 0; i < n; i++) scratch_[i] *= window_[i];
            if (!fftOverride(scratch_.data(), n, re_.data(), im_.data()))
                fft_.Forward(scratch_.data(), re_.data(), im_.data());
            float* p = power_[t].data();
            for (int k = 0; k <= n / 2; k++) p[k] = re_[k] * re_[k] + im_[k] * im_[k];
            valid_[t] = true;
            any = true;
        }
        if (!any) return false;
        const bool peakDet = cfg_.detector == Detector::Peak;
        for (size_t i = 0; i < maps_.size(); i++) {
            const BandMap& m = maps_[i];
            const float* p = power_[m.tier].data();
            double v;
            if (peakDet) {
                float mx = 0.f;
                for (int k = m.k0; k <= m.k1; k++) mx = std::max(mx, p[k]);
                v = (double)DbFromPower(mx) + m.peakOffsetDb;  // 10log10(|X|^2) = 20log10|X|
            } else {
                double s;
                if (m.k0 == m.k1) {
                    s = p[m.k0] * m.w0;
                } else {
                    s = p[m.k0] * m.w0 + p[m.k1] * m.w1;
                    for (int k = m.k0 + 1; k < m.k1; k++) s += p[k];
                }
                v = (double)DbFromPower(s) + m.offsetDb;
            }
            levelsDb_[i] = (float)v;
        }
        return true;
    }
    bool Analyze(bool force) {
        return Analyze(force, [](const float*, int, float*, float*) { return false; });
    }

    // Loudest bin between fmin and fmax, refined by a parabola through the
    // log magnitudes of the bin and its neighbours (accurate to a fraction of
    // a bin for a steady tone). Uses the finest tier that covers each range.
    double DominantHz(double minLevelDbfs) const {
        double bestDb = -1e9, bestHz = 0.0;
        const int n = cfg_.fftSize;
        for (int t = 0; t < 3; t++) {
            if (!valid_[t]) continue;
            double fs = TierRate(t);
            double df = fs / n;
            // This tier owns [lo, hi): above the next tier's passband edge.
            double hi = (t == 0) ? std::min(cfg_.fmax, 0.49 * fs) : 0.4 * fs;
            double lo = (t < 2 && valid_[t + 1]) ? 0.4 * TierRate(t + 1) : cfg_.fmin;
            int k0 = std::max(2, (int)ceil(lo / df));
            int k1 = std::min(n / 2 - 2, (int)floor(hi / df));
            const float* p = power_[t].data();
            for (int k = k0; k <= k1; k++) {
                double db = DbFromPower(p[k]) + PeakCal();
                if (db > bestDb && p[k] >= p[k - 1] && p[k] >= p[k + 1]) {
                    double a = DbFromPower(p[k - 1]), b = DbFromPower(p[k]), c = DbFromPower(p[k + 1]);
                    double den = a - 2 * b + c;
                    double delta = (fabs(den) > 1e-9) ? 0.5 * (a - c) / den : 0.0;
                    bestDb = db;
                    bestHz = (k + std::clamp(delta, -0.5, 0.5)) * df;
                }
            }
        }
        return (bestDb >= minLevelDbfs) ? bestHz : 0.0;
    }

    double TierRate(int t) const { return cfg_.sampleRate / (double)(1 << (2 * t)); }
    int NumBands() const { return (int)bands_.size(); }
    const std::vector<Band>& Bands() const { return bands_; }
    const std::vector<BandMap>& Maps() const { return maps_; }
    const float* LevelsDb() const { return levelsDb_.data(); }
    const Config& Cfg() const { return cfg_; }
    bool TierUsed(int t) const { return tierUsed_[t]; }
    const Ring& TierRing(int t) const { return ring_[t]; }
    const float* Window() const { return window_.data(); }
    double PeakCal() const {
        double s = 0;
        for (float w : window_) s += w;
        return 20.0 * log10(2.0 / s);
    }
    // For the GPU workload: the newest window of each tier, unwindowed, and
    // whether the tier has moved on enough to be re-transformed.
    bool TakeTierBlock(int t, float* out, bool force) {
        if (!tierUsed_[t]) return false;
        if (!force && fresh_[t] < hop_[t]) return false;
        fresh_[t] = 0;
        ring_[t].Latest(out, cfg_.fftSize);
        return true;
    }

private:
    Config cfg_;
    bool configured_ = false;
    std::vector<Band> bands_;
    std::vector<BandMap> maps_;
    std::vector<float> window_, re_, im_, scratch_, levelsDb_;
    std::vector<float> power_[3];
    Ring ring_[3];
    HalfBand dec_[4];
    int fresh_[3] = {0, 0, 0};
    int hop_[3] = {1, 1, 1};
    bool valid_[3] = {false, false, false};
    bool tierUsed_[3] = {true, false, false};
    RealFft fft_;
};

// ---- Display mapping and ballistics ----------------------------------------------------
//
// Level in dBFS -> bar height 0..1 across [floor, ceiling], then ballistics in
// that (dB-linear) domain, which is how meters behave: the same number of dB
// per second falls the same distance anywhere on the scale.
//
// Every coefficient is computed from dt, the time since the previous update:
//   rise / exponential fall: y += (x - y) (1 - e^(-dt / tau))
//   linear fall:             y  = max(x, y - rate dt)
// so a frame that runs late moves further, by exactly the right amount.
enum class ReleaseKind { Exponential, Linear };

struct Ballistics {
    double attackMs = 10.0;
    ReleaseKind release = ReleaseKind::Exponential;
    double releaseMs = 70.0;      // tau, for Exponential
    double releaseDbPerSec = 20;  // for Linear
};

inline Ballistics BallisticsPreset(int preset) {
    // 0 snappy (closest to the 1.4 feel), 1 smooth, 2 analyzer, 3 VU,
    // 4 EBU / IEC 60268-10 Type II PPM, 5 DIN / Type I PPM.
    Ballistics b;
    switch (preset) {
        case 1: b.attackMs = 25; b.releaseMs = 180; break;
        case 2: b.attackMs = 10; b.release = ReleaseKind::Linear; b.releaseDbPerSec = 20; break;
        case 3: b.attackMs = 65; b.releaseMs = 65; break;  // 300 ms to 99%
        case 4: b.attackMs = 10; b.release = ReleaseKind::Linear; b.releaseDbPerSec = 24.0 / 2.8; break;
        case 5: b.attackMs = 5; b.release = ReleaseKind::Linear; b.releaseDbPerSec = 20.0 / 1.5; break;
        default: b.attackMs = 10; b.releaseMs = 70; break;
    }
    return b;
}

enum class Curve { Exponential, Knee, Power, Linear };

inline float ApplyCurve(Curve c, float x) {
    if (x <= 0.f) return 0.f;
    switch (c) {
        // Soft from the start, scaled so full scale still reaches the top.
        case Curve::Exponential: return std::min(1.f, (1.f - expf(-2.f * x)) / (1.f - expf(-2.f)));
        case Curve::Power: return std::min(1.f, powf(x, 0.6f));
        case Curve::Linear: return std::min(1.f, x);
        default: {
            const float knee = 0.7f;
            return (x <= knee) ? x : knee + (1.f - knee) * tanhf((x - knee) / (1.f - knee));
        }
    }
}

struct DisplayMap {
    float floorDb = -72.f, ceilDb = -12.f;
    float gainDb = 0.f;  // sensitivity + auto gain
    Curve curve = Curve::Knee;
    float ToNorm(float db) const {
        float x = (db + gainDb - floorDb) / std::max(1.f, ceilDb - floorDb);
        return std::clamp(ApplyCurve(curve, x), 0.f, 1.f);
    }
};

// One state per band.
inline float BallisticsStep(float y, float x, double dt, const Ballistics& b, float rangeDb) {
    if (x > y) {
        double a = 1.0 - exp(-dt * 1000.0 / std::max(0.1, b.attackMs));
        return (float)(y + (x - y) * a);
    }
    if (b.release == ReleaseKind::Linear) {
        float fall = (float)(b.releaseDbPerSec * dt / std::max(1.f, rangeDb));
        return std::max(x, y - fall);
    }
    double a = 1.0 - exp(-dt * 1000.0 / std::max(0.1, b.releaseMs));
    return (float)(y + (x - y) * a);
}

// Peak hold per bar: hangs for holdMs, then falls with gravity (accelerating,
// like a dropped object) or at a constant rate.
struct PeakHold {
    float level = 0.f, timer = 0.f, vel = 0.f;
};
inline void PeakHoldStep(PeakHold& p, float x, double dt, double holdMs, bool gravity, float gPerSec2,
                         float linPerSec) {
    if (x >= p.level) {
        p.level = x;
        p.timer = 0.f;
        p.vel = 0.f;
        return;
    }
    p.timer += (float)dt;
    if (p.timer * 1000.f < holdMs) return;
    if (gravity) {
        p.vel += gPerSec2 * (float)dt;
        p.level -= p.vel * (float)dt;
    } else {
        p.level -= linPerSec * (float)dt;
    }
    if (p.level < x) {
        p.level = x;
        p.vel = 0.f;
    }
    if (p.level < 0.f) p.level = 0.f;
}

// ---- Biquad (transposed direct form II) --------------------------------------------------
struct Biquad {
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    double z1 = 0, z2 = 0;
    inline double Run(double x) {
        double y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }
    void Reset() { z1 = z2 = 0; }
};

// BS.1770 K-weighting for any sample rate, from the analogue prototypes (the
// libebur128 / pyloudnorm derivation). At 48 kHz this reproduces the
// coefficients printed in the Recommendation to about 1e-8.
inline void KWeightingFilters(double fs, Biquad& shelf, Biquad& hp) {
    {
        const double f0 = 1681.974450955533, G = 3.999843853973347, Q = 0.7071752369554196;
        double K = tan(kPi * f0 / fs);
        double Vh = pow(10.0, G / 20.0);
        double Vb = pow(Vh, 0.4996667741545416);
        double a0 = 1.0 + K / Q + K * K;
        shelf.b0 = (Vh + Vb * K / Q + K * K) / a0;
        shelf.b1 = 2.0 * (K * K - Vh) / a0;
        shelf.b2 = (Vh - Vb * K / Q + K * K) / a0;
        shelf.a1 = 2.0 * (K * K - 1.0) / a0;
        shelf.a2 = (1.0 - K / Q + K * K) / a0;
    }
    {
        const double f0 = 38.13547087602444, Q = 0.5003270373238773;
        double K = tan(kPi * f0 / fs);
        double a0 = 1.0 + K / Q + K * K;
        hp.b0 = 1.0;
        hp.b1 = -2.0;
        hp.b2 = 1.0;
        hp.a1 = 2.0 * (K * K - 1.0) / a0;
        hp.a2 = (1.0 - K / Q + K * K) / a0;
    }
}

// ---- Loudness and true peak (ITU-R BS.1770-5 / EBU R128) --------------------------------
//
//   K-weighting -> mean square per channel -> sum with channel weights G_i
//   L = -0.691 + 10 log10(sum G_i z_i)   LUFS
// Momentary = 400 ms, short-term = 3 s (EBU Tech 3341), integrated = the mean
// of every 400 ms block (75% overlap, so one every 100 ms) above the absolute
// gate (-70 LUFS) and the relative gate (10 LU below the ungated mean).
// Integrated uses a 0.1 LU histogram of block loudness, the libebur128 trick,
// so memory and time stay constant however long a track runs.
//
// True peak: 4x oversampling through a 64-tap polyphase interpolator
// (Kaiser-windowed sinc, cut off at the original Nyquist), the maximum
// absolute value over every original and interpolated sample, in dBTP.
class LoudnessMeter {
public:
    static constexpr int kMaxCh = 8;
    static constexpr int kTpTaps = 16;  // per phase

    void Configure(int fs, int channels, const float* weights) {
        fs_ = std::max(8000, fs);
        ch_ = std::clamp(channels, 1, kMaxCh);
        for (int c = 0; c < ch_; c++) {
            w_[c] = weights ? weights[c] : 1.f;
            KWeightingFilters(fs_, shelf_[c], hp_[c]);
        }
        subLen_ = fs_ / 10;
        // Interpolator.
        const int L = 4, taps = L * kTpTaps;
        double beta = 7.0;
        auto bessel0 = [](double x) {
            double s = 1.0, t = 1.0;
            for (int k = 1; k < 50; k++) {
                t *= (x / (2.0 * k)) * (x / (2.0 * k));
                s += t;
            }
            return s;
        };
        double den = bessel0(beta);
        for (int i = 0; i < taps; i++) {
            double n = i - (taps - 1) / 2.0;
            double x = n / L;
            double h = (fabs(x) < 1e-12) ? 1.0 : sin(kPi * x) / (kPi * x);
            double r = n / ((taps - 1) / 2.0);
            h *= bessel0(beta * sqrt(std::max(0.0, 1.0 - r * r))) / den;
            // Phase p uses taps i = p + L j.
            tp_[i % L][i / L] = (float)h;
        }
        Reset();
    }
    void Reset() {
        for (int c = 0; c < ch_; c++) {
            shelf_[c].Reset();
            hp_[c].Reset();
            for (int j = 0; j < kTpTaps; j++) tpHist_[c][j] = 0.f;
        }
        tpPos_ = 0;
        subAcc_ = 0.0;
        subCount_ = 0;
        for (int i = 0; i < 30; i++) sub_[i] = 0.0;
        subHead_ = 0;
        subFilled_ = 0;
        for (int i = 0; i < kHist; i++) {
            histCount_[i] = 0;
            histEnergy_[i] = 0.0;
        }
        truePeak_ = 0.f;
        blocks_ = 0;
    }
    // Interleaved frames.
    void Process(const float* x, int frames, int stride) {
        for (int f = 0; f < frames; f++) {
            const float* s = x + (size_t)f * stride;
            double e = 0.0;
            for (int c = 0; c < ch_; c++) {
                double v = hp_[c].Run(shelf_[c].Run(s[c]));
                e += w_[c] * v * v;
                // True peak: shift in the original sample, evaluate 4 phases.
                tpHist_[c][tpPos_] = s[c];
            }
            for (int c = 0; c < ch_; c++) {
                truePeak_ = std::max(truePeak_, fabsf(s[c]));
                for (int p = 0; p < 4; p++) {
                    float acc = 0.f;
                    int idx = tpPos_;
                    for (int j = 0; j < kTpTaps; j++) {
                        acc += tp_[p][j] * tpHist_[c][idx];
                        idx = (idx == 0) ? kTpTaps - 1 : idx - 1;
                    }
                    truePeak_ = std::max(truePeak_, fabsf(acc));
                }
            }
            tpPos_ = (tpPos_ + 1 == kTpTaps) ? 0 : tpPos_ + 1;
            subAcc_ += e;
            if (++subCount_ >= subLen_) EndSubBlock();
        }
    }
    double Momentary() const { return Window(4); }
    double ShortTerm() const { return Window(30); }
    double Integrated() const {
        // Absolute gate already applied on entry; relative gate here.
        double sumE = 0.0;
        long long n = 0;
        for (int i = 0; i < kHist; i++) {
            sumE += histEnergy_[i];
            n += histCount_[i];
        }
        if (n == 0) return -HUGE_VAL;
        double ungated = -0.691 + 10.0 * log10(sumE / n);
        double rel = ungated - 10.0;
        int start = std::clamp((int)ceil((rel - kHistMin) * 10.0), 0, kHist);
        sumE = 0.0;
        n = 0;
        for (int i = start; i < kHist; i++) {
            sumE += histEnergy_[i];
            n += histCount_[i];
        }
        if (n == 0) return -HUGE_VAL;
        return -0.691 + 10.0 * log10(sumE / n);
    }
    double TruePeakDb() const { return 20.0 * log10(std::max(1e-10f, truePeak_)); }
    long long Blocks() const { return blocks_; }

private:
    static constexpr int kHist = 800;  // -70 .. +10 LUFS in 0.1 LU bins
    static constexpr double kHistMin = -70.0;

    double Window(int subs) const {
        if (subFilled_ < subs) return -HUGE_VAL;
        double s = 0.0;
        int i = subHead_;
        for (int k = 0; k < subs; k++) {
            i = (i == 0) ? 29 : i - 1;
            s += sub_[i];
        }
        double ms = s / ((double)subs * subLen_);
        return -0.691 + 10.0 * log10(std::max(ms, 1e-20));
    }
    void EndSubBlock() {
        sub_[subHead_] = subAcc_;
        subHead_ = (subHead_ + 1) % 30;
        if (subFilled_ < 30) subFilled_++;
        subAcc_ = 0.0;
        subCount_ = 0;
        if (subFilled_ >= 4) {
            double l = Window(4);
            if (l > kHistMin) {
                int bin = std::clamp((int)((l - kHistMin) * 10.0), 0, kHist - 1);
                histCount_[bin]++;
                histEnergy_[bin] += pow(10.0, (l + 0.691) / 10.0);
                blocks_++;
            }
        }
    }

    int fs_ = 48000, ch_ = 2, subLen_ = 4800;
    float w_[kMaxCh] = {};
    Biquad shelf_[kMaxCh], hp_[kMaxCh];
    float tp_[4][kTpTaps] = {};
    float tpHist_[kMaxCh][kTpTaps] = {};
    int tpPos_ = 0;
    float truePeak_ = 0.f;
    double subAcc_ = 0.0;
    int subCount_ = 0;
    double sub_[30] = {};
    int subHead_ = 0, subFilled_ = 0;
    int histCount_[kHist] = {};
    double histEnergy_[kHist] = {};
    long long blocks_ = 0;
};

// Stereo correlation, r = sum LR / sqrt(sum L^2 sum R^2), each sum an
// exponential average with time constant tau (SPAN averages over 500 ms).
// +1 mono, 0 unrelated, -1 out of phase.
class Correlation {
public:
    void Configure(int fs, double tauMs) {
        k_ = 1.0 - exp(-1000.0 / (std::max(1.0, tauMs) * std::max(1, fs)));
        Reset();
    }
    void Reset() { lr_ = ll_ = rr_ = 0.0; }
    void Push(float l, float r) {
        lr_ += (l * (double)r - lr_) * k_;
        ll_ += (l * (double)l - ll_) * k_;
        rr_ += (r * (double)r - rr_) * k_;
    }
    double Value() const {
        double d = sqrt(ll_ * rr_);
        return (d > 1e-12) ? std::clamp(lr_ / d, -1.0, 1.0) : 0.0;
    }

private:
    double k_ = 0.0, lr_ = 0.0, ll_ = 0.0, rr_ = 0.0;
};

}  // namespace ttdsp
