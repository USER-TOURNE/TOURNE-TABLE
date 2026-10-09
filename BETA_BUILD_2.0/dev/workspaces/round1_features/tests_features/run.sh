#!/bin/bash
# Runs the feature-fix harnesses. Usage: run.sh <new v2b.cpp> <original v2b.cpp>
set -u
D=$(cd "$(dirname "$0")" && pwd)
NEW=${1:-$D/../../v2b.cpp}
OLD=${2:-$D/../../../v2b.cpp}
B=$D/build
mkdir -p $D/gen $B
sed -n '/^\/\/ ---- track timeline: begin/,/^\/\/ ---- track timeline: end/p' "$NEW" > $D/gen/timeline_new.inc
awk '/^std::atomic<bool> g_tlValid/{p=1} p{print} p&&/^}/{exit}' "$OLD" > $D/gen/timeline_old.inc
sed -n '/^\/\/ ---- waterfall scroll: begin/,/^\/\/ ---- waterfall scroll: end/p' "$NEW" > $D/gen/waterfall_new.inc
wc -l $D/gen/*.inc
SAN="-fsanitize=address,undefined -fno-sanitize-recover=all -D_GLIBCXX_ASSERTIONS -g -O1"
echo "== timeline (ASan/UBSan)"; g++ -std=c++20 $SAN -I$D $D/test_timeline.cpp -o $B/tl -lpthread && $B/tl
echo "== timeline (TSan)"; g++ -std=c++20 -fsanitize=thread -g -O1 -I$D $D/test_timeline.cpp -o $B/tl_tsan -lpthread && $B/tl_tsan | tail -2
echo "== waterfall, OLD code (ASan/UBSan, _GLIBCXX_ASSERTIONS)"; g++ -std=c++20 $SAN -I$D $D/test_waterfall.cpp -o $B/wf && $B/wf old 2>&1 | grep -v "^    #" | head -12
echo "== waterfall, OLD code without the bounds assertion (shows stale rows)"; g++ -std=c++20 -g -O1 -I$D $D/test_waterfall.cpp -o $B/wf_plain && $B/wf_plain old
echo "== waterfall, NEW code"; $B/wf
echo "== layout"; g++ -std=c++20 $SAN $D/test_layout.cpp -o $B/lay && $B/lay
sed -n '/^static winrt::event_token g_gsmtcMediaPropsToken/,/^static bool g_gsmtcStarted/p' "$NEW" > $D/gen/gsmtc_new.inc
sed -n '/^static winrt::event_token g_gsmtcMediaPropsToken/,/^static bool g_gsmtcStarted/p' "$OLD" > $D/gen/gsmtc_old.inc
TS="-std=c++20 -fsanitize=thread -g -O1 -Wno-tsan -I$D"
echo "== GSMTC region on a C++/WinRT mock, OLD (TSan)"; g++ $TS -DUSE_OLD $D/test_gsmtc_mock.cpp -o $B/gs_old -lpthread && TSAN_OPTIONS=halt_on_error=0 $B/gs_old 2>&1 | grep -E "WARNING: ThreadSanitizer|^    #[01] .*(Refresh|Setup|Init)|swaps done|SUMMARY" | sort | uniq -c | sort -rn | head -12
echo "== GSMTC region on a C++/WinRT mock, NEW (TSan)"; g++ $TS $D/test_gsmtc_mock.cpp -o $B/gs_new -lpthread && $B/gs_new 2>&1 | grep -E "WARNING|stale|swaps done|SUMMARY"; echo "exit=$?"
