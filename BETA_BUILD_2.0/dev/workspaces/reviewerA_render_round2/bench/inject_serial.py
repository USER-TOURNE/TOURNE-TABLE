# Build-only shim: until reviewer C's p2_term.cpp (which owns g_termGridSerial)
# is merged, define the counter in the *generated* v2b.cpp so the syntax checks
# can run. Never applied to sources; a no-op once C's definition is present.
import sys
p = sys.argv[1]
s = open(p, encoding="utf-8").read()
if "uint32_t g_termGridSerial" not in s:
    a = "VizTermGrid g_termGrid;\n"
    assert s.count(a) == 1
    s = s.replace(a, a + "uint32_t g_termGridSerial = 0;  // [wA build shim; reviewer C owns this]\n")
    open(p, "w", encoding="utf-8", newline="\n").write(s)
    print("injected g_termGridSerial shim")
