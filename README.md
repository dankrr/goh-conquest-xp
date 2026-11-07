# Conquest XP with Mods

Earn player XP in **single-player Dynamic Conquest** with your Workshop mods enabled. The game calculates and awards XP normally.

**Works with:** Gates of Hell: Ostfront `1.0.6.7-aaa36ed4d` on Linux using Proton. Steam must be online. Other game versions are not supported.

**Single-player only.** Remove the mod before co-op, multiplayer, or anti-cheat use.

## Install

1. [Download the ZIP](https://github.com/dankrr/goh-conquest-xp/archive/refs/heads/main.zip) and extract it. Close the game.
2. In Steam, right-click the game and select **Manage → Browse local files**. Copy the folder path.
3. Open a terminal in the extracted folder and run this command, replacing the example path with yours:

   ```sh
   python3 install.py install "/path/to/Call to Arms - Gates of Hell"
   ```

4. In Steam, open the game’s **Properties → General → Launch Options** and enter:

   ```text
   WINEDLLOVERRIDES="version=n,b" %command%
   ```

   If you already use launch options, add the override without adding a second `%command%`.
5. Launch the game, enable your mods as usual, and play solo Dynamic Conquest.

The installer needs Python 3 and will not overwrite an existing `version.dll`. If you use another loader, remove it with its own uninstaller first.

## Uninstall

Close the game and run this command from the extracted folder, using the same game path:

```sh
python3 install.py uninstall "/path/to/Call to Arms - Gates of Hell"
```

Remove the Steam launch override if you no longer need it. Your saves and logs are kept.

## Good to know

- A live game with four mods awarded XP and updated the local Steam stats cache. Persistence after restarting and Steam server synchronization have not been verified.
- Achievement support is experimental, turned off by default, and unverified.
- If XP is not awarded, check `binaries/x64/goh_scoped.log` in the game folder.

For developer instructions, see [Building and testing](BUILDING.md). Licensed under [MIT](LICENSE); see the [third-party notices](THIRD_PARTY_NOTICES.md).
