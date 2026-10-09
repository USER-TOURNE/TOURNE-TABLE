#!/bin/bash
# usage: syntax_check_w.sh <workdir> <x86_64|i686>   (checks <workdir>/v2b.cpp; TU kept in <workdir>)
S=/tmp/claude-0/-home-claude/3e030293-ab48-53ab-8384-51f504be00af/scratchpad
W="$1"; ARCH="${2:-x86_64}"
TU=$W/tu_$ARCH.cpp
python3 -I $S/tools/make_test_tu.py "$W/v2b.cpp" "$TU" >/dev/null || exit 2
RES=$(clang -print-resource-dir)
M=$S/mingw/mingw-w64-headers
clang++ --target=$ARCH-w64-mingw32 -fsyntax-only -std=c++23 -nostdinc -nostdinc++ \
  -isystem $S/sysroot/cxx -isystem $S/llvm18/libcxx/include -isystem $RES/include \
  -isystem $S/sysroot/include -isystem $M/include -isystem $M/crt -I $S/sysroot/stub \
  -DUNICODE -D_UNICODE -DWINVER=0x0A00 -D_WIN32_WINNT=0x0A00 -DNTDDI_VERSION=0x0A00000C \
  -D__USE_MINGW_ANSI_STDIO=0 -DWH_MOD -DWH_MOD_ID=L\"tourne-table-beta-build\" \
  -include windhawk_api.h -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers \
  -Wno-sign-compare -Wno-unused-function -ferror-limit=60 "$TU" 2>&1 | grep -v "g_albumArtFetchPending\|^1 warning generated\|^ *[0-9]* | \|^ *| *\^"
