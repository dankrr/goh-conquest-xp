import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('installer', ROOT / 'install.py')
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)

class InstallerTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.game = Path(self.tmp.name) / 'game'
        self.d = self.game / 'binaries' / 'x64'
        self.d.mkdir(parents=True)
        (self.d / 'call_to_arms.exe').write_bytes(b'disposable test executable')
        self.old_hash = mod.GAME_HASH
        mod.GAME_HASH = mod.digest(self.d / 'call_to_arms.exe')
        self.addCleanup(setattr, mod, 'GAME_HASH', self.old_hash)

    def test_clean_install_remove(self):
        self.assertEqual(mod.binary_dir(self.game), self.d)
        mod.install(self.d)
        self.assertEqual(mod.digest(self.d / 'version.dll'), mod.DLL_HASH)
        self.assertIn(b'xp=1', (self.d / mod.CONFIG_NAME).read_bytes())
        self.assertIn(b'achievements=0', (self.d / mod.CONFIG_NAME).read_bytes())
        (self.d / 'goh_scoped.log').write_text('keep')
        mod.uninstall(self.d)
        self.assertFalse((self.d / 'version.dll').exists())
        self.assertFalse((self.d / mod.CONFIG_NAME).exists())
        self.assertFalse((self.d / mod.STATE_NAME).exists())
        self.assertEqual((self.d / 'goh_scoped.log').read_text(), 'keep')

    def test_unknown_dll_and_existing_config_refused(self):
        original = self.d / 'version.dll'
        original.write_bytes(b'other mod')
        with self.assertRaises(RuntimeError): mod.install(self.d)
        self.assertEqual(original.read_bytes(), b'other mod')
        original.unlink()
        (self.d / mod.CONFIG_NAME).write_text('existing config')
        with self.assertRaises(RuntimeError): mod.install(self.d)
        self.assertFalse(original.exists())

    def test_unsupported_game_and_dangling_symlink_refused(self):
        (self.d / 'call_to_arms.exe').write_bytes(b'new game build')
        with self.assertRaises(RuntimeError): mod.install(self.d)
        self.assertFalse((self.d / 'version.dll').exists())
        (self.d / 'call_to_arms.exe').write_bytes(b'disposable test executable')
        (self.d / 'version.dll').symlink_to('missing.dll')
        with self.assertRaises(RuntimeError): mod.install(self.d)
        self.assertTrue((self.d / 'version.dll').is_symlink())

    def test_tampered_dll_is_not_removed(self):
        mod.install(self.d)
        (self.d / 'version.dll').write_bytes(b'modified')
        with self.assertRaises(RuntimeError): mod.uninstall(self.d)
        self.assertTrue((self.d / mod.STATE_NAME).exists())
        self.assertEqual((self.d / 'version.dll').read_bytes(), b'modified')

    def test_modified_config_preserved(self):
        mod.install(self.d)
        (self.d / mod.CONFIG_NAME).write_text('custom')
        mod.uninstall(self.d)
        self.assertEqual((self.d / mod.CONFIG_NAME).read_text(), 'custom')
        self.assertFalse((self.d / 'version.dll').exists())

    def test_explicit_build_round_trip(self):
        custom = ROOT / 'build' / 'version.dll'
        if not custom.is_file(): self.skipTest('build/version.dll is absent')
        mod.install(self.d, built=True)
        self.assertEqual(mod.digest(self.d / 'version.dll'), mod.digest(custom))
        mod.uninstall(self.d)
        self.assertFalse((self.d / 'version.dll').exists())

    def test_tampered_state_is_not_removed(self):
        mod.install(self.d)
        state = self.d / mod.STATE_NAME
        content = json.loads(state.read_text())
        content['dll_sha256'] = '0' * 64
        state.write_text(json.dumps(content))
        with self.assertRaises(RuntimeError): mod.uninstall(self.d)
        self.assertTrue((self.d / 'version.dll').exists())

if __name__ == '__main__': unittest.main()
