import re
from dataclasses import dataclass
from pathlib import Path

from dataclasses_json import LetterCase, dataclass_json
from PySide6.QtCore import QUrl, Slot
from PySide6.QtGui import QColor
from PySide6.QtQuick import QQuickView
from PySide6.QtWidgets import QDialog, QVBoxLayout, QWidget

_QML_FILE = Path(__file__).parent / "simulation_dialog.qml"
_BG = "#efefe8"

_DC_SWEEP_MODES = {"LIN", "DEC", "OCT", "LIST", "DATA"}
_DC_SECONDARY_MODES = {"LIN", "DEC", "OCT"}


@dataclass_json(letter_case=LetterCase.CAMEL)
@dataclass(frozen=True)
class OpSimulationParameters:

    def to_xyce_directive(self) -> str:
        # return the standard op directive string
        return ".OP"


@dataclass_json(letter_case=LetterCase.CAMEL)
@dataclass(frozen=True)
class TransientSchedulePoint:
    time_value: str
    max_time_step_value: str


@dataclass_json(letter_case=LetterCase.CAMEL)
@dataclass(frozen=True)
class TransientSimulationParameters:

    initial_step_value: str
    final_time_value: str
    start_time_value: str
    step_ceiling_value: str
    op_keyword: str
    schedule_points: tuple[TransientSchedulePoint, ...]

    def to_xyce_directive(self) -> str:
        # build the required transient directive fields first
        tokens = [".TRAN", self.initial_step_value, self.final_time_value]
        # include start and step ceiling in positional order when either is provided
        if self.start_time_value or self.step_ceiling_value:
            # insert the default start time when only step ceiling was provided
            tokens.append(self.start_time_value if self.start_time_value else "0")
        # include optional step ceiling value only when provided
        if self.step_ceiling_value:
            tokens.append(self.step_ceiling_value)
        # append the selected operating-point behavior keyword when requested
        if self.op_keyword:
            tokens.append(self.op_keyword)
        # append schedule clause when schedule points are configured
        if self.schedule_points:
            # flatten alternating time and step entries for schedule syntax
            schedule_tokens = []
            # iterate all schedule points in user-provided order
            for schedule_point in self.schedule_points:
                # append the schedule time token
                schedule_tokens.append(schedule_point.time_value)
                # append the max-step token paired with that time
                schedule_tokens.append(schedule_point.max_time_step_value)
            # append the full schedule clause wrapped as a brace expression
            tokens.append("{schedule(" + ", ".join(schedule_tokens) + ")}")
        # return a single directive string that can be placed in a netlist
        return " ".join(tokens)


@dataclass_json(letter_case=LetterCase.CAMEL)
@dataclass(frozen=True)
class DCSimulationParameters:

    sweep_mode: str
    primary_variable: str
    start: str
    stop: str
    step: str
    points: str
    list_values: tuple[str, ...]
    data_table_name: str
    secondary_variable: str
    secondary_start: str
    secondary_stop: str
    secondary_step: str
    secondary_points: str

    def to_xyce_directive(self) -> str:
        # dispatch to the correct builder based on the selected sweep mode
        if self.sweep_mode == "DATA":
            return self._build_data_directive()
        # build list directive when sweep mode is LIST
        if self.sweep_mode == "LIST":
            return self._build_list_directive()
        # build linear directive when sweep mode is LIN
        if self.sweep_mode == "LIN":
            return self._build_lin_directive()
        # build decade or octave directive for DEC and OCT modes
        return self._build_log_directive()

    def _build_data_directive(self) -> str:
        # data-driven sweep references an existing .DATA table by name
        return f".DC DATA={self.data_table_name}"

    def _build_list_directive(self) -> str:
        # join the explicit sweep values with a single space separator
        values_str = " ".join(self.list_values)
        # combine variable name, LIST keyword, and the value sequence
        return f".DC {self.primary_variable} LIST {values_str}"

    def _build_lin_directive(self) -> str:
        # build the primary linear sweep token sequence
        tokens = [".DC", self.primary_variable, self.start, self.stop, self.step]
        # append secondary sweep tokens when a secondary variable is configured
        if self.secondary_variable:
            tokens.extend([self.secondary_variable, self.secondary_start, self.secondary_stop, self.secondary_step])
        # combine all tokens into a single directive string
        return " ".join(tokens)

    def _build_log_directive(self) -> str:
        # build the primary decade/octave sweep token sequence
        tokens = [".DC", self.sweep_mode, self.primary_variable, self.start, self.stop, self.points]
        # append secondary sweep tokens when a secondary variable is configured
        if self.secondary_variable:
            tokens.extend([self.secondary_variable, self.secondary_start, self.secondary_stop, self.secondary_points])
        # combine all tokens into a single directive string
        return " ".join(tokens)


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


class SimulationDialog(QDialog):

    def __init__(self, parent: QWidget | None = None, initial_parameters: TransientSimulationParameters | DCSimulationParameters | OpSimulationParameters | None = None):
        # initialize the modal dialog container
        super().__init__(parent)
        # capture initial parameters for form pre-population
        self._initial_parameters = initial_parameters
        # keep the result empty until the user confirms valid values
        self._result: TransientSimulationParameters | DCSimulationParameters | OpSimulationParameters | None = None
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

        # initialize defaults and apply initial parameters if available
        self._apply_initial_parameters()

        # connect qml submit signals to python validation handlers
        self._root.submitTransient.connect(self._on_submit_transient)
        self._root.submitDC.connect(self._on_submit_dc)
        self._root.submitOP.connect(self._on_submit_op)
        # connect qml cancel signal to close without a result
        self._root.cancelRequested.connect(self.reject)

    def _apply_initial_parameters(self) -> None:
        # dispatch to specialized methods based on parameter type
        p = self._initial_parameters

        # always initialize all tabs to ensure a clean state
        self._apply_transient_parameters(p if isinstance(p, TransientSimulationParameters) else None)
        self._apply_dc_parameters(p if isinstance(p, DCSimulationParameters) else None)

        # select the appropriate tab based on the parameter type
        if isinstance(p, DCSimulationParameters):
            self._root.setProperty("initialTabIndex", 1)
        elif isinstance(p, OpSimulationParameters):
            self._root.setProperty("initialTabIndex", 2)
        else:
            self._root.setProperty("initialTabIndex", 0)

    def _apply_transient_parameters(self, p: TransientSimulationParameters | None) -> None:
        # set values from parameters or defaults
        self._root.setProperty("initialStep", p.initial_step_value if p else "1u")
        self._root.setProperty("finalTime", p.final_time_value if p else "1m")
        self._root.setProperty("startTime", p.start_time_value if p else "0")
        self._root.setProperty("stepCeiling", p.step_ceiling_value if p else "")

        # map op_keyword back to index
        op_index = 0
        if p:
            if p.op_keyword == "NOOP":
                op_index = 1
            elif p.op_keyword == "UIC":
                op_index = 2
        self._root.setProperty("opModeIndex", op_index)

        # schedule pairs text
        schedule_text = ""
        if p and p.schedule_points:
            schedule_text = " ".join(f"{pt.time_value},{pt.max_time_step_value}" for pt in p.schedule_points)
        self._root.setProperty("scheduleEnabled", bool(p and p.schedule_points))
        self._root.setProperty("schedulePairsText", schedule_text)
        self._root.setProperty("transientErrorText", "")

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
        self._root.setProperty("dcErrorText", "")

    @Slot(str, str, str, str, str, bool, str)
    def _on_submit_transient(self, initial_step: str, final_time: str, start_time: str, step_ceiling: str, op_keyword: str, schedule_enabled: bool, schedule_pairs_text: str) -> None:
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
        # clear any stale validation message now that inputs are valid
        self._root.setProperty("transientErrorText", "")
        # capture the validated dialog output for the caller
        self._result = TransientSimulationParameters(normalized_initial_step, normalized_final_time, normalized_start_time, normalized_step_ceiling, normalized_op_keyword, schedule_points)
        # close the dialog and return acceptance to the caller
        self.accept()

    @Slot()
    def _on_submit_op(self) -> None:
        # capture the result for OP simulation which has no parameters
        self._result = OpSimulationParameters()
        # close the dialog and return acceptance to the caller
        self.accept()

    @Slot(str, str, str, str, str, str, str, str, bool, str, str, str, str, str)
    def _on_submit_dc(self, sweep_mode: str, primary_variable: str, start: str, stop: str, step: str, points: str, list_values_text: str, data_table_name: str, secondary_enabled: bool, secondary_variable: str, secondary_start: str, secondary_stop: str, secondary_step: str, secondary_points: str) -> None:
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
        # capture the validated dialog output for the caller
        self._result = DCSimulationParameters(normalized_mode, normalized_primary, normalized_start, normalized_stop, normalized_step, normalized_points, list_values, normalized_data_table, effective_sec_variable, normalized_sec_start, normalized_sec_stop, normalized_sec_step, normalized_sec_points)
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

    def get_parameters(self) -> TransientSimulationParameters | DCSimulationParameters | None:
        # run the modal dialog event loop and wait for user input
        dialog_code = self.exec()
        # return no value when the dialog was canceled
        if dialog_code != QDialog.DialogCode.Accepted:
            return None
        # return the previously validated simulation parameter set
        return self._result
