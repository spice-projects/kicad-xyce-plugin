import re

from .fft_parameters import FftParameters
from .four_parameters import FourParameters
from .measure_parameters import MeasureEntry
from .print_parameters import PrintParameters
from .sens_parameter import SensParameter
from .transient_simulation_parameters import TransientSchedulePoint, TransientSimulationParameters

# print format values matching the combo model order (index 0 is the empty/default value)
_PRINT_FORMATS = ["", "STD", "NOINDEX", "PROBE", "TECPLOT", "RAW", "CSV", "GNUPLOT", "SPLOT"]

# print type values for transient analysis
_PRINT_TYPES = ["TRAN", "TRANADJOINT"]

# wildcard tokens that map to dedicated checkbox shortcuts in the print section
_PRINT_WILDCARDS = {"V(*)", "I(*)", "P(*)", "W(*)", "IB(*)", "IC(*)", "ID(*)", "IE(*)", "IG(*)", "IS(*)"}

# ordered lead current wildcards per device family for output emission order
_BJT_WILDCARDS = ("IB(*)", "IC(*)", "IE(*)", "IS(*)")
_FET_WILDCARDS = ("IB(*)", "ID(*)", "IG(*)", "IS(*)")


class TranPanel:

    def __init__(self, root):
        # retain root reference for property reads and writes
        self._root = root

    def apply(self, p: TransientSimulationParameters | None, has_bjt: bool, has_fet: bool) -> None:
        # set values from parameters or defaults
        self._root.setProperty("initialStep", p.initial_step_value if p else "1u")
        self._root.setProperty("finalTime", p.final_time_value if p else "1m")
        self._root.setProperty("startTime", p.start_time_value if p else "0")
        self._root.setProperty("stepCeiling", p.step_ceiling_value if p else "")
        # map op_keyword back to index
        self._root.setProperty("opModeIndex", 1 if p and p.op_keyword == "NOOP" else 2 if p and p.op_keyword == "UIC" else 0)
        # restore schedule state
        self._root.setProperty("scheduleEnabled", bool(p and p.schedule_points))
        self._root.setProperty("schedulePairsText", " ".join(f"{pt.time_value},{pt.max_time_step_value}" for pt in p.schedule_points) if p else "")
        # restore fft directives text
        self._root.setProperty("fftParametersText", "\n".join(fft.to_xyce_statement() for fft in p.fft_parameters) if p else "")
        # restore four directives text
        self._root.setProperty("fourParametersText", "\n".join(four.to_xyce_statement() for four in p.four_parameters) if p else "")
        # restore measure directives text
        self._root.setProperty("tranMeasureParametersText", "\n".join(measure.to_xyce_statement() for measure in p.measure_parameters) if p else "")
        # clear stale error message
        self._root.setProperty("errorText", "")
        # extract print parameters for pre-population (prefer print_parameters over legacy fields)
        pp = p.print_parameters if p else None
        # default to enabled with all wildcards and RAW format when no saved print parameters exist
        if pp is None:
            # only default to enabled when there is no saved transient state at all (first open);
            # when p exists but pp is None the user explicitly disabled print, so honour that
            self._root.setProperty("tranPrintEnabled", p is None)
            self._root.setProperty("tranPrintAllNodes", True)
            self._root.setProperty("tranPrintAllCurrents", True)
            self._root.setProperty("tranPrintPower", True)
            # default lead current checkboxes to on when the device family is present
            self._root.setProperty("tranPrintBjtLeads", has_bjt)
            self._root.setProperty("tranPrintFetLeads", has_fet)
            # expose device family flags for conditional checkbox visibility
            self._root.setProperty("tranHasBjtDevices", has_bjt)
            self._root.setProperty("tranHasFetDevices", has_fet)
            self._root.setProperty("tranPrintTypeIndex", 0)
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
            self._root.setProperty("tranHasBjtDevices", has_bjt)
            self._root.setProperty("tranHasFetDevices", has_fet)
            # map print type string to combo index
            type_str = pp.print_type.upper()
            self._root.setProperty("tranPrintTypeIndex", _PRINT_TYPES.index(type_str) if type_str in _PRINT_TYPES else 0)
            # map format string to combo index (index 0 is the default/empty value)
            fmt_str = pp.print_format.upper() if pp.print_format else ""
            self._root.setProperty("tranPrintFormatIndex", _PRINT_FORMATS.index(fmt_str) if fmt_str in _PRINT_FORMATS else 0)
            # restore saved output file path
            self._root.setProperty("tranPrintFile", pp.print_file)
            # specific vars: only saved non-wildcard vars (no automatic topology pre-fill)
            self._root.setProperty("tranPrintSpecificVars", " ".join(v for v in pp.output_variables if v not in _PRINT_WILDCARDS))

    def handle_submit(self, initial_step: str, final_time: str, start_time: str, step_ceiling: str, op_keyword: str, schedule_enabled: bool, schedule_pairs_text: str, fft_parameters_text: str, four_parameters_text: str, measure_parameters_text: str, print_enabled: bool, print_type: str, print_all_nodes: bool, print_all_currents: bool, print_power: bool, print_bjt_leads: bool, print_fet_leads: bool, print_specific_vars: str, print_format: str, print_file: str, replace_ground: bool, sensitivity: SensParameter | None = None) -> TransientSimulationParameters | None:
        # normalize user-entered values by trimming surrounding spaces
        normalized_initial_step = initial_step.strip()
        # normalize final time field
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
            self._root.setProperty("errorText", "Initial step and final time are required")
            # signal validation failure to caller
            return None
        # enforce allowed operating-point keywords from the transient grammar
        if normalized_op_keyword not in ["", "NOOP", "UIC"]:
            # show a form-level message when the keyword is unsupported
            self._root.setProperty("errorText", "Operating-point mode must be Default, NOOP, or UIC")
            # signal validation failure to caller
            return None
        # enforce schedule input when schedule mode is enabled
        if schedule_enabled and not normalized_schedule_pairs_text:
            # show a form-level message for missing schedule pairs
            self._root.setProperty("errorText", "Schedule is enabled but no time,max-step pairs were provided")
            # signal validation failure to caller
            return None
        # parse schedule pairs into structured entries when schedule mode is enabled
        try:
            # collect structured schedule entries from the user text format
            schedule_points = self._parse_schedule_points(normalized_schedule_pairs_text) if schedule_enabled else tuple()
        # handle invalid schedule format and surface a readable message to the user
        except ValueError as schedule_error:
            # show parse failure details in the form-level validation message
            self._root.setProperty("errorText", str(schedule_error))
            # signal validation failure to caller
            return None
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
            else:
                # report invalid directive to user
                self._root.setProperty("errorText", f"Invalid .FFT directive: {line}")
                # signal validation failure to caller
                return None
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
            else:
                # report invalid directive to user
                self._root.setProperty("errorText", f"Invalid .FOUR directive: {line}")
                # signal validation failure to caller
                return None
        # init measure parameters
        measure_parameters: list[MeasureEntry] = []
        # parse each line as a separate measure directive
        for line in measure_parameters_text.splitlines():
            # normalize line
            stripped_line = line.strip()
            # skip empties
            if not stripped_line:
                # next
                continue
            # prepend command prefix if missing to simplify user input
            measure_statement = stripped_line if stripped_line.upper().startswith((".MEASURE", ".MEAS")) else f".MEASURE TRAN {stripped_line}"
            # parse measure parameters
            measure = MeasureEntry.from_xyce_statement(measure_statement)
            # check parse success and analysis type match
            if measure and measure.analysis_type == "TRAN":
                # store the parsed measure parameters
                measure_parameters.append(measure)
            else:
                # report invalid directive to user
                self._root.setProperty("errorText", f"Invalid .MEASURE directive for TRAN: {line}")
                # signal validation failure to caller
                return None
        # clear any stale validation message now that inputs are valid
        self._root.setProperty("errorText", "")
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
            print_parameters = PrintParameters(print_type=print_type.strip().upper() if print_type.strip() else "TRAN", print_format=print_format.strip().upper() if print_format.strip() else "", print_file=print_file.strip(), output_variables=tuple(output_vars))
        # construct parameters instance
        analysis = TransientSimulationParameters(normalized_initial_step, normalized_final_time, normalized_start_time, normalized_step_ceiling, normalized_op_keyword, schedule_points, print_parameters=print_parameters, fft_parameters=tuple(fft_parameters), four_parameters=tuple(four_parameters), measure_parameters=tuple(measure_parameters), replace_ground=replace_ground, sensitivity=sensitivity)
        # return parameters to caller for config assembly
        return analysis

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
