import unittest

from slint_automation.assertions import expect
from slint_automation.errors import LocatorError, SlintAssertionError
from slint_automation.locator import Locator


class SequencedClient:

    def __init__(self, handles: dict[str, list[dict]], properties_sequence: list[dict]) -> None:
        # handles maps the requested id to the canned handle list
        self._handles = handles
        # properties_sequence supplies the properties returned per poll
        self._properties_sequence = properties_sequence
        # _calls counts the property reads so far
        self._calls = 0

    def find_elements_by_id(self, elements_id: str) -> list[dict]:
        # return the canned handles for the requested id
        return self._handles.get(elements_id, [])

    def get_element_properties(self, element_handle: dict) -> dict:
        # serve the next canned properties in sequence
        index = min(self._calls, len(self._properties_sequence) - 1)
        # count the property read for sequencing
        self._calls += 1
        # return the canned properties for this poll
        return self._properties_sequence[index]


class ExpectationChecks(unittest.TestCase):

    def test_to_have_text_passes_immediately(self) -> None:
        # arrange
        client = SequencedClient({"App::status": [{"index": "7", "generation": "1"}]}, [{"accessibleValue": "Ready"}])
        locator = Locator(client, "App::status")
        # act / assert
        expect(locator).to_have_text("Ready", timeout=0.5)

    def test_to_have_text_polls_until_state_changes(self) -> None:
        # arrange
        client = SequencedClient({"App::status": [{"index": "7", "generation": "1"}]}, [{"accessibleValue": "Running"}, {"accessibleValue": "Simulation complete"}])
        locator = Locator(client, "App::status")
        # act / assert
        expect(locator).to_have_text("Simulation complete", timeout=2.0, poll_interval=0.05)

    def test_to_have_text_times_out_with_context(self) -> None:
        # arrange
        client = SequencedClient({"App::status": [{"index": "7", "generation": "1"}]}, [{"accessibleValue": "Running"}])
        locator = Locator(client, "App::status")
        # act
        with self.assertRaises(SlintAssertionError) as context:
            expect(locator).to_have_text("Ready", timeout=0.2, poll_interval=0.05)
        # assert
        self.assertIn("to have text 'Ready'", str(context.exception))
        self.assertIn("last observed: text 'Running'", str(context.exception))
        self.assertIn("waited: 0.2 seconds", str(context.exception))

    def test_negated_to_have_text_passes_when_different(self) -> None:
        # arrange
        client = SequencedClient({"App::status": [{"index": "7", "generation": "1"}]}, [{"accessibleValue": "Running"}])
        locator = Locator(client, "App::status")
        # act / assert
        expect(locator).not_.to_have_text("Ready", timeout=0.5)

    def test_negated_to_have_text_times_out_when_equal(self) -> None:
        # arrange
        client = SequencedClient({"App::status": [{"index": "7", "generation": "1"}]}, [{"accessibleValue": "Error"}])
        locator = Locator(client, "App::status")
        # act
        with self.assertRaises(SlintAssertionError) as context:
            expect(locator).not_.to_have_text("Error", timeout=0.2, poll_interval=0.05)
        # assert
        self.assertIn("not to have text 'Error'", str(context.exception))

    def test_negated_to_have_text_passes_when_element_missing(self) -> None:
        # arrange
        client = SequencedClient({}, [])
        locator = Locator(client, "App::missing")
        # act / assert
        expect(locator).not_.to_have_text("Ready", timeout=0.5)

    def test_to_exist_passes_when_present(self) -> None:
        # arrange
        client = SequencedClient({"App::button": [{"index": "3", "generation": "1"}]}, [])
        locator = Locator(client, "App::button")
        # act / assert
        expect(locator).to_exist(timeout=0.5)

    def test_to_exist_times_out_when_missing(self) -> None:
        # arrange
        client = SequencedClient({}, [])
        locator = Locator(client, "App::missing")
        # act
        with self.assertRaises(SlintAssertionError) as context:
            expect(locator).to_exist(timeout=0.2, poll_interval=0.05)
        # assert
        self.assertIn("to exist", str(context.exception))
        self.assertIn("last observed: missing", str(context.exception))

    def test_negated_to_exist_passes_when_missing(self) -> None:
        # arrange
        client = SequencedClient({}, [])
        locator = Locator(client, "App::missing")
        # act / assert
        expect(locator).not_.to_exist(timeout=0.5)

    def test_negated_to_exist_times_out_when_present(self) -> None:
        # arrange
        client = SequencedClient({"App::button": [{"index": "3", "generation": "1"}]}, [])
        locator = Locator(client, "App::button")
        # act / assert
        with self.assertRaises(SlintAssertionError):
            expect(locator).not_.to_exist(timeout=0.2, poll_interval=0.05)

    def test_to_be_visible_passes_with_positive_opacity(self) -> None:
        # arrange
        client = SequencedClient({"App::panel": [{"index": "9", "generation": "1"}]}, [{"computedOpacity": 1.0}])
        locator = Locator(client, "App::panel")
        # act / assert
        expect(locator).to_be_visible(timeout=0.5)

    def test_to_be_visible_times_out_with_omitted_opacity(self) -> None:
        # arrange: the server omits zero valued fields so opacity 0.0 is absent
        client = SequencedClient({"App::panel": [{"index": "9", "generation": "1"}]}, [{}])
        locator = Locator(client, "App::panel")
        # act
        with self.assertRaises(SlintAssertionError) as context:
            expect(locator).to_be_visible(timeout=0.2, poll_interval=0.05)
        # assert
        self.assertIn("to be visible", str(context.exception))
        self.assertIn("computed opacity 0.0", str(context.exception))

    def test_negated_to_be_visible_passes_when_opacity_zero(self) -> None:
        # arrange
        client = SequencedClient({"App::panel": [{"index": "9", "generation": "1"}]}, [{}])
        locator = Locator(client, "App::panel")
        # act / assert
        expect(locator).not_.to_be_visible(timeout=0.5)

    def test_to_be_enabled_passes_when_flag_present(self) -> None:
        # arrange
        client = SequencedClient({"App::button": [{"index": "3", "generation": "1"}]}, [{"accessibleEnabled": True}])
        locator = Locator(client, "App::button")
        # act / assert
        expect(locator).to_be_enabled(timeout=0.5)

    def test_to_be_enabled_times_out_when_flag_absent(self) -> None:
        # arrange: the server omits false valued fields so absence means false
        client = SequencedClient({"App::button": [{"index": "3", "generation": "1"}]}, [{}])
        locator = Locator(client, "App::button")
        # act
        with self.assertRaises(SlintAssertionError) as context:
            expect(locator).to_be_enabled(timeout=0.2, poll_interval=0.05)
        # assert
        self.assertIn("to be enabled", str(context.exception))
        self.assertIn("last observed: enabled False", str(context.exception))

    def test_negated_to_be_enabled_passes_when_flag_absent(self) -> None:
        # arrange
        client = SequencedClient({"App::button": [{"index": "3", "generation": "1"}]}, [{}])
        locator = Locator(client, "App::button")
        # act / assert
        expect(locator).not_.to_be_enabled(timeout=0.5)

    def test_to_have_property_passes_when_equal(self) -> None:
        # arrange
        client = SequencedClient({"App::button": [{"index": "3", "generation": "1"}]}, [{"accessibleRole": "Button"}])
        locator = Locator(client, "App::button")
        # act / assert
        expect(locator).to_have_property("accessibleRole", "Button", timeout=0.5)

    def test_to_have_property_times_out_when_different(self) -> None:
        # arrange
        client = SequencedClient({"App::button": [{"index": "3", "generation": "1"}]}, [{"accessibleRole": "Button"}])
        locator = Locator(client, "App::button")
        # act
        with self.assertRaises(SlintAssertionError) as context:
            expect(locator).to_have_property("accessibleRole", "Text", timeout=0.2, poll_interval=0.05)
        # assert
        self.assertIn("to have property 'accessibleRole' == 'Text'", str(context.exception))
        self.assertIn("last observed: accessibleRole 'Button'", str(context.exception))

    def test_negated_to_have_property_passes_when_different(self) -> None:
        # arrange
        client = SequencedClient({"App::button": [{"index": "3", "generation": "1"}]}, [{"accessibleRole": "Button"}])
        locator = Locator(client, "App::button")
        # act / assert
        expect(locator).not_.to_have_property("accessibleRole", "Text", timeout=0.5)

    def test_to_have_opacity_passes_when_equal(self) -> None:
        # arrange
        client = SequencedClient({"App::icon": [{"index": "9", "generation": "1"}]}, [{"computedOpacity": 1.0}])
        locator = Locator(client, "App::icon")
        # act / assert
        expect(locator).to_have_opacity(1.0, timeout=0.5)

    def test_to_have_opacity_times_out_when_different(self) -> None:
        # arrange
        client = SequencedClient({"App::icon": [{"index": "9", "generation": "1"}]}, [{"computedOpacity": 0.4000000059604645}])
        locator = Locator(client, "App::icon")
        # act
        with self.assertRaises(SlintAssertionError) as context:
            expect(locator).to_have_opacity(1.0, timeout=0.2, poll_interval=0.05)
        # assert
        self.assertIn("to have opacity 1.0", str(context.exception))
        self.assertIn("last observed: opacity 0.4", str(context.exception))

    def test_to_have_opacity_reports_only_test_frames(self) -> None:
        # arrange
        client = SequencedClient({"App::icon": [{"index": "9", "generation": "1"}]}, [{"computedOpacity": 0.4000000059604645}])
        locator = Locator(client, "App::icon")
        # arrange: capture the raised failure manually since assertRaises strips the traceback
        failure = None
        try:
            # act: assert a wrong opacity with a tiny timeout
            expect(locator).to_have_opacity(1.0, timeout=0.2, poll_interval=0.05)
        except SlintAssertionError as error:
            failure = error
        # arrange: collect the reported traceback frames
        frames = []
        entry = failure.__traceback__
        while entry is not None:
            frames.append(entry)
            entry = entry.tb_next
        # assert: the framework internals are hidden so only the test frame is reported
        self.assertEqual(len(frames), 1)
        self.assertIn("assertions_test", frames[0].tb_frame.f_code.co_filename)

    def test_negated_to_have_opacity_passes_when_different(self) -> None:
        # arrange
        client = SequencedClient({"App::icon": [{"index": "9", "generation": "1"}]}, [{"computedOpacity": 0.4000000059604645}])
        locator = Locator(client, "App::icon")
        # act / assert
        expect(locator).not_.to_have_opacity(1.0, timeout=0.5)


class NegationAbsenceChecks(unittest.TestCase):

    def test_negated_assertions_surface_ambiguous_locators(self) -> None:
        # arrange: a locator matching two elements cannot be asserted safely
        client = SequencedClient({"App::status": [{"index": "1", "generation": "1"}, {"index": "2", "generation": "1"}]}, [{"accessibleValue": "Ready"}])
        locator = Locator(client, "App::status")
        # act / assert
        with self.assertRaises(LocatorError):
            expect(locator).not_.to_have_text("Ready", timeout=0.2, poll_interval=0.05)

    def test_negated_assertions_pass_on_absent_elements(self) -> None:
        # arrange: a locator matching no element
        client = SequencedClient({"App::status": []}, [])
        locator = Locator(client, "App::status")
        # act / assert
        expect(locator).not_.to_have_text("Ready", timeout=0.2, poll_interval=0.05)
