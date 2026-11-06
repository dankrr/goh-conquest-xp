#!/usr/bin/env bash
set -euo pipefail
here=$(cd -- "$(dirname -- "$0")" && pwd)
sysroot=${MINGW_SYSROOT:-/usr/x86_64-w64-mingw32}
if [[ ! -f "$sysroot/include/windows.h" ]]; then
  printf 'MinGW-w64 headers not found at %s; set MINGW_SYSROOT.\n' "$sysroot" >&2
  exit 1
fi
mkdir -p "$here/build"
clang --target=x86_64-w64-windows-gnu --sysroot="$sysroot" -fuse-ld=lld \
  -O2 -ffreestanding -fno-builtin -shared -nostdlib -Wl,-e,DllMain \
  "$here/src/version.c" "$here/src/version.def" -o "$here/build/version.dll" \
  -lbcrypt -lkernel32 -luser32
printf 'Built %s\n' "$here/build/version.dll"
