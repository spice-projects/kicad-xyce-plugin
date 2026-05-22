from __future__ import annotations

import logging
from dataclasses import dataclass


# init module logger
logger = logging.getLogger(__name__)

# init allowed analysis types from the reference guide
_ALLOWED_ANALYSIS_TYPES = {
    "TRAN",
    "AC",
    "DC",
    "NOISE",
    "TRAN_CONT",
    "AC_CONT",
    "DC_CONT",
    "NOISE_CONT",
}

# init allowed measurement types from the reference guide
_ALLOWED_MEASURE_TYPES = {
    "AVG",
    "DERIV",
    "DUTY",
    "EQN",
    "ERR",
    "ERR1",
    "ERR2",
    "ERROR",
    "FIND",
    "FOUR",
    "FREQ",
    "INTEG",
    "MAX",
    "MIN",
    "OFF_TIME",
    "ON_TIME",
    "PP",
    "RMS",
    "WHEN",
    # FFT types
    "ENOB",
    "SFDR",
    "SNDR",
    "SNR",
    "THD",
}

# init measure type aliases
_MEASURE_TYPE_ALIASES = {
    "DERIVATIVE": "DERIV",
    "INTEGRAL": "INTEG",
    "PARAM": "EQN",
}

# init measurement types that support TD qualifier (TRAN only)
_TD_SUPPORTED_TYPES = {
    "AVG",
    "DERIV",
    "DUTY",
    "EQN",
    "FIND",
    "FREQ",
    "INTEG",
    "MAX",
    "MIN",
    "OFF_TIME",
    "ON_TIME",
    "PP",
    "RMS",
    "WHEN",
}

# init measurement types that support RISE/FALL/CROSS qualifiers
_RFC_SUPPORTED_TYPES = {
    "DERIV",
    "FIND",
    "MAX",
    "MIN",
    "PP",
    "WHEN",
}

# init measurement types that support AT qualifier
_AT_SUPPORTED_TYPES = {
    "DERIV",
    "FIND",
    "FOUR",
}

# init measurement types that support ON/OFF qualifiers
_ON_OFF_SUPPORTED_TYPES = {
    "DUTY",
    "FREQ",
    "ON_TIME",
    "OFF_TIME",
}

# init measurement types that support RFC_LEVEL qualifier
_RFC_LEVEL_SUPPORTED_TYPES = {
    "MAX",
    "MIN",
    "PP",
}

# init measurement types that support OUTPUT qualifier
_OUTPUT_SUPPORTED_TYPES = {
    "MAX",
    "MIN",
}

# init measurement types that support FRAC_MAX qualifier
_FRAC_MAX_SUPPORTED_TYPES = {
    "TRIG",
    "TARG",
}

# init measurement types that support BINSIZ qualifier
_BINSIZ_SUPPORTED_TYPES = {
    "ENOB",
    "SFDR",
    "SNDR",
}

# init measurement types that support MAXFREQ qualifier
_MAXFREQ_SUPPORTED_TYPES = {
    "SFDR",
    "SNR",
    "THD",
}

# init measurement types that support MINFREQ qualifier
_MINFREQ_SUPPORTED_TYPES = {
    "SFDR",
    "THD",
}

# init measurement types that support NBHARM qualifier
_NBHARM_SUPPORTED_TYPES = {
    "THD",
}

# init measurement types specific to TRAN analysis
_TRAN_ONLY_TYPES = {
    "DUTY",
    "FOUR",
    "FREQ",
    "OFF_TIME",
    "ON_TIME",
    "ENOB",
    "SFDR",
    "SNDR",
    "SNR",
    "THD",
}

# init error function types
_ERROR_FUNCTION_TYPES = {
    "ERR1",
    "ERR2",
}


def _tokenize_measure_statement(measure_statement: str) -> list[str]:
    # init token list
    tokens: list[str] = []
    # init current token buffer
    current_chars: list[str] = []
    # init brace nesting depth
    brace_depth = 0
    # iterate characters
    for char in measure_statement:
        # check opening brace
        if char == "{":
            # append char
            current_chars.append(char)
            # increment depth
            brace_depth += 1
            # next
            continue
        # check closing brace
        if char == "}":
            # append char
            current_chars.append(char)
            # decrement depth
            brace_depth = max(0, brace_depth - 1)
            # next
            continue
        # check whitespace splitter
        if char.isspace() and brace_depth == 0:
            # check token has chars
            if current_chars:
                # append token
                tokens.append("".join(current_chars))
                # reset buffer
                current_chars = []
            # next
            continue
        # append regular char
        current_chars.append(char)
    # check trailing token
    if current_chars:
        # append trailing token
        tokens.append("".join(current_chars))
    # return tokens
    return tokens


@dataclass(frozen=True)
class MeasureEntry:

    analysis_type: str
    result_name: str
    measure_type: str
    variable: str
    # common qualifiers
    from_val: str = ""
    to_val: str = ""
    td_val: str = ""
    rise_val: str = ""
    fall_val: str = ""
    cross_val: str = ""
    minval: str = ""
    default_val: str = ""
    precision: str = ""
    print_val: str = ""
    # type-specific qualifiers
    at_val: str = ""
    on_val: str = ""
    off_val: str = ""
    rfc_level: str = ""
    output: str = ""
    min_thresh: str = ""
    max_thresh: str = ""
    frac_max: str = ""
    # WHEN clause support
    when_variable: str = ""
    when_condition: str = ""
    # second variable for ERR1/ERR2
    variable2: str = ""
    # TRIG-TARG qualifiers
    trig_variable: str = ""
    trig_condition: str = ""
    trig_val: str = ""
    trig_frac_max: str = ""
    trig_td: str = ""
    trig_rise: str = ""
    trig_fall: str = ""
    trig_cross: str = ""
    trig_at_val: str = ""
    targ_variable: str = ""
    targ_condition: str = ""
    targ_val: str = ""
    targ_frac_max: str = ""
    targ_td: str = ""
    targ_rise: str = ""
    targ_fall: str = ""
    targ_cross: str = ""
    targ_at_val: str = ""
    # ERROR-specific qualifiers
    error_file: str = ""
    indepvarcol: str = ""
    depvarcol: str = ""
    comp_function: str = ""
    # FOUR-specific qualifiers
    numfreq: str = ""
    gridsize: str = ""
    # FFT-specific qualifiers
    binsiz: str = ""
    maxfreq: str = ""
    minfreq: str = ""
    nbharm: str = ""
    # HSPICE compatibility (ignored by Xyce)
    goal: str = ""
    weight: str = ""

    @classmethod
    def from_xyce_statement(cls, measure_statement: str) -> "MeasureEntry" | None:
        # parse tokens
        tokens = _tokenize_measure_statement(measure_statement)
        # reject statements that are too short
        if len(tokens) < 4:
            # return none
            return None
        # normalize command
        cmd = tokens[0].upper()
        # check for .MEASURE or .MEAS
        if cmd not in (".MEASURE", ".MEAS"):
            # return none
            return None
        # parse analysis type and result name
        analysis_type = tokens[1].upper()
        # validate analysis type
        if analysis_type not in _ALLOWED_ANALYSIS_TYPES:
            # return none
            return None
        result_name = tokens[2]

        # check for FFT keyword
        idx = 3
        if tokens[idx].upper() == "FFT":
            idx += 1
            if idx >= len(tokens):
                return None

        # determine measure type
        measure_type_raw = tokens[idx].upper()
        idx += 1
        # resolve aliases
        measure_type_upper = _MEASURE_TYPE_ALIASES.get(measure_type_raw, measure_type_raw)
        # check for TRIG keyword (special case)
        if measure_type_upper == "TRIG":
            measure_type = "TRIG"
        # check for other keywords
        elif measure_type_upper in _ALLOWED_MEASURE_TYPES:
            measure_type = measure_type_upper
        # handle unknown measure type
        else:
            return None

        # init fields
        fields: dict[str, str] = {
            "analysis_type": analysis_type,
            "result_name": result_name,
            "measure_type": measure_type,
            "variable": "",
        }

        # handle TRIG-TARG syntax
        if measure_type == "TRIG":
            # parse TRIG clause

            def parse_clause(start_idx: int, prefix: str) -> tuple[int, dict[str, str]]:
                c_idx = start_idx
                clause_fields: dict[str, str] = {}
                # check for AT form (AT=<value>)
                if c_idx < len(tokens) and tokens[c_idx].upper().startswith("AT="):
                    # set at value
                    clause_fields[f"{prefix}_at_val"] = tokens[c_idx].split("=", 1)[1] if "=" in tokens[c_idx] else ""
                    # advance index
                    c_idx += 1
                # check for variable form
                elif c_idx < len(tokens):
                    # split variable and condition
                    token = tokens[c_idx]
                    # check for equals sign in token
                    if "=" in token:
                        # split at first equals
                        parts = token.split("=", 1)
                        # set variable
                        clause_fields[f"{prefix}_variable"] = parts[0]
                        # set condition
                        clause_fields[f"{prefix}_condition"] = "=" + parts[1]
                    # handle simple variable
                    else:
                        # set variable
                        clause_fields[f"{prefix}_variable"] = token
                    # advance index
                    c_idx += 1
                    # parse qualifiers
                    while c_idx < len(tokens):
                        # normalize token
                        token_upper = tokens[c_idx].upper()
                        # stop on TARG if we are in TRIG
                        if prefix == "trig" and token_upper == "TARG":
                            break
                        # stop on major qualifiers that follow TRIG-TARG
                        if token_upper in ("MINVAL", "DEFAULT_VAL", "PRECISION", "PRINT"):
                            break
                        # handle qualifiers
                        if "=" in tokens[c_idx]:
                            key, val = tokens[c_idx].split("=", 1)
                            key_upper = key.upper()
                            if key_upper == "TD":
                                clause_fields[f"{prefix}_td"] = val
                            elif key_upper == "RISE":
                                clause_fields[f"{prefix}_rise"] = val
                            elif key_upper == "FALL":
                                clause_fields[f"{prefix}_fall"] = val
                            elif key_upper == "CROSS":
                                clause_fields[f"{prefix}_cross"] = val
                            elif key_upper == "VAL":
                                clause_fields[f"{prefix}_val"] = val
                            elif key_upper == "FRAC_MAX":
                                clause_fields[f"{prefix}_frac_max"] = val
                            else:
                                # unknown qualifier, log and continue
                                logger.warning("Ignoring unknown qualifier '%s' in %s clause", key, prefix)
                            c_idx += 1
                        else:
                            # Not an assignment and not TARG, likely end of clause or invalid
                            break
                return c_idx, clause_fields

            idx, trig_fields = parse_clause(idx, "trig")
            fields.update(trig_fields)

            # parse TARG clause
            if idx < len(tokens) and tokens[idx].upper() == "TARG":
                idx += 1
                idx, targ_fields = parse_clause(idx, "targ")
                fields.update(targ_fields)

            # Standard qualifiers can follow TRIG-TARG
            for i in range(idx, len(tokens)):
                token = tokens[i]
                if "=" in token:
                    key, val = token.split("=", 1)
                    key_upper = key.upper()
                    if key_upper == "MINVAL":
                        fields["minval"] = val
                    elif key_upper == "DEFAULT_VAL":
                        fields["default_val"] = val
                    elif key_upper == "PRECISION":
                        fields["precision"] = val
                    elif key_upper == "PRINT":
                        fields["print_val"] = val

        # handle standard measurement types
        else:
            # check if variable is present
            if idx < len(tokens):
                # check for WHEN keyword (WHEN measure type has no variable)
                if measure_type == "WHEN":
                    # set variable to empty for WHEN measure type
                    fields["variable"] = ""
                    # parse WHEN condition
                    when_token = tokens[idx]
                    idx += 1
                    # check for equals sign in token
                    if "=" in when_token:
                        # split variable and condition
                        parts = when_token.split("=", 1)
                        # set when variable
                        fields["when_variable"] = parts[0]
                        # set when condition
                        fields["when_condition"] = "=" + parts[1]
                    else:
                        fields["when_variable"] = when_token
                # handle normal variable
                else:
                    # set variable
                    fields["variable"] = tokens[idx]
                    idx += 1

            # handle ERR1/ERR2 two-variable syntax
            if measure_type in ("ERR1", "ERR2"):
                # check for second variable
                if idx < len(tokens):
                    # set second variable
                    fields["variable2"] = tokens[idx]
                    idx += 1

            # iterate remaining tokens
            i = idx
            while i < len(tokens):
                # get token
                token = tokens[i]
                # check for WHEN keyword
                if token.upper() == "WHEN" and i + 1 < len(tokens):
                    # set when variable
                    fields["when_variable"] = tokens[i + 1]
                    # check for condition in same token
                    if "=" in fields["when_variable"]:
                        # split variable and condition
                        parts = fields["when_variable"].split("=", 1)
                        # set when variable
                        fields["when_variable"] = parts[0]
                        # set when condition without space
                        fields["when_condition"] = "=" + parts[1]
                        i += 2
                    # check if next token starts with equals (separate tokens)
                    elif i + 2 < len(tokens) and tokens[i + 2].startswith("="):
                        # set when condition from next token
                        fields["when_condition"] = tokens[i + 2]
                        # advance index by 3
                        i += 3
                    else:
                        i += 2
                    # continue to next token
                    continue
                # check for equals sign
                if "=" in token:
                    # split key and value
                    key, val = token.split("=", 1)
                    # normalize key
                    key_upper = key.upper()
                    # map FROM
                    if key_upper == "FROM":
                        fields["from_val"] = val
                    # map TO
                    elif key_upper == "TO":
                        fields["to_val"] = val
                    # map TD
                    elif key_upper == "TD":
                        if measure_type in _TD_SUPPORTED_TYPES:
                            fields["td_val"] = val
                        else:
                            logger.warning("Ignoring TD qualifier for measure type '%s'", measure_type)
                    # map RISE
                    elif key_upper == "RISE":
                        if measure_type in _RFC_SUPPORTED_TYPES or measure_type == "WHEN":
                            fields["rise_val"] = val
                        else:
                            logger.warning("Ignoring RISE qualifier for measure type '%s'", measure_type)
                    # map FALL
                    elif key_upper == "FALL":
                        if measure_type in _RFC_SUPPORTED_TYPES or measure_type == "WHEN":
                            fields["fall_val"] = val
                        else:
                            logger.warning("Ignoring FALL qualifier for measure type '%s'", measure_type)
                    # map CROSS
                    elif key_upper == "CROSS":
                        if measure_type in _RFC_SUPPORTED_TYPES or measure_type == "WHEN":
                            fields["cross_val"] = val
                        else:
                            logger.warning("Ignoring CROSS qualifier for measure type '%s'", measure_type)
                    # map MINVAL
                    elif key_upper == "MINVAL":
                        fields["minval"] = val
                    # map DEFAULT_VAL
                    elif key_upper == "DEFAULT_VAL":
                        fields["default_val"] = val
                    # map PRECISION
                    elif key_upper == "PRECISION":
                        fields["precision"] = val
                    # map FRAC_MAX
                    elif key_upper == "FRAC_MAX":
                        fields["frac_max"] = val
                    # map PRINT
                    elif key_upper == "PRINT":
                        fields["print_val"] = val
                    # map AT
                    elif key_upper == "AT":
                        if measure_type in _AT_SUPPORTED_TYPES:
                            fields["at_val"] = val
                        else:
                            logger.warning("Ignoring AT qualifier for measure type '%s'", measure_type)
                    # map ON
                    elif key_upper == "ON":
                        if measure_type in _ON_OFF_SUPPORTED_TYPES:
                            fields["on_val"] = val
                        else:
                            logger.warning("Ignoring ON qualifier for measure type '%s'", measure_type)
                    # map OFF
                    elif key_upper == "OFF":
                        if measure_type in _ON_OFF_SUPPORTED_TYPES:
                            fields["off_val"] = val
                        else:
                            logger.warning("Ignoring OFF qualifier for measure type '%s'", measure_type)
                    # map RFC_LEVEL
                    elif key_upper == "RFC_LEVEL":
                        if measure_type in _RFC_LEVEL_SUPPORTED_TYPES:
                            fields["rfc_level"] = val
                        else:
                            logger.warning("Ignoring RFC_LEVEL qualifier for measure type '%s'", measure_type)
                    # map OUTPUT
                    elif key_upper == "OUTPUT":
                        if measure_type in _OUTPUT_SUPPORTED_TYPES:
                            fields["output"] = val
                        else:
                            logger.warning("Ignoring OUTPUT qualifier for measure type '%s'", measure_type)
                    # map MIN_THRESH
                    elif key_upper == "MIN_THRESH":
                        if measure_type == "AVG":
                            fields["min_thresh"] = val
                        else:
                            logger.warning("Ignoring MIN_THRESH qualifier for non-AVG measure type '%s'", measure_type)
                    # map MAX_THRESH
                    elif key_upper == "MAX_THRESH":
                        if measure_type == "AVG":
                            fields["max_thresh"] = val
                        else:
                            logger.warning("Ignoring MAX_THRESH qualifier for non-AVG measure type '%s'", measure_type)
                    # map FILE (ERROR-specific)
                    elif key_upper == "FILE":
                        if measure_type == "ERROR":
                            fields["error_file"] = val
                        else:
                            logger.warning("Ignoring FILE qualifier for non-ERROR measure type '%s'", measure_type)
                    # map INDEPVARCOL (ERROR-specific)
                    elif key_upper == "INDEPVARCOL":
                        if measure_type == "ERROR":
                            fields["indepvarcol"] = val
                        else:
                            logger.warning("Ignoring INDEPVARCOL qualifier for non-ERROR measure type '%s'", measure_type)
                    # map DEPVARCOL (ERROR-specific)
                    elif key_upper == "DEPVARCOL":
                        if measure_type == "ERROR":
                            fields["depvarcol"] = val
                        else:
                            logger.warning("Ignoring DEPVARCOL qualifier for non-ERROR measure type '%s'", measure_type)
                    # map COMP_FUNCTION (ERROR-specific)
                    elif key_upper == "COMP_FUNCTION":
                        if measure_type == "ERROR":
                            fields["comp_function"] = val
                        else:
                            logger.warning("Ignoring COMP_FUNCTION qualifier for non-ERROR measure type '%s'", measure_type)
                    # map NUMFREQ (FOUR-specific)
                    elif key_upper == "NUMFREQ":
                        if measure_type == "FOUR":
                            fields["numfreq"] = val
                        else:
                            logger.warning("Ignoring NUMFREQ qualifier for non-FOUR measure type '%s'", measure_type)
                    # map GRIDSIZE (FOUR-specific)
                    elif key_upper == "GRIDSIZE":
                        if measure_type == "FOUR":
                            fields["gridsize"] = val
                        else:
                            logger.warning("Ignoring GRIDSIZE qualifier for non-FOUR measure type '%s'", measure_type)
                    # map BINSIZ (FFT-specific)
                    elif key_upper == "BINSIZ":
                        if measure_type in _BINSIZ_SUPPORTED_TYPES:
                            fields["binsiz"] = val
                        else:
                            logger.warning("Ignoring BINSIZ qualifier for measure type '%s'", measure_type)
                    # map MAXFREQ (FFT-specific)
                    elif key_upper == "MAXFREQ":
                        if measure_type in _MAXFREQ_SUPPORTED_TYPES:
                            fields["maxfreq"] = val
                        else:
                            logger.warning("Ignoring MAXFREQ qualifier for measure type '%s'", measure_type)
                    # map MINFREQ (FFT-specific)
                    elif key_upper == "MINFREQ":
                        if measure_type in _MINFREQ_SUPPORTED_TYPES:
                            fields["minfreq"] = val
                        else:
                            logger.warning("Ignoring MINFREQ qualifier for measure type '%s'", measure_type)
                    # map NBHARM (FFT-specific)
                    elif key_upper == "NBHARM":
                        if measure_type in _NBHARM_SUPPORTED_TYPES:
                            fields["nbharm"] = val
                        else:
                            logger.warning("Ignoring NBHARM qualifier for measure type '%s'", measure_type)
                    # map GOAL (Compatibility)
                    elif key_upper == "GOAL":
                        fields["goal"] = val
                    # map WEIGHT (Compatibility)
                    elif key_upper == "WEIGHT":
                        fields["weight"] = val
                    # handle unknown option
                    else:
                        # log warning
                        logger.warning("Ignoring unknown .MEASURE option '%s'", key)
                i += 1
        # return model
        return cls(**fields)

    def to_xyce_statement(self) -> str:
        # init tokens
        tokens = [".MEASURE", self.analysis_type, self.result_name]
        # handle TRIG-TARG syntax
        if self.measure_type == "TRIG":
            # append TRIG keyword
            tokens.append("TRIG")
            # check for AT form
            if self.trig_at_val:
                # append AT clause with equals
                tokens.append(f"AT={self.trig_at_val}")
            # check for variable form
            elif self.trig_variable:
                # check for condition
                if self.trig_condition:
                    # append combined variable and condition
                    tokens.append(self.trig_variable + self.trig_condition)
                elif self.trig_val:
                    # handle compatibility VAL=
                    tokens.append(f"{self.trig_variable} VAL={self.trig_val}")
                else:
                    # append trig variable
                    tokens.append(self.trig_variable)
                # append trig qualifiers
                if self.trig_td:
                    tokens.append(f"TD={self.trig_td}")
                if self.trig_rise:
                    tokens.append(f"RISE={self.trig_rise}")
                if self.trig_fall:
                    tokens.append(f"FALL={self.trig_fall}")
                if self.trig_cross:
                    tokens.append(f"CROSS={self.trig_cross}")
                if self.trig_frac_max:
                    tokens.append(f"FRAC_MAX={self.trig_frac_max}")
            # append TARG keyword
            tokens.append("TARG")
            # check for AT form
            if self.targ_at_val:
                # append AT clause with equals
                tokens.append(f"AT={self.targ_at_val}")
            # check for variable form
            elif self.targ_variable:
                # check for condition
                if self.targ_condition:
                    # append combined variable and condition
                    tokens.append(self.targ_variable + self.targ_condition)
                elif self.targ_val:
                    # handle compatibility VAL=
                    tokens.append(f"{self.targ_variable} VAL={self.targ_val}")
                else:
                    # append targ variable
                    tokens.append(self.targ_variable)
                # append targ qualifiers
                if self.targ_td:
                    tokens.append(f"TD={self.targ_td}")
                if self.targ_rise:
                    tokens.append(f"RISE={self.targ_rise}")
                if self.targ_fall:
                    tokens.append(f"FALL={self.targ_fall}")
                if self.targ_cross:
                    tokens.append(f"CROSS={self.targ_cross}")
                if self.targ_frac_max:
                    tokens.append(f"FRAC_MAX={self.targ_frac_max}")
            # Standard qualifiers can follow TRIG-TARG
            if self.minval:
                tokens.append(f"MINVAL={self.minval}")
            if self.default_val:
                tokens.append(f"DEFAULT_VAL={self.default_val}")
            if self.precision:
                tokens.append(f"PRECISION={self.precision}")
            if self.frac_max:
                tokens.append(f"FRAC_MAX={self.frac_max}")
            if self.print_val:
                tokens.append(f"PRINT={self.print_val}")
        # handle standard measurement types
        else:
            # check for FFT measure type
            if self.measure_type in ("ENOB", "SFDR", "SNDR", "SNR", "THD"):
                tokens.append("FFT")
            # append measure type
            tokens.append(self.measure_type)
            # append variable
            if self.variable:
                tokens.append(self.variable)
            # append second variable for ERR1/ERR2
            if self.variable2:
                tokens.append(self.variable2)
            # append WHEN clause (except for WHEN measure type)
            if self.when_variable and self.measure_type != "WHEN":
                # append WHEN keyword
                tokens.append("WHEN")
                # check for condition
                if self.when_condition:
                    # append combined variable and condition
                    tokens.append(self.when_variable + self.when_condition)
                else:
                    # append when variable
                    tokens.append(self.when_variable)
            # handle WHEN measure type specifically
            elif self.measure_type == "WHEN" and self.when_variable:
                # append when variable and condition combined
                if self.when_condition:
                    tokens.append(self.when_variable + self.when_condition)
                else:
                    tokens.append(self.when_variable)
            # append common qualifiers
            if self.from_val:
                tokens.append(f"FROM={self.from_val}")
            if self.to_val:
                tokens.append(f"TO={self.to_val}")
            if self.td_val:
                tokens.append(f"TD={self.td_val}")
            if self.rise_val:
                tokens.append(f"RISE={self.rise_val}")
            if self.fall_val:
                tokens.append(f"FALL={self.fall_val}")
            if self.cross_val:
                tokens.append(f"CROSS={self.cross_val}")
            if self.minval:
                tokens.append(f"MINVAL={self.minval}")
            if self.default_val:
                tokens.append(f"DEFAULT_VAL={self.default_val}")
            if self.precision:
                tokens.append(f"PRECISION={self.precision}")
            if self.frac_max:
                tokens.append(f"FRAC_MAX={self.frac_max}")
            if self.print_val:
                tokens.append(f"PRINT={self.print_val}")
            # type-specific qualifiers
            if self.at_val:
                tokens.append(f"AT={self.at_val}")
            if self.on_val:
                tokens.append(f"ON={self.on_val}")
            if self.off_val:
                tokens.append(f"OFF={self.off_val}")
            if self.rfc_level:
                tokens.append(f"RFC_LEVEL={self.rfc_level}")
            if self.output:
                tokens.append(f"OUTPUT={self.output}")
            if self.min_thresh:
                tokens.append(f"MIN_THRESH={self.min_thresh}")
            if self.max_thresh:
                tokens.append(f"MAX_THRESH={self.max_thresh}")
            # ERROR-specific qualifiers
            if self.error_file:
                tokens.append(f"FILE={self.error_file}")
            if self.indepvarcol:
                tokens.append(f"INDEPVARCOL={self.indepvarcol}")
            if self.depvarcol:
                tokens.append(f"DEPVARCOL={self.depvarcol}")
            if self.comp_function:
                tokens.append(f"COMP_FUNCTION={self.comp_function}")
            # FOUR-specific qualifiers
            if self.numfreq:
                tokens.append(f"NUMFREQ={self.numfreq}")
            if self.gridsize:
                tokens.append(f"GRIDSIZE={self.gridsize}")
            # FFT-specific qualifiers
            if self.binsiz:
                tokens.append(f"BINSIZ={self.binsiz}")
            if self.maxfreq:
                tokens.append(f"MAXFREQ={self.maxfreq}")
            if self.minfreq:
                tokens.append(f"MINFREQ={self.minfreq}")
            if self.nbharm:
                tokens.append(f"NBHARM={self.nbharm}")
            # Compatibility
            if self.goal:
                tokens.append(f"GOAL={self.goal}")
            if self.weight:
                tokens.append(f"WEIGHT={self.weight}")
        # return joined statement
        return " ".join(tokens)
