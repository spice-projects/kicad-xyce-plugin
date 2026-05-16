import os
import sys

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

from unittest import TestCase
from unittest.mock import MagicMock, patch

from PySide6.QtQuick import QQuickView
from PySide6.QtWidgets import QApplication, QDialog

from plugin.simulation_dialog import (
    DCSimulationParameters,
    OpSimulationParameters,
    SimulationDialog,
    TransientSchedulePoint,
    TransientSimulationParameters,
)

_app = QApplication.instance() or QApplication(sys.argv)


def _make_dialog(initial_parameters=None) -> SimulationDialog:
    """Create a SimulationDialog with a mock QML root so tests can call slots directly."""
    dialog = SimulationDialog.__new__(SimulationDialog)
    # call QDialog.__init__ without triggering the full SimulationDialog.__init__
    QDialog.__init__(dialog)
    dialog._initial_parameters = initial_parameters
    dialog._result = None
    dialog._root = MagicMock()
    return dialog


def _make_dialog_with_accept(initial_parameters=None) -> SimulationDialog:
    dialog = _make_dialog(initial_parameters)
    dialog.accept = MagicMock()
    return dialog


class TestSimulationDialogConstruction(TestCase):

    def test_dialog_can_be_instantiated(self):
        # act — full construction path with Qt offscreen
        dialog = SimulationDialog()
        # assert
        self.assertIsInstance(dialog, SimulationDialog)
        dialog.reject()

    def test_dialog_result_is_none_initially(self):
        # act
        dialog = SimulationDialog()
        # assert
        self.assertIsNone(dialog._result)
        dialog.reject()

    def test_on_qml_ready_skips_when_not_ready(self):
        # arrange
        dialog = _make_dialog()
        dialog._qml_view = MagicMock()
        # act — non-Ready status should be a no-op
        dialog._on_qml_ready(QQuickView.Status.Loading)
        # assert — _root was not re-assigned by this call (mock stays)
        self.assertIsInstance(dialog._root, MagicMock)

    def test_apply_initial_parameters_with_none(self):
        # arrange
        dialog = _make_dialog(initial_parameters=None)
        # act
        dialog._apply_initial_parameters()
        # assert — setProperty was called for tab index (defaults to transient=1)
        dialog._root.setProperty.assert_any_call("initialTabIndex", 1)

    def test_apply_initial_parameters_selects_dc_tab(self):
        # arrange
        params = DCSimulationParameters("LIN", "VIN", "0", "5", "0.1", "", tuple(), "", "", "", "", "", "")
        dialog = _make_dialog(initial_parameters=params)
        # act
        dialog._apply_initial_parameters()
        # assert — DC tab is index 2 (QML order: 0=OP, 1=Transient, 2=DC)
        dialog._root.setProperty.assert_any_call("initialTabIndex", 2)

    def test_apply_initial_parameters_selects_op_tab(self):
        # arrange
        params = OpSimulationParameters()
        dialog = _make_dialog(initial_parameters=params)
        # act
        dialog._apply_initial_parameters()
        # assert — OP tab is index 0 (QML order: 0=OP, 1=Transient, 2=DC)
        dialog._root.setProperty.assert_any_call("initialTabIndex", 0)

    def test_apply_initial_parameters_selects_transient_tab(self):
        # arrange
        params = TransientSimulationParameters("1u", "1m", "", "", "", tuple())
        dialog = _make_dialog(initial_parameters=params)
        # act
        dialog._apply_initial_parameters()
        # assert — Transient tab is index 1 (QML order: 0=OP, 1=Transient, 2=DC)
        dialog._root.setProperty.assert_any_call("initialTabIndex", 1)


class TestSimulationDialogApplyTransientParameters(TestCase):

    def test_apply_transient_defaults_when_no_params(self):
        # arrange
        dialog = _make_dialog()
        # act
        dialog._apply_transient_parameters(None)
        # assert defaults applied
        dialog._root.setProperty.assert_any_call("initialStep", "1u")
        dialog._root.setProperty.assert_any_call("finalTime", "1m")

    def test_apply_transient_fills_values_from_params(self):
        # arrange
        params = TransientSimulationParameters("2n", "10u", "0", "5n", "NOOP", tuple())
        dialog = _make_dialog()
        # act
        dialog._apply_transient_parameters(params)
        # assert
        dialog._root.setProperty.assert_any_call("initialStep", "2n")
        dialog._root.setProperty.assert_any_call("finalTime", "10u")
        dialog._root.setProperty.assert_any_call("startTime", "0")

    def test_apply_transient_maps_noop_to_index_1(self):
        # arrange
        params = TransientSimulationParameters("1u", "1m", "", "", "NOOP", tuple())
        dialog = _make_dialog()
        # act
        dialog._apply_transient_parameters(params)
        # assert
        dialog._root.setProperty.assert_any_call("opModeIndex", 1)

    def test_apply_transient_maps_uic_to_index_2(self):
        # arrange
        params = TransientSimulationParameters("1u", "1m", "", "", "UIC", tuple())
        dialog = _make_dialog()
        # act
        dialog._apply_transient_parameters(params)
        # assert
        dialog._root.setProperty.assert_any_call("opModeIndex", 2)

    def test_apply_transient_maps_default_op_to_index_0(self):
        # arrange
        params = TransientSimulationParameters("1u", "1m", "", "", "", tuple())
        dialog = _make_dialog()
        # act
        dialog._apply_transient_parameters(params)
        # assert
        dialog._root.setProperty.assert_any_call("opModeIndex", 0)

    def test_apply_transient_fills_schedule_text(self):
        # arrange
        points = (TransientSchedulePoint("1u", "10n"), TransientSchedulePoint("5u", "50n"))
        params = TransientSimulationParameters("1n", "10u", "", "", "", points)
        dialog = _make_dialog()
        # act
        dialog._apply_transient_parameters(params)
        # assert — schedule text contains both pairs
        dialog._root.setProperty.assert_any_call("schedulePairsText", "1u,10n 5u,50n")
        dialog._root.setProperty.assert_any_call("scheduleEnabled", True)


class TestSimulationDialogApplyDCParameters(TestCase):

    def test_apply_dc_defaults_when_no_params(self):
        # arrange
        dialog = _make_dialog()
        # act
        dialog._apply_dc_parameters(None)
        # assert defaults
        dialog._root.setProperty.assert_any_call("sweepModeIndex", 0)
        dialog._root.setProperty.assert_any_call("primaryVariable", "VIN")

    def test_apply_dc_selects_correct_sweep_mode_index(self):
        # arrange — DEC is index 1
        params = DCSimulationParameters("DEC", "VIN", "1", "100", "", "10", tuple(), "", "", "", "", "", "")
        dialog = _make_dialog()
        # act
        dialog._apply_dc_parameters(params)
        # assert
        dialog._root.setProperty.assert_any_call("sweepModeIndex", 1)

    def test_apply_dc_fills_list_values(self):
        # arrange
        params = DCSimulationParameters("LIST", "TEMP", "", "", "", "", ("10", "20", "30"), "", "", "", "", "", "")
        dialog = _make_dialog()
        # act
        dialog._apply_dc_parameters(params)
        # assert
        dialog._root.setProperty.assert_any_call("listValuesText", "10 20 30")

    def test_apply_dc_sets_secondary_enabled_true(self):
        # arrange
        params = DCSimulationParameters("LIN", "VIN", "0", "5", "0.1", "", tuple(), "", "VCC", "3", "5", "0.5", "")
        dialog = _make_dialog()
        # act
        dialog._apply_dc_parameters(params)
        # assert
        dialog._root.setProperty.assert_any_call("secondaryEnabled", True)

    def test_apply_dc_sets_secondary_enabled_false_when_no_secondary(self):
        # arrange
        params = DCSimulationParameters("LIN", "VIN", "0", "5", "0.1", "", tuple(), "", "", "", "", "", "")
        dialog = _make_dialog()
        # act
        dialog._apply_dc_parameters(params)
        # assert
        dialog._root.setProperty.assert_any_call("secondaryEnabled", False)


class TestSimulationDialogOnSubmitOP(TestCase):

    def test_on_submit_op_sets_result_and_accepts(self):
        # arrange
        dialog = _make_dialog()
        # replace accept with a tracker
        accepted = []
        dialog.accept = lambda: accepted.append(True)
        # act
        dialog._on_submit_op()
        # assert
        self.assertIsInstance(dialog._result, OpSimulationParameters)
        self.assertEqual(len(accepted), 1)


class TestSimulationDialogOnSubmitTransient(TestCase):

    def test_accepts_valid_transient_params(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_transient("1u", "1m", "", "", "", False, "")
        # assert
        dialog.accept.assert_called_once()
        self.assertIsInstance(dialog._result, TransientSimulationParameters)

    def test_result_has_correct_values(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_transient("2u", "2m", "100n", "10u", "NOOP", False, "")
        # assert
        result = dialog._result
        self.assertEqual(result.initial_step_value, "2u")
        self.assertEqual(result.final_time_value, "2m")
        self.assertEqual(result.start_time_value, "100n")
        self.assertEqual(result.step_ceiling_value, "10u")
        self.assertEqual(result.op_keyword, "NOOP")

    def test_rejects_when_initial_step_empty(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_transient("", "1m", "", "", "", False, "")
        # assert
        dialog.accept.assert_not_called()
        dialog._root.setProperty.assert_any_call("transientErrorText", "Initial step and final time are required")

    def test_rejects_when_final_time_empty(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_transient("1u", "", "", "", "", False, "")
        # assert
        dialog.accept.assert_not_called()

    def test_rejects_invalid_op_keyword(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_transient("1u", "1m", "", "", "INVALID", False, "")
        # assert
        dialog.accept.assert_not_called()
        dialog._root.setProperty.assert_any_call("transientErrorText", "Operating-point mode must be Default, NOOP, or UIC")

    def test_rejects_when_schedule_enabled_but_no_pairs(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_transient("1u", "1m", "", "", "", True, "")
        # assert
        dialog.accept.assert_not_called()
        dialog._root.setProperty.assert_any_call("transientErrorText", "Schedule is enabled but no time,max-step pairs were provided")

    def test_accepts_valid_schedule_pairs(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_transient("1n", "10u", "", "", "", True, "1u,10n 5u,50n")
        # assert
        dialog.accept.assert_called_once()
        result = dialog._result
        self.assertEqual(len(result.schedule_points), 2)

    def test_rejects_odd_number_of_schedule_tokens(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_transient("1n", "10u", "", "", "", True, "1u 10n 5u")
        # assert — odd tokens is invalid format
        dialog.accept.assert_not_called()

    def test_clears_error_text_on_success(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_transient("1u", "1m", "", "", "", False, "")
        # assert
        dialog._root.setProperty.assert_any_call("transientErrorText", "")

    def test_strips_whitespace_from_inputs(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_transient("  1u  ", "  1m  ", "", "", "", False, "")
        # assert
        result = dialog._result
        self.assertEqual(result.initial_step_value, "1u")
        self.assertEqual(result.final_time_value, "1m")


class TestSimulationDialogOnSubmitDC(TestCase):

    def test_accepts_valid_lin_params(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_dc("LIN", "VIN", "0", "5", "0.1", "", "", "", False, "", "", "", "", "")
        # assert
        dialog.accept.assert_called_once()
        self.assertIsInstance(dialog._result, DCSimulationParameters)

    def test_rejects_invalid_sweep_mode(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_dc("INVALID", "VIN", "0", "5", "0.1", "", "", "", False, "", "", "", "", "")
        # assert
        dialog.accept.assert_not_called()
        dialog._root.setProperty.assert_any_call("dcErrorText", "Sweep mode must be one of LIN, DEC, OCT, LIST, or DATA")

    def test_rejects_lin_when_primary_variable_missing(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_dc("LIN", "", "0", "5", "0.1", "", "", "", False, "", "", "", "", "")
        # assert
        dialog.accept.assert_not_called()
        dialog._root.setProperty.assert_any_call("dcErrorText", "Primary sweep variable is required")

    def test_rejects_lin_when_start_missing(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_dc("LIN", "VIN", "", "5", "0.1", "", "", "", False, "", "", "", "", "")
        # assert
        dialog.accept.assert_not_called()

    def test_rejects_lin_when_stop_missing(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_dc("LIN", "VIN", "0", "", "0.1", "", "", "", False, "", "", "", "", "")
        # assert
        dialog.accept.assert_not_called()

    def test_rejects_lin_when_step_missing(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_dc("LIN", "VIN", "0", "5", "", "", "", "", False, "", "", "", "", "")
        # assert
        dialog.accept.assert_not_called()
        dialog._root.setProperty.assert_any_call("dcErrorText", "Start, stop, and step values are required for LIN sweep")

    def test_accepts_dec_with_points(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_dc("DEC", "VIN", "1", "100", "", "10", "", "", False, "", "", "", "", "")
        # assert
        dialog.accept.assert_called_once()

    def test_rejects_dec_when_points_zero(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_dc("DEC", "VIN", "1", "100", "", "0", "", "", False, "", "", "", "", "")
        # assert
        dialog.accept.assert_not_called()
        dialog._root.setProperty.assert_any_call("dcErrorText", "Points must be an integer \u2265 1")

    def test_rejects_dec_when_points_non_integer(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_dc("DEC", "VIN", "1", "100", "", "abc", "", "", False, "", "", "", "", "")
        # assert
        dialog.accept.assert_not_called()

    def test_rejects_dec_when_start_stop_missing(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_dc("DEC", "VIN", "", "", "", "10", "", "", False, "", "", "", "", "")
        # assert
        dialog.accept.assert_not_called()
        dialog._root.setProperty.assert_any_call("dcErrorText", "Start, stop, and points are required for DEC/OCT sweep")

    def test_accepts_oct_mode(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_dc("OCT", "VIN", "0.125", "64", "", "2", "", "", False, "", "", "", "", "")
        # assert
        dialog.accept.assert_called_once()

    def test_accepts_list_mode_with_values(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_dc("LIST", "TEMP", "", "", "", "", "10 20 30", "", False, "", "", "", "", "")
        # assert
        dialog.accept.assert_called_once()

    def test_rejects_list_mode_when_no_values(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_dc("LIST", "TEMP", "", "", "", "", "", "", False, "", "", "", "", "")
        # assert
        dialog.accept.assert_not_called()
        dialog._root.setProperty.assert_any_call("dcErrorText", "At least one list value is required for LIST sweep")

    def test_accepts_data_mode_with_table_name(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_dc("DATA", "", "", "", "", "", "", "resistorValues", False, "", "", "", "", "")
        # assert
        dialog.accept.assert_called_once()

    def test_rejects_data_mode_when_table_name_missing(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_dc("DATA", "", "", "", "", "", "", "", False, "", "", "", "", "")
        # assert
        dialog.accept.assert_not_called()
        dialog._root.setProperty.assert_any_call("dcErrorText", "Data table name is required for DATA sweep")

    def test_rejects_secondary_lin_when_variable_missing(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act — secondary enabled but variable empty
        dialog._on_submit_dc("LIN", "VIN", "0", "5", "0.1", "", "", "", True, "", "0", "3", "0.5", "")
        # assert
        dialog.accept.assert_not_called()
        dialog._root.setProperty.assert_any_call("dcErrorText", "Secondary sweep variable is required when secondary sweep is enabled")

    def test_rejects_secondary_lin_when_step_missing(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_dc("LIN", "VIN", "0", "5", "0.1", "", "", "", True, "VCC", "3", "5", "", "")
        # assert
        dialog.accept.assert_not_called()
        dialog._root.setProperty.assert_any_call("dcErrorText", "Secondary sweep step is required for LIN mode")

    def test_rejects_secondary_dec_when_points_missing(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_dc("DEC", "VIN", "1", "100", "", "10", "", "", True, "VCC", "1", "10", "", "")
        # assert
        dialog.accept.assert_not_called()
        dialog._root.setProperty.assert_any_call("dcErrorText", "Secondary sweep points are required for DEC/OCT mode")

    def test_rejects_secondary_dec_when_points_invalid(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_dc("DEC", "VIN", "1", "100", "", "10", "", "", True, "VCC", "1", "10", "", "0")
        # assert
        dialog.accept.assert_not_called()
        dialog._root.setProperty.assert_any_call("dcErrorText", "Secondary points must be an integer \u2265 1")

    def test_rejects_secondary_lin_when_start_stop_missing(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_dc("LIN", "VIN", "0", "5", "0.1", "", "", "", True, "VCC", "", "", "0.5", "")
        # assert
        dialog.accept.assert_not_called()
        dialog._root.setProperty.assert_any_call("dcErrorText", "Secondary sweep start and stop are required")

    def test_accepts_full_lin_with_secondary(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_dc("LIN", "VIN", "0", "5", "0.1", "", "", "", True, "VCC", "3", "5", "0.5", "")
        # assert
        dialog.accept.assert_called_once()
        self.assertEqual(dialog._result.secondary_variable, "VCC")

    def test_data_mode_ignores_secondary_sweep(self):
        # arrange — DATA mode does not support secondary
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_dc("DATA", "", "", "", "", "", "", "myTable", True, "VCC", "0", "5", "1", "")
        # assert — DATA is not in _DC_SECONDARY_MODES so secondary is ignored
        dialog.accept.assert_called_once()

    def test_clears_dc_error_text_on_success(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_dc("LIN", "VIN", "0", "5", "0.1", "", "", "", False, "", "", "", "", "")
        # assert
        dialog._root.setProperty.assert_any_call("dcErrorText", "")


class TestSimulationDialogParseSchedulePoints(TestCase):

    def test_empty_text_returns_empty_tuple(self):
        # arrange
        dialog = _make_dialog()
        # act
        result = dialog._parse_schedule_points("")
        # assert
        self.assertEqual(result, tuple())

    def test_parses_single_pair(self):
        # arrange
        dialog = _make_dialog()
        # act
        result = dialog._parse_schedule_points("1u,10n")
        # assert
        self.assertEqual(len(result), 1)
        self.assertEqual(result[0].time_value, "1u")
        self.assertEqual(result[0].max_time_step_value, "10n")

    def test_parses_multiple_pairs_space_separated(self):
        # arrange
        dialog = _make_dialog()
        # act
        result = dialog._parse_schedule_points("1u 10n 5u 50n")
        # assert
        self.assertEqual(len(result), 2)

    def test_parses_mixed_comma_space_separators(self):
        # arrange
        dialog = _make_dialog()
        # act
        result = dialog._parse_schedule_points("1u, 10n, 5u, 50n")
        # assert
        self.assertEqual(len(result), 2)

    def test_raises_value_error_for_odd_number_of_tokens(self):
        # arrange
        dialog = _make_dialog()
        # act / assert
        with self.assertRaises(ValueError):
            dialog._parse_schedule_points("1u 10n 5u")


class TestSimulationDialogOnSubmitDCListParseError(TestCase):

    def test_shows_error_when_list_parse_raises_value_error(self):
        # arrange
        dialog = _make_dialog_with_accept()
        with patch("plugin.simulation_dialog._parse_list_values", side_effect=ValueError("bad values")):
            # act
            dialog._on_submit_dc("LIST", "TEMP", "", "", "", "", "bad input", "", False, "", "", "", "", "")
        # assert
        dialog.accept.assert_not_called()
        dialog._root.setProperty.assert_any_call("dcErrorText", "bad values")


class TestSimulationDialogGetParameters(TestCase):

    def test_returns_none_when_dialog_rejected(self):
        # arrange
        dialog = _make_dialog()
        with patch.object(dialog, "exec", return_value=QDialog.DialogCode.Rejected):
            # act
            result = dialog.get_parameters()
        # assert
        self.assertIsNone(result)

    def test_returns_result_when_dialog_accepted(self):
        # arrange
        dialog = _make_dialog()
        dialog._result = OpSimulationParameters()
        with patch.object(dialog, "exec", return_value=QDialog.DialogCode.Accepted):
            # act
            result = dialog.get_parameters()
        # assert
        self.assertIsInstance(result, OpSimulationParameters)
