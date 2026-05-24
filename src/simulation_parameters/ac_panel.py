from .ac_simulation_parameters import AcSimulationParameters
from .measure_parameters import MeasureEntry
from .print_parameters import PrintParameters
from .sens_parameter import SensParameter

# valid sweep modes for the ac directive
_AC_SWEEP_MODES = {"LIN", "DEC", "OCT", "DATA"}

# print format values matching the combo model order (index 0 is the empty/default value)
_PRINT_FORMATS = ["", "STD", "NOINDEX", "PROBE", "TECPLOT", "RAW", "CSV", "GNUPLOT", "SPLOT"]

# print type values for ac analysis
_PRINT_TYPES = ["AC", "AC_IC"]

# wildcard tokens that map to dedicated checkbox shortcuts in the print section
_PRINT_WILDCARDS = {"V(*)", "I(*)", "P(*)", "W(*)", "IB(*)", "IC(*)", "ID(*)", "IE(*)", "IG(*)", "IS(*)"}


class AcPanel:

    def __init__(self, root):
        # retain root reference for property reads and writes
        self._root = root

    def apply(self, p: AcSimulationParameters | None) -> None:
        # sweep mode index mapping (ac supports LIN, DEC, OCT, DATA — no LIST)
        modes = ["LIN", "DEC", "OCT", "DATA"]
        # resolve index for the saved sweep mode or default to LIN
        mode_index = modes.index(p.sweep_mode) if p and p.sweep_mode in modes else 0
        # restore sweep mode selection
        self._root.setProperty("acSweepModeIndex", mode_index)
        # restore sweep numeric fields
        self._root.setProperty("acPoints", p.points if p else "100")
        self._root.setProperty("acStart", p.start if p else "1")
        self._root.setProperty("acEnd", p.end if p else "1MEG")
        # restore data table name
        self._root.setProperty("acDataTableName", p.data_table_name if p else "")
        # restore measure parameters text
        self._root.setProperty("acMeasureParametersText", "\n".join(measure.to_xyce_statement() for measure in p.measure_parameters) if p else "")
        # clear stale error message
        self._root.setProperty("errorText", "")
        # extract print parameters for pre-population
        pp = p.print_parameters if p else None
        # default to enabled with V(*) and I(*) and RAW format when no saved print parameters exist
        if pp is None:
            self._root.setProperty("acPrintEnabled", p is None)
            self._root.setProperty("acPrintAllNodes", True)
            self._root.setProperty("acPrintAllCurrents", True)
            self._root.setProperty("acPrintTypeIndex", 0)
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
            # map print type string to combo index
            type_str = pp.print_type.upper()
            self._root.setProperty("acPrintTypeIndex", _PRINT_TYPES.index(type_str) if type_str in _PRINT_TYPES else 0)
            # map format string to combo index (index 0 is the default/empty value)
            fmt_str = pp.print_format.upper() if pp.print_format else ""
            self._root.setProperty("acPrintFormatIndex", _PRINT_FORMATS.index(fmt_str) if fmt_str in _PRINT_FORMATS else 0)
            # restore saved output file path
            self._root.setProperty("acPrintFile", pp.print_file)
            # specific vars: only saved non-wildcard vars (no automatic topology pre-fill)
            self._root.setProperty("acPrintSpecificVars", " ".join(v for v in pp.output_variables if v not in _PRINT_WILDCARDS))

    def handle_submit(self, sweep_mode: str, points: str, start: str, end: str, data_table_name: str, measure_parameters_text: str, print_enabled: bool, print_type: str, print_all_nodes: bool, print_all_currents: bool, print_specific_vars: str, print_format: str, print_file: str, replace_ground: bool, sensitivity: SensParameter | None = None) -> AcSimulationParameters | None:
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
            self._root.setProperty("errorText", "Sweep mode must be one of LIN, DEC, OCT, or DATA")
            # signal validation failure to caller
            return None
        # validate required fields for non-DATA sweep modes
        if normalized_mode != "DATA" and (not normalized_points or not normalized_start or not normalized_end):
            self._root.setProperty("errorText", "Points, start frequency, and end frequency are required")
            # signal validation failure to caller
            return None
        # validate DATA-specific required fields
        if normalized_mode == "DATA" and not normalized_data_table:
            self._root.setProperty("errorText", "Data table name is required for DATA sweep")
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
            measure_statement = stripped_line if stripped_line.upper().startswith((".MEASURE", ".MEAS")) else f".MEASURE AC {stripped_line}"
            # parse measure parameters
            measure = MeasureEntry.from_xyce_statement(measure_statement)
            # check parse success and analysis type match
            if measure and measure.analysis_type == "AC":
                # store the parsed measure parameters
                measure_parameters.append(measure)
            else:
                # report invalid directive to user
                self._root.setProperty("errorText", f"Invalid .MEASURE directive for AC: {line}")
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
            # append any explicitly listed specific variables (complex-domain vars like VR(), VM(), etc.)
            output_vars.extend(v for v in print_specific_vars.split() if v)
            # construct print parameters for the ac analysis type
            print_parameters = PrintParameters(print_type=print_type.strip().upper() if print_type.strip() else "AC", print_format=print_format.strip().upper() if print_format.strip() else "", print_file=print_file.strip(), output_variables=tuple(output_vars))
        # construct parameters instance
        analysis = AcSimulationParameters(normalized_mode, normalized_points, normalized_start, normalized_end, normalized_data_table, replace_ground, print_parameters, measure_parameters=tuple(measure_parameters), sensitivity=sensitivity)
        # return parameters to caller for config assembly
        return analysis
