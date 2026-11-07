# Conquest XP with Mods

Earn normal player XP in single-player Dynamic Conquest with Workshop mods enabled. The mod keeps the game's XP calculations and Steam progression path; Steam must be connected.

**Supported:** Call to Arms – Gates of Hell: Ostfront `1.0.6.7-aaa36ed4d` (Steam AppID 400750), on Linux with Proton. The executable must have SHA-256 `4b6a807ad867c4f6eb1019376224d74fa3a6ac048355739ab0b76d73c7c0a267`. Other builds are refused. Use only for solo Conquest; uninstall before co-op, multiplayer or anti-cheat use.

A live test with four scene-affecting mods awarded 179 XP through the game's normal update path and updated the local Steam stats cache. Persistence after restarting and Steam server synchronization have not been verified. Achievement support is experimental, disabled by default and has not been verified.

## Install

1. On GitHub, select **Code → Download ZIP** and extract it. Close the game and editor.
2. In Steam, right-click the game → **Manage → Browse local files**. Copy the game folder's path.
3. In a terminal, change to the extracted folder and run:

   ```sh
   python3 install.py install "/path/to/Call to Arms - Gates of Hell"
   ```

4. In the game's Steam **Properties → General → Launch Options**, set:

   ```text
   WINEDLLOVERRIDES="version=n,b" %command%
   ```

   If you already have launch options, incorporate the override without adding a second `%command%`.
5. Keep Steam online, enable your mods in the normal game menu, and start a solo Dynamic Conquest.

The installer checks the game and release DLL hashes. It refuses to overwrite an existing `version.dll`, configuration or install record; remove any other loader through its own uninstaller first. It does not edit saves or Steam settings.

## Uninstall

Close the game, then run from the extracted folder:

```sh
python3 install.py uninstall "/path/to/Call to Arms - Gates of Hell"
```

Remove the launch override if nothing else needs it. Uninstall verifies the installed DLL, removes its own files, and keeps logs, saves and any edited config. The installer never replaces an existing DLL, so there is no original DLL to restore. The log is `binaries/x64/goh_scoped.log`.

## Build from source

On Arch Linux install Clang, LLD, the x86_64 MinGW-w64 headers and runtime (for example `clang`, `lld`, `mingw-w64-headers`, and `mingw-w64-crt`). Python 3 is required for installation. Run:

```sh
./build.sh
python3 -m unittest discover -s tests -v
```

`MINGW_SYSROOT` can override `/usr/x86_64-w64-mingw32`. The build produces `build/version.dll`; `python3 install.py install "/path/to/game" --built` explicitly installs that build. The release installer pins the shipped, live-tested DLL by hash. The source build can differ byte-for-byte because of toolchain and PE metadata; do not assume a rebuild is identical to the release DLL.

The native policy test can be run with `cc -std=c11 -I src tests/policy_test.c -o /tmp/goh-policy-test && /tmp/goh-policy-test`. `tests/test_proton.sh` builds a synthetic Windows harness; set `MINGW_SYSROOT`, `PROTON_PATH`, `STEAM_COMPAT_DATA_PATH` to a **disposable** Proton compatdata directory, and `STEAM_COMPAT_CLIENT_INSTALL_PATH` before running it. To run the original-instruction emulation tests, install the Python packages `pefile` and `unicorn`, then run `python3 tests/gate_emulation.py "/path/to/game/binaries/x64/call_to_arms.exe"` and `python3 tests/result_path_emulation.py "/path/to/game/binaries/x64/call_to_arms.exe"`. They read the executable but do not modify it. No game files are included in this release.

These tests do not establish live awards or synchronization.

## License

MIT. See [LICENSE](LICENSE) and [third-party notices](THIRD_PARTY_NOTICES.md).
