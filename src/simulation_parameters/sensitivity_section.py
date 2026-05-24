from .print_parameters import PrintParameters
from .sens_parameter import SensParameter

# print format values matching the combo model order (index 0 is the empty/default value)
_PRINT_FORMATS = ["", "STD", "NOINDEX", "PROBE", "TECPLOT", "RAW", "CSV", "GNUPLOT", "SPLOT"]

_PANEL_PREFIX = {
    "AC": "acSens",
    "DC": "dcSens",
    "TRAN": "tranSens",
}

_TAB_PANEL = {
    1: "TRAN",
    2: "DC",
    3: "AC",
}


class SensitivitySection:

    def __init__(self, root):
        self._root = root

    def apply(self, sensitivity: SensParameter | None, panel_type: str | None = None) -> None:
        # clean up everything
        if panel_type is None:
            # clean up all panels
            for panel in _PANEL_PREFIX:
                self._apply_panel(panel, None)
            # exit
            return
        # apply the selected sensitivity panel settings
        self._apply_panel(panel_type, sensitivity)

    def _apply_panel(self, panel_type: str, sensitivity: SensParameter | None) -> None:
        # resolve the qml property prefix for the selected panel
        prefix = _PANEL_PREFIX[panel_type]
        # check sensitivity is defined
        if sensitivity:
            # restore values from an existing sensitivity model
            self._set(prefix, "ObjectiveMode", sensitivity.objective_mode)
            self._set(prefix, "ObjectiveValues", ",".join(sensitivity.objective_values))
            self._set(prefix, "Parameters", ",".join(sensitivity.parameter_list))
            self._set(prefix, "Direct", sensitivity.direct)
            self._set(prefix, "Adjoint", sensitivity.adjoint)
            # print parameters
            if sensitivity.print_parameters:
                # restore enabled print options
                self._set(prefix, "PrintEnabled", True)
                self._set(prefix, "PrintSpecificVars", " ".join(sensitivity.print_parameters.output_variables))
                fmt_str = sensitivity.print_parameters.print_format.upper() if sensitivity.print_parameters.print_format else ""
                self._set(prefix, "PrintFormatIndex", _PRINT_FORMATS.index(fmt_str) if fmt_str in _PRINT_FORMATS else 0)
                self._set(prefix, "PrintFile", sensitivity.print_parameters.print_file)
            else:
                # clear print options when none were saved
                self._set(prefix, "PrintEnabled", False)
                self._set(prefix, "PrintSpecificVars", "")
                self._set(prefix, "PrintFormatIndex", 0)
                self._set(prefix, "PrintFile", "")
        else:
            # reset panel state to defaults
            self._set(prefix, "ObjectiveMode", "objfunc")
            self._set(prefix, "ObjectiveValues", "")
            self._set(prefix, "Parameters", "")
            self._set(prefix, "Direct", False)
            self._set(prefix, "Adjoint", False)
            self._set(prefix, "PrintEnabled", False)
            self._set(prefix, "PrintSpecificVars", "")
            self._set(prefix, "PrintFormatIndex", 0)
            self._set(prefix, "PrintFile", "")

    def get_current(self) -> SensParameter | None:
        # select the panel type for the current tab
        current_tab = self._root.property("currentTabIndex")
        panel_type = _TAB_PANEL.get(current_tab)
        return self._build_from_panel(panel_type) if panel_type else None

    def _build_from_panel(self, panel_type: str | None) -> SensParameter | None:
        if panel_type is None:
            return None
        prefix = _PANEL_PREFIX[panel_type]
        if not self._root.property(f"{prefix}Enabled"):
            return None
        # read configured values from the selected sensitivity section
        objective_mode = self._root.property(f"{prefix}ObjectiveMode")
        objective_values_text = self._root.property(f"{prefix}ObjectiveValues")
        parameters_text = self._root.property(f"{prefix}Parameters")
        direct = self._root.property(f"{prefix}Direct")
        adjoint = self._root.property(f"{prefix}Adjoint")
        print_enabled = self._root.property(f"{prefix}PrintEnabled")
        print_specific_vars = self._root.property(f"{prefix}PrintSpecificVars")
        print_format = self._root.property(f"{prefix}PrintFormatValue")
        print_file = self._root.property(f"{prefix}PrintFile")
        # normalize comma-separated input into tuples
        objective_values = tuple(value.strip() for value in objective_values_text.split(",") if value.strip())
        parameters = tuple(parameter.strip() for parameter in parameters_text.split(",") if parameter.strip())
        # print parameters
        print_parameters: PrintParameters | None = None
        # check print is enabled
        if print_enabled:
            # variables
            output_variables = tuple(variable for variable in print_specific_vars.split() if variable)
            # create print parameters
            print_parameters = PrintParameters(print_type="SENS", print_format=print_format.strip().upper() if print_format and print_format.strip() else "", print_file=print_file.strip(), output_variables=output_variables)
        # exit
        return SensParameter(panel_type, objective_mode, objective_values, parameters, direct, adjoint, print_parameters)

    def _set(self, prefix: str, suffix: str, value):
        self._root.setProperty(f"{prefix}{suffix}", value)
