import sys, re
src, dst = sys.argv[1], sys.argv[2]
L = open(src, encoding='utf-8').read().split('\n')
def find(pat, start=0):
    for i in range(start, len(L)):
        if re.match(pat, L[i]): return i
    raise SystemExit('pattern not found: ' + pat)
a = find(r'^void FetchAlbumArtColorAsync\(\) \{')
b = find(r'^std::vector<BYTE> g_mediaIconPixels\[4\];', a)
stub = '''// ---- winrt region stubbed for the syntax check ----
void FetchAlbumArtColorAsync() {}
void RefreshMediaPlaybackStatus() {}
void SetupGsmtcSessionListener() {}
void InitGsmtcListener() {}
static bool g_gsmtcStarted = false;
std::thread* g_mediaCmdThread = nullptr;
std::atomic<bool> g_mediaCmdPending{false};
std::atomic<int> g_mediaCmdQueued{-1};
void SendMediaCommand(int cmd) { (void)cmd; }'''.split('\n')
out = L[:a] + stub + L[b:]
out = [l for l in out if not re.match(r'^#include <winrt/', l) and not re.match(r'^using namespace winrt::', l)]
# Keep line count stable-ish isn't needed; report mapping offset.
open(dst, 'w', encoding='utf-8').write('\n'.join(out))
print('stubbed lines', a+1, '-', b, 'removed', b-a, 'inserted', len(stub))
