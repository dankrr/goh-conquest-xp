# Building and testing

The supported Steam AppID is `400750`; the supported executable SHA-256 is `4b6a807ad867c4f6eb1019376224d74fa3a6ac048355739ab0b76d73c7c0a267`.

On Arch Linux install Clang, LLD, the x86_64 MinGW-w64 headers and runtime (for example `clang`, `lld`, `mingw-w64-headers`, and `mingw-w64-crt`). Python 3 is required for installation. Run:

```sh
./build.sh
python3 -m unittest discover -s tests -v
```

`MINGW_SYSROOT` can override `/usr/x86_64-w64-mingw32`. The build produces `build/version.dll`; `python3 install.py install "/path/to/game" --built` explicitly installs that build. The release installer pins the shipped, live-tested DLL by hash. The source build can differ byte-for-byte because of toolchain and PE metadata; do not assume a rebuild is identical to the release DLL.

The native policy test can be run with `cc -std=c11 -I src tests/policy_test.c -o /tmp/goh-policy-test && /tmp/goh-policy-test`. `tests/test_proton.sh` builds a synthetic Windows harness; set `MINGW_SYSROOT`, `PROTON_PATH`, `STEAM_COMPAT_DATA_PATH` to a **disposable** Proton compatdata directory, and `STEAM_COMPAT_CLIENT_INSTALL_PATH` before running it. To run the original-instruction emulation tests, install the Python packages `pefile` and `unicorn`, then run `python3 tests/gate_emulation.py "/path/to/game/binaries/x64/call_to_arms.exe"` and `python3 tests/result_path_emulation.py "/path/to/game/binaries/x64/call_to_arms.exe"`. They read the executable but do not modify it. No game files are included in this release.

These tests do not establish live awards or synchronization.
