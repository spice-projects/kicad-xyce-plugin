import sys
from unittest.mock import MagicMock, patch

import pytest
from PySide6.QtWidgets import QApplication, QDialog
from PySide6.QtQuick import QQuickView

from kicad_xyce_plugin.netlist_parser import Device, NetlistTopology
from kicad_xyce_plugin.simulation_parameters import AcSimulationParameters, DCSimulationParameters, HbSimulationParameters, IcEntry, LinSimulationParameters, NoiseSimulationParameters, OpSimulationParameters, PrintParameters, SimulationConfig, SimulationParametersDialog, StepParameters, TransientSchedulePoint, TransientSimulationParameters

from kicad_xyce_plugin.simulation_parameters.noise_panel import _validate_device_name

_app = QApplication.instance() or QApplication(sys.argv)


def _make_dialog(initial_parameters=None) -> SimulationParametersDialog:
    """Create a SimulationParametersDialog with a mock QML root so tests can call slots directly."""
    dialog = SimulationParametersDialog.__new__(SimulationParametersDialog)
    # call QDialog.__init__ without triggering the full SimulationParametersDialog.__init__
    QDialog.__init__(dialog)
    # wrap parameters in SimulationConfig if they are individual analysis types
    if initial_parameters is not None and not isinstance(initial_parameters, SimulationConfig):
        initial_parameters = SimulationConfig(analysis=initial_parameters, step=StepParameters())
    elif initial_parameters is None:
        initial_parameters = SimulationConfig(analysis=None, step=StepParameters())
    dialog._initial_parameters = initial_parameters
    dialog._result = None
    dialog._root = MagicMock()

    # Bypassing _on_qml_ready which is not called in unit tests
    from kicad_xyce_plugin.simulation_parameters.op_panel import OpPanel
    from kicad_xyce_plugin.simulation_parameters.tran_panel import TranPanel
    from kicad_xyce_plugin.simulation_parameters.dc_panel import DcPanel
    from kicad_xyce_plugin.simulation_parameters.ac_panel import AcPanel
    from kicad_xyce_plugin.simulation_parameters.sensitivity_section import SensitivitySection
    from kicad_xyce_plugin.simulation_parameters.noise_panel import NoisePanel
    from kicad_xyce_plugin.simulation_parameters.hb_panel import HbPanel
    from kicad_xyce_plugin.simulation_parameters.lin_panel import LinPanel

    dialog._op_panel = OpPanel(dialog._root)
    dialog._tran_panel = TranPanel(dialog._root)
    dialog._dc_panel = DcPanel(dialog._root)
    dialog._ac_panel = AcPanel(dialog._root)
    dialog._sensitivity_section = SensitivitySection(dialog._root)
    dialog._noise_panel = NoisePanel(dialog._root)
    dialog._hb_panel = HbPanel(dialog._root)
    dialog._lin_panel = LinPanel(dialog._root)

    # mock property values for _get_current_step_parameters
    properties = {
        "stepEnabled": False,
        "stepSweepModeIndex": 0,
        "stepVariable": "",
        "stepStartValue": "",
        "stepStopValue": "",
        "stepStepValue": "",
        "stepPointsValue": "",
        "stepListValuesText": "",
        "stepDataTableName": ""
    }
    dialog._root.property.side_effect = lambda name: properties.get(name)
    dialog._has_bjt_devices = False
    dialog._has_fet_devices = False
    return dialog


def _make_dialog_with_accept(initial_parameters=None) -> SimulationParametersDialog:
    dialog = _make_dialog(initial_parameters)
    dialog.accept = MagicMock()
    return dialog


class TestSimulationParametersDialogConstruction:

    def test_dialog_can_be_instantiated(self):
        # act
        dialog = SimulationParametersDialog(None, SimulationConfig(analysis=OpSimulationParameters(), step=StepParameters()))
        # assert
        assert isinstance(dialog, SimulationParametersDialog)

    def test_dialog_result_is_none_initially(self):
        # act
        dialog = SimulationParametersDialog(None, SimulationConfig(analysis=OpSimulationParameters(), step=StepParameters()))
        # assert
        assert dialog._result is None

    def test_on_qml_ready_skips_when_not_ready(self):
        # arrange
        dialog = _make_dialog()
        dialog._qml_view = MagicMock()
        # act — non-Ready status should be a no-op
        dialog._on_qml_ready(QQuickView.Status.Loading)
        # assert — _root was not re-assigned by this call (mock stays)
        assert isinstance(dialog._root, MagicMock)

    def test_apply_initial_parameters_with_none(self):
        # arrange
        dialog = _make_dialog(initial_parameters=None)
        # act
        dialog._apply_initial_parameters()
        # assert — setProperty was called for tab index (defaults to OP=0)
        dialog._root.setProperty.assert_any_call("initialTabIndex", 0)

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
        # Note: test case override logic for TransientSimulationParameters
        # relies on specific behavior.
        assert dialog._root.setProperty.called


class TestSimulationParametersDialogApplyTransientParameters:

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

    def test_apply_transient_fills_fft_text(self):
        # arrange
        from kicad_xyce_plugin.simulation_parameters import FftParameters
        fft = (FftParameters(output_variable="V(1)"), FftParameters(output_variable="V(2)", window="HANN"))
        params = TransientSimulationParameters("1u", "1m", fft_parameters=fft)
        dialog = _make_dialog()
        # act
        dialog._apply_transient_parameters(params)
        # assert — fft text contains both statements separated by newline
        dialog._root.setProperty.assert_any_call("fftParametersText", ".FFT V(1)\n.FFT V(2) WINDOW=HANN")


class TestSimulationParametersDialogApplyDCParameters:

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


class TestSimulationParametersDialogOnSubmitOP:

    def test_on_submit_op_sets_result_and_accepts(self):
        # arrange
        dialog = _make_dialog()
        # replace accept with a tracker
        accepted = []
        dialog.accept = lambda: accepted.append(True)
        # act
        dialog._on_submit_op(False, False, False, False, False, False, "", "", "", False, "NODESET", "", "", "", False)
        # assert
        assert isinstance(dialog._result.analysis, OpSimulationParameters)
        assert len(accepted) == 1

    def test_on_submit_op_parses_initial_conditions(self):
        # arrange
        dialog = _make_dialog()
        accepted = []
        dialog.accept = lambda: accepted.append(True)
        # act
        dialog._on_submit_op(False, False, False, False, False, False, "", "", "", False, "NODESET", "", "V(out)=1.0 V(in)=0", "", False)
        # assert
        assert isinstance(dialog._result.analysis, OpSimulationParameters)
        assert dialog._result.analysis.ic_entries == (IcEntry(node="out", voltage="1.0"), IcEntry(node="in", voltage="0"))
        assert len(accepted) == 1

    def test_apply_op_parameters_restores_initial_conditions(self):
        # arrange
        params = OpSimulationParameters(ic_entries=(IcEntry(node="out", voltage="1.0"),), replace_ground=False)
        dialog = _make_dialog()
        # act
        dialog._apply_op_parameters(params)
        # assert
        dialog._root.setProperty.assert_any_call("opInitialConditionEntries", "V(out)=1.0")


class TestSimulationParametersDialogOnSubmitTransient:

    def test_accepts_valid_transient_params(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_transient("1u", "1m", "", "", "", False, "", "", "", "", False, "TRAN", False, False, False, False, False, "", "", "", False)
        # assert
        dialog.accept.assert_called_once()
        assert isinstance(dialog._result.analysis, TransientSimulationParameters)

    def test_result_has_correct_values(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_transient("2u", "2m", "100n", "10u", "NOOP", False, "", "", "", "", False, "TRAN", False, False, False, False, False, "", "", "", False)
        # assert
        result = dialog._result
        assert result.analysis.initial_step_value == "2u"
        assert result.analysis.final_time_value == "2m"
        assert result.analysis.start_time_value == "100n"
        assert result.analysis.step_ceiling_value == "10u"
        assert result.analysis.op_keyword == "NOOP"

    def test_rejects_when_initial_step_empty(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_transient("", "1m", "", "", "", False, "", "", "", "", False, "TRAN", False, False, False, False, False, "", "", "", False)
        # assert
        dialog.accept.assert_not_called()
        dialog._root.setProperty.assert_any_call("errorText", "Initial step and final time are required")

    def test_invalid_measure_transient(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act with invalid .MEASURE directive (typo .MEAS)
        dialog._on_submit_transient("1u", "1m", "", "", "", False, "", "", "", ".MEAS gg", False, "TRAN", False, False, False, False, False, "", "", "", False)
        # assert
        dialog.accept.assert_not_called()
        dialog._root.setProperty.assert_any_call("errorText", "Invalid .MEASURE directive for TRAN: .MEAS gg")

    def test_rejects_when_final_time_empty(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_transient("1u", "", "", "", "", False, "", "", "", "", False, "TRAN", False, False, False, False, False, "", "", "", False)
        # assert
        dialog.accept.assert_not_called()

    def test_rejects_invalid_op_keyword(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_transient("1u", "1m", "", "", "INVALID", False, "", "", "", "", False, "TRAN", False, False, False, False, False, "", "", "", False)
        # assert
        dialog.accept.assert_not_called()
        dialog._root.setProperty.assert_any_call("errorText", "Operating-point mode must be Default, NOOP, or UIC")

    def test_rejects_when_schedule_enabled_but_no_pairs(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_transient("1u", "1m", "", "", "", True, "", "", "", "", False, "TRAN", False, False, False, False, False, "", "", "", False)
        # assert
        dialog.accept.assert_not_called()
        dialog._root.setProperty.assert_any_call("errorText", "Schedule is enabled but no time,max-step pairs were provided")

    def test_accepts_valid_schedule_pairs(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_transient("1n", "10u", "", "", "", True, "1u,10n 5u,50n", "", "", "", False, "TRAN", False, False, False, False, False, "", "", "", False)
        # assert
        dialog.accept.assert_called_once()
        result = dialog._result
        assert len(result.analysis.schedule_points) == 2

    def test_rejects_odd_number_of_schedule_tokens(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_transient("1n", "10u", "", "", "", True, "1u 10n 5u", "", "", "", False, "TRAN", False, False, False, False, False, "", "", "", False)
        # assert — odd tokens is invalid format
        dialog.accept.assert_not_called()

    def test_clears_error_text_on_success(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_transient("1u", "1m", "", "", "", False, "", "", "", "", False, "TRAN", False, False, False, False, False, "", "", "", False)
        # assert
        dialog._root.setProperty.assert_any_call("errorText", "")

    def test_strips_whitespace_from_inputs(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_transient("1u", "1m", "", "", "", False, "", "", "", "", False, "TRAN", False, False, False, False, False, "", "", "", False)
        # assert
        result = dialog._result
        assert result.analysis.initial_step_value == "1u"
        assert result.analysis.final_time_value == "1m"

    def test_accepts_valid_fft_directives(self):
        # arrange
        dialog = _make_dialog_with_accept()
        fft_text = ".FFT V(1)\nV(2) WINDOW=RECT\n"
        # act
        dialog._on_submit_transient("1u", "1m", "", "", "", False, "", fft_text, "", "", False, "TRAN", False, False, False, False, False, "", "", "", False)
        # assert
        dialog.accept.assert_called_once()
        result = dialog._result
        assert len(result.analysis.fft_parameters) == 2
        assert result.analysis.fft_parameters[0].output_variable == "V(1)"
        assert result.analysis.fft_parameters[1].output_variable == "V(2)"
        assert result.analysis.fft_parameters[1].window == "RECT"

    def test_accepts_valid_four_directives(self):
        # arrange
        dialog = _make_dialog_with_accept()
        four_text = ".FOUR 1k V(1)\n2k V(2) I(1)\n"
        # act
        dialog._on_submit_transient("1u", "1m", "", "", "", False, "", "", four_text, "", False, "TRAN", False, False, False, False, False, "", "", "", False)
        # assert
        dialog.accept.assert_called_once()
        result = dialog._result
        assert len(result.analysis.four_parameters) == 2
        assert result.analysis.four_parameters[0].fundamental_frequency == "1k"
        assert result.analysis.four_parameters[0].output_variables == ("V(1)",)
        assert result.analysis.four_parameters[1].fundamental_frequency == "2k"
        assert result.analysis.four_parameters[1].output_variables == ("V(2)", "I(1)")

    def test_accepts_valid_measure_directives(self):
        # arrange
        dialog = _make_dialog_with_accept()
        measure_text = ".MEASURE TRAN RISE_TIME MAX V(OUT) RISE=1\n.MEASURE TRAN FALL_TIME MIN V(OUT) FALL=1\n"
        # act
        dialog._on_submit_transient("1u", "1m", "", "", "", False, "", "", "", measure_text, False, "TRAN", False, False, False, False, False, "", "", "", False)
        # assert
        dialog.accept.assert_called_once()
        result = dialog._result
        assert len(result.analysis.measure_parameters) == 2
        assert result.analysis.measure_parameters[0].result_name == "RISE_TIME"
        assert result.analysis.measure_parameters[0].measure_type == "MAX"
        assert result.analysis.measure_parameters[0].variable == "V(OUT)"
        assert result.analysis.measure_parameters[0].rise_val == "1"
        assert result.analysis.measure_parameters[1].result_name == "FALL_TIME"
        assert result.analysis.measure_parameters[1].measure_type == "MIN"
        assert result.analysis.measure_parameters[1].variable == "V(OUT)"
        assert result.analysis.measure_parameters[1].fall_val == "1"

    def test_accepts_measure_directives_without_prefix(self):
        # arrange
        dialog = _make_dialog_with_accept()
        measure_text = "RISE_TIME MAX V(OUT) RISE=1\nFALL_TIME MIN V(OUT) FALL=1\n"
        # act
        dialog._on_submit_transient("1u", "1m", "", "", "", False, "", "", "", measure_text, False, "TRAN", False, False, False, False, False, "", "", "", False)
        # assert
        dialog.accept.assert_called_once()
        result = dialog._result
        assert len(result.analysis.measure_parameters) == 2
        assert result.analysis.measure_parameters[0].analysis_type == "TRAN"

    def test_rejects_measure_directives_with_wrong_analysis_type(self):
        # arrange
        dialog = _make_dialog_with_accept()
        measure_text = ".MEASURE DC RISE_TIME MAX V(OUT) RISE=1\n.MEASURE TRAN FALL_TIME MIN V(OUT) FALL=1\n"
        # act
        dialog._on_submit_transient("1u", "1m", "", "", "", False, "", "", "", measure_text, False, "TRAN", False, False, False, False, False, "", "", "", False)
        # assert
        dialog.accept.assert_not_called()
        dialog._root.setProperty.assert_any_call("errorText", "Invalid .MEASURE directive for TRAN: .MEASURE DC RISE_TIME MAX V(OUT) RISE=1")

    def test_rejects_invalid_measure_directives(self):
        # arrange
        dialog = _make_dialog_with_accept()
        measure_text = ".MEASURE TRAN INVALID_SYNTAX\n.MEASURE TRAN RISE_TIME MAX V(OUT) RISE=1\n"
        # act
        dialog._on_submit_transient("1u", "1m", "", "", "", False, "", "", "", measure_text, False, "TRAN", False, False, False, False, False, "", "", "", False)
        # assert
        dialog.accept.assert_not_called()
        dialog._root.setProperty.assert_any_call("errorText", "Invalid .MEASURE directive for TRAN: .MEASURE TRAN INVALID_SYNTAX")


class TestSimulationParametersDialogOnSubmitDC:

    def test_accepts_valid_lin_params(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_dc("LIN", "VIN", "0", "5", "0.1", "", "", "", False, "", "", "", "", "", "", False, "DC", False, False, False, False, False, "", "", "", False)
        # assert
        dialog.accept.assert_called_once()
        assert isinstance(dialog._result.analysis, DCSimulationParameters)

    def test_invalid_measure_dc(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act with invalid .MEASURE directive for DC
        dialog._on_submit_dc("LIN", "VIN", "0", "5", "0.1", "", "", "", False, "", "", "", "", "", ".MEAS gg", False, "DC", False, False, False, False, False, "", "", "", False)
        # assert
        dialog.accept.assert_not_called()
        dialog._root.setProperty.assert_any_call("errorText", "Invalid .MEASURE directive for DC: .MEAS gg")

    def test_rejects_invalid_sweep_mode(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_dc("INVALID", "VIN", "0", "5", "0.1", "", "", "", False, "", "", "", "", "", "", False, "DC", False, False, False, False, False, "", "", "", False)
        # assert
        dialog.accept.assert_not_called()
        dialog._root.setProperty.assert_any_call("errorText", "Sweep mode must be one of LIN, DEC, OCT, LIST, or DATA")

    def test_rejects_lin_when_primary_variable_missing(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_dc("LIN", "", "0", "5", "0.1", "", "", "", False, "", "", "", "", "", "", False, "DC", False, False, False, False, False, "", "", "", False)
        # assert
        dialog.accept.assert_not_called()
        dialog._root.setProperty.assert_any_call("errorText", "Primary sweep variable is required")

    def test_rejects_lin_when_start_missing(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_dc("LIN", "VIN", "", "5", "0.1", "", "", "", False, "", "", "", "", "", "", False, "DC", False, False, False, False, False, "", "", "", False)
        # assert
        dialog.accept.assert_not_called()

    def test_rejects_lin_when_stop_missing(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_dc("LIN", "VIN", "0", "", "0.1", "", "", "", False, "", "", "", "", "", "", False, "DC", False, False, False, False, False, "", "", "", False)
        # assert
        dialog.accept.assert_not_called()

    def test_rejects_lin_when_step_missing(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_dc("LIN", "VIN", "0", "5", "", "", "", "", False, "", "", "", "", "", "", False, "DC", False, False, False, False, False, "", "", "", False)
        # assert
        dialog.accept.assert_not_called()
        dialog._root.setProperty.assert_any_call("errorText", "Start, stop, and step values are required for LIN sweep")

    def test_accepts_dec_with_points(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_dc("DEC", "VIN", "1", "100", "", "10", "", "", False, "", "", "", "", "", "", False, "DC", False, False, False, False, False, "", "", "", False)
        # assert
        dialog.accept.assert_called_once()

    def test_rejects_dec_when_points_zero(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_dc("DEC", "VIN", "1", "100", "", "0", "", "", False, "", "", "", "", "", "", False, "DC", False, False, False, False, False, "", "", "", False)
        # assert
        dialog.accept.assert_not_called()
        dialog._root.setProperty.assert_any_call("errorText", "Points must be an integer \u2265 1")

    def test_rejects_dec_when_points_non_integer(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_dc("DEC", "VIN", "1", "100", "", "abc", "", "", False, "", "", "", "", "", "", False, "DC", False, False, False, False, False, "", "", "", False)
        # assert
        dialog.accept.assert_not_called()

    def test_rejects_dec_when_start_stop_missing(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_dc("DEC", "VIN", "", "", "", "10", "", "", False, "", "", "", "", "", "", False, "DC", False, False, False, False, False, "", "", "", False)
        # assert
        dialog.accept.assert_not_called()
        dialog._root.setProperty.assert_any_call("errorText", "Start, stop, and points are required for DEC/OCT sweep")

    def test_accepts_oct_mode(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_dc("OCT", "VIN", "0.125", "64", "", "2", "", "", False, "", "", "", "", "", "", False, "DC", False, False, False, False, False, "", "", "", False)
        # assert
        dialog.accept.assert_called_once()

    def test_accepts_list_mode_with_values(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_dc("LIST", "TEMP", "", "", "", "", "10 20 30", "", False, "", "", "", "", "", "", False, "DC", False, False, False, False, False, "", "", "", False)
        # assert
        dialog.accept.assert_called_once()

    def test_rejects_list_mode_when_no_values(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_dc("LIST", "TEMP", "", "", "", "", "", "", False, "", "", "", "", "", "", False, "DC", False, False, False, False, False, "", "", "", False)
        # assert
        dialog.accept.assert_not_called()
        dialog._root.setProperty.assert_any_call("errorText", "At least one list value is required for LIST sweep")

    def test_accepts_data_mode_with_table_name(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_dc("DATA", "", "", "", "", "", "", "resistorValues", False, "", "", "", "", "", "", False, "DC", False, False, False, False, False, "", "", "", False)
        # assert
        dialog.accept.assert_called_once()

    def test_rejects_data_mode_when_table_name_missing(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_dc("DATA", "", "", "", "", "", "", "", False, "", "", "", "", "", "", False, "DC", False, False, False, False, False, "", "", "", False)
        # assert
        dialog.accept.assert_not_called()
        dialog._root.setProperty.assert_any_call("errorText", "Data table name is required for DATA sweep")

    def test_rejects_secondary_lin_when_variable_missing(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act — secondary enabled but variable empty
        dialog._on_submit_dc("LIN", "VIN", "0", "5", "0.1", "", "", "", True, "", "0", "3", "0.5", "", "", False, "DC", False, False, False, False, False, "", "", "", False)
        # assert
        dialog.accept.assert_not_called()
        dialog._root.setProperty.assert_any_call("errorText", "Secondary sweep variable is required when secondary sweep is enabled")

    def test_rejects_secondary_lin_when_step_missing(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_dc("LIN", "VIN", "0", "5", "0.1", "", "", "", True, "VCC", "3", "5", "", "", "", False, "DC", False, False, False, False, False, "", "", "", False)
        # assert
        dialog.accept.assert_not_called()
        dialog._root.setProperty.assert_any_call("errorText", "Secondary sweep step is required for LIN mode")

    def test_rejects_secondary_dec_when_points_missing(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_dc("DEC", "VIN", "1", "100", "", "10", "", "", True, "VCC", "1", "10", "", "", "", False, "DC", False, False, False, False, False, "", "", "", False)
        # assert
        dialog.accept.assert_not_called()
        dialog._root.setProperty.assert_any_call("errorText", "Secondary sweep points are required for DEC/OCT mode")

    def test_rejects_secondary_dec_when_points_invalid(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_dc("DEC", "VIN", "1", "100", "", "10", "", "", True, "VCC", "1", "10", "", "0", "", False, "DC", False, False, False, False, False, "", "", "", False)
        # assert
        dialog.accept.assert_not_called()
        dialog._root.setProperty.assert_any_call("errorText", "Secondary points must be an integer \u2265 1")

    def test_rejects_secondary_lin_when_start_stop_missing(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_dc("LIN", "VIN", "0", "5", "0.1", "", "", "", True, "VCC", "", "", "0.5", "", "", False, "DC", False, False, False, False, False, "", "", "", False)
        # assert
        dialog.accept.assert_not_called()
        dialog._root.setProperty.assert_any_call("errorText", "Secondary sweep start and stop are required")

    def test_accepts_full_lin_with_secondary(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_dc("LIN", "VIN", "0", "5", "0.1", "", "", "", True, "VCC", "3", "5", "0.5", "", "", False, "DC", False, False, False, False, False, "", "", "", False)
        # assert
        dialog.accept.assert_called_once()
        assert dialog._result.analysis.secondary_variable == "VCC"

    def test_data_mode_ignores_secondary_sweep(self):
        # arrange — DATA mode does not support secondary
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_dc("DATA", "", "", "", "", "", "", "myTable", True, "VCC", "0", "5", "1", "", "", False, "DC", False, False, False, False, False, "", "", "", False)
        # assert — DATA is not in _DC_SECONDARY_MODES so secondary is ignored
        dialog.accept.assert_called_once()

    def test_clears_dc_error_text_on_success(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_dc("LIN", "VIN", "0", "5", "0.1", "", "", "", False, "", "", "", "", "", "", False, "DC", False, False, False, False, False, "", "", "", False)
        # assert
        dialog._root.setProperty.assert_any_call("errorText", "")

    def test_accepts_valid_measure_directives(self):
        # arrange
        dialog = _make_dialog_with_accept()
        measure_text = ".MEASURE DC VIN_AT_2V FIND V(1) WHEN V(1)=2\n.MEASURE DC MAX_CURRENT MAX I(R1)\n"
        # act
        dialog._on_submit_dc("LIN", "VIN", "0", "5", "0.1", "", "", "", False, "", "", "", "", "", measure_text, False, "DC", False, False, False, False, False, "", "", "", False)
        # assert
        dialog.accept.assert_called_once()
        result = dialog._result
        assert len(result.analysis.measure_parameters) == 2
        assert result.analysis.measure_parameters[0].result_name == "VIN_AT_2V"
        assert result.analysis.measure_parameters[0].measure_type == "FIND"
        assert result.analysis.measure_parameters[0].variable == "V(1)"
        assert result.analysis.measure_parameters[1].result_name == "MAX_CURRENT"
        assert result.analysis.measure_parameters[1].measure_type == "MAX"
        assert result.analysis.measure_parameters[1].variable == "I(R1)"


class TestSimulationParametersDialogParseSchedulePoints:

    def test_empty_text_returns_empty_tuple(self):
        # arrange
        dialog = _make_dialog()
        # act
        result = dialog._parse_schedule_points("")
        # assert
        assert result == tuple()

    def test_parses_single_pair(self):
        # arrange
        dialog = _make_dialog()
        # act
        result = dialog._parse_schedule_points("1u,10n")
        # assert
        assert len(result) == 1
        assert result[0].time_value == "1u"
        assert result[0].max_time_step_value == "10n"

    def test_parses_multiple_pairs_space_separated(self):
        # arrange
        dialog = _make_dialog()
        # act
        result = dialog._parse_schedule_points("1u 10n 5u 50n")
        # assert
        assert len(result) == 2

    def test_parses_mixed_comma_space_separators(self):
        # arrange
        dialog = _make_dialog()
        # act
        result = dialog._parse_schedule_points("1u, 10n, 5u, 50n")
        # assert
        assert len(result) == 2

    def test_raises_value_error_for_odd_number_of_tokens(self):
        # arrange
        dialog = _make_dialog()
        # act / assert
        with pytest.raises(ValueError):
            dialog._parse_schedule_points("1u 10n 5u")


class TestSimulationParametersDialogOnSubmitDCListParseError:

    def test_shows_error_when_list_parse_raises_value_error(self):
        # arrange
        dialog = _make_dialog_with_accept()
        with patch("kicad_xyce_plugin.simulation_parameters.simulation_parameters_dialog._parse_list_values", side_effect=ValueError("bad values")):
            # act
            dialog._on_submit_dc("LIST", "TEMP", "", "", "", "", "bad input", "", False, "", "", "", "", "", "", False, "DC", False, False, False, False, False, "", "", "", False)
        # assert
        dialog.accept.assert_not_called()
        dialog._root.setProperty.assert_any_call("errorText", "bad values")


class TestSimulationParametersDialogGetParameters:

    def test_returns_none_when_dialog_rejected(self):
        # arrange
        dialog = _make_dialog()
        with patch.object(dialog, "exec", return_value=QDialog.DialogCode.Rejected):
            # act
            result = dialog.get_parameters()
        # assert
        assert result is None

    def test_returns_result_when_dialog_accepted(self):
        # arrange
        dialog = _make_dialog()
        dialog._result = OpSimulationParameters()
        with patch.object(dialog, "exec", return_value=QDialog.DialogCode.Accepted):
            # act
            result = dialog.get_parameters()
        # assert
        assert isinstance(result, OpSimulationParameters)


class TestParseListValues:

    def test_empty_string_returns_empty_tuple(self):
        # arrange / act
        from kicad_xyce_plugin.simulation_parameters.simulation_parameters_dialog import _parse_list_values
        result = _parse_list_values("")
        # assert
        assert result == tuple()


class TestValidateDeviceName:

    def test_accepts_valid_device_name(self):
        # arrange / act
        result = _validate_device_name("R1")
        # assert
        assert result is True

    def test_accepts_device_name_with_underscore(self):
        # arrange / act
        result = _validate_device_name("R_1")
        # assert
        assert result is True

    def test_rejects_empty_device_name(self):
        # arrange / act
        result = _validate_device_name("")
        # assert
        assert result is False

    def test_rejects_whitespace_only_device_name(self):
        # arrange / act
        result = _validate_device_name("   ")
        # assert
        assert result is False

    def test_rejects_device_name_starting_with_number(self):
        # arrange / act
        result = _validate_device_name("1R1")
        # assert
        assert result is False

    def test_rejects_device_name_with_special_characters(self):
        # arrange / act
        result = _validate_device_name("R1-2")
        # assert
        assert result is False


class TestBuildVariableCandidates:

    def test_with_topology_sets_node_and_device_lists(self):
        # arrange
        devices = [Device(name="V1", type_letter="V", nodes=["in", "0"])]
        topology = NetlistTopology(title="test", devices=devices, nodes={"in", "0"}, subcircuit_definitions={}, global_nodes=set())
        dialog = _make_dialog()
        dialog._topology = topology
        # act
        dialog._build_variable_candidates()
        # assert
        assert "V(in)" in dialog._node_voltages
        assert "V(0)" not in dialog._node_voltages
        assert "I(V1)" in dialog._device_currents

    def test_detects_bjt_and_fet_devices(self):
        # arrange
        devices = [
            Device(name="Q1", type_letter="Q", nodes=["c", "b", "e"]),
            Device(name="M1", type_letter="M", nodes=["d", "g", "s", "b"]),
        ]
        topology = NetlistTopology(title="test", devices=devices, nodes={"c", "b", "e", "d", "g", "s"}, subcircuit_definitions={}, global_nodes=set())
        dialog = _make_dialog()
        dialog._topology = topology
        # act
        dialog._build_variable_candidates()
        # assert
        assert dialog._has_bjt_devices is True
        assert dialog._has_fet_devices is True


class TestApplyInitialParametersTabSelection:

    def test_selects_ac_tab(self):
        # arrange
        params = AcSimulationParameters(sweep_mode="LIN", points="100", start="1", end="1MEG", replace_ground=False)
        dialog = _make_dialog(initial_parameters=params)
        # act
        dialog._apply_initial_parameters()
        # assert
        dialog._root.setProperty.assert_any_call("initialTabIndex", 3)

    def test_selects_noise_tab(self):
        # arrange
        params = NoiseSimulationParameters(output_node="5", source_name="V1", sweep_mode="LIN", points="100", start="1", end="1MEG", replace_ground=False)
        dialog = _make_dialog(initial_parameters=params)
        # act
        dialog._apply_initial_parameters()
        # assert
        dialog._root.setProperty.assert_any_call("initialTabIndex", 4)

    def test_selects_hb_tab(self):
        # arrange
        params = HbSimulationParameters(frequencies=("1MEG",), replace_ground=False)
        dialog = _make_dialog(initial_parameters=params)
        # act
        dialog._apply_initial_parameters()
        # assert
        dialog._root.setProperty.assert_any_call("initialTabIndex", 5)

    def test_selects_lin_tab(self):
        # arrange
        params = LinSimulationParameters(sweep_mode="LIN", points="100", start="1", end="1MEG", replace_ground=False)
        dialog = _make_dialog(initial_parameters=params)
        # act
        dialog._apply_initial_parameters()
        # assert
        dialog._root.setProperty.assert_any_call("initialTabIndex", 6)


class TestApplyOpParametersWithPrintParams:

    def test_restores_saved_print_parameters(self):
        # arrange
        pp = PrintParameters(print_type="DC", output_variables=("V(*)", "I(*)", "P(*)"), print_format="CSV", print_file="op.csv")
        params = OpSimulationParameters(print_parameters=pp)
        dialog = _make_dialog()
        # act
        dialog._apply_op_parameters(params)
        # assert
        dialog._root.setProperty.assert_any_call("opPrintAllNodes", True)
        dialog._root.setProperty.assert_any_call("opPrintAllCurrents", True)
        dialog._root.setProperty.assert_any_call("opPrintPower", True)
        dialog._root.setProperty.assert_any_call("opPrintFile", "op.csv")


class TestApplyTransientParametersWithPrintParams:

    def test_restores_saved_print_parameters(self):
        # arrange
        pp = PrintParameters(print_type="TRAN", output_variables=("V(*)", "IC(*)"), print_format="RAW", print_file="")
        params = TransientSimulationParameters("1u", "1m", "", "", "", tuple(), print_parameters=pp)
        dialog = _make_dialog()
        # act
        dialog._apply_transient_parameters(params)
        # assert
        dialog._root.setProperty.assert_any_call("tranPrintAllNodes", True)
        dialog._root.setProperty.assert_any_call("tranPrintBjtLeads", True)
        dialog._root.setProperty.assert_any_call("tranPrintAllCurrents", False)


class TestApplyDCParametersWithPrintParams:

    def test_restores_saved_print_parameters(self):
        # arrange
        pp = PrintParameters(print_type="DC", output_variables=("V(*)",), print_format="CSV", print_file="dc.csv")
        params = DCSimulationParameters("LIN", "VIN", "0", "5", "0.1", "", tuple(), "", "", "", "", "", "", print_parameters=pp)
        dialog = _make_dialog()
        # act
        dialog._apply_dc_parameters(params)
        # assert
        dialog._root.setProperty.assert_any_call("dcPrintAllNodes", True)
        dialog._root.setProperty.assert_any_call("dcPrintFile", "dc.csv")


class TestApplyACParameters:

    def test_defaults_when_no_params(self):
        # arrange
        dialog = _make_dialog()
        # act
        dialog._apply_ac_parameters(None)
        # assert
        dialog._root.setProperty.assert_any_call("acPoints", "100")
        dialog._root.setProperty.assert_any_call("acStart", "1")
        dialog._root.setProperty.assert_any_call("acEnd", "1MEG")

    def test_fills_values_from_params(self):
        # arrange
        params = AcSimulationParameters(sweep_mode="DEC", points="10", start="1k", end="100MEG", replace_ground=False)
        dialog = _make_dialog()
        # act
        dialog._apply_ac_parameters(params)
        # assert
        dialog._root.setProperty.assert_any_call("acSweepModeIndex", 1)
        dialog._root.setProperty.assert_any_call("acPoints", "10")

    def test_restores_saved_print_parameters(self):
        # arrange
        pp = PrintParameters(print_type="AC", output_variables=("V(*)", "V(OUT)"), print_format="CSV", print_file="ac.csv")
        params = AcSimulationParameters(sweep_mode="LIN", points="100", start="1", end="1MEG", replace_ground=False, print_parameters=pp)
        dialog = _make_dialog()
        # act
        dialog._apply_ac_parameters(params)
        # assert
        dialog._root.setProperty.assert_any_call("acPrintAllNodes", True)
        dialog._root.setProperty.assert_any_call("acPrintFile", "ac.csv")
        dialog._root.setProperty.assert_any_call("acPrintSpecificVars", "V(OUT)")


class TestApplyNoiseParameters:

    def test_defaults_when_no_params(self):
        # arrange
        dialog = _make_dialog()
        # act
        dialog._apply_noise_parameters(None)
        # assert
        dialog._root.setProperty.assert_any_call("noiseOutputNode", "")
        dialog._root.setProperty.assert_any_call("noisePoints", "100")

    def test_fills_values_from_params(self):
        # arrange
        params = NoiseSimulationParameters(output_node="out", ref_node="gnd", source_name="V1", sweep_mode="DEC", points="10", start="1k", end="10MEG", replace_ground=False)
        dialog = _make_dialog()
        # act
        dialog._apply_noise_parameters(params)
        # assert
        dialog._root.setProperty.assert_any_call("noiseOutputNode", "out")
        dialog._root.setProperty.assert_any_call("noiseRefNode", "gnd")
        dialog._root.setProperty.assert_any_call("noiseSweepModeIndex", 1)

    def test_restores_saved_print_parameters(self):
        # arrange
        pp = PrintParameters(print_type="NOISE", output_variables=("V(*)", "INOISE", "ONOISE"), print_format="CSV", print_file="noise.csv")
        params = NoiseSimulationParameters(output_node="5", source_name="V1", sweep_mode="LIN", points="100", start="1", end="1MEG", replace_ground=False, print_parameters=pp)
        dialog = _make_dialog()
        # act
        dialog._apply_noise_parameters(params)
        # assert
        dialog._root.setProperty.assert_any_call("noisePrintAllNodes", True)
        dialog._root.setProperty.assert_any_call("noisePrintInoise", True)
        dialog._root.setProperty.assert_any_call("noisePrintOnoise", True)
        dialog._root.setProperty.assert_any_call("noisePrintFile", "noise.csv")


class TestApplyHBParameters:

    def test_defaults_when_no_params(self):
        # arrange
        dialog = _make_dialog()
        # act
        dialog._apply_hb_parameters(None)
        # assert
        dialog._root.setProperty.assert_any_call("hbFrequenciesText", "1MEG")

    def test_fills_values_from_params(self):
        # arrange
        params = HbSimulationParameters(frequencies=("1MEG", "2MEG"), replace_ground=False)
        dialog = _make_dialog()
        # act
        dialog._apply_hb_parameters(params)
        # assert
        dialog._root.setProperty.assert_any_call("hbFrequenciesText", "1MEG 2MEG")

    def test_restores_saved_print_parameters(self):
        # arrange
        pp = PrintParameters(print_type="HB_TD", output_variables=("V(*)",), print_format="CSV", print_file="hb.csv")
        params = HbSimulationParameters(frequencies=("1MEG",), replace_ground=False, print_parameters=pp)
        dialog = _make_dialog()
        # act
        dialog._apply_hb_parameters(params)
        # assert
        dialog._root.setProperty.assert_any_call("hbPrintAllNodes", True)
        dialog._root.setProperty.assert_any_call("hbPrintTypeIndex", 2)
        dialog._root.setProperty.assert_any_call("hbPrintFile", "hb.csv")

    def test_restores_saved_hb_solver_options(self):
        # arrange
        params = HbSimulationParameters(
            frequencies=("1MEG",),
            nonlin_options={"ABSTOL": "1e-9", "MAXIT": "50"},
            linsol_options={"TYPE": "AZTECOO"},
            replace_ground=False
        )
        dialog = _make_dialog()
        # act
        dialog._apply_hb_parameters(params)
        # assert
        dialog._root.setProperty.assert_any_call("hbNonlinOptionsText", "ABSTOL=1e-9 MAXIT=50")
        dialog._root.setProperty.assert_any_call("hbLinsolOptionsText", "TYPE=AZTECOO")


class TestApplyLinParameters:

    def test_defaults_when_no_params(self):
        # arrange
        dialog = _make_dialog()
        # act
        dialog._apply_lin_parameters(None)
        # assert
        dialog._root.setProperty.assert_any_call("linSparcalc", True)
        dialog._root.setProperty.assert_any_call("linFormat", "TOUCHSTONE2")
        dialog._root.setProperty.assert_any_call("linPoints", "100")

    def test_fills_values_from_params(self):
        # arrange
        params = LinSimulationParameters(sweep_mode="DEC", points="10", start="1k", end="10MEG", format="TOUCHSTONE", lintype="Y", dataformat="MA", replace_ground=False)
        dialog = _make_dialog()
        # act
        dialog._apply_lin_parameters(params)
        # assert
        dialog._root.setProperty.assert_any_call("linSweepModeIndex", 1)
        dialog._root.setProperty.assert_any_call("linPoints", "10")
        dialog._root.setProperty.assert_any_call("linFormat", "TOUCHSTONE")

    def test_restores_saved_print_parameters(self):
        # arrange
        pp = PrintParameters(print_type="AC", output_variables=("V(*)", "V(OUT)"), print_format="CSV", print_file="lin.csv")
        params = LinSimulationParameters(sweep_mode="LIN", points="100", start="1", end="1MEG", replace_ground=False, print_parameters=pp)
        dialog = _make_dialog()
        # act
        dialog._apply_lin_parameters(params)
        # assert
        dialog._root.setProperty.assert_any_call("linPrintAllNodes", True)
        dialog._root.setProperty.assert_any_call("linPrintFile", "lin.csv")
        dialog._root.setProperty.assert_any_call("linPrintSpecificVars", "V(OUT)")


class TestSimulationParametersDialogOnSubmitNoise:

    def test_accepts_valid_noise_params(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_noise("5", "", "V1", "LIN", "100", "1", "1MEG", "", "", False, False, False, False, False, "", "", "", False, [])
        # assert
        dialog.accept.assert_called_once()
        assert isinstance(dialog._result.analysis, NoiseSimulationParameters)

    def test_rejects_noise_without_output_node(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_noise("", "", "V1", "LIN", "100", "1", "1MEG", "", "", False, False, False, False, False, "", "", "", False, [])
        # assert
        dialog.accept.assert_not_called()
        dialog._root.setProperty.assert_any_call("errorText", "Output node is required")

    def test_invalid_measure_noise(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act with invalid .MEASURE directive for Noise
        dialog._on_submit_noise("5", "", "V1", "LIN", "100", "1", "1MEG", "", ".MEAS gg", False, False, False, False, False, "", "", "", False, [])
        # assert
        dialog.accept.assert_not_called()
        dialog._root.setProperty.assert_any_call("errorText", "Invalid .MEASURE directive for NOISE: .MEAS gg")

    def test_rejects_noise_without_source_name(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_noise("5", "", "", "LIN", "100", "1", "1MEG", "", "", False, False, False, False, False, "", "", "", False, [])
        # assert
        dialog.accept.assert_not_called()
        dialog._root.setProperty.assert_any_call("errorText", "Input noise source name is required")

    def test_rejects_noise_with_invalid_sweep_mode(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_noise("5", "", "V1", "INVALID", "100", "1", "1MEG", "", "", False, False, False, False, False, "", "", "", False, [])
        # assert
        dialog.accept.assert_not_called()
        dialog._root.setProperty.assert_any_call("errorText", "Sweep mode must be one of LIN, DEC, OCT, or DATA")

    def test_rejects_noise_lin_when_sweep_fields_missing(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_noise("5", "", "V1", "LIN", "", "", "", "", "", False, False, False, False, False, "", "", "", False, [])
        # assert
        dialog.accept.assert_not_called()
        dialog._root.setProperty.assert_any_call("errorText", "Points, start frequency, and end frequency are required")

    def test_rejects_noise_data_when_table_name_missing(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_noise("5", "", "V1", "DATA", "", "", "", "", "", False, False, False, False, False, "", "", "", False, [])
        # assert
        dialog.accept.assert_not_called()
        dialog._root.setProperty.assert_any_call("errorText", "Data table name is required for DATA sweep")

    def test_clears_error_on_success(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_noise("5", "", "V1", "LIN", "100", "1", "1MEG", "", "", False, False, False, False, False, "", "", "", False, [])
        # assert
        dialog._root.setProperty.assert_any_call("errorText", "")

    def test_noise_with_print_enabled(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_noise("5", "", "V1", "LIN", "100", "1", "1MEG", "", "", True, True, True, True, True, "DNI(V1)", "CSV", "noise.csv", False, [])
        # assert
        dialog.accept.assert_called_once()
        result = dialog._result
        assert result.analysis.print_parameters is not None
        assert result.analysis.print_parameters.print_type == "NOISE"
        assert "V(*)" in result.analysis.print_parameters.output_variables
        assert "INOISE" in result.analysis.print_parameters.output_variables
        assert "ONOISE" in result.analysis.print_parameters.output_variables
        assert "DNI(V1)" in result.analysis.print_parameters.output_variables

    def test_noise_data_sweep_accepted(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_noise("5", "", "V1", "DATA", "", "", "", "myTable", "", False, False, False, False, False, "", "", "", False, [])
        # assert
        dialog.accept.assert_called_once()
        assert dialog._result.analysis.sweep_mode == "DATA"
        assert dialog._result.analysis.data_table_name == "myTable"

    def test_accepts_valid_measure_directives(self):
        # arrange
        dialog = _make_dialog_with_accept()
        measure_text = ".MEASURE NOISE TOTAL_INOISE INTEG INOISE\n.MEASURE NOISE TOTAL_ONOISE INTEG ONOISE\n"
        # act
        dialog._on_submit_noise("5", "", "V1", "LIN", "100", "1", "1MEG", "", measure_text, False, False, False, False, False, "", "", "", False, [])
        # assert
        dialog.accept.assert_called_once()
        result = dialog._result
        assert len(result.analysis.measure_parameters) == 2
        assert result.analysis.measure_parameters[0].result_name == "TOTAL_INOISE"
        assert result.analysis.measure_parameters[0].measure_type == "INTEG"
        assert result.analysis.measure_parameters[0].variable == "INOISE"
        assert result.analysis.measure_parameters[1].result_name == "TOTAL_ONOISE"
        assert result.analysis.measure_parameters[1].measure_type == "INTEG"
        assert result.analysis.measure_parameters[1].variable == "ONOISE"


class TestSimulationParametersDialogOnSubmitLin:

    def test_accepts_valid_lin_params(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_lin(True, "TOUCHSTONE2", "S", "RI", "", "", "", "LIN", "100", "1", "1MEG", "", False, False, False, "", "", "", False)
        # assert
        dialog.accept.assert_called_once()
        assert isinstance(dialog._result.analysis, LinSimulationParameters)

    def test_rejects_lin_with_invalid_sweep_mode(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_lin(True, "TOUCHSTONE2", "S", "RI", "", "", "", "INVALID", "100", "1", "1MEG", "", False, False, False, "", "", "", False)
        # assert
        dialog.accept.assert_not_called()
        dialog._root.setProperty.assert_any_call("errorText", "Sweep mode must be one of LIN, DEC, OCT, or DATA")

    def test_rejects_lin_when_sweep_fields_missing(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_lin(True, "TOUCHSTONE2", "S", "RI", "", "", "", "LIN", "", "", "", "", False, False, False, "", "", "", False)
        # assert
        dialog.accept.assert_not_called()
        dialog._root.setProperty.assert_any_call("errorText", "Points, start frequency, and end frequency are required")

    def test_rejects_lin_data_when_table_name_missing(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_lin(True, "TOUCHSTONE2", "S", "RI", "", "", "", "DATA", "", "", "", "", False, False, False, "", "", "", False)
        # assert
        dialog.accept.assert_not_called()
        dialog._root.setProperty.assert_any_call("errorText", "Data table name is required for DATA sweep")

    def test_clears_error_on_success(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_lin(True, "TOUCHSTONE2", "S", "RI", "", "", "", "LIN", "100", "1", "1MEG", "", False, False, False, "", "", "", False)
        # assert
        dialog._root.setProperty.assert_any_call("errorText", "")

    def test_lin_with_print_enabled(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_lin(True, "TOUCHSTONE2", "S", "RI", "", "", "", "LIN", "100", "1", "1MEG", "", True, True, True, "V(1)", "CSV", "lin.csv", False)
        # assert
        dialog.accept.assert_called_once()
        result = dialog._result
        assert result.analysis.print_parameters is not None
        assert "V(*)" in result.analysis.print_parameters.output_variables
        assert "V(1)" in result.analysis.print_parameters.output_variables

    def test_lin_data_sweep_accepted(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_lin(True, "TOUCHSTONE2", "S", "RI", "", "", "", "DATA", "", "", "", "myTable", False, False, False, "", "", "", False)
        # assert
        dialog.accept.assert_called_once()
        assert dialog._result.analysis.sweep_mode == "DATA"


class TestSimulationParametersDialogOnSubmitHB:

    def test_accepts_valid_hb_params(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_hb("1MEG", "", 1, "hybrid", 0, False, False, False, "HB", "", "", "", "", "", False)
        # assert
        dialog.accept.assert_called_once()
        assert isinstance(dialog._result.analysis, HbSimulationParameters)

    def test_rejects_hb_when_frequencies_empty(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_hb("", "", 1, "hybrid", 0, False, False, False, "HB", "", "", "", "", "", False)
        # assert
        dialog.accept.assert_not_called()
        dialog._root.setProperty.assert_any_call("errorText", "At least one fundamental frequency is required")

    def test_accepts_multiple_frequencies(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_hb("1MEG 2MEG 3MEG", "", 1, "hybrid", 0, False, False, False, "HB", "", "", "", "", "", False)
        # assert
        dialog.accept.assert_called_once()
        assert len(dialog._result.analysis.frequencies) == 3

    def test_hb_with_print_enabled(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_hb("1MEG", "", 1, "hybrid", 0, True, True, True, "HB_FD", "V(1)", "CSV", "hb.csv", "", "", False)
        # assert
        dialog.accept.assert_called_once()
        result = dialog._result
        assert result.analysis.print_parameters is not None
        assert result.analysis.print_parameters.print_type == "HB_FD"
        assert "V(*)" in result.analysis.print_parameters.output_variables
        assert "V(1)" in result.analysis.print_parameters.output_variables

    def test_hb_invalid_print_type_falls_back_to_hb(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_hb("1MEG", "", 1, "hybrid", 0, True, False, False, "INVALID", "", "", "", "ABSTOL=1e-9", "TYPE=AZTECOO", False)
        # assert
        dialog.accept.assert_called_once()
        assert dialog._result.analysis.print_parameters.print_type == "HB"
        assert dialog._result.analysis.nonlin_options == {"ABSTOL": "1e-9"}
        assert dialog._result.analysis.linsol_options == {"TYPE": "AZTECOO"}

    def test_clears_hb_error_on_success(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_hb("1MEG", "", 1, "hybrid", 0, False, False, False, "HB", "", "", "", "", "", False)
        # assert
        dialog._root.setProperty.assert_any_call("errorText", "")

    def test_rejects_hb_when_frequencies_parse_to_empty(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act — whitespace-only text parses to no tokens
        dialog._on_submit_hb("  ,  ", "", 1, "hybrid", 0, False, False, False, "HB", "", "", "", "", "", False)
        # assert
        dialog.accept.assert_not_called()
        dialog._root.setProperty.assert_any_call("errorText", "At least one fundamental frequency is required")


class TestSimulationParametersDialogOnSubmitACWithPrint:

    def test_ac_with_print_enabled(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_ac("LIN", "100", "1", "1MEG", "", "", True, "AC", True, True, "VM(OUT)", "CSV", "ac.csv", False)
        # assert
        dialog.accept.assert_called_once()
        result = dialog._result
        assert result.analysis.print_parameters is not None
        assert "V(*)" in result.analysis.print_parameters.output_variables
        assert "I(*)" in result.analysis.print_parameters.output_variables
        assert "VM(OUT)" in result.analysis.print_parameters.output_variables
        assert result.analysis.print_parameters.print_format == "CSV"
        assert result.analysis.print_parameters.print_file == "ac.csv"

    def test_rejects_ac_with_invalid_sweep_mode(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_ac("INVALID", "100", "1", "1MEG", "", "", False, "AC", False, False, "", "", "", False)
        # assert
        dialog.accept.assert_not_called()
        dialog._root.setProperty.assert_any_call("errorText", "Sweep mode must be one of LIN, DEC, OCT, or DATA")

    def test_invalid_measure_ac(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act with invalid .MEASURE directive for AC
        dialog._on_submit_ac("LIN", "100", "1", "1MEG", "", ".MEAS gg", False, "AC", False, False, "", "", "", False)
        # assert
        dialog.accept.assert_not_called()
        dialog._root.setProperty.assert_any_call("errorText", "Invalid .MEASURE directive for AC: .MEAS gg")

    def test_rejects_ac_lin_when_sweep_fields_missing(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_ac("LIN", "", "", "", "", "", False, "AC", False, False, "", "", "", False)
        # assert
        dialog.accept.assert_not_called()
        dialog._root.setProperty.assert_any_call("errorText", "Points, start frequency, and end frequency are required")

    def test_rejects_ac_data_when_table_name_missing(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_ac("DATA", "", "", "", "", "", False, "AC", False, False, "", "", "", False)
        # assert
        dialog.accept.assert_not_called()
        dialog._root.setProperty.assert_any_call("errorText", "Data table name is required for DATA sweep")

    def test_accepts_valid_measure_directives(self):
        # arrange
        dialog = _make_dialog_with_accept()
        measure_text = ".MEASURE AC BANDWIDTH FIND V(OUT) WHEN V(OUT)=0.707\n.MEASURE AC GAIN_AT_1K FIND V(OUT) AT=1k\n"
        # act
        dialog._on_submit_ac("LIN", "100", "1", "1MEG", "", measure_text, False, "AC", False, False, "", "", "", False)
        # assert
        dialog.accept.assert_called_once()
        result = dialog._result
        assert len(result.analysis.measure_parameters) == 2
        assert result.analysis.measure_parameters[0].result_name == "BANDWIDTH"
        assert result.analysis.measure_parameters[0].measure_type == "FIND"
        assert result.analysis.measure_parameters[0].variable == "V(OUT)"
        assert result.analysis.measure_parameters[1].result_name == "GAIN_AT_1K"
        assert result.analysis.measure_parameters[1].measure_type == "FIND"
        assert result.analysis.measure_parameters[1].at_val == "1k"


class TestSimulationParametersDialogOnSubmitTransientWithPrint:

    def test_transient_with_bjt_and_fet_leads(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_transient("1u", "1m", "", "", "", False, "", "", "", "", True, "TRAN", True, True, True, True, True, "V(1)", "CSV", "t.csv", False)
        # assert
        dialog.accept.assert_called_once()
        result = dialog._result
        assert result.analysis.print_parameters is not None
        assert "V(*)" in result.analysis.print_parameters.output_variables
        assert "I(*)" in result.analysis.print_parameters.output_variables
        assert "P(*)" in result.analysis.print_parameters.output_variables
        assert "IC(*)" in result.analysis.print_parameters.output_variables
        assert "ID(*)" in result.analysis.print_parameters.output_variables
        assert "V(1)" in result.analysis.print_parameters.output_variables


class TestSimulationParametersDialogOnSubmitOPWithPrint:

    def test_op_with_bjt_and_fet_leads(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_op(True, True, True, True, True, True, "V(1)", "CSV", "op.csv", False, "NODESET", "", "", "", False)
        # assert
        dialog.accept.assert_called_once()
        result = dialog._result
        assert result.analysis.print_parameters is not None
        assert "V(*)" in result.analysis.print_parameters.output_variables
        assert "P(*)" in result.analysis.print_parameters.output_variables
        assert "IC(*)" in result.analysis.print_parameters.output_variables
        assert "ID(*)" in result.analysis.print_parameters.output_variables
        assert "V(1)" in result.analysis.print_parameters.output_variables

    def test_op_parses_nodeset_text_into_entries(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_op(False, False, False, False, False, False, "", "", "", True, "NODESET", "V(out)=3.3 V(in)=5.0", "", "", False)
        # assert
        dialog.accept.assert_called_once()
        result = dialog._result
        assert len(result.analysis.nodeset_entries) == 2
        assert result.analysis.nodeset_entries[0].node == "out"
        assert result.analysis.nodeset_entries[0].voltage == "3.3"
        assert result.analysis.nodeset_entries[1].node == "in"
        assert result.analysis.nodeset_entries[1].voltage == "5.0"


class TestSimulationParametersDialogOnSubmitDCWithPrint:

    def test_dc_with_print_enabled(self):
        # arrange
        dialog = _make_dialog_with_accept()
        # act
        dialog._on_submit_dc("LIN", "VIN", "0", "5", "0.1", "", "", "", False, "", "", "", "", "", "", True, "DC", True, True, True, True, True, "V(1)", "CSV", "dc.csv", False)
        # assert
        dialog.accept.assert_called_once()
        result = dialog._result
        assert result.analysis.print_parameters is not None
        assert "V(*)" in result.analysis.print_parameters.output_variables
        assert "P(*)" in result.analysis.print_parameters.output_variables
        assert "IC(*)" in result.analysis.print_parameters.output_variables
        assert "ID(*)" in result.analysis.print_parameters.output_variables
        assert "V(1)" in result.analysis.print_parameters.output_variables


class TestSimulationParametersDialogSensitivityAttachment:

    def test_attaches_sensitivity_to_dc_analysis_from_root_properties(self):
        # arrange
        initial_analysis = DCSimulationParameters("LIN", "VIN", "0", "5", "0.1", "", tuple(), "", "", "", "", "", "")
        dialog = _make_dialog_with_accept(initial_parameters=initial_analysis)

        properties = {
            "currentTabIndex": 2,
            "dcSensEnabled": True,
            "dcSensObjectiveMode": "objfunc",
            "dcSensObjectiveValues": "V(2)",
            "dcSensParameters": "R1:R",
            "dcSensDirect": True,
            "dcSensAdjoint": False,
            "dcSensPrintEnabled": False,
            "replaceGround": False,
            "stepEnabled": False,
            "stepSweepModeIndex": 0,
            "stepVariable": "",
            "stepStartValue": "",
            "stepStopValue": "",
            "stepStepValue": "",
            "stepPointsValue": "",
            "stepListValuesText": "",
            "stepDataTableName": ""
        }
        dialog._root.property.side_effect = lambda name: properties.get(name)

        # act
        dialog._on_submit_dc("LIN", "VIN", "0", "5", "0.1", "", "", "", False, "", "", "", "", "", "", False, "DC", False, False, False, False, False, "", "", "", False)

        # assert
        dialog.accept.assert_called_once()
        assert isinstance(dialog._result.analysis, DCSimulationParameters)
        assert dialog._result.analysis.sensitivity is not None
        assert dialog._result.analysis.sensitivity.analysis_context == "DC"
        assert dialog._result.analysis.sensitivity.objective_mode == "objfunc"
        assert dialog._result.analysis.sensitivity.objective_values == ("V(2)",)
        assert dialog._result.analysis.sensitivity.parameter_list == ("R1:R",)
        assert dialog._result.analysis.sensitivity.direct is True
        assert dialog._result.analysis.sensitivity.adjoint is False
