#!/bin/bash
# Extracts the embedded HLSL from v2b.cpp and compiles all six entries with DXC.
W=$(cd "$(dirname "$0")/.." && pwd)
D=$W/../dxc/bin/dxc
python3 -I - "$W/v2b.cpp" "$W/bench/embedded.hlsl" <<'PY'
import sys
s = open(sys.argv[1], encoding="utf-8").read()
a = s.index('R"HLSL(') + len('R"HLSL(')
b = s.index(')HLSL"', a)
open(sys.argv[2], "w").write(s[a:b])
PY
rc=0
for e in "vs_6_0 VSMain" "ps_6_0 PSMain" "cs_6_0 CsFft" "cs_6_0 CsBands" "cs_6_0 CsReduce" "cs_6_0 CsShape"; do
  set -- $e
  if $D -T $1 -E $2 "$W/bench/embedded.hlsl" -Fo /dev/null >/tmp/dxc_$2.log 2>&1; then echo "dxc $2 ok"; else echo "dxc $2 FAIL"; cat /tmp/dxc_$2.log; rc=1; fi
done
exit $rc
