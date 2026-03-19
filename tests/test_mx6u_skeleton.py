import os
import pathlib
import subprocess
import tempfile
import unittest


REPO_ROOT = pathlib.Path(__file__).resolve().parents[1]
MX6U_DIR = REPO_ROOT / "MX6U"


class Mx6uSkeletonTests(unittest.TestCase):
    def test_expected_project_files_exist(self):
        expected_paths = [
            MX6U_DIR / "Makefile",
            MX6U_DIR / "README.md",
            MX6U_DIR / "include" / "mx6u_app.h",
            MX6U_DIR / "src" / "main.c",
        ]

        for path in expected_paths:
            self.assertTrue(path.exists(), f"missing expected file: {path}")

    def test_makefile_exposes_cross_compile_variables(self):
        makefile = (MX6U_DIR / "Makefile").read_text(encoding="utf-8")

        self.assertIn("CROSS_COMPILE ?=", makefile)
        self.assertIn("SYSROOT ?=", makefile)
        self.assertIn("APP_NAME :=", makefile)
        self.assertIn("BUILD_DIR :=", makefile)

    def test_makefile_supports_windows_local_validation(self):
        makefile = (MX6U_DIR / "Makefile").read_text(encoding="utf-8")

        self.assertIn("ifeq ($(OS),Windows_NT)", makefile)
        self.assertIn("rmdir /S /Q", makefile)

    def test_readme_describes_nfs_workflow_with_mqtt_publish_path(self):
        readme = (MX6U_DIR / "README.md").read_text(encoding="utf-8")

        self.assertIn("/home/lan/linux/nfs/smart-greenhouse/mx6u", readme)
        self.assertIn("/mnt/nfs/smart-greenhouse/mx6u", readme)
        self.assertIn("broker.emqx.io", readme)
        self.assertIn("smart-greenhouse/node1/state", readme)
        self.assertIn("make", readme)

    def test_main_program_compiles_and_runs_on_host_gcc(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            compile_result = subprocess.run(
                ["mingw32-make", "-C", str(MX6U_DIR), "clean", "all", "CROSS_COMPILE=", "MX6U_ENABLE_OPENCV=0"],
                capture_output=True,
                text=True,
                check=False,
            )

            self.assertEqual(compile_result.returncode, 0, compile_result.stderr)

            run_result = subprocess.run(
                [str(MX6U_DIR / "build" / "mx6u_skeleton.exe")],
                input="ENV,N=1,T=25.3,H=60.1,SOIL=41.2,L=1234,CO2=567\n",
                capture_output=True,
                text=True,
                check=False,
                env={**os.environ, "MX6U_SERIAL_DEVICE": "stdin", "MX6U_MQTT_ENABLE": "0"},
            )

            self.assertEqual(run_result.returncode, 0, run_result.stderr)
            self.assertIn("MX6U parser ready", run_result.stdout)
            self.assertIn("ENV node=1", run_result.stdout)
            self.assertIn("SENSOR 1 ONLINE", run_result.stdout)
            self.assertIn("EXECUTOR 4 UNKNOWN", run_result.stdout)
            self.assertIn("VISION SNAPSHOT", run_result.stdout)
            self.assertIn("available=NO", run_result.stdout)
            self.assertIn("DECISION SNAPSHOT", run_result.stdout)
            self.assertIn("RECOMMEND fan=", run_result.stdout)
            self.assertIn("MQTT SNAPSHOT", run_result.stdout)
            self.assertIn("reason=disabled", run_result.stdout)


if __name__ == "__main__":
    unittest.main()
