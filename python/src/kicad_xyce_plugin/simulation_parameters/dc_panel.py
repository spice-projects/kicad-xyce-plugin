import re

from .dc_simulation_parameters import DCSimulationParameters
from .measure_parameters import MeasureEntry
from .print_parameters import PrintParameters
from .sens_parameter import SensParameter

# valid primary sweep modes for the dc directive
_DC_SWEEP_MODES = {"LIN", "DEC", "OCT", "LIST", "DATA"}

# sweep modes that support a nested secondary sweep
_DC_SECONDARY_MODES = {"LIN", "DEC", "OCT"}

# print format values matching the combo model order (index 0 is the empty/default value)
_PRINT_FORMATS = ["", "STD", "NOINDEX", "PROBE", "TECPLOT", "RAW", "CSV", "GNUPLOT", "SPLOT"]

# print type values for dc analysis
_PRINT_TYPES = ["DC", "HOMOTOPY"]

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


class DcPanel:
    def __init__(self, root):
        # retain root reference for property reads and writes
        self._root = root

    def apply(self, p: DCSimulationParameters | None, has_bjt: bool, has_fet: bool) -> None:
        # sweep mode index mapping
        modes = ["LIN", "DEC", "OCT", "LIST", "DATA"]
        # resolve index for the saved sweep mode or default to LIN
        mode_index = modes.index(p.sweep_mode) if p and p.sweep_mode in modes else 0
        # restore sweep mode selection
        self._root.setProperty("sweepModeIndex", mode_index)
        # restore primary sweep variable
        self._root.setProperty("primaryVariable", p.primary_variable if p else "VIN")
        # restore sweep range fields
        self._root.setProperty("startValue", p.start if p else "0")
        self._root.setProperty("stopValue", p.stop if p else "5")
        self._root.setProperty("stepValue", p.step if p else "0.1")
        self._root.setProperty("pointsValue", p.points if p else "10")
        # restore list and data table fields
        self._root.setProperty("listValuesText", " ".join(p.list_values) if p else "")
        self._root.setProperty("dataTableName", p.data_table_name if p else "")
        # determine whether a secondary sweep variable is configured
        has_secondary = bool(p and p.secondary_variable)
        # restore secondary sweep enabled state
        self._root.setProperty("secondaryEnabled", has_secondary)
        # restore secondary sweep fields using empty defaults when disabled
        self._root.setProperty("secondaryVariable", p.secondary_variable if has_secondary else "")
        self._root.setProperty("secondaryStart", p.secondary_start if has_secondary else "")
        self._root.setProperty("secondaryStop", p.secondary_stop if has_secondary else "")
        self._root.setProperty("secondaryStep", p.secondary_step if has_secondary else "")
        self._root.setProperty("secondaryPoints", p.secondary_points if has_secondary else "")
        # restore measure parameters text
        self._root.setProperty("dcMeasureParametersText", "\n".join(measure.to_xyce_statement() for measure in p.measure_parameters) if p else "")
        # extract print parameters for pre-population
        pp = p.print_parameters if p else None
        # default to enabled with all wildcards and RAW format when no saved print parameters exist
        if pp is None:
            self._root.setProperty("dcPrintEnabled", p is None)
            self._root.setProperty("dcPrintAllNodes", True)
            self._root.setProperty("dcPrintAllCurrents", True)
            self._root.setProperty("dcPrintPower", True)
            # default lead current checkboxes to on when the device family is present
            self._root.setProperty("dcPrintBjtLeads", has_bjt)
            self._root.setProperty("dcPrintFetLeads", has_fet)
            # expose device family flags for conditional checkbox visibility
            self._root.setProperty("dcHasBjtDevices", has_bjt)
            self._root.setProperty("dcHasFetDevices", has_fet)
            self._root.setProperty("dcPrintTypeIndex", 0)
            self._root.setProperty("dcPrintFormatIndex", _PRINT_FORMATS.index("RAW"))
            self._root.setProperty("dcPrintFile", "")
            self._root.setProperty("dcPrintSpecificVars", "")
        else:
            # restore enabled state from saved parameters
            self._root.setProperty("dcPrintEnabled", True)
            # index saved output variables for quick wildcard lookup
            selected = set(pp.output_variables)
            # check wildcard shortcuts based on saved output variables
            self._root.setProperty("dcPrintAllNodes", "V(*)" in selected)
            self._root.setProperty("dcPrintAllCurrents", "I(*)" in selected)
            self._root.setProperty("dcPrintPower", "P(*)" in selected)
            # restore lead current state using family-unique tokens as indicators
            self._root.setProperty("dcPrintBjtLeads", bool(selected & {"IC(*)", "IE(*)"}))
            self._root.setProperty("dcPrintFetLeads", bool(selected & {"ID(*)", "IG(*)"}))
            # expose device family flags for conditional checkbox visibility
            self._root.setProperty("dcHasBjtDevices", has_bjt)
            self._root.setProperty("dcHasFetDevices", has_fet)
            # map print type string to combo index
            type_str = pp.print_type.upper()
            self._root.setProperty("dcPrintTypeIndex", _PRINT_TYPES.index(type_str) if type_str in _PRINT_TYPES else 0)
            # map format string to combo index (index 0 is the default/empty value)
            fmt_str = pp.print_format.upper() if pp.print_format else ""
            self._root.setProperty("dcPrintFormatIndex", _PRINT_FORMATS.index(fmt_str) if fmt_str in _PRINT_FORMATS else 0)
            # restore saved output file path
            self._root.setProperty("dcPrintFile", pp.print_file)
            # specific vars: only saved non-wildcard vars (no automatic topology pre-fill)
            self._root.setProperty("dcPrintSpecificVars", " ".join(v for v in pp.output_variables if v not in _PRINT_WILDCARDS))
        # clear stale error message
        self._root.setProperty("errorText", "")

    def handle_submit(self, sweep_mode: str, primary_variable: str, start: str, stop: str, step: str, points: str, list_values_text: str, data_table_name: str, secondary_enabled: bool, secondary_variable: str, secondary_start: str, secondary_stop: str, secondary_step: str, secondary_points: str, measure_parameters_text: str, print_enabled: bool, print_type: str, print_all_nodes: bool, print_all_currents: bool, print_power: bool, print_bjt_leads: bool, print_fet_leads: bool, print_specific_vars: str, print_format: str, print_file: str, replace_ground: bool, sensitivity: SensParameter | None = None) -> DCSimulationParameters | None:
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
            # report error
            self._root.setProperty("errorText", "Sweep mode must be one of LIN, DEC, OCT, LIST, or DATA")
            # signal validation failure to caller
            return None
        # require primary variable for all non-DATA modes
        if normalized_mode != "DATA" and not normalized_primary:
            # report error
            self._root.setProperty("errorText", "Primary sweep variable is required")
            # signal validation failure to caller
            return None
        # validate LIN-specific required fields
        if normalized_mode == "LIN":
            if not normalized_start or not normalized_stop or not normalized_step:
                # report error
                self._root.setProperty("errorText", "Start, stop, and step values are required for LIN sweep")
                # signal validation failure to caller
                return None
        # validate DEC/OCT-specific required fields
        if normalized_mode in ("DEC", "OCT"):
            if not normalized_start or not normalized_stop or not normalized_points:
                # report error
                self._root.setProperty("errorText", "Start, stop, and points are required for DEC/OCT sweep")
                # signal validation failure to caller
                return None
            # enforce integer constraint on points value
            if not normalized_points.isdigit() or int(normalized_points) < 1:
                # report error
                self._root.setProperty("errorText", "Points must be an integer \u2265 1")
                # signal validation failure to caller
                return None
        # validate LIST-specific required fields
        if normalized_mode == "LIST":
            if not normalized_list_text:
                # report error
                self._root.setProperty("errorText", "At least one list value is required for LIST sweep")
                # signal validation failure to caller
                return None
        # validate DATA-specific required fields
        if normalized_mode == "DATA":
            if not normalized_data_table:
                # report error
                self._root.setProperty("errorText", "Data table name is required for DATA sweep")
                # signal validation failure to caller
                return None
        # validate secondary sweep completeness when it is enabled
        if secondary_enabled and normalized_mode in _DC_SECONDARY_MODES:
            if not normalized_sec_variable:
                # report error
                self._root.setProperty("errorText", "Secondary sweep variable is required when secondary sweep is enabled")
                # signal validation failure to caller
                return None
            if not normalized_sec_start or not normalized_sec_stop:
                # report error
                self._root.setProperty("errorText", "Secondary sweep start and stop are required")
                # signal validation failure to caller
                return None
            if normalized_mode == "LIN" and not normalized_sec_step:
                # report error
                self._root.setProperty("errorText", "Secondary sweep step is required for LIN mode")
                # signal validation failure to caller
                return None
            if normalized_mode in ("DEC", "OCT"):
                if not normalized_sec_points:
                    # report error
                    self._root.setProperty("errorText", "Secondary sweep points are required for DEC/OCT mode")
                    # signal validation failure to caller
                    return None
                # enforce integer constraint on secondary points value
                if not normalized_sec_points.isdigit() or int(normalized_sec_points) < 1:
                    self._root.setProperty("errorText", "Secondary points must be an integer \u2265 1")
                    # signal validation failure to caller
                    return None
        # parse the list values text into an immutable sequence for the dataclass
        try:
            # parse list values only in LIST mode
            list_values = _parse_list_values(normalized_list_text) if normalized_mode == "LIST" else tuple()
        # surface list parse errors as form-level validation messages
        except ValueError as parse_error:
            # report error
            self._root.setProperty("errorText", str(parse_error))
            # signal validation failure to caller
            return None
        # resolve the effective secondary variable (empty when secondary is disabled)
        effective_sec_variable = normalized_sec_variable if secondary_enabled and normalized_mode in _DC_SECONDARY_MODES else ""
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
            measure_statement = stripped_line if stripped_line.upper().startswith((".MEASURE", ".MEAS")) else f".MEASURE DC {stripped_line}"
            # parse measure parameters
            measure = MeasureEntry.from_xyce_statement(measure_statement)
            # check parse success and analysis type match
            if measure and measure.analysis_type == "DC":
                # store the parsed measure parameters
                measure_parameters.append(measure)
            else:
                # report invalid directive to user
                self._root.setProperty("errorText", f"Invalid .MEASURE directive for DC: {line}")
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
            # construct print parameters for the DC sweep analysis type
            print_parameters = PrintParameters(print_type=print_type.strip().upper() if print_type.strip() else "DC", print_format=print_format.strip().upper() if print_format.strip() else "", print_file=print_file.strip(), output_variables=tuple(output_vars))
        # construct parameters instance
        analysis = DCSimulationParameters(normalized_mode, normalized_primary, normalized_start, normalized_stop, normalized_step, normalized_points, list_values, normalized_data_table, effective_sec_variable, normalized_sec_start, normalized_sec_stop, normalized_sec_step, normalized_sec_points, replace_ground, print_parameters, measure_parameters=tuple(measure_parameters), sensitivity=sensitivity)
        # return parameters to caller for config assembly
        return analysis
