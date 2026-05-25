import logging
import re
from pathlib import Path

from PySide6.QtCore import Qt, QUrl, Slot
from PySide6.QtGui import QColor
from PySide6.QtQuick import QQuickView
from PySide6.QtWidgets import QDialog, QVBoxLayout, QWidget

from netlist_parser import NetlistTopology
from .ac_simulation_parameters import AcSimulationParameters
from .ac_panel import AcPanel
from .dc_simulation_parameters import DCSimulationParameters
from .dc_panel import DcPanel
from .hb_simulation_parameters import HbSimulationParameters
from .hb_panel import HbPanel
from .lin_simulation_parameters import LinSimulationParameters
from .lin_panel import LinPanel
from .noise_simulation_parameters import NoiseSimulationParameters
from .noise_panel import NoisePanel
from .op_simulation_parameters import OpSimulationParameters
from .op_panel import OpPanel
from .sensitivity_section import SensitivitySection
from .sens_parameter import SensParameter
from .simulation_config import SimulationConfig
from .step_parameters import StepParameters
from .transient_simulation_parameters import TransientSimulationParameters, TransientSchedulePoint
from .tran_panel import TranPanel

logger = logging.getLogger(__name__)

_QML_FILE = Path(__file__).parent / "simulation_parameters_dialog.qml"
_BG = "#efefe8"


def _parse_list_values(list_values_text: str) -> tuple[str, ...]:
    # return empty tuple immediately when no text was provided
    if not list_values_text:
        return tuple()
    # split on any combination of whitespace and commas to support flexible input
    raw_tokens = re.split(r"[\s,]+", list_values_text)
    # filter out empty fragments introduced by leading/trailing separators
    tokens = [token for token in raw_tokens if token]
    # freeze the parsed value sequence into an immutable tuple
    return tuple(tokens)


class SimulationParametersDialog(QDialog):

    def __init__(self, parent: QWidget | None, initial_parameters: SimulationConfig, topology: NetlistTopology | None = None):
        super().__init__(parent)
        # set modal
        self.setWindowModality(Qt.WindowModality.ApplicationModal)
        # capture initial config for form pre-population
        self._initial_parameters = initial_parameters
        # store topology for deriving variable candidates for the print sections
        self._topology = topology
        # keep the result empty until the user confirms valid values
        self._result: SimulationConfig | None = None
        # run ui setup logic
        self._setup_ui()

    def _setup_ui(self) -> None:
        # set dialog metadata for the native window frame
        self.setWindowTitle("Xyce Simulation")
        # create the qml surface that renders the form
        self._qml_view = QQuickView()
        # connect qml-ready lifecycle hook before loading qml
        self._qml_view.statusChanged.connect(self._on_qml_ready)
        # keep the qml root sized to the embedded window container
        self._qml_view.setResizeMode(QQuickView.ResizeMode.SizeRootObjectToView)
        # match the window background to the rest of the application
        self._qml_view.setColor(QColor(_BG))
        # load the simulation dialog qml component
        self._qml_view.setSource(QUrl.fromLocalFile(str(_QML_FILE)))
        # wrap the qml view in a widget so it can be hosted by qdialog
        self._container = QWidget.createWindowContainer(self._qml_view, self)
        # set up a simple one-widget dialog layout
        self._layout = QVBoxLayout(self)
        # remove margins so qml controls align edge-to-edge
        self._layout.setContentsMargins(0, 0, 0, 0)
        # insert the qml container into the dialog layout
        self._layout.addWidget(self._container)
        # set an initial size that fits both tab forms on first open
        self.resize(800, 600)

    @Slot(QQuickView.Status)
    def _on_qml_ready(self, status: QQuickView.Status) -> None:
        # skip setup until qml finishes loading
        if status != QQuickView.Status.Ready:
            return
        # capture the qml root for signal wiring and property updates
        self._root = self._qml_view.rootObject()
        # instantiate panel helpers that own apply/handle_submit logic per simulation type
        self._op_panel = OpPanel(self._root)
        self._tran_panel = TranPanel(self._root)
        self._dc_panel = DcPanel(self._root)
        self._ac_panel = AcPanel(self._root)
        self._sensitivity_section = SensitivitySection(self._root)
        self._noise_panel = NoisePanel(self._root)
        self._hb_panel = HbPanel(self._root)
        self._lin_panel = LinPanel(self._root)
        # build topology-derived variable candidate lists before applying parameters
        self._build_variable_candidates()
        # initialize defaults and apply initial parameters if available
        self._apply_initial_parameters()
        # connect qml submit signals to python validation handlers
        self._root.submitOP.connect(self._on_submit_op)
        self._root.submitTransient.connect(self._on_submit_transient)
        self._root.submitDC.connect(self._on_submit_dc)
        self._root.submitAC.connect(self._on_submit_ac)
        self._root.submitNoise.connect(self._on_submit_noise)
        self._root.submitHB.connect(self._on_submit_hb)
        self._root.submitLIN.connect(self._on_submit_lin)
        # connect qml cancel signal to close without a result
        self._root.cancelRequested.connect(self.reject)

    def _build_variable_candidates(self) -> None:
        # return empty lists when no topology is available
        if self._topology is None:
            self._node_voltages: list[str] = []
            self._device_currents: list[str] = []
            self._has_bjt_devices: bool = False
            self._has_fet_devices: bool = False
            return
        # derive node voltage expressions for all non-ground nodes
        self._node_voltages = sorted(f"V({n})" for n in self._topology.nodes if n != "0")
        # derive device current expressions for current-carrying element types
        self._device_currents = sorted(f"I({d.name})" for d in self._topology.devices if d.type_letter in {"V", "L", "E", "H"})
        # detect device families that have topology-specific lead current wildcards
        self._has_bjt_devices = any(d.type_letter == "Q" for d in self._topology.devices)
        self._has_fet_devices = any(d.type_letter in {"M", "J", "Z"} for d in self._topology.devices)

    def _apply_initial_parameters(self) -> None:
        # always initialize all tabs to ensure a clean state
        p = self._initial_parameters.analysis
        # delegate panel initialization to each panel's apply() method
        self._apply_op_parameters(p if isinstance(p, OpSimulationParameters) else None)
        self._apply_transient_parameters(p if isinstance(p, TransientSimulationParameters) else None)
        self._apply_dc_parameters(p if isinstance(p, DCSimulationParameters) else None)
        self._apply_ac_parameters(p if isinstance(p, AcSimulationParameters) else None)
        self._apply_noise_parameters(p if isinstance(p, NoiseSimulationParameters) else None)
        self._apply_hb_parameters(p if isinstance(p, HbSimulationParameters) else None)
        self._apply_lin_parameters(p if isinstance(p, LinSimulationParameters) else None)

        # initialize the shared step parameters
        self._apply_step_parameters(self._initial_parameters.step)
        # initialize embedded sensitivity sections and apply the companion sensitivity parameters
        self._sensitivity_section.apply(None, None)
        if isinstance(p, TransientSimulationParameters):
            self._sensitivity_section.apply(p.sensitivity, "TRAN")
        elif isinstance(p, DCSimulationParameters):
            self._sensitivity_section.apply(p.sensitivity, "DC")
        elif isinstance(p, AcSimulationParameters):
            self._sensitivity_section.apply(p.sensitivity, "AC")
        # initialize the shared replace ground checkbox
        self._root.setProperty("replaceGround", p.replace_ground if p else False)
        # select the appropriate tab based on the parameter type
        if isinstance(p, OpSimulationParameters):
            self._root.setProperty("initialTabIndex", 0)
        elif isinstance(p, TransientSimulationParameters):
            self._root.setProperty("initialTabIndex", 1)
        elif isinstance(p, DCSimulationParameters):
            self._root.setProperty("initialTabIndex", 2)
        elif isinstance(p, AcSimulationParameters):
            self._root.setProperty("initialTabIndex", 3)
        elif isinstance(p, NoiseSimulationParameters):
            self._root.setProperty("initialTabIndex", 4)
        elif isinstance(p, HbSimulationParameters):
            self._root.setProperty("initialTabIndex", 5)
        elif isinstance(p, LinSimulationParameters):
            self._root.setProperty("initialTabIndex", 6)
        else:
            self._root.setProperty("initialTabIndex", 0)

    def _apply_op_parameters(self, p: OpSimulationParameters | None) -> None:
        self._op_panel.apply(p, self._has_bjt_devices, self._has_fet_devices)

    def _apply_transient_parameters(self, p: TransientSimulationParameters | None) -> None:
        self._tran_panel.apply(p, self._has_bjt_devices, self._has_fet_devices)

    def _apply_dc_parameters(self, p: DCSimulationParameters | None) -> None:
        self._dc_panel.apply(p, self._has_bjt_devices, self._has_fet_devices)

    def _apply_ac_parameters(self, p: AcSimulationParameters | None) -> None:
        self._ac_panel.apply(p)

    def _apply_noise_parameters(self, p: NoiseSimulationParameters | None) -> None:
        self._noise_panel.apply(p)

    def _apply_hb_parameters(self, p: HbSimulationParameters | None) -> None:
        self._hb_panel.apply(p)

    def _apply_lin_parameters(self, p: LinSimulationParameters | None) -> None:
        self._lin_panel.apply(p)

    def _apply_step_parameters(self, p: StepParameters) -> None:
        # sweep mode index mapping
        modes = ["LIN", "DEC", "OCT", "LIST", "DATA"]
        mode_index = modes.index(p.sweep_mode) if p.sweep_mode in modes else 0
        # set properties in qml
        self._root.setProperty("stepEnabled", p.enabled)
        self._root.setProperty("stepSweepModeIndex", mode_index)
        self._root.setProperty("stepVariable", p.variable)
        self._root.setProperty("stepStartValue", p.start)
        self._root.setProperty("stepStopValue", p.stop)
        self._root.setProperty("stepStepValue", p.step)
        self._root.setProperty("stepPointsValue", p.points)
        self._root.setProperty("stepListValuesText", " ".join(p.list_values))
        self._root.setProperty("stepDataTableName", p.data_table_name)

    @Slot(bool, str, str, str, str, str, str, str, str)
    def _on_submit_step(self, enabled: bool, sweep_mode: str, variable: str, start: str, stop: str, step: str, points: str, list_values_text: str, data_table_name: str) -> None:
        # this slot captures step changes from the ui — no immediate validation required as it is additive
        pass

    def _get_current_step_parameters(self) -> StepParameters:
        # extract current step values from qml properties
        enabled = self._root.property("stepEnabled")
        # map index back to keyword
        modes = ["LIN", "DEC", "OCT", "LIST", "DATA"]
        mode_index = self._root.property("stepSweepModeIndex")
        sweep_mode = modes[mode_index] if 0 <= mode_index < len(modes) else "LIN"
        # collect other fields
        variable = self._root.property("stepVariable")
        start = self._root.property("stepStartValue")
        stop = self._root.property("stepStopValue")
        step = self._root.property("stepStepValue")
        points = self._root.property("stepPointsValue")
        list_text = self._root.property("stepListValuesText")
        data_table = self._root.property("stepDataTableName")
        # parse list values
        list_values = _parse_list_values(list_text)
        # return model
        return StepParameters(sweep_mode, variable, start, stop, step, points, list_values, data_table, enabled)

    def _get_current_sens_parameters(self) -> SensParameter | None:
        return self._sensitivity_section.get_current()

    def _parse_schedule_points(self, schedule_pairs_text: str) -> tuple[TransientSchedulePoint, ...]:
        # return an empty schedule when no schedule text was provided
        if not schedule_pairs_text:
            return tuple()
        # split schedule text on commas and whitespace to support flexible input
        raw_tokens = re.split(r"[\s,]+", schedule_pairs_text)
        # remove empty token fragments introduced by split boundaries
        tokens = [raw_token for raw_token in raw_tokens if raw_token]
        # enforce alternating time,max-step token pairs
        if len(tokens) % 2 != 0:
            # raise parse error with expected input format guidance
            raise ValueError("Schedule format must use time,max-step pairs")
        # initialize list used to collect structured schedule points
        schedule_points = []
        # iterate all token pairs in the original user-provided order
        for index in range(0, len(tokens), 2):
            # build one schedule point from the current token pair
            schedule_points.append(TransientSchedulePoint(tokens[index], tokens[index + 1]))
        # freeze the schedule points so dialog result stays immutable
        return tuple(schedule_points)

    @Slot(str, str, str, str, str, str, str, str, str, bool, bool, bool, bool, bool, str, str, str, bool, 'QVariantList')
    def _on_submit_noise(self, output_node: str, ref_node: str, source_name: str, sweep_mode: str, points: str, start: str, end: str, data_table_name: str, measure_parameters_text: str, print_enabled: bool, print_all_nodes: bool, print_all_currents: bool, print_inoise: bool, print_onoise: bool, print_specific_vars: str, print_format: str, print_file: str, replace_ground: bool, device_operators_list) -> None:
        # delegate validation and construction to the noise panel
        analysis = self._noise_panel.handle_submit(output_node, ref_node, source_name, sweep_mode, points, start, end, data_table_name, measure_parameters_text, print_enabled, print_all_nodes, print_all_currents, print_inoise, print_onoise, print_specific_vars, print_format, print_file, replace_ground, device_operators_list)
        # return without accepting when validation failed
        if analysis is None:
            return
        # assemble final config and close dialog
        self._result = SimulationConfig(analysis=analysis, step=self._get_current_step_parameters())
        # close the dialog and return acceptance to the caller
        self.accept()

    @Slot(bool, str, str, str, str, str, str, str, str, str, str, str, bool, bool, bool, str, str, str, bool)
    def _on_submit_lin(self, sparcalc: bool, fmt: str, lintype: str, dataformat: str, file: str, width: str, precision: str, sweep_mode: str, points: str, start: str, end: str, data_table_name: str, print_enabled: bool, print_all_nodes: bool, print_all_currents: bool, print_specific_vars: str, print_format: str, print_file: str, replace_ground: bool) -> None:
        # delegate validation and construction to the lin panel
        analysis = self._lin_panel.handle_submit(sparcalc, fmt, lintype, dataformat, file, width, precision, sweep_mode, points, start, end, data_table_name, print_enabled, print_all_nodes, print_all_currents, print_specific_vars, print_format, print_file, replace_ground)
        # return without accepting when validation failed
        if analysis is None:
            return
        # assemble final config and close dialog
        try:
            self._result = SimulationConfig(analysis=analysis, step=self._get_current_step_parameters())
        except ValueError as e:
            self._root.setProperty("errorText", str(e))
            return
        self.accept()

    @Slot(str, str, int, str, int, bool, bool, bool, str, str, str, str, str, str, bool)
    def _on_submit_hb(self, frequencies_text: str, harmonics_text: str, tahb: int, selectharms: str, startup_periods: int, print_enabled: bool, print_all_nodes: bool, print_all_currents: bool, print_type: str, print_specific_vars: str, print_format: str, print_file: str, nonlin_options_text: str, linsol_options_text: str, replace_ground: bool) -> None:
        # delegate validation and construction to the hb panel
        analysis = self._hb_panel.handle_submit(frequencies_text, harmonics_text, tahb, selectharms, startup_periods, print_enabled, print_all_nodes, print_all_currents, print_type, print_specific_vars, print_format, print_file, nonlin_options_text, linsol_options_text, replace_ground)
        # return without accepting when validation failed
        if analysis is None:
            return
        # assemble final config and close dialog
        try:
            self._result = SimulationConfig(analysis=analysis, step=self._get_current_step_parameters())
        except ValueError as e:
            self._root.setProperty("errorText", str(e))
            return
        self.accept()

    @Slot(str, str, str, str, str, str, bool, str, bool, bool, str, str, str, bool)
    def _on_submit_ac(self, sweep_mode: str, points: str, start: str, end: str, data_table_name: str, measure_parameters_text: str, print_enabled: bool, print_type: str, print_all_nodes: bool, print_all_currents: bool, print_specific_vars: str, print_format: str, print_file: str, replace_ground: bool) -> None:
        # delegate validation and construction to the ac panel
        analysis = self._ac_panel.handle_submit(sweep_mode, points, start, end, data_table_name, measure_parameters_text, print_enabled, print_type, print_all_nodes, print_all_currents, print_specific_vars, print_format, print_file, replace_ground, self._get_current_sens_parameters())
        # return without accepting when validation failed
        if analysis is None:
            return
        # assemble final config and close dialog
        try:
            self._result = SimulationConfig(analysis=analysis, step=self._get_current_step_parameters())
        except ValueError as e:
            self._root.setProperty("errorText", str(e))
            return
        self.accept()

    @Slot(str, str, str, str, str, bool, str, str, str, str, bool, str, bool, bool, bool, bool, bool, str, str, str, bool)
    def _on_submit_transient(self, initial_step: str, final_time: str, start_time: str, step_ceiling: str, op_keyword: str, schedule_enabled: bool, schedule_pairs_text: str, fft_parameters_text: str, four_parameters_text: str, measure_parameters_text: str, print_enabled: bool, print_type: str, print_all_nodes: bool, print_all_currents: bool, print_power: bool, print_bjt_leads: bool, print_fet_leads: bool, print_specific_vars: str, print_format: str, print_file: str, replace_ground: bool) -> None:
        # delegate validation and construction to the tran panel
        analysis = self._tran_panel.handle_submit(initial_step, final_time, start_time, step_ceiling, op_keyword, schedule_enabled, schedule_pairs_text, fft_parameters_text, four_parameters_text, measure_parameters_text, print_enabled, print_type, print_all_nodes, print_all_currents, print_power, print_bjt_leads, print_fet_leads, print_specific_vars, print_format, print_file, replace_ground, self._get_current_sens_parameters())
        # return without accepting when validation failed
        if analysis is None:
            return
        # assemble final config and close dialog
        try:
            self._result = SimulationConfig(analysis=analysis, step=self._get_current_step_parameters())
        except ValueError as e:
            self._root.setProperty("errorText", str(e))
            return
        self.accept()

    @Slot(bool, bool, bool, bool, bool, bool, str, str, str, bool, str, str, str, str, bool)
    def _on_submit_op(self, print_enabled: bool, print_all_nodes: bool, print_all_currents: bool, print_power: bool, print_bjt_leads: bool, print_fet_leads: bool, print_specific_vars: str, print_format: str, print_file: str, save_enabled: bool, save_type: str, nodeset_text: str, initial_conditions_text: str, save_file: str, replace_ground: bool) -> None:
        # delegate construction to the op panel (no validation failure path for OP)
        analysis = self._op_panel.handle_submit(print_enabled, print_all_nodes, print_all_currents, print_power, print_bjt_leads, print_fet_leads, print_specific_vars, print_format, print_file, save_enabled, save_type, nodeset_text, initial_conditions_text, save_file, replace_ground)
        # assemble final config and close dialog
        self._result = SimulationConfig(analysis=analysis, step=self._get_current_step_parameters())
        # close the dialog and return acceptance to the caller
        self.accept()

    @Slot(str, str, str, str, str, str, str, str, bool, str, str, str, str, str, str, bool, str, bool, bool, bool, bool, bool, str, str, str, bool)
    def _on_submit_dc(self, sweep_mode: str, primary_variable: str, start: str, stop: str, step: str, points: str, list_values_text: str, data_table_name: str, secondary_enabled: bool, secondary_variable: str, secondary_start: str, secondary_stop: str, secondary_step: str, secondary_points: str, measure_parameters_text: str, print_enabled: bool, print_type: str, print_all_nodes: bool, print_all_currents: bool, print_power: bool, print_bjt_leads: bool, print_fet_leads: bool, print_specific_vars: str, print_format: str, print_file: str, replace_ground: bool) -> None:
        # delegate validation and construction to the dc panel
        analysis = self._dc_panel.handle_submit(sweep_mode, primary_variable, start, stop, step, points, list_values_text, data_table_name, secondary_enabled, secondary_variable, secondary_start, secondary_stop, secondary_step, secondary_points, measure_parameters_text, print_enabled, print_type, print_all_nodes, print_all_currents, print_power, print_bjt_leads, print_fet_leads, print_specific_vars, print_format, print_file, replace_ground, self._get_current_sens_parameters())
        # return without accepting when validation failed
        if analysis is None:
            return
        # assemble final config and close dialog
        try:
            self._result = SimulationConfig(analysis=analysis, step=self._get_current_step_parameters())
        except ValueError as e:
            self._root.setProperty("errorText", str(e))
            return
        self.accept()

    def get_parameters(self) -> SimulationConfig | None:
        return self._result
