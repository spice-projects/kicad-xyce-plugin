from .print_parameters import PrintParameters
from .sens_simulation_parameters import SensSimulationParameters

# print format values matching the combo model order (index 0 is the empty/default value)
_PRINT_FORMATS = ["", "STD", "NOINDEX", "PROBE", "TECPLOT", "RAW", "CSV", "GNUPLOT", "SPLOT"]


class SensPanel:
    def __init__(self, root):
        # retain root reference for property reads and writes
        self._root = root

    def apply(self, p: SensSimulationParameters | None) -> None:
        # set enabled flag based on presence of parameters
        self._root.setProperty("sensEnabled", p is not None)
        # initialize sensitivity controls when parameters are available
        if p:
            # restore objective mode selection
            self._root.setProperty("sensObjectiveMode", p.objective_mode)
            # restore objective values as comma-separated text
            self._root.setProperty("sensObjectiveValues", ",".join(p.objective_values))
            # restore parameter list as comma-separated text
            self._root.setProperty("sensParameters", ",".join(p.parameter_list))
            # restore method checkboxes
            self._root.setProperty("sensDirect", p.direct)
            self._root.setProperty("sensAdjoint", p.adjoint)
            # handle print parameter initialization
            if p.print_parameters:
                # restore print enabled state
                self._root.setProperty("sensPrintEnabled", True)
                # restore output variables
                self._root.setProperty("sensPrintSpecificVars", " ".join(p.print_parameters.output_variables))
                # map format string to combo index
                fmt_str = p.print_parameters.print_format.upper() if p.print_parameters.print_format else ""
                self._root.setProperty("sensPrintFormatIndex", _PRINT_FORMATS.index(fmt_str) if fmt_str in _PRINT_FORMATS else 0)
                # restore saved output file path
                self._root.setProperty("sensPrintFile", p.print_parameters.print_file)
            else:
                # disable print section when no print parameters were saved
                self._root.setProperty("sensPrintEnabled", False)
                self._root.setProperty("sensPrintSpecificVars", "")
                self._root.setProperty("sensPrintFormatIndex", 0)
                self._root.setProperty("sensPrintFile", "")
        else:
            # reset all fields to defaults when no parameters are provided
            self._root.setProperty("sensObjectiveMode", "objfunc")
            self._root.setProperty("sensObjectiveValues", "")
            self._root.setProperty("sensParameters", "")
            self._root.setProperty("sensDirect", False)
            self._root.setProperty("sensAdjoint", False)
            # reset print section to defaults
            self._root.setProperty("sensPrintEnabled", False)
            self._root.setProperty("sensPrintSpecificVars", "")
            self._root.setProperty("sensPrintFormatIndex", 0)
            self._root.setProperty("sensPrintFile", "")

    def handle_submit(self, objective_mode: str, objective_values: str, parameters: str, direct: bool, adjoint: bool, replace_ground: bool, print_enabled: bool, print_specific_vars: str, print_format: str, print_file: str) -> SensSimulationParameters | None:
        # validate objective mode is present
        if not objective_mode.strip():
            # report error
            self._root.setProperty("errorText", "Objective mode is required")
            # signal validation failure to caller
            return None
        # validate objective values are present
        if not objective_values.strip():
            # report error
            self._root.setProperty("errorText", "Objective values are required")
            # signal validation failure to caller
            return None
        # validate parameters are present
        if not parameters.strip():
            # report error
            self._root.setProperty("errorText", "Parameters are required")
            # signal validation failure to caller
            return None
        # clear any previous errors
        self._root.setProperty("errorText", "")
        # build print parameters when enabled
        print_parameters = None
        if print_enabled:
            # construct print parameters for sens
            print_parameters = PrintParameters(print_type="SENS", print_format=print_format.strip().upper() if print_format.strip() else "", print_file=print_file.strip(), output_variables=tuple(v for v in print_specific_vars.split() if v))
        # construct parameters instance
        analysis = SensSimulationParameters("DC", objective_mode, tuple(objective_values.split(",")), tuple(parameters.split(",")), direct, adjoint, print_parameters, replace_ground)
        # return parameters to caller for config assembly
        return analysis
