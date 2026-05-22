import re

from .hb_simulation_parameters import HbSimulationParameters
from .print_parameters import PrintParameters

# print type values matching the combo model order
_HB_PRINT_TYPES = ("HB", "HB_FD", "HB_TD")

# print format values matching the combo model order (index 0 is the empty/default value)
_PRINT_FORMATS = ["", "STD", "NOINDEX", "PROBE", "TECPLOT", "RAW", "CSV", "GNUPLOT", "SPLOT"]

# wildcard tokens that map to dedicated checkbox shortcuts in the print section
_PRINT_WILDCARDS = {"V(*)", "I(*)", "P(*)", "W(*)", "IB(*)", "IC(*)", "ID(*)", "IE(*)", "IG(*)", "IS(*)"}


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


class HbPanel:
    def __init__(self, root):
        # retain root reference for property reads and writes
        self._root = root

    def apply(self, p: HbSimulationParameters | None) -> None:
        # restore frequency values text
        self._root.setProperty("hbFrequenciesText", " ".join(p.frequencies) if p else "")
        # restore harmonics text
        self._root.setProperty("hbHarmonicsText", " ".join(str(h) for h in p.harmonics) if p else "")
        # tahb option index: 0 = False, 1 = True
        self._root.setProperty("hbTahbIndex", 1 if p and p.tahb else 0)
        # select_harms option: map to combo index
        select_harms_options = ["none", "all", "acknowledged"]
        # resolve index for the saved select_harms option or default to none
        harms_index = select_harms_options.index(p.select_harms.lower()) if p and p.select_harms.lower() in select_harms_options else 0
        # restore select harms combo selection
        self._root.setProperty("hbSelectHarmsIndex", harms_index)
        # restore startup periods text
        self._root.setProperty("hbStartupPeriodsText", str(p.startup_periods) if p and p.startup_periods else "")
        # clear stale error message
        self._root.setProperty("errorText", "")
        # extract print parameters for pre-population
        pp = p.print_parameters if p else None
        # default to enabled with all wildcards and HB print type when no saved print parameters exist
        if pp is None:
            self._root.setProperty("hbPrintEnabled", p is None)
            self._root.setProperty("hbPrintAllNodes", True)
            self._root.setProperty("hbPrintAllCurrents", True)
            self._root.setProperty("hbPrintTypeIndex", 0)
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
            # map saved print type to combo index
            pt_upper = pp.print_type.upper() if pp.print_type else "HB"
            self._root.setProperty("hbPrintTypeIndex", list(_HB_PRINT_TYPES).index(pt_upper) if pt_upper in _HB_PRINT_TYPES else 0)
            # map format string to combo index (index 0 is the default/empty value)
            fmt_str = pp.print_format.upper() if pp.print_format else ""
            self._root.setProperty("hbPrintFormatIndex", _PRINT_FORMATS.index(fmt_str) if fmt_str in _PRINT_FORMATS else 0)
            # restore saved output file path
            self._root.setProperty("hbPrintFile", pp.print_file)
            # specific vars: only saved non-wildcard vars (no automatic topology pre-fill)
            self._root.setProperty("hbPrintSpecificVars", " ".join(v for v in pp.output_variables if v not in _PRINT_WILDCARDS))

    def handle_submit(self, frequencies_text: str, harmonics_text: str, tahb_index: int, select_harms_value: str, startup_periods: int, print_enabled: bool, print_all_nodes: bool, print_all_currents: bool, print_type: str, print_specific_vars: str, print_format: str, print_file: str, replace_ground: bool) -> HbSimulationParameters | None:
        # parse frequencies list from text
        frequencies = _parse_list_values(frequencies_text)
        # require at least one fundamental frequency
        if not frequencies:
            self._root.setProperty("errorText", "At least one fundamental frequency is required")
            # signal validation failure to caller
            return None
        # parse harmonics list from text (may be empty — defaults applied later in dataclass)
        harmonics_raw = _parse_list_values(harmonics_text)
        # convert harmonics to int and validate
        harmonics: list[int] = []
        for h in harmonics_raw:
            # attempt integer conversion
            try:
                harmonics.append(int(h))
            # surface invalid harmonic value as form-level message
            except ValueError:
                self._root.setProperty("errorText", f"Invalid harmonic value: {h}")
                # signal validation failure to caller
                return None
        # resolve tahb flag from index
        tahb = bool(tahb_index)
        # resolve select_harms string
        normalized_select_harms = select_harms_value.strip().lower()
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
            # construct print parameters using the selected HB print type
            print_parameters = PrintParameters(print_type=print_type.strip().upper() if print_type.strip() else "HB", print_format=print_format.strip().upper() if print_format.strip() else "", print_file=print_file.strip(), output_variables=tuple(output_vars))
        # construct parameters instance
        analysis = HbSimulationParameters(frequencies, tuple(harmonics), tahb, normalized_select_harms, startup_periods, replace_ground, print_parameters)
        # return parameters to caller for config assembly
        return analysis
