"""Catches the things Windows' own shader compiler (fxc / d3dcompiler_47)
rejects but DXC accepts, so a DXC-clean shader can't fail on a real PC again.

Usage: python3 fxc_lint.py <mod.wh.cpp | shader.hlsl>

Checks the embedded HLSL for:
  * function-like macros with no parameters (`#define F() ...`), which the
    fxc preprocessor rejects outright;
  * fxc reserved words used as identifiers (`pass`, `technique`, `sampler`,
    `line`, `point`, ...), a hard syntax error in fxc;
  * non-ASCII characters, which fxc can refuse even inside comments.
"""
import re
import sys

RESERVED = set("""
asm asm_fragment auto catch centroid char class compile compile_fragment CompileShader const_cast delete
discard dword dynamic_cast enum explicit export friend fxgroup goto half inline interface line lineadj
linear long matrix mutable namespace new noperspective NULL operator packoffset pass pixelfragment
PixelShader point precise private protected public reinterpret_cast sample sampler shared short signed
sizeof snorm stateblock stateblock_state static_cast string technique technique10 technique11 template
texture this throw triangle triangleadj try typedef typename uniform union unorm unsigned using vector
vertexfragment VertexShader virtual volatile
""".split())


def shader_text(path):
    s = open(path, encoding="utf-8").read()
    if 'R"HLSL(' in s:
        a = s.index('R"HLSL(') + len('R"HLSL(')
        return s[a:s.index(')HLSL"', a)]
    return s


def main():
    src = shader_text(sys.argv[1]).split("\n")
    problems = []
    for n, line in enumerate(src, 1):
        if re.search(r"[^\x00-\x7f]", line):
            problems.append((n, "non-ASCII character", line))
        if re.match(r"\s*#\s*define\s+\w+\(\s*\)", line):
            problems.append((n, "zero-parameter function-like macro", line))
        code = re.sub(r"//.*", "", line)
        if code.lstrip().startswith("#"):
            continue
        code = re.sub(r'"[^"]*"', "", code)
        for w in re.findall(r"(?<![\w.])([A-Za-z_]\w*)", code):
            if w in RESERVED:
                problems.append((n, f"fxc reserved word '{w}'", line))
    for n, what, line in problems:
        print(f"line {n}: {what}: {line.strip()[:100]}")
    print("fxc lint:", "FAILED" if problems else "clean", f"({len(src)} lines)")
    sys.exit(1 if problems else 0)


if __name__ == "__main__":
    main()
