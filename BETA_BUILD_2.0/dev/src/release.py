"""Turns the 2.0 beta build (v2b.cpp) into the catalog release: the published
@id, name and description, and the README without the test-build wording."""
import os, sys
S = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
src = open(os.path.join(S, "v2b.cpp"), encoding="utf-8").read()

def rep(old, new, count=1):
    global src
    n = src.count(old)
    if n != count:
        sys.exit(f"anchor count {n} != {count}:\n{old[:200]}")
    src = src.replace(old, new)

rep("// @id                  tourne-table-desktop-audio-visualizer-scope\n",
    "// @id                  tourne-table-desktop-audio-visualizer\n")
rep("// @name                Tourne'Table [Audio Visualizer] (Beta 2.0)\n",
    "// @name                Tourne'Table [Audio Visualizer]\n")
a = src.index("// @description         BETA BUILD.")
b = src.index("\n", a)
src = src[:a] + ("// @description         A real-time audio visualizer for the Windows desktop. Advanced settings "
                 "without sacrificing resource efficiency. Near-headless rendering with CPU optimization for audio capture.") + src[b:]
rep("## ✧ NEW IN 2.0 (BETA)\n", "## ✧ NEW IN 2.0\n")
rep("This is a test build. It installs alongside the release build and is not for publishing. "
    "The performance figures further down were measured on 1.x and haven't been re-measured on 2.0 yet.",
    "The performance figures further down were measured on 1.x and haven't been re-measured on 2.0 yet.")
open(os.path.join(S, "release.cpp"), "w", encoding="utf-8", newline="\n").write(src)
print("wrote release.cpp", src.count("\n"), "lines")
