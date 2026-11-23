import importlib.util
import pathlib
from pathlib import Path
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "quick4_projects", ROOT / "tools" / "quick4_projects.py"
)
MODULE = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
SPEC.loader.exec_module(MODULE)


class Quick4CaptureCommandTests(unittest.TestCase):
    def test_choir_requires_popup_capture_dependencies(self):
        self.assertEqual(
            MODULE.PROJECTS["skell-choir-vst"]["runtime_bins"],
            ["scrot", "openbox", "xprop"],
        )
        self.assertTrue(MODULE.PROJECTS["skell-choir-vst"]["window_manager"])
        self.assertEqual(MODULE.PROJECTS["skell-choir-vst"]["status"], "verified-ci-local-replay-blocked")

    def test_choir_status_docs_record_verified_ci_boundary(self):
        doc = (ROOT / "docs" / "PLUGIN_QUICK4_ROLLOUT.md").read_text()
        self.assertIn("VERIFIED IN CI", doc)
        self.assertIn("A local C++ replay was attempted", doc)

    def test_window_manager_is_started_inside_xvfb(self):
        command = MODULE.capture_command(
            pathlib.Path("/build/SkellyChoirQuick4"),
            pathlib.Path("/tmp/quick4"),
            window_manager=True,
        )
        joined = " ".join(command)
        self.assertIn("xvfb-run", command)
        self.assertIn("openbox --sm-disable", joined)
        self.assertIn("xprop -root _NET_SUPPORTING_WM_CHECK", joined)
        self.assertIn('exec "$@"', joined)

    def test_other_plugins_keep_their_existing_capture_command(self):
        command = MODULE.capture_command(
            pathlib.Path("/build/OtherQuick4"),
            pathlib.Path("/tmp/other"),
            window_manager=False,
        )
        self.assertNotIn("openbox", " ".join(command))
        self.assertEqual(Path(command[-3]), Path("/build/OtherQuick4"))
        self.assertEqual(command[-2], "--output")
        self.assertEqual(Path(command[-1]), Path("/tmp/other"))


if __name__ == "__main__":
    unittest.main()
