import logging
import re
from pathlib import Path

from PySide6.QtCore import Qt, QUrl, Slot
from PySide6.QtGui import QColor
from PySide6.QtQuick import QQuickView
from PySide6.QtWidgets import QDialog, QVBoxLayout, QWidget

from netlist_parser import NetlistTopology
from .ac_simulation_parameters import AcSimulationParameters
from .dc_simulation_parameters import DCSimulationParameters
from .fft_parameters import FftParameters
from .four_parameters import FourParameters
from .hb_simulation_parameters import HbSimulationParameters
from .lin_simulation_parameters import LinSimulationParameters
from .noise_simulation_parameters import DeviceNoiseOperator, NoiseSimulationParameters
from .op_simulation_parameters import OpSimulationParameters, NodesetEntry
from .print_parameters import PrintParameters
from .transient_simulation_parameters import TransientSchedulePoint, TransientSimulationParameters

logger = logging.getLogger(__name__)

_QML_FILE = Path(__file__).parent / "simulation_parameters_dialog.qml"
_BG = "#efefe8"

_DC_SWEEP_MODES = {"LIN", "DEC", "OCT", "LIST", "DATA"}
_DC_SECONDARY_MODES = {"LIN", "DEC", "OCT"}
# valid sweep modes for the ac directive
_AC_SWEEP_MODES = {"LIN", "DEC", "OCT", "DATA"}
# supported hb print output types
_HB_PRINT_TYPES = ("HB", "HB_FD", "HB_TD")

# print format values matching the combo model order (index 0 is the empty/default value)
_PRINT_FORMATS = ["", "STD", "NOINDEX", "PROBE", "TECPLOT", "RAW", "CSV", "GNUPLOT", "SPLOT"]

# wildcard tokens that map to dedicated checkbox shortcuts in the print section
_PRINT_WILDCARDS = {"V(*)", "I(*)", "P(*)", "W(*)", "IB(*)", "IC(*)", "ID(*)", "IE(*)", "IG(*)", "IS(*)"}

# ordered lead current wildcards per device family for output emission order
_BJT_WILDCARDS = ("IB(*)", "IC(*)", "IE(*)", "IS(*)")
_FET_WILDCARDS = ("IB(*)", "ID(*)", "IG(*)", "IS(*)")


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


def _validate_device_name(device_name: str) -> bool:
    # check for empty or whitespace-only device name
    if not device_name or not device_name.strip():
        return False
    # validate device name as identifier (alphanumeric, underscore, must start with letter or underscore)
    return bool(re.match(r"^[a-zA-Z_][a-zA-Z0-9_]*$", device_name.strip()))


class SimulationParametersDialog(QDialog):

    def __init__(self, parent: QWidget | None, initial_parameters: AcSimulationParameters | HbSimulationParameters | LinSimulationParameters | NoiseSimulationParameters | TransientSimulationParameters | DCSimulationParameters | OpSimulationParameters, topology: NetlistTopology | None = None):
        super().__init__(parent)
        # set modal
        self.setWindowModality(Qt.WindowModality.ApplicationModal)
        # capture initial parameters for form pre-population
        self._initial_parameters = initial_parameters
        # store topology for deriving variable candidates for the print sections
        self._topology = topology
        # keep the result empty until the user confirms valid values
        self._result: AcSimulationParameters | HbSimulationParameters | LinSimulationParameters | NoiseSimulationParameters | TransientSimulationParameters | DCSimulationParameters | OpSimulationParameters | None = None
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
        self.resize(640, 680)

    @Slot(QQuickView.Status)
    def _on_qml_ready(self, status: QQuickView.Status) -> None:
        # skip setup until qml finishes loading
        if status != QQuickView.Status.Ready:
            return
        # capture the qml root for signal wiring and property updates
        self._root = self._qml_view.rootObject()
        # build topology-derived variable candidate lists before applying parameters
        self._build_variable_candidates()
        # initialize defaults and apply initial parameters if available
        self._apply_initial_parameters()
        # connect qml submit signals to python validation handlers
        self._root.submitTransient.connect(self._on_submit_transient)
        self._root.submitDC.connect(self._on_submit_dc)
        self._root.submitOP.connect(self._on_submit_op)
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
        self._apply_transient_parameters(self._initial_parameters if isinstance(self._initial_parameters, TransientSimulationParameters) else None)
        self._apply_dc_parameters(self._initial_parameters if isinstance(self._initial_parameters, DCSimulationParameters) else None)
        self._apply_op_parameters(self._initial_parameters if isinstance(self._initial_parameters, OpSimulationParameters) else None)
        self._apply_ac_parameters(self._initial_parameters if isinstance(self._initial_parameters, AcSimulationParameters) else None)
        self._apply_hb_parameters(self._initial_parameters if isinstance(self._initial_parameters, HbSimulationParameters) else None)
        self._apply_lin_parameters(self._initial_parameters if isinstance(self._initial_parameters, LinSimulationParameters) else None)
        self._apply_noise_parameters(self._initial_parameters if isinstance(self._initial_parameters, NoiseSimulationParameters) else None)
        # initialize the shared replace ground checkbox
        self._root.setProperty("replaceGround", self._initial_parameters.replace_ground if self._initial_parameters else False)
        # select the appropriate tab based on the parameter type
        if isinstance(self._initial_parameters, TransientSimulationParameters):
            self._root.setProperty("initialTabIndex", 1)
        elif isinstance(self._initial_parameters, DCSimulationParameters):
            self._root.setProperty("initialTabIndex", 2)
        elif isinstance(self._initial_parameters, AcSimulationParameters):
            self._root.setProperty("initialTabIndex", 3)
        elif isinstance(self._initial_parameters, NoiseSimulationParameters):
            self._root.setProperty("initialTabIndex", 4)
        elif isinstance(self._initial_parameters, HbSimulationParameters):
            self._root.setProperty("initialTabIndex", 5)
        elif isinstance(self._initial_parameters, LinSimulationParameters):
            self._root.setProperty("initialTabIndex", 6)
        else:
            self._root.setProperty("initialTabIndex", 0)

    def _apply_op_parameters(self, p: OpSimulationParameters | None) -> None:
        # extract print parameters for pre-population (prefer print_parameters over legacy fields)
        pp = p.print_parameters if p else None
        # default to enabled with all wildcards and RAW format when no saved print parameters exist
        if pp is None:
            self._root.setProperty("opPrintEnabled", True)
            self._root.setProperty("opPrintAllNodes", True)
            self._root.setProperty("opPrintAllCurrents", True)
            self._root.setProperty("opPrintPower", True)
            # default lead current checkboxes to on when the device family is present
            self._root.setProperty("opPrintBjtLeads", self._has_bjt_devices)
            self._root.setProperty("opPrintFetLeads", self._has_fet_devices)
            # expose device family flags for conditional checkbox visibility
            self._root.setProperty("opHasBjtDevices", self._has_bjt_devices)
            self._root.setProperty("opHasFetDevices", self._has_fet_devices)
            self._root.setProperty("opPrintFormatIndex", _PRINT_FORMATS.index("RAW"))
            self._root.setProperty("opPrintFile", "")
            self._root.setProperty("opPrintSpecificVars", "")
        else:
            # restore enabled state from saved parameters
            self._root.setProperty("opPrintEnabled", True)
            # index saved output variables for quick wildcard lookup
            selected = set(pp.output_variables)
            # check wildcard shortcuts based on saved output variables
            self._root.setProperty("opPrintAllNodes", "V(*)" in selected)
            self._root.setProperty("opPrintAllCurrents", "I(*)" in selected)
            self._root.setProperty("opPrintPower", "P(*)" in selected)
            # restore lead current state using family-unique tokens as indicators
            self._root.setProperty("opPrintBjtLeads", bool(selected & {"IC(*)", "IE(*)"}))
            self._root.setProperty("opPrintFetLeads", bool(selected & {"ID(*)", "IG(*)"}))
            # expose device family flags for conditional checkbox visibility
            self._root.setProperty("opHasBjtDevices", self._has_bjt_devices)
            self._root.setProperty("opHasFetDevices", self._has_fet_devices)
            # map format string to combo index (index 0 is the default/empty value)
            fmt_str = pp.print_format.upper() if pp.print_format else ""
            self._root.setProperty("opPrintFormatIndex", _PRINT_FORMATS.index(fmt_str) if fmt_str in _PRINT_FORMATS else 0)
            # restore saved output file path
            self._root.setProperty("opPrintFile", pp.print_file)
            # specific vars: only saved non-wildcard vars (no automatic topology pre-fill)
            self._root.setProperty("opPrintSpecificVars", " ".join(v for v in pp.output_variables if v not in _PRINT_WILDCARDS))
        # restore save/nodeset state
        self._root.setProperty("saveEnabled", p.save_enabled if p else False)
        self._root.setProperty("saveType", p.save_type if p else "NODESET")
        self._root.setProperty("saveFile", p.save_file if p else "")
        self._root.setProperty("nodesetEntries", " ".join(f"V({e.node})={e.voltage}" for e in p.nodeset_entries) if p else "")

    def _apply_transient_parameters(self, p: TransientSimulationParameters | None) -> None:
        # set values from parameters or defaults
        self._root.setProperty("initialStep", p.initial_step_value if p else "1u")
        self._root.setProperty("finalTime", p.final_time_value if p else "1m")
        self._root.setProperty("startTime", p.start_time_value if p else "0")
        self._root.setProperty("stepCeiling", p.step_ceiling_value if p else "")
        # map op_keyword back to index
        self._root.setProperty("opModeIndex", 1 if p and p.op_keyword == "NOOP" else 2 if p and p.op_keyword == "UIC" else 0)
        # schedule pairs text
        self._root.setProperty("scheduleEnabled", bool(p and p.schedule_points))
        self._root.setProperty("schedulePairsText", " ".join(f"{pt.time_value},{pt.max_time_step_value}" for pt in p.schedule_points) if p else "")
        # fft parameters text
        self._root.setProperty("fftParametersText", "\n".join(fft.to_xyce_statement() for fft in p.fft_parameters) if p else "")
        # four parameters text
        self._root.setProperty("fourParametersText", "\n".join(four.to_xyce_statement() for four in p.four_parameters) if p else "")
        self._root.setProperty("transientErrorText", "")
        # extract existing print parameters for pre-population
        pp = p.print_parameters if p else None
        # default to enabled with all wildcards and RAW format when no saved print parameters exist
        if pp is None:
            self._root.setProperty("tranPrintEnabled", True)
            self._root.setProperty("tranPrintAllNodes", True)
            self._root.setProperty("tranPrintAllCurrents", True)
            self._root.setProperty("tranPrintPower", True)
            # default lead current checkboxes to on when the device family is present
            self._root.setProperty("tranPrintBjtLeads", self._has_bjt_devices)
            self._root.setProperty("tranPrintFetLeads", self._has_fet_devices)
            # expose device family flags for conditional checkbox visibility
            self._root.setProperty("tranHasBjtDevices", self._has_bjt_devices)
            self._root.setProperty("tranHasFetDevices", self._has_fet_devices)
            self._root.setProperty("tranPrintFormatIndex", _PRINT_FORMATS.index("RAW"))
            self._root.setProperty("tranPrintFile", "")
            self._root.setProperty("tranPrintSpecificVars", "")
        else:
            # restore enabled state from saved parameters
            self._root.setProperty("tranPrintEnabled", True)
            # index saved output variables for quick wildcard lookup
            selected = set(pp.output_variables)
            # check wildcard shortcuts based on saved output variables
            self._root.setProperty("tranPrintAllNodes", "V(*)" in selected)
            self._root.setProperty("tranPrintAllCurrents", "I(*)" in selected)
            self._root.setProperty("tranPrintPower", "P(*)" in selected)
            # restore lead current state using family-unique tokens as indicators
            self._root.setProperty("tranPrintBjtLeads", bool(selected & {"IC(*)", "IE(*)"}))
            self._root.setProperty("tranPrintFetLeads", bool(selected & {"ID(*)", "IG(*)"}))
            # expose device family flags for conditional checkbox visibility
            self._root.setProperty("tranHasBjtDevices", self._has_bjt_devices)
            self._root.setProperty("tranHasFetDevices", self._has_fet_devices)
            # map format string to combo index (index 0 is the default/empty value)
            fmt_str = pp.print_format.upper() if pp.print_format else ""
            self._root.setProperty("tranPrintFormatIndex", _PRINT_FORMATS.index(fmt_str) if fmt_str in _PRINT_FORMATS else 0)
            # restore saved output file path
            self._root.setProperty("tranPrintFile", pp.print_file)
            # specific vars: only saved non-wildcard vars (no automatic topology pre-fill)
            self._root.setProperty("tranPrintSpecificVars", " ".join(v for v in pp.output_variables if v not in _PRINT_WILDCARDS))

    def _apply_dc_parameters(self, p: DCSimulationParameters | None) -> None:
        # sweep mode index mapping
        modes = ["LIN", "DEC", "OCT", "LIST", "DATA"]
        mode_index = modes.index(p.sweep_mode) if p and p.sweep_mode in modes else 0

        self._root.setProperty("sweepModeIndex", mode_index)
        self._root.setProperty("primaryVariable", p.primary_variable if p else "VIN")
        self._root.setProperty("startValue", p.start if p else "0")
        self._root.setProperty("stopValue", p.stop if p else "5")
        self._root.setProperty("stepValue", p.step if p else "0.1")
        self._root.setProperty("pointsValue", p.points if p else "10")
        self._root.setProperty("listValuesText", " ".join(p.list_values) if p else "")
        self._root.setProperty("dataTableName", p.data_table_name if p else "")

        has_secondary = bool(p and p.secondary_variable)
        self._root.setProperty("secondaryEnabled", has_secondary)
        self._root.setProperty("secondaryVariable", p.secondary_variable if has_secondary else "")
        self._root.setProperty("secondaryStart", p.secondary_start if has_secondary else "")
        self._root.setProperty("secondaryStop", p.secondary_stop if has_secondary else "")
        self._root.setProperty("secondaryStep", p.secondary_step if has_secondary else "")
        self._root.setProperty("secondaryPoints", p.secondary_points if has_secondary else "")
        # extract print parameters for pre-population
        pp = p.print_parameters if p else None
        # default to enabled with all wildcards and RAW format when no saved print parameters exist
        if pp is None:
            self._root.setProperty("dcPrintEnabled", True)
            self._root.setProperty("dcPrintAllNodes", True)
            self._root.setProperty("dcPrintAllCurrents", True)
            self._root.setProperty("dcPrintPower", True)
            self._root.setProperty("dcPrintBjtLeads", self._has_bjt_devices)
            self._root.setProperty("dcPrintFetLeads", self._has_fet_devices)
            self._root.setProperty("dcHasBjtDevices", self._has_bjt_devices)
            self._root.setProperty("dcHasFetDevices", self._has_fet_devices)
            self._root.setProperty("dcPrintFormatIndex", _PRINT_FORMATS.index("RAW"))
            self._root.setProperty("dcPrintFile", "")
            self._root.setProperty("dcPrintSpecificVars", "")
        else:
            self._root.setProperty("dcPrintEnabled", True)
            selected = set(pp.output_variables)
            self._root.setProperty("dcPrintAllNodes", "V(*)" in selected)
            self._root.setProperty("dcPrintAllCurrents", "I(*)" in selected)
            self._root.setProperty("dcPrintPower", "P(*)" in selected)
            self._root.setProperty("dcPrintBjtLeads", bool(selected & {"IC(*)", "IE(*)"}))
            self._root.setProperty("dcPrintFetLeads", bool(selected & {"ID(*)", "IG(*)"}))
            self._root.setProperty("dcHasBjtDevices", self._has_bjt_devices)
            self._root.setProperty("dcHasFetDevices", self._has_fet_devices)
            fmt_str = pp.print_format.upper() if pp.print_format else ""
            self._root.setProperty("dcPrintFormatIndex", _PRINT_FORMATS.index(fmt_str) if fmt_str in _PRINT_FORMATS else 0)
            self._root.setProperty("dcPrintFile", pp.print_file)
            self._root.setProperty("dcPrintSpecificVars", " ".join(v for v in pp.output_variables if v not in _PRINT_WILDCARDS))
        self._root.setProperty("dcErrorText", "")

    def _apply_ac_parameters(self, p: AcSimulationParameters | None) -> None:
        # sweep mode index mapping (ac supports LIN, DEC, OCT, DATA — no LIST)
        modes = ["LIN", "DEC", "OCT", "DATA"]
        mode_index = modes.index(p.sweep_mode) if p and p.sweep_mode in modes else 0
        self._root.setProperty("acSweepModeIndex", mode_index)
        self._root.setProperty("acPoints", p.points if p else "100")
        self._root.setProperty("acStart", p.start if p else "1")
        self._root.setProperty("acEnd", p.end if p else "1MEG")
        self._root.setProperty("acDataTableName", p.data_table_name if p else "")
        self._root.setProperty("acErrorText", "")
        # extract print parameters for pre-population
        pp = p.print_parameters if p else None
        # default to enabled with V(*) and I(*) and RAW format when no saved print parameters exist
        if pp is None:
            self._root.setProperty("acPrintEnabled", True)
            self._root.setProperty("acPrintAllNodes", True)
            self._root.setProperty("acPrintAllCurrents", True)
            self._root.setProperty("acPrintFormatIndex", _PRINT_FORMATS.index("RAW"))
            self._root.setProperty("acPrintFile", "")
            self._root.setProperty("acPrintSpecificVars", "")
        else:
            # restore enabled state from saved parameters
            self._root.setProperty("acPrintEnabled", True)
            # index saved output variables for quick wildcard lookup
            selected = set(pp.output_variables)
            # check wildcard shortcuts based on saved output variables
            self._root.setProperty("acPrintAllNodes", "V(*)" in selected)
            self._root.setProperty("acPrintAllCurrents", "I(*)" in selected)
            # map format string to combo index (index 0 is the default/empty value)
            fmt_str = pp.print_format.upper() if pp.print_format else ""
            self._root.setProperty("acPrintFormatIndex", _PRINT_FORMATS.index(fmt_str) if fmt_str in _PRINT_FORMATS else 0)
            # restore saved output file path
            self._root.setProperty("acPrintFile", pp.print_file)
            # specific vars: only saved non-wildcard vars (no automatic topology pre-fill)
            self._root.setProperty("acPrintSpecificVars", " ".join(v for v in pp.output_variables if v not in _PRINT_WILDCARDS))

    def _apply_noise_parameters(self, p: NoiseSimulationParameters | None) -> None:
        # populate required noise analysis fields
        self._root.setProperty("noiseOutputNode", p.output_node if p else "")
        self._root.setProperty("noiseRefNode", p.ref_node if p else "")
        self._root.setProperty("noiseSourceName", p.source_name if p else "")
        # sweep mode index mapping (noise supports LIN, DEC, OCT, DATA — no LIST)
        modes = ["LIN", "DEC", "OCT", "DATA"]
        mode_index = modes.index(p.sweep_mode) if p and p.sweep_mode in modes else 0
        self._root.setProperty("noiseSweepModeIndex", mode_index)
        self._root.setProperty("noisePoints", p.points if p else "100")
        self._root.setProperty("noiseStart", p.start if p else "1")
        self._root.setProperty("noiseEnd", p.end if p else "1MEG")
        self._root.setProperty("noiseDataTableName", p.data_table_name if p else "")
        self._root.setProperty("noiseErrorText", "")
        # extract print parameters for pre-population
        pp = p.print_parameters if p else None
        # default to enabled with V(*) and I(*) and RAW format when no saved print parameters exist
        if pp is None:
            self._root.setProperty("noisePrintEnabled", True)
            self._root.setProperty("noisePrintAllNodes", True)
            self._root.setProperty("noisePrintAllCurrents", True)
            self._root.setProperty("noisePrintInoise", True)
            self._root.setProperty("noisePrintOnoise", True)
            self._root.setProperty("noisePrintFormatIndex", _PRINT_FORMATS.index("RAW"))
            self._root.setProperty("noisePrintFile", "")
            self._root.setProperty("noisePrintSpecificVars", "")
            # initialize empty device operator list
            self._root.setProperty("noiseDeviceOperators", [])
        else:
            # restore enabled state from saved parameters
            self._root.setProperty("noisePrintEnabled", True)
            # index saved output variables for quick wildcard lookup
            selected = set(pp.output_variables)
            # check wildcard shortcuts based on saved output variables
            self._root.setProperty("noisePrintAllNodes", "V(*)" in selected)
            self._root.setProperty("noisePrintAllCurrents", "I(*)" in selected)
            # restore noise-specific named output variable checkboxes
            self._root.setProperty("noisePrintInoise", "INOISE" in selected)
            self._root.setProperty("noisePrintOnoise", "ONOISE" in selected)
            # map format string to combo index (index 0 is the default/empty value)
            fmt_str = pp.print_format.upper() if pp.print_format else ""
            self._root.setProperty("noisePrintFormatIndex", _PRINT_FORMATS.index(fmt_str) if fmt_str in _PRINT_FORMATS else 0)
            # restore saved output file path
            self._root.setProperty("noisePrintFile", pp.print_file)
            # specific vars: exclude wildcards and known named vars from the free-form field
            _NOISE_NAMED = {"INOISE", "ONOISE"}
            self._root.setProperty("noisePrintSpecificVars", " ".join(v for v in pp.output_variables if v not in _PRINT_WILDCARDS and v not in _NOISE_NAMED))
            # restore device noise operators as a list of dictionaries for QML
            device_operators_list = []
            if p and p.device_noise_operators:
                for operator in p.device_noise_operators:
                    device_operators_list.append({"deviceName": operator.device_name, "operatorType": operator.operator_type, "noiseSource": operator.noise_source})
            self._root.setProperty("noiseDeviceOperators", device_operators_list)

    def _apply_hb_parameters(self, p: HbSimulationParameters | None) -> None:
        # populate the hb frequency list field
        self._root.setProperty("hbFrequenciesText", " ".join(p.frequencies) if p else "1MEG")
        self._root.setProperty("hbErrorText", "")
        # extract print parameters for pre-population
        pp = p.print_parameters if p else None
        # default to enabled with V(*) and I(*) and raw output when no saved print parameters exist
        if pp is None:
            self._root.setProperty("hbPrintEnabled", True)
            self._root.setProperty("hbPrintAllNodes", True)
            self._root.setProperty("hbPrintAllCurrents", True)
            self._root.setProperty("hbPrintTypeIndex", _HB_PRINT_TYPES.index("HB"))
            self._root.setProperty("hbPrintFormatIndex", _PRINT_FORMATS.index("RAW"))
            self._root.setProperty("hbPrintFile", "")
            self._root.setProperty("hbPrintSpecificVars", "")
        else:
            # restore enabled state from saved parameters
            self._root.setProperty("hbPrintEnabled", True)
            # index saved output variables for quick wildcard lookup
            selected = set(pp.output_variables)
            # check wildcard shortcuts based on saved output variables
            self._root.setProperty("hbPrintAllNodes", "V(*)" in selected)
            self._root.setProperty("hbPrintAllCurrents", "I(*)" in selected)
            # map print type to combo index
            self._root.setProperty("hbPrintTypeIndex", _HB_PRINT_TYPES.index(pp.print_type) if pp.print_type in _HB_PRINT_TYPES else 0)
            # map format string to combo index (index 0 is the default/empty value)
            fmt_str = pp.print_format.upper() if pp.print_format else ""
            self._root.setProperty("hbPrintFormatIndex", _PRINT_FORMATS.index(fmt_str) if fmt_str in _PRINT_FORMATS else 0)
            # restore saved output file path
            self._root.setProperty("hbPrintFile", pp.print_file)
            # specific vars: only saved non-wildcard vars (no automatic topology pre-fill)
            self._root.setProperty("hbPrintSpecificVars", " ".join(v for v in pp.output_variables if v not in _PRINT_WILDCARDS))

    def _apply_lin_parameters(self, p: LinSimulationParameters | None) -> None:
        # populate lin keyword options and embedded ac fields
        self._root.setProperty("linSparcalc", p.sparcalc if p else True)
        self._root.setProperty("linFormat", p.format if p else "TOUCHSTONE2")
        self._root.setProperty("linType", p.lintype if p else "S")
        self._root.setProperty("linDataFormat", p.dataformat if p else "RI")
        self._root.setProperty("linFile", p.file if p else "")
        self._root.setProperty("linWidth", p.width if p else "")
        self._root.setProperty("linPrecision", p.precision if p else "")
        self._root.setProperty("linSweepModeIndex", ["LIN", "DEC", "OCT", "DATA"].index(p.sweep_mode) if p and p.sweep_mode in ("LIN", "DEC", "OCT", "DATA") else 0)
        self._root.setProperty("linPoints", p.points if p else "100")
        self._root.setProperty("linStart", p.start if p else "1")
        self._root.setProperty("linEnd", p.end if p else "1MEG")
        self._root.setProperty("linDataTableName", p.data_table_name if p else "")
        self._root.setProperty("linErrorText", "")
        # extract print parameters for pre-population
        pp = p.print_parameters if p else None
        if pp is None:
            self._root.setProperty("linPrintEnabled", True)
            self._root.setProperty("linPrintAllNodes", True)
            self._root.setProperty("linPrintAllCurrents", True)
            self._root.setProperty("linPrintFormatIndex", _PRINT_FORMATS.index("RAW"))
            self._root.setProperty("linPrintFile", "")
            self._root.setProperty("linPrintSpecificVars", "")
        else:
            self._root.setProperty("linPrintEnabled", True)
            selected = set(pp.output_variables)
            self._root.setProperty("linPrintAllNodes", "V(*)" in selected)
            self._root.setProperty("linPrintAllCurrents", "I(*)" in selected)
            fmt_str = pp.print_format.upper() if pp.print_format else ""
            self._root.setProperty("linPrintFormatIndex", _PRINT_FORMATS.index(fmt_str) if fmt_str in _PRINT_FORMATS else 0)
            self._root.setProperty("linPrintFile", pp.print_file)
            self._root.setProperty("linPrintSpecificVars", " ".join(v for v in pp.output_variables if v not in _PRINT_WILDCARDS))

    @Slot(str, str, str, str, str, str, str, str, bool, bool, bool, bool, bool, str, str, str, bool, 'QVariantList')
    def _on_submit_noise(self, output_node: str, ref_node: str, source_name: str, sweep_mode: str, points: str, start: str, end: str, data_table_name: str, print_enabled: bool, print_all_nodes: bool, print_all_currents: bool, print_inoise: bool, print_onoise: bool, print_specific_vars: str, print_format: str, print_file: str, replace_ground: bool, device_operators_list) -> None:
        # normalize required noise analysis fields
        normalized_output_node = output_node.strip()
        normalized_ref_node = ref_node.strip()
        normalized_source_name = source_name.strip()
        # normalize the sweep mode to uppercase for comparison
        normalized_mode = sweep_mode.strip().upper()
        # normalize numeric sweep fields
        normalized_points = points.strip()
        normalized_start = start.strip()
        normalized_end = end.strip()
        # normalize data table name
        normalized_data_table = data_table_name.strip()
        # require output node before any sweep validation
        if not normalized_output_node:
            self._root.setProperty("noiseErrorText", "Output node is required")
            # keep dialog open for correction
            return
        # require source name before any sweep validation
        if not normalized_source_name:
            self._root.setProperty("noiseErrorText", "Input noise source name is required")
            # keep dialog open for correction
            return
        # reject unrecognized sweep modes before any further validation
        if normalized_mode not in _AC_SWEEP_MODES:
            self._root.setProperty("noiseErrorText", "Sweep mode must be one of LIN, DEC, OCT, or DATA")
            # keep dialog open for correction
            return
        # validate required fields for non-DATA sweep modes
        if normalized_mode != "DATA" and (not normalized_points or not normalized_start or not normalized_end):
            self._root.setProperty("noiseErrorText", "Points, start frequency, and end frequency are required")
            # keep dialog open for correction
            return
        # validate DATA-specific required fields
        if normalized_mode == "DATA" and not normalized_data_table:
            self._root.setProperty("noiseErrorText", "Data table name is required for DATA sweep")
            # keep dialog open for correction
            return
        # validate device noise operators
        device_noise_operators: list[DeviceNoiseOperator] = []
        # convert QJSValue to Python list
        operator_list = []
        if device_operators_list and device_operators_list.isArray():
            operator_list = device_operators_list.toVariant()
        for operator_dict in operator_list:
            # extract device name and validate
            device_name = operator_dict.get("deviceName", "")
            if not _validate_device_name(device_name):
                self._root.setProperty("noiseErrorText", f"Invalid device name: {device_name}")
                # keep dialog open for correction
                return
            # extract operator type
            operator_type = operator_dict.get("operatorType", "")
            if operator_type not in ("DNI", "DNO"):
                self._root.setProperty("noiseErrorText", f"Invalid operator type: {operator_type}")
                # keep dialog open for correction
                return
            # extract optional noise source
            noise_source = operator_dict.get("noiseSource", "")
            # create device noise operator and add to list
            device_noise_operators.append(DeviceNoiseOperator(device_name=device_name, operator_type=operator_type, noise_source=noise_source))
        # clear any stale validation message now that inputs are valid
        self._root.setProperty("noiseErrorText", "")
        # build print parameters when the print section is enabled
        print_parameters = None
        if print_enabled:
            # collect wildcard tokens for each enabled shortcut
            output_vars: list[str] = []
            if print_all_nodes:
                output_vars.append("V(*)")
            if print_all_currents:
                output_vars.append("I(*)")
            # append noise-specific named output variables when their checkboxes are checked
            if print_inoise:
                output_vars.append("INOISE")
            if print_onoise:
                output_vars.append("ONOISE")
            # append any explicitly listed specific variables (e.g. DNI(), DNO() expressions)
            output_vars.extend(v for v in print_specific_vars.split() if v)
            # construct print parameters for the noise analysis type
            print_parameters = PrintParameters(print_type="NOISE", print_format=print_format.strip().upper() if print_format.strip() else "", print_file=print_file.strip(), output_variables=tuple(output_vars))
        # capture the validated dialog output for the caller
        self._result = NoiseSimulationParameters(normalized_output_node, normalized_ref_node, normalized_source_name, normalized_mode, normalized_points, normalized_start, normalized_end, normalized_data_table, replace_ground, print_parameters, tuple(device_noise_operators))
        # close the dialog and return acceptance to the caller
        self.accept()

    @Slot(bool, str, str, str, str, str, str, str, str, str, str, str, bool, bool, bool, str, str, str, bool)
    def _on_submit_lin(self, sparcalc: bool, fmt: str, lintype: str, dataformat: str, file: str, width: str, precision: str, sweep_mode: str, points: str, start: str, end: str, data_table_name: str, print_enabled: bool, print_all_nodes: bool, print_all_currents: bool, print_specific_vars: str, print_format: str, print_file: str, replace_ground: bool) -> None:
        # normalize all keyword values and free-form text fields
        normalized_format = fmt.strip().upper()
        normalized_lintype = lintype.strip().upper()
        normalized_dataformat = dataformat.strip().upper()
        normalized_file = file.strip()
        normalized_width = width.strip()
        normalized_precision = precision.strip()
        # normalize the sweep mode to uppercase for comparison
        normalized_mode = sweep_mode.strip().upper()
        normalized_points = points.strip()
        normalized_start = start.strip()
        normalized_end = end.strip()
        normalized_data_table_name = data_table_name.strip()
        # reject unrecognized sweep modes before any further validation
        if normalized_mode not in _AC_SWEEP_MODES:
            self._root.setProperty("linErrorText", "Sweep mode must be one of LIN, DEC, OCT, or DATA")
            # keep dialog open for correction
            return
        # validate required fields for non-DATA sweep modes
        if normalized_mode != "DATA" and (not normalized_points or not normalized_start or not normalized_end):
            self._root.setProperty("linErrorText", "Points, start frequency, and end frequency are required")
            # keep dialog open for correction
            return
        # validate DATA-specific required fields
        if normalized_mode == "DATA" and not normalized_data_table_name:
            self._root.setProperty("linErrorText", "Data table name is required for DATA sweep")
            # keep dialog open for correction
            return
        # clear any stale validation message now that inputs are valid
        self._root.setProperty("linErrorText", "")
        # build print parameters when the print section is enabled
        print_parameters = None
        if print_enabled:
            output_vars: list[str] = []
            if print_all_nodes:
                output_vars.append("V(*)")
            if print_all_currents:
                output_vars.append("I(*)")
            output_vars.extend(v for v in print_specific_vars.split() if v)
            print_parameters = PrintParameters(print_type="AC", print_format=print_format.strip().upper() if print_format.strip() else "", print_file=print_file.strip(), output_variables=tuple(output_vars))
        self._result = LinSimulationParameters(sparcalc=sparcalc, format=normalized_format if normalized_format else "TOUCHSTONE2", lintype=normalized_lintype if normalized_lintype else "S", dataformat=normalized_dataformat if normalized_dataformat else "RI", file=normalized_file, width=normalized_width, precision=normalized_precision, sweep_mode=normalized_mode, points=normalized_points, start=normalized_start, end=normalized_end, data_table_name=normalized_data_table_name, replace_ground=replace_ground, print_parameters=print_parameters)
        self.accept()

    @Slot(str, bool, bool, bool, str, str, str, bool)
    def _on_submit_hb(self, frequencies_text: str, print_enabled: bool, print_all_nodes: bool, print_all_currents: bool, print_type: str, print_specific_vars: str, print_format: str, print_file: str, replace_ground: bool) -> None:
        # normalize the frequency list text
        normalized_frequencies_text = frequencies_text.strip()
        # require at least one frequency for hb analysis
        if not normalized_frequencies_text:
            self._root.setProperty("hbErrorText", "At least one fundamental frequency is required")
            # keep dialog open for correction
            return
        # split the frequency field on spaces and commas
        frequencies = _parse_list_values(normalized_frequencies_text)
        # reject empty token lists after parsing
        if not frequencies:
            self._root.setProperty("hbErrorText", "At least one fundamental frequency is required")
            # keep dialog open for correction
            return
        # clear any stale validation message now that inputs are valid
        self._root.setProperty("hbErrorText", "")
        # build print parameters when the print section is enabled
        print_parameters = None
        if print_enabled:
            # collect wildcard tokens for each enabled shortcut
            output_vars: list[str] = []
            if print_all_nodes:
                output_vars.append("V(*)")
            if print_all_currents:
                output_vars.append("I(*)")
            # append any explicitly listed specific variables
            output_vars.extend(v for v in print_specific_vars.split() if v)
            # construct print parameters for the hb analysis type
            normalized_print_type = print_type.strip().upper()
            if normalized_print_type not in _HB_PRINT_TYPES:
                normalized_print_type = "HB"
            print_parameters = PrintParameters(print_type=normalized_print_type, print_format=print_format.strip().upper() if print_format.strip() else "", print_file=print_file.strip(), output_variables=tuple(output_vars))
        # capture the validated dialog output for the caller
        self._result = HbSimulationParameters(frequencies=tuple(frequencies), replace_ground=replace_ground, print_parameters=print_parameters)
        # close the dialog and return acceptance to the caller
        self.accept()

    @Slot(str, str, str, str, str, bool, bool, bool, str, str, str, bool)
    def _on_submit_ac(self, sweep_mode: str, points: str, start: str, end: str, data_table_name: str, print_enabled: bool, print_all_nodes: bool, print_all_currents: bool, print_specific_vars: str, print_format: str, print_file: str, replace_ground: bool) -> None:
        # normalize the sweep mode to uppercase for comparison
        normalized_mode = sweep_mode.strip().upper()
        # normalize numeric sweep fields
        normalized_points = points.strip()
        normalized_start = start.strip()
        normalized_end = end.strip()
        # normalize data table name
        normalized_data_table = data_table_name.strip()
        # reject unrecognized sweep modes before any further validation
        if normalized_mode not in _AC_SWEEP_MODES:
            self._root.setProperty("acErrorText", "Sweep mode must be one of LIN, DEC, OCT, or DATA")
            # keep dialog open for correction
            return
        # validate required fields for non-DATA sweep modes
        if normalized_mode != "DATA" and (not normalized_points or not normalized_start or not normalized_end):
            self._root.setProperty("acErrorText", "Points, start frequency, and end frequency are required")
            # keep dialog open for correction
            return
        # validate DATA-specific required fields
        if normalized_mode == "DATA" and not normalized_data_table:
            self._root.setProperty("acErrorText", "Data table name is required for DATA sweep")
            # keep dialog open for correction
            return
        # clear any stale validation message now that inputs are valid
        self._root.setProperty("acErrorText", "")
        # build print parameters when the print section is enabled
        print_parameters = None
        if print_enabled:
            # collect wildcard tokens for each enabled shortcut
            output_vars: list[str] = []
            if print_all_nodes:
                output_vars.append("V(*)")
            if print_all_currents:
                output_vars.append("I(*)")
            # append any explicitly listed specific variables (complex-domain vars like VR(), VM(), etc.)
            output_vars.extend(v for v in print_specific_vars.split() if v)
            # construct print parameters for the ac analysis type
            print_parameters = PrintParameters(print_type="AC", print_format=print_format.strip().upper() if print_format.strip() else "", print_file=print_file.strip(), output_variables=tuple(output_vars))
        # capture the validated dialog output for the caller
        self._result = AcSimulationParameters(normalized_mode, normalized_points, normalized_start, normalized_end, normalized_data_table, replace_ground, print_parameters)
        # close the dialog and return acceptance to the caller
        self.accept()

    @Slot(str, str, str, str, str, bool, str, str, str, bool, bool, bool, bool, bool, bool, str, str, str, bool)
    def _on_submit_transient(self, initial_step: str, final_time: str, start_time: str, step_ceiling: str, op_keyword: str, schedule_enabled: bool, schedule_pairs_text: str, fft_parameters_text: str, four_parameters_text: str, print_enabled: bool, print_all_nodes: bool, print_all_currents: bool, print_power: bool, print_bjt_leads: bool, print_fet_leads: bool, print_specific_vars: str, print_format: str, print_file: str, replace_ground: bool) -> None:
        # normalize user-entered values by trimming surrounding spaces
        normalized_initial_step = initial_step.strip()
        # normalize user-entered values by trimming surrounding spaces
        normalized_final_time = final_time.strip()
        # normalize optional start time while allowing an empty value
        normalized_start_time = start_time.strip()
        # normalize optional max step while allowing an empty value
        normalized_step_ceiling = step_ceiling.strip()
        # normalize operating-point mode keyword
        normalized_op_keyword = op_keyword.strip().upper()
        # normalize schedule text content
        normalized_schedule_pairs_text = schedule_pairs_text.strip()
        # enforce required transient fields before accepting the dialog
        if not normalized_initial_step or not normalized_final_time:
            # show a form-level message when required values are missing
            self._root.setProperty("transientErrorText", "Initial step and final time are required")
            # keep dialog open for correction
            return
        # enforce allowed operating-point keywords from the transient grammar
        if normalized_op_keyword not in ["", "NOOP", "UIC"]:
            # show a form-level message when the keyword is unsupported
            self._root.setProperty("transientErrorText", "Operating-point mode must be Default, NOOP, or UIC")
            # keep dialog open for correction
            return
        # enforce schedule input when schedule mode is enabled
        if schedule_enabled and not normalized_schedule_pairs_text:
            # show a form-level message for missing schedule pairs
            self._root.setProperty("transientErrorText", "Schedule is enabled but no time,max-step pairs were provided")
            # keep dialog open for correction
            return
        # parse schedule pairs into structured entries when schedule mode is enabled
        try:
            # collect structured schedule entries from the user text format
            schedule_points = self._parse_schedule_points(normalized_schedule_pairs_text) if schedule_enabled else tuple()
        # handle invalid schedule format and surface a readable message to the user
        except ValueError as schedule_error:
            # show parse failure details in the form-level validation message
            self._root.setProperty("transientErrorText", str(schedule_error))
            # keep dialog open for correction
            return
        # init fft parameters
        fft_parameters: list[FftParameters] = []
        # parse each line as a separate fft directive
        for line in fft_parameters_text.splitlines():
            # normalize line
            stripped_line = line.strip()
            # skip empties
            if not stripped_line:
                # next
                continue
            # prepend command prefix if missing to simplify user input
            fft_statement = stripped_line if stripped_line.upper().startswith(".FFT") else f".FFT {stripped_line}"
            # parse fft parameters
            fft = FftParameters.from_xyce_statement(fft_statement)
            # check parse success
            if fft:
                # store the parsed fft parameters
                fft_parameters.append(fft)
            # handle parse failure
            else:
                # log warning
                logger.warning("Ignoring invalid .FFT directive: %s", line)
        # init four parameters
        four_parameters: list[FourParameters] = []
        # parse each line as a separate four directive
        for line in four_parameters_text.splitlines():
            # normalize line
            stripped_line = line.strip()
            # skip empties
            if not stripped_line:
                # next
                continue
            # prepend command prefix if missing to simplify user input
            four_statement = stripped_line if stripped_line.upper().startswith(".FOUR") else f".FOUR {stripped_line}"
            # parse four parameters
            four = FourParameters.from_xyce_statement(four_statement)
            # check parse success
            if four:
                # store the parsed four parameters
                four_parameters.append(four)
            # handle parse failure
            else:
                # log warning
                logger.warning("Ignoring invalid .FOUR directive: %s", line)
        # clear any stale validation message now that inputs are valid
        self._root.setProperty("transientErrorText", "")
        # build print parameters when the print section is enabled
        print_parameters = None
        if print_enabled:
            # collect wildcard tokens for each enabled shortcut
            output_vars: list[str] = []
            if print_all_nodes:
                output_vars.append("V(*)")
            if print_all_currents:
                output_vars.append("I(*)")
            if print_power:
                output_vars.append("P(*)")
            # append BJT lead wildcards when the BJT checkbox is checked
            if print_bjt_leads:
                output_vars.extend(t for t in _BJT_WILDCARDS if t not in output_vars)
            # append FET lead wildcards deduplicating tokens shared with the BJT group
            if print_fet_leads:
                output_vars.extend(t for t in _FET_WILDCARDS if t not in output_vars)
            # append any explicitly listed specific variables
            output_vars.extend(v for v in print_specific_vars.split() if v)
            # construct print parameters for the transient analysis type
            print_parameters = PrintParameters(print_type="TRAN", print_format=print_format.strip().upper() if print_format.strip() else "", print_file=print_file.strip(), output_variables=tuple(output_vars))
        # capture the validated dialog output for the caller
        self._result = TransientSimulationParameters(normalized_initial_step, normalized_final_time, normalized_start_time, normalized_step_ceiling, normalized_op_keyword, schedule_points, print_parameters=print_parameters, fft_parameters=tuple(fft_parameters), four_parameters=tuple(four_parameters), replace_ground=replace_ground)
        # close the dialog and return acceptance to the caller
        self.accept()

    @Slot(bool, bool, bool, bool, bool, bool, str, str, str, bool, str, str, str, bool)
    def _on_submit_op(self, print_enabled: bool, print_all_nodes: bool, print_all_currents: bool, print_power: bool, print_bjt_leads: bool, print_fet_leads: bool, print_specific_vars: str, print_format: str, print_file: str, save_enabled: bool, save_type: str, nodeset_text: str, save_file: str, replace_ground: bool) -> None:
        # parse nodeset text into entry objects
        nodeset_entries = []
        # basic pattern for V(node)=voltage
        for pair in nodeset_text.split():
            if "=" in pair:
                node_part, voltage = pair.split("=", 1)
                if node_part.startswith("V(") and node_part.endswith(")"):
                    node = node_part[2:-1]
                    nodeset_entries.append(NodesetEntry(node=node, voltage=voltage))
        # build print parameters when the print section is enabled
        print_parameters = None
        if print_enabled:
            # collect wildcard tokens for each enabled shortcut
            output_vars: list[str] = []
            if print_all_nodes:
                output_vars.append("V(*)")
            if print_all_currents:
                output_vars.append("I(*)")
            if print_power:
                output_vars.append("P(*)")
            # append BJT lead wildcards when the BJT checkbox is checked
            if print_bjt_leads:
                output_vars.extend(t for t in _BJT_WILDCARDS if t not in output_vars)
            # append FET lead wildcards deduplicating tokens shared with the BJT group
            if print_fet_leads:
                output_vars.extend(t for t in _FET_WILDCARDS if t not in output_vars)
            # append any explicitly listed specific variables
            output_vars.extend(v for v in print_specific_vars.split() if v)
            # construct print parameters for the DC analysis type
            print_parameters = PrintParameters(print_type="DC", print_format=print_format.strip().upper() if print_format.strip() else "", print_file=print_file.strip(), output_variables=tuple(output_vars))
        # capture the validated dialog output for the caller
        self._result = OpSimulationParameters(save_enabled=save_enabled, save_type=save_type, save_file=save_file.strip(), nodeset_entries=tuple(nodeset_entries), print_parameters=print_parameters, replace_ground=replace_ground)
        # close the dialog and return acceptance to the caller
        self.accept()

    @Slot(str, str, str, str, str, str, str, str, bool, str, str, str, str, str, bool, bool, bool, bool, bool, bool, str, str, str, bool)
    def _on_submit_dc(self, sweep_mode: str, primary_variable: str, start: str, stop: str, step: str, points: str, list_values_text: str, data_table_name: str, secondary_enabled: bool, secondary_variable: str, secondary_start: str, secondary_stop: str, secondary_step: str, secondary_points: str, print_enabled: bool, print_all_nodes: bool, print_all_currents: bool, print_power: bool, print_bjt_leads: bool, print_fet_leads: bool, print_specific_vars: str, print_format: str, print_file: str, replace_ground: bool) -> None:
        # normalize the sweep mode to uppercase for comparison
        normalized_mode = sweep_mode.strip().upper()
        # normalize primary variable by trimming surrounding spaces
        normalized_primary = primary_variable.strip()
        # normalize all numeric sweep fields
        normalized_start = start.strip()
        normalized_stop = stop.strip()
        normalized_step = step.strip()
        normalized_points = points.strip()
        # normalize list values text
        normalized_list_text = list_values_text.strip()
        # normalize data table name
        normalized_data_table = data_table_name.strip()
        # normalize secondary sweep fields
        normalized_sec_variable = secondary_variable.strip()
        normalized_sec_start = secondary_start.strip()
        normalized_sec_stop = secondary_stop.strip()
        normalized_sec_step = secondary_step.strip()
        normalized_sec_points = secondary_points.strip()
        # reject unrecognized sweep modes before any further validation
        if normalized_mode not in _DC_SWEEP_MODES:
            self._root.setProperty("dcErrorText", "Sweep mode must be one of LIN, DEC, OCT, LIST, or DATA")
            # keep dialog open for correction
            return
        # require primary variable for all non-DATA modes
        if normalized_mode != "DATA" and not normalized_primary:
            self._root.setProperty("dcErrorText", "Primary sweep variable is required")
            # keep dialog open for correction
            return
        # validate LIN-specific required fields
        if normalized_mode == "LIN":
            if not normalized_start or not normalized_stop or not normalized_step:
                self._root.setProperty("dcErrorText", "Start, stop, and step values are required for LIN sweep")
                # keep dialog open for correction
                return
        # validate DEC/OCT-specific required fields
        if normalized_mode in ("DEC", "OCT"):
            if not normalized_start or not normalized_stop or not normalized_points:
                self._root.setProperty("dcErrorText", "Start, stop, and points are required for DEC/OCT sweep")
                # keep dialog open for correction
                return
            # enforce integer constraint on points value
            if not normalized_points.isdigit() or int(normalized_points) < 1:
                self._root.setProperty("dcErrorText", "Points must be an integer \u2265 1")
                # keep dialog open for correction
                return
        # validate LIST-specific required fields
        if normalized_mode == "LIST":
            if not normalized_list_text:
                self._root.setProperty("dcErrorText", "At least one list value is required for LIST sweep")
                # keep dialog open for correction
                return
        # validate DATA-specific required fields
        if normalized_mode == "DATA":
            if not normalized_data_table:
                self._root.setProperty("dcErrorText", "Data table name is required for DATA sweep")
                # keep dialog open for correction
                return
        # validate secondary sweep completeness when it is enabled
        if secondary_enabled and normalized_mode in _DC_SECONDARY_MODES:
            if not normalized_sec_variable:
                self._root.setProperty("dcErrorText", "Secondary sweep variable is required when secondary sweep is enabled")
                # keep dialog open for correction
                return
            if not normalized_sec_start or not normalized_sec_stop:
                self._root.setProperty("dcErrorText", "Secondary sweep start and stop are required")
                # keep dialog open for correction
                return
            if normalized_mode == "LIN" and not normalized_sec_step:
                self._root.setProperty("dcErrorText", "Secondary sweep step is required for LIN mode")
                # keep dialog open for correction
                return
            if normalized_mode in ("DEC", "OCT"):
                if not normalized_sec_points:
                    self._root.setProperty("dcErrorText", "Secondary sweep points are required for DEC/OCT mode")
                    # keep dialog open for correction
                    return
                # enforce integer constraint on secondary points value
                if not normalized_sec_points.isdigit() or int(normalized_sec_points) < 1:
                    self._root.setProperty("dcErrorText", "Secondary points must be an integer \u2265 1")
                    # keep dialog open for correction
                    return
        # parse the list values text into an immutable sequence for the dataclass
        try:
            list_values = _parse_list_values(normalized_list_text) if normalized_mode == "LIST" else tuple()
        # surface list parse errors as form-level validation messages
        except ValueError as parse_error:
            self._root.setProperty("dcErrorText", str(parse_error))
            # keep dialog open for correction
            return
        # resolve the effective secondary variable (empty when secondary is disabled)
        effective_sec_variable = normalized_sec_variable if secondary_enabled and normalized_mode in _DC_SECONDARY_MODES else ""
        # clear any stale validation message now that inputs are valid
        self._root.setProperty("dcErrorText", "")
        # build print parameters when the print section is enabled
        print_parameters = None
        if print_enabled:
            # collect wildcard tokens for each enabled shortcut
            output_vars: list[str] = []
            if print_all_nodes:
                output_vars.append("V(*)")
            if print_all_currents:
                output_vars.append("I(*)")
            if print_power:
                output_vars.append("P(*)")
            # append BJT lead wildcards when the BJT checkbox is checked
            if print_bjt_leads:
                output_vars.extend(t for t in _BJT_WILDCARDS if t not in output_vars)
            # append FET lead wildcards deduplicating tokens shared with the BJT group
            if print_fet_leads:
                output_vars.extend(t for t in _FET_WILDCARDS if t not in output_vars)
            # append any explicitly listed specific variables
            output_vars.extend(v for v in print_specific_vars.split() if v)
            # construct print parameters for the DC sweep analysis type
            print_parameters = PrintParameters(print_type="DC", print_format=print_format.strip().upper() if print_format.strip() else "", print_file=print_file.strip(), output_variables=tuple(output_vars))
        # capture the validated dialog output for the caller
        self._result = DCSimulationParameters(normalized_mode, normalized_primary, normalized_start, normalized_stop, normalized_step, normalized_points, list_values, normalized_data_table, effective_sec_variable, normalized_sec_start, normalized_sec_stop, normalized_sec_step, normalized_sec_points, replace_ground=replace_ground, print_parameters=print_parameters)
        # close the dialog and return acceptance to the caller
        self.accept()

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

    def get_parameters(self) -> AcSimulationParameters | HbSimulationParameters | LinSimulationParameters | NoiseSimulationParameters | TransientSimulationParameters | DCSimulationParameters | OpSimulationParameters | None:
        return self._result
