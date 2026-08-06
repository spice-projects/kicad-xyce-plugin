from .lin_simulation_parameters import LinSimulationParameters
from .print_parameters import PrintParameters

# valid sweep modes for the lin directive (same as AC)
_AC_SWEEP_MODES = {"LIN", "DEC", "OCT", "DATA"}

# print format values matching the combo model order (index 0 is the empty/default value)
_PRINT_FORMATS = ["", "STD", "NOINDEX", "PROBE", "TECPLOT", "RAW", "CSV", "GNUPLOT", "SPLOT"]

# wildcard tokens that map to dedicated checkbox shortcuts in the print section
_PRINT_WILDCARDS = {"V(*)", "I(*)", "P(*)", "W(*)", "IB(*)", "IC(*)", "ID(*)", "IE(*)", "IG(*)", "IS(*)"}


class LinPanel:
    def __init__(self, root):
        # retain root reference for property reads and writes
        self._root = root

    def apply(self, p: LinSimulationParameters | None) -> None:
        # restore sparcalc flag
        self._root.setProperty("linSparcalc", bool(p.sparcalc) if p else True)
        # restore lin format (S, Y, Z, etc.)
        self._root.setProperty("linFormat", p.format if p else "TOUCHSTONE2")
        # restore lin type (S, Y, Z, etc.)
        self._root.setProperty("linType", p.lintype if p else "")
        # restore lin data format
        self._root.setProperty("linDataFormat", p.dataformat if p else "")
        # restore lin file path
        self._root.setProperty("linFile", p.file if p else "")
        # restore lin width
        self._root.setProperty("linWidth", p.width if p else "")
        # restore lin precision
        self._root.setProperty("linPrecision", p.precision if p else "")
        # sweep mode index mapping
        modes = ["LIN", "DEC", "OCT", "DATA"]
        # resolve index for the saved sweep mode or default to LIN
        mode_index = modes.index(p.sweep_mode) if p and p.sweep_mode in modes else 0
        # restore sweep mode selection
        self._root.setProperty("linSweepModeIndex", mode_index)
        # restore sweep numeric fields
        self._root.setProperty("linPoints", p.points if p else "100")
        self._root.setProperty("linStart", p.start if p else "1")
        self._root.setProperty("linEnd", p.end if p else "1MEG")
        # restore data table name
        self._root.setProperty("linDataTableName", p.data_table_name if p else "")
        # clear stale error message
        self._root.setProperty("errorText", "")
        # extract print parameters for pre-population
        pp = p.print_parameters if p else None
        # default to enabled with V(*) and I(*) and RAW format when no saved print parameters exist
        if pp is None:
            self._root.setProperty("linPrintEnabled", p is None)
            self._root.setProperty("linPrintAllNodes", True)
            self._root.setProperty("linPrintAllCurrents", True)
            self._root.setProperty("linPrintFormatIndex", _PRINT_FORMATS.index("RAW"))
            self._root.setProperty("linPrintFile", "")
            self._root.setProperty("linPrintSpecificVars", "")
        else:
            # restore enabled state from saved parameters
            self._root.setProperty("linPrintEnabled", True)
            # index saved output variables for quick wildcard lookup
            selected = set(pp.output_variables)
            # check wildcard shortcuts based on saved output variables
            self._root.setProperty("linPrintAllNodes", "V(*)" in selected)
            self._root.setProperty("linPrintAllCurrents", "I(*)" in selected)
            # map format string to combo index (index 0 is the default/empty value)
            fmt_str = pp.print_format.upper() if pp.print_format else ""
            self._root.setProperty("linPrintFormatIndex", _PRINT_FORMATS.index(fmt_str) if fmt_str in _PRINT_FORMATS else 0)
            # restore saved output file path
            self._root.setProperty("linPrintFile", pp.print_file)
            # specific vars: only saved non-wildcard vars (no automatic topology pre-fill)
            self._root.setProperty("linPrintSpecificVars", " ".join(v for v in pp.output_variables if v not in _PRINT_WILDCARDS))

    def handle_submit(self, sparcalc: bool, lin_format: str, lin_type: str, lin_data_format: str, lin_file: str, lin_width: str, lin_precision: str, sweep_mode: str, points: str, start: str, end: str, data_table_name: str, print_enabled: bool, print_all_nodes: bool, print_all_currents: bool, print_specific_vars: str, print_format: str, print_file: str, replace_ground: bool) -> LinSimulationParameters | None:
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
            # append any explicitly listed specific variables
            output_vars.extend(v for v in print_specific_vars.split() if v)
            # construct print parameters for the lin analysis type
            print_parameters = PrintParameters(print_type="LIN", print_format=print_format.strip().upper() if print_format.strip() else "", print_file=print_file.strip(), output_variables=tuple(output_vars))
        # construct parameters instance
        analysis = LinSimulationParameters(sparcalc, lin_format.strip(), lin_type.strip(), lin_data_format.strip(), lin_file.strip(), lin_width.strip(), lin_precision.strip(), normalized_mode, normalized_points, normalized_start, normalized_end, normalized_data_table, replace_ground, print_parameters)
        # return parameters to caller for config assembly
        return analysis
