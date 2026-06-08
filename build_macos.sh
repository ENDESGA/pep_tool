#!/usr/bin/env bash
#
# build_macos.sh — build pep_tool on macOS without altar.
#
# The official build uses `altar` (the H-language build tool), which only ships
# Linux/Windows binaries. This script does what `altar ... setup` does by hand:
# it fetches the declared dependencies, then compiles pep_tool.h with clang.
#
# The H framework (H.h) only implements Linux + Windows code paths. macOS is
# POSIX, so we patch a *local copy* of H.h (never the project source) to route
# the macOS build through the Linux paths. Three minimal changes are applied;
# see PATCH sections below.
#
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
BUILD="$ROOT/compile_macos_build"   # matches the compile_* .gitignore entry
OUT="$ROOT/pep_tool"

# Dependency sources (from setup.altar / framework.altar manifests).
H_URL="https://raw.githubusercontent.com/H-language/H/main/H.h"
PEP_URL="https://raw.githubusercontent.com/ENDESGA/pep/main/pep.h"
STBI_URL="https://raw.githubusercontent.com/nothings/stb/master/stb_image.h"
STBIW_URL="https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h"

echo "==> preparing $BUILD"
mkdir -p "$BUILD"

fetch() {  # fetch <url> <dest> — only download if missing
	local url="$1" dest="$2"
	if [ ! -f "$dest" ]; then
		echo "    fetching $(basename "$dest")"
		curl -fsSL "$url" -o "$dest"
	fi
}

echo "==> fetching dependencies"
fetch "$H_URL"    "$BUILD/H.h"
fetch "$PEP_URL"  "$BUILD/pep.h"
fetch "$STBI_URL" "$BUILD/stb_image.h"
fetch "$STBIW_URL" "$BUILD/stb_image_write.h"

echo "==> patching H.h for macOS (idempotent)"
python3 - "$BUILD/H.h" <<'PY'
import sys
p = sys.argv[1]
s = open(p, encoding="utf-8").read()
orig = s

# PATCH 1: make the __APPLE__ branch share the Linux POSIX code paths.
apple_old = (
	'#elif defined( __APPLE__ )\n'
	'\t#undef OS_MACOS\n'
	'\t#define OS_MACOS 1\n'
	'\t#define OS_NAME "macOS"\n'
)
apple_new = apple_old + (
	'\t// macOS is POSIX: share the Linux code paths (mmap/dirent/stat/...).\n'
	'\t#undef OS_LINUX\n'
	'\t#define OS_LINUX 1\n'
	'\t#include <sys/stat.h>\n'
	'\t#include <sys/mman.h>\n'
	'\t#include <dirent.h>\n'
	'\t#include <fcntl.h>\n'
	'\t#include <unistd.h>\n'
)
if apple_new not in s:
	assert apple_old in s, "PATCH 1 anchor not found (H.h layout changed upstream)"
	s = s.replace(apple_old, apple_new, 1)

# PATCH 2: exclude the Linux-only mremap fast-path on macOS (no mremap there);
# the generic alloc-copy-free fallback below handles the resize instead.
mre_old = '\t#if OS_LINUX\n\t\tif( preserve is yes )\n\t\t{\n\t\t\tanon ref const new_alloc = mremap('
mre_new = '\t#if OS_LINUX && !OS_MACOS\n\t\tif( preserve is yes )\n\t\t{\n\t\t\tanon ref const new_alloc = mremap('
if mre_new not in s:
	assert mre_old in s, "PATCH 2 anchor not found (H.h layout changed upstream)"
	s = s.replace(mre_old, mre_new, 1)

# PATCH 2b: enable the alloc-copy-free fallback on macOS too.
cp_old = '\t#if OS_WINDOWS\n\t\tif( preserve is yes )\n\t\t{\n\t\t\tn8 const old_size'
cp_new = '\t#if OS_WINDOWS || OS_MACOS\n\t\tif( preserve is yes )\n\t\t{\n\t\t\tn8 const old_size'
if cp_new not in s:
	assert cp_old in s, "PATCH 2b anchor not found (H.h layout changed upstream)"
	s = s.replace(cp_old, cp_new, 1)

# PATCH 3: macOS WEXITSTATUS() takes the address of its argument, so it can't
# wrap the rvalue returned by command(); store it in a variable first.
we_old = '\t\tout to( out_state, WEXITSTATUS( command( command_bytes ) ) );'
we_new = ('\t\tint command_status = command( command_bytes );\n'
          '\t\tout to( out_state, WEXITSTATUS( command_status ) );')
if we_new not in s:
	assert we_old in s, "PATCH 3 anchor not found (H.h layout changed upstream)"
	s = s.replace(we_old, we_new, 1)

if s != orig:
	open(p, "w", encoding="utf-8").write(s)
	print("    patches applied")
else:
	print("    already patched")
PY

echo "==> compiling"
cp "$ROOT/pep_tool.h" "$BUILD/pep_tool_main.c"
clang -std=gnu11 -I"$BUILD" -O2 -o "$OUT" "$BUILD/pep_tool_main.c" -lm

echo "==> done: $OUT"
file "$OUT"
"$OUT" version
