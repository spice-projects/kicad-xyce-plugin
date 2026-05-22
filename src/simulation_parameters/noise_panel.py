import re

from .measure_parameters import MeasureEntry
from .noise_simulation_parameters import DeviceNoiseOperator, NoiseSimulationParameters
from .print_parameters import PrintParameters

# valid sweep modes for the noise directive
_AC_SWEEP_MODES = {"LIN", "DEC", "OCT", "DATA"}

# print format values matching the combo model order (index 0 is the empty/default value)
_PRINT_FORMATS = ["", "STD", "NOINDEX", "PROBE", "TECPLOT", "RAW", "CSV", "GNUPLOT", "SPLOT"]

# wildcard tokens that map to dedicated checkbox shortcuts in the print section
_PRINT_WILDCARDS = {"V(*)", "I(*)", "P(*)", "W(*)", "IB(*)", "IC(*)", "ID(*)", "IE(*)", "IG(*)", "IS(*)"}

# named noise output variables that appear as dedicated checkboxes
_NOISE_NAMED = {"INOISE", "ONOISE"}


def _validate_device_name(device_name: str) -> bool:
    # check for empty or whitespace-only device name
    if not device_name or not device_name.strip():
        return False
    # validate device name as identifier (alphanumeric, underscore, must start with letter or underscore)
    return bool(re.match(r"^[a-zA-Z_][a-zA-Z0-9_]*$", device_name.strip()))


class NoisePanel:
    def __init__(self, root):
        # retain root reference for property reads and writes
        self._root = root

    def apply(self, p: NoiseSimulationParameters | None) -> None:
        # populate required noise analysis fields
        self._root.setProperty("noiseOutputNode", p.output_node if p else "")
        self._root.setProperty("noiseRefNode", p.ref_node if p else "")
        self._root.setProperty("noiseSourceName", p.source_name if p else "")
        # sweep mode index mapping (noise supports LIN, DEC, OCT, DATA — no LIST)
        modes = ["LIN", "DEC", "OCT", "DATA"]
        # resolve index for the saved sweep mode or default to LIN
        mode_index = modes.index(p.sweep_mode) if p and p.sweep_mode in modes else 0
        # restore sweep mode selection
        self._root.setProperty("noiseSweepModeIndex", mode_index)
        # restore sweep numeric fields
        self._root.setProperty("noisePoints", p.points if p else "100")
        self._root.setProperty("noiseStart", p.start if p else "1")
        self._root.setProperty("noiseEnd", p.end if p else "1MEG")
        # restore data table name
        self._root.setProperty("noiseDataTableName", p.data_table_name if p else "")
        # restore measure parameters text
        self._root.setProperty("noiseMeasureParametersText", "\n".join(measure.to_xyce_statement() for measure in p.measure_parameters) if p else "")
        # clear stale error message
        self._root.setProperty("errorText", "")
        # extract print parameters for pre-population
        pp = p.print_parameters if p else None
        # default to enabled with V(*) and I(*) and RAW format when no saved print parameters exist
        if pp is None:
            self._root.setProperty("noisePrintEnabled", p is None)
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
            self._root.setProperty("noisePrintSpecificVars", " ".join(v for v in pp.output_variables if v not in _PRINT_WILDCARDS and v not in _NOISE_NAMED))
            # restore device noise operators as a list of dictionaries for QML
            device_operators_list = []
            if p and p.device_noise_operators:
                for operator in p.device_noise_operators:
                    device_operators_list.append({"deviceName": operator.device_name, "operatorType": operator.operator_type, "noiseSource": operator.noise_source})
            self._root.setProperty("noiseDeviceOperators", device_operators_list)

    def handle_submit(self, output_node: str, ref_node: str, source_name: str, sweep_mode: str, points: str, start: str, end: str, data_table_name: str, measure_parameters_text: str, print_enabled: bool, print_all_nodes: bool, print_all_currents: bool, print_inoise: bool, print_onoise: bool, print_specific_vars: str, print_format: str, print_file: str, replace_ground: bool, device_operators_list) -> NoiseSimulationParameters | None:
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
            self._root.setProperty("errorText", "Output node is required")
            # signal validation failure to caller
            return None
        # require source name before any sweep validation
        if not normalized_source_name:
            self._root.setProperty("errorText", "Input noise source name is required")
            # signal validation failure to caller
            return None
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
        # validate device noise operators
        device_noise_operators: list[DeviceNoiseOperator] = []
        # device_operators_list is a Python list (QVariantList mapping)
        operator_list = list(device_operators_list) if device_operators_list else []
        for operator_dict in operator_list:
            # extract device name and validate
            device_name = operator_dict.get("deviceName", "")
            if not _validate_device_name(device_name):
                self._root.setProperty("errorText", f"Invalid device name: {device_name}")
                # signal validation failure to caller
                return None
            # extract operator type
            operator_type = operator_dict.get("operatorType", "")
            if operator_type not in ("DNI", "DNO"):
                self._root.setProperty("errorText", f"Invalid operator type: {operator_type}")
                # signal validation failure to caller
                return None
            # extract optional noise source
            noise_source = operator_dict.get("noiseSource", "")
            # create device noise operator and add to list
            device_noise_operators.append(DeviceNoiseOperator(device_name=device_name, operator_type=operator_type, noise_source=noise_source))
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
            measure_statement = stripped_line if stripped_line.upper().startswith((".MEASURE", ".MEAS")) else f".MEASURE NOISE {stripped_line}"
            # parse measure parameters
            measure = MeasureEntry.from_xyce_statement(measure_statement)
            # check parse success and analysis type match
            if measure and measure.analysis_type == "NOISE":
                # store the parsed measure parameters
                measure_parameters.append(measure)
            else:
                # report invalid directive to user
                self._root.setProperty("errorText", f"Invalid .MEASURE directive for NOISE: {line}")
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
            # append noise-specific named output variables when their checkboxes are checked
            if print_inoise:
                output_vars.append("INOISE")
            if print_onoise:
                output_vars.append("ONOISE")
            # append any explicitly listed specific variables (e.g. DNI(), DNO() expressions)
            output_vars.extend(v for v in print_specific_vars.split() if v)
            # construct print parameters for the noise analysis type
            print_parameters = PrintParameters(print_type="NOISE", print_format=print_format.strip().upper() if print_format.strip() else "", print_file=print_file.strip(), output_variables=tuple(output_vars))
        # construct parameters instance
        analysis = NoiseSimulationParameters(normalized_output_node, normalized_ref_node, normalized_source_name, normalized_mode, normalized_points, normalized_start, normalized_end, normalized_data_table, replace_ground, print_parameters, tuple(device_noise_operators), measure_parameters=tuple(measure_parameters))
        # return parameters to caller for config assembly
        return analysis
