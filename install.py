#!/usr/bin/env python3
"""Install or remove the Proton release without overwriting other DLLs."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import sys
import tempfile

GAME_HASH = '4b6a807ad867c4f6eb1019376224d74fa3a6ac048355739ab0b76d73c7c0a267'
DLL_HASH = '89209e95417a02032aa9bd23f6665d478988847b87455e4368a921cc979f40c8'
STATE_NAME = 'goh-conquest-xp-install.json'
CONFIG_NAME = 'goh_scoped.ini'
ROOT = Path(__file__).resolve().parent

def digest(path):
    h = hashlib.sha256()
    with path.open('rb') as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b''):
            h.update(chunk)
    return h.hexdigest()

def exists(path):
    return path.exists() or path.is_symlink()

def regular(path):
    return path.is_file() and not path.is_symlink()

def game_running():
    proc = Path('/proc')
    if not proc.is_dir():
        raise RuntimeError('Linux /proc is unavailable; close the game and run this on Linux.')
    for entry in proc.glob('[0-9]*/comm'):
        try:
            if entry.read_text().strip().lower().startswith('call_to_arms'):
                return True
        except OSError:
            continue
    return False

def binary_dir(path):
    p = Path(path).expanduser().resolve(strict=True)
    if (p / 'binaries' / 'x64').is_dir():
        p = p / 'binaries' / 'x64'
    if not p.is_dir() or not regular(p / 'call_to_arms.exe'):
        raise RuntimeError('Choose the game folder or its binaries/x64 directory.')
    return p

def install(directory, built=False):
    exe = directory / 'call_to_arms.exe'
    dll, config, state = (directory / n for n in ('version.dll', CONFIG_NAME, STATE_NAME))
    if digest(exe) != GAME_HASH:
        raise RuntimeError('Unsupported game executable. No files were changed.')
    source = ROOT / ('build/version.dll' if built else 'version.dll')
    if not regular(source):
        raise RuntimeError('The requested DLL is missing. Build it first if using --built.')
    source_hash = digest(source)
    if not built and source_hash != DLL_HASH:
        raise RuntimeError('The release DLL has changed.')
    if any(exists(p) for p in (dll, config, state)):
        raise RuntimeError('version.dll, the config, or install state already exists. Nothing was overwritten.')
    config_bytes = (ROOT / 'settings.ini').read_bytes()
    config_hash = hashlib.sha256(config_bytes).hexdigest()
    record = (json.dumps({'version': 1, 'dll_sha256': source_hash, 'config_sha256': config_hash, 'source': 'built' if built else 'release'}, sort_keys=True) + '\n').encode()
    installed = []
    # Temporary files live on the same filesystem so hard-link creation is atomic
    # and fails if another process creates a destination first.
    try:
        with tempfile.TemporaryDirectory(prefix='.goh-xp-', dir=directory) as temp:
            stage = Path(temp)
            shutil.copyfile(source, stage / 'version.dll')
            if digest(stage / 'version.dll') != source_hash:
                raise RuntimeError('Staged DLL failed its checksum.')
            (stage / CONFIG_NAME).write_bytes(config_bytes)
            (stage / STATE_NAME).write_bytes(record)
            for name in ('version.dll', CONFIG_NAME, STATE_NAME):
                os.link(stage / name, directory / name)
                installed.append(directory / name)
    except BaseException:
        for path in reversed(installed):
            try:
                path.unlink()
            except FileNotFoundError:
                pass
        raise
    print('Installed. XP is enabled; achievements are off. Set the Steam launch option shown in README.md.')

def uninstall(directory):
    dll, config, state = (directory / n for n in ('version.dll', CONFIG_NAME, STATE_NAME))
    if not regular(state) or not regular(dll):
        raise RuntimeError('Installed DLL or install state is missing or is a symlink; nothing changed.')
    record = json.loads(state.read_text())
    installed_hash = record.get('dll_sha256')
    source = record.get('source')
    if (record.get('version') != 1 or source not in ('release', 'built')
            or not isinstance(installed_hash, str) or len(installed_hash) != 64
            or (source == 'release' and installed_hash != DLL_HASH) or digest(dll) != installed_hash):
        raise RuntimeError('The installed DLL or install state has changed; nothing was removed.')
    config_hash = record.get('config_sha256')
    if not isinstance(config_hash, str) or len(config_hash) != 64:
        raise RuntimeError('Invalid install state; nothing removed.')
    dll.unlink()
    if regular(config) and digest(config) == config_hash:
        config.unlink()
    elif exists(config):
        print('Modified config preserved:', config)
    state.unlink()
    print('Removed the release DLL. Logs and game saves were left in place.')

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('action', choices=('install', 'uninstall'))
    parser.add_argument('game_folder', help='Steam game folder, or its binaries/x64 directory')
    parser.add_argument('--built', action='store_true', help='install your own build/version.dll instead of the pinned release')
    args = parser.parse_args()
    if game_running():
        raise RuntimeError('Exit the game and editor before installing or removing the mod.')
    folder = binary_dir(args.game_folder)
    if args.action == 'uninstall':
        if args.built:
            raise RuntimeError('--built applies only to installation.')
        uninstall(folder)
    else:
        install(folder, built=args.built)

if __name__ == '__main__':
    try:
        main()
    except (OSError, ValueError, RuntimeError, KeyError) as exc:
        print('Error:', exc, file=sys.stderr)
        sys.exit(1)
