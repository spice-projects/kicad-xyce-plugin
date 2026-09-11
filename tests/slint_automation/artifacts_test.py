import json
import os
import tempfile
import unittest
from io import BytesIO
from pathlib import Path

from slint_automation.slint_application import SlintApplication
from slint_automation.test_case import SlintTestCase


class FakeProcess:

    def __init__(self) -> None:
        # pid is read by the correlation label when present
        self.pid = 4242

    def poll(self) -> int | None:
        # report a running process
        return None

    def terminate(self) -> None:
        # accept the graceful termination request
        pass

    def wait(self, timeout: float | None = None) -> int:
        # report a clean exit
        return 0


class FakeArtifactClient:

    def __init__(self, fail_screenshot: bool = False, fail_tree: bool = False) -> None:
        # fail_screenshot makes take_screenshot raise on demand
        self._fail_screenshot = fail_screenshot
        # fail_tree makes get_element_tree raise on demand
        self._fail_tree = fail_tree

    def take_screenshot(self) -> bytes:
        # raise when the caller requested a screenshot failure
        if self._fail_screenshot:
            raise RuntimeError("screenshot unavailable")
        # serve the canned png payload
        return b"png-bytes"

    def get_element_tree(self) -> dict:
        # raise when the caller requested a tree failure
        if self._fail_tree:
            raise RuntimeError("tree unavailable")
        # serve the canned element tree
        return {"totalCount": 3, "elements": []}


class ArtifactCollectionChecks(unittest.TestCase):

    def test_collect_artifacts_writes_expected_files(self) -> None:
        # arrange
        with tempfile.TemporaryDirectory() as tmp:
            stdout = BytesIO(b"app stdout line\n")
            stderr = BytesIO(b"mcp server listening\n")
            app = SlintApplication(FakeProcess(), FakeArtifactClient(), 1, stdout, stderr)
            # act
            directory = app.collect_artifacts(f"{tmp}/failing-test")
            # assert
            self.assertEqual(directory, f"{tmp}/failing-test")
            self.assertEqual((Path(directory) / "screenshot.png").read_bytes(), b"png-bytes")
            self.assertEqual(json.loads((Path(directory) / "ui-tree.json").read_text())["totalCount"], 3)
            self.assertEqual((Path(directory) / "stdout.txt").read_text(), "app stdout line\n")
            self.assertEqual((Path(directory) / "stderr.txt").read_text(), "mcp server listening\n")

    def test_collect_artifacts_tolerates_failures(self) -> None:
        # arrange
        with tempfile.TemporaryDirectory() as tmp:
            app = SlintApplication(FakeProcess(), FakeArtifactClient(fail_screenshot=True, fail_tree=True), 1)
            # act
            directory = app.collect_artifacts(f"{tmp}/degraded-test")
            # assert: the text artifacts exist while the best effort ones are skipped
            self.assertFalse((Path(directory) / "screenshot.png").exists())
            self.assertFalse((Path(directory) / "ui-tree.json").exists())

    def test_screenshot_writes_png_bytes(self) -> None:
        # arrange
        with tempfile.TemporaryDirectory() as tmp:
            app = SlintApplication(FakeProcess(), FakeArtifactClient(), 1)
            # act
            app.screenshot(f"{tmp}/shot.png")
            # assert
            self.assertEqual((Path(tmp) / "shot.png").read_bytes(), b"png-bytes")

    def test_stdout_text_reads_captured_output(self) -> None:
        # arrange
        app = SlintApplication(FakeProcess(), FakeArtifactClient(), 1, BytesIO(b"captured line\n"), BytesIO(b"captured error\n"))
        # act
        stdout = app.stdout_text()
        stderr = app.stderr_text()
        # assert
        self.assertEqual(stdout, "captured line\n")
        self.assertEqual(stderr, "captured error\n")

    def test_stdout_text_without_capture_returns_empty(self) -> None:
        # arrange
        app = SlintApplication(FakeProcess(), FakeArtifactClient(), 1)
        # act
        stdout = app.stdout_text()
        stderr = app.stderr_text()
        # assert
        self.assertEqual(stdout, "")
        self.assertEqual(stderr, "")


class DeliberatelyFailingCase(SlintTestCase):

    def setUp(self) -> None:
        # arrange: initialize the base test case hooks
        super().setUp()
        # arrange: start the session around the fake application
        self.start_session(SlintApplication(FakeProcess(), FakeArtifactClient(), 1))

    def run_and_fail(self) -> None:
        # act: fail the inner test so its teardown collects the artifacts
        self.fail("deliberate failure for artifact collection")


class FailureArtifactChecks(unittest.TestCase):

    def test_failure_collects_artifacts(self) -> None:
        # arrange
        with tempfile.TemporaryDirectory() as tmp:
            # arrange: route the artifact root into the temp directory
            os.environ["SLINT_TEST_ARTIFACTS"] = tmp
            self.addCleanup(os.environ.pop, "SLINT_TEST_ARTIFACTS", None)
            case = DeliberatelyFailingCase("run_and_fail")
            # act: run the failing inner case through a real test result
            result = unittest.TestResult()
            case.run(result)
            # assert: the inner test failed and the artifacts were collected
            self.assertEqual(len(result.failures), 1)
            directory = Path(tmp) / case.id()
            self.assertTrue((directory / "screenshot.png").exists())
            self.assertEqual((directory / "screenshot.png").read_bytes(), b"png-bytes")
            self.assertTrue((directory / "ui-tree.json").exists())
            self.assertIn("totalCount", (directory / "ui-tree.json").read_text())
