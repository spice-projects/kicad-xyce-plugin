from .op_simulation_parameters import IcEntry, NodesetEntry, OpSimulationParameters
from .print_parameters import PrintParameters

# print format values matching the combo model order (index 0 is the empty/default value)
_PRINT_FORMATS = ["", "STD", "NOINDEX", "PROBE", "TECPLOT", "RAW", "CSV", "GNUPLOT", "SPLOT"]

# wildcard tokens that map to dedicated checkbox shortcuts in the print section
_PRINT_WILDCARDS = {"V(*)", "I(*)", "P(*)", "W(*)", "IB(*)", "IC(*)", "ID(*)", "IE(*)", "IG(*)", "IS(*)"}

# ordered lead current wildcards per device family for output emission order
_BJT_WILDCARDS = ("IB(*)", "IC(*)", "IE(*)", "IS(*)")
_FET_WILDCARDS = ("IB(*)", "ID(*)", "IG(*)", "IS(*)")


class OpPanel:
    def __init__(self, root):
        # retain root reference for property reads and writes
        self._root = root

    def apply(self, p: OpSimulationParameters | None, has_bjt: bool, has_fet: bool) -> None:
        # extract print parameters for pre-population (prefer print_parameters over legacy fields)
        pp = p.print_parameters if p else None
        # default to enabled with all wildcards and RAW format when no saved print parameters exist
        if pp is None:
            self._root.setProperty("opPrintEnabled", p is None)
            self._root.setProperty("opPrintAllNodes", True)
            self._root.setProperty("opPrintAllCurrents", True)
            self._root.setProperty("opPrintPower", True)
            # default lead current checkboxes to on when the device family is present
            self._root.setProperty("opPrintBjtLeads", has_bjt)
            self._root.setProperty("opPrintFetLeads", has_fet)
            # expose device family flags for conditional checkbox visibility
            self._root.setProperty("opHasBjtDevices", has_bjt)
            self._root.setProperty("opHasFetDevices", has_fet)
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
            self._root.setProperty("opHasBjtDevices", has_bjt)
            self._root.setProperty("opHasFetDevices", has_fet)
            # map format string to combo index (index 0 is the default/empty value)
            fmt_str = pp.print_format.upper() if pp.print_format else ""
            self._root.setProperty("opPrintFormatIndex", _PRINT_FORMATS.index(fmt_str) if fmt_str in _PRINT_FORMATS else 0)
            # restore saved output file path
            self._root.setProperty("opPrintFile", pp.print_file)
            # specific vars: only saved non-wildcard vars (no automatic topology pre-fill)
            self._root.setProperty("opPrintSpecificVars", " ".join(v for v in pp.output_variables if v not in _PRINT_WILDCARDS))
        # restore save, initial-condition, and nodeset state
        self._root.setProperty("saveEnabled", p.save_enabled if p else False)
        self._root.setProperty("saveType", p.save_type if p else "NODESET")
        self._root.setProperty("saveFile", p.save_file if p else "")
        self._root.setProperty("opInitialConditionEntries", " ".join(f"V({e.node})={e.voltage}" for e in p.ic_entries) if p else "")
        self._root.setProperty("nodesetEntries", " ".join(f"V({e.node})={e.voltage}" for e in p.nodeset_entries) if p else "")

    def handle_submit(self, print_enabled: bool, print_all_nodes: bool, print_all_currents: bool, print_power: bool, print_bjt_leads: bool, print_fet_leads: bool, print_specific_vars: str, print_format: str, print_file: str, save_enabled: bool, save_type: str, nodeset_text: str, initial_conditions_text: str, save_file: str, replace_ground: bool) -> OpSimulationParameters:
        # parse nodeset text into entry objects
        nodeset_entries = []
        # basic pattern for V(node)=voltage
        for pair in nodeset_text.split():
            if "=" in pair:
                node_part, voltage = pair.split("=", 1)
                if node_part.startswith("V(") and node_part.endswith(")"):
                    node = node_part[2:-1]
                    nodeset_entries.append(NodesetEntry(node=node, voltage=voltage))
        # parse initial condition text into entry objects
        ic_entries = []
        remaining = initial_conditions_text.split()
        i = 0
        while i < len(remaining):
            token = remaining[i]
            if "=" in token:
                lhs, voltage = token.split("=", 1)
                if lhs.startswith("V(") and lhs.endswith(")"):
                    node = lhs[2:-1]
                else:
                    node = lhs
                ic_entries.append(IcEntry(node=node, voltage=voltage))
                i += 1
            elif i + 1 < len(remaining):
                ic_entries.append(IcEntry(node=token, voltage=remaining[i + 1]))
                i += 2
            else:
                i += 1
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
        # construct parameters instance
        analysis = OpSimulationParameters(save_enabled=save_enabled, save_type=save_type, save_file=save_file.strip(), nodeset_entries=tuple(nodeset_entries), ic_entries=tuple(ic_entries), print_parameters=print_parameters, replace_ground=replace_ground)
        # return parameters to caller for config assembly
        return analysis
