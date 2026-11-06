#!/usr/bin/env bash
set -euo pipefail
here=$(cd -- "$(dirname -- "$0")" && pwd)
root=$(cd -- "$here/.." && pwd)
sysroot=${MINGW_SYSROOT:-/usr/x86_64-w64-mingw32}
out=${DIAGNOSTIC_TEST_DIR:-"$root/build/test"}
mkdir -p "$out"
cat > "$out/diagtest.def" <<'DEF'
LIBRARY diagtest
EXPORTS
 test_lifecycle_bridges
 test_bridge_init
 test_bridge_end
 test_scoped_setup
 test_scoped_init
 test_scoped_end
 test_scoped_enabled
 test_prepare
 test_builder
 test_page
 test_setting
 test_runtime_policy
 test_patch_stub
 test_set_game
 test_gate
 test_result
DEF
clang --target=x86_64-w64-windows-gnu --sysroot="$sysroot" -fuse-ld=lld -O2 -ffreestanding -fno-builtin -DDIAGNOSTIC_TEST -shared -nostdlib -Wl,-e,DllMain "$root/src/version.c" "$root/src/version.def" -o "$out/diagtest.dll" -lbcrypt -lkernel32 -luser32
llvm-dlltool -m i386:x86-64 -d "$out/diagtest.def" -l "$out/libdiagtest.a"
clang --target=x86_64-w64-windows-gnu --sysroot="$sysroot" -fuse-ld=lld -O2 -ffreestanding -nostdlib -Wl,-e,mainCRTStartup "$here/hook_test.c" "$out/libdiagtest.a" -lkernel32 -o "$out/hook_test.exe"
if [[ -n ${PROTON_PATH:-} ]]; then
 : "${STEAM_COMPAT_DATA_PATH:?Set to isolated Proton compatdata directory}"
 : "${STEAM_COMPAT_CLIENT_INSTALL_PATH:?Set to Steam installation directory}"
 WINEDEBUG=-all "$PROTON_PATH" run "$out/hook_test.exe"
 echo 'PASS: solo session selector, lifecycle bridges, gate and result wrappers under Proton'
else
 echo "Harness built at $out/hook_test.exe; set PROTON_PATH and isolated Steam compatibility variables to execute."
fi
