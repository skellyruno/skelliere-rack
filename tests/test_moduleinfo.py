import importlib.util
import json
import tempfile
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("fix_vst3_moduleinfo", ROOT / "tools" / "fix_vst3_moduleinfo.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

class ModuleInfoTests(unittest.TestCase):
    def test_trailing_comma_and_quoted_lookalikes(self):
        source = '{"text": ", } literal", "slash": "\\\\", "items": [1,2,], "nested":{"x":true,},}'
        parsed = json.loads(module.without_trailing_commas(source))
        self.assertEqual(parsed["text"], ", } literal")
        self.assertEqual(parsed["slash"], "\\")
        self.assertEqual(parsed["items"], [1,2])
        self.assertTrue(parsed["nested"]["x"])

    def test_valid_json_is_unmodified(self):
        source = '{"items":[1,2],"text":"hello"}'
        self.assertEqual(module.without_trailing_commas(source), source)

    def test_repairs_file_and_is_idempotent(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder)/"moduleinfo.json"
            path.write_text('{"Name":"Rack","Classes":[{"CID":"ABC",},],}')
            self.assertTrue(module.normalize(path))
            self.assertEqual(json.loads(path.read_text())["Classes"][0]["CID"],"ABC")
            self.assertFalse(module.normalize(path))

if __name__ == "__main__":
    unittest.main()
