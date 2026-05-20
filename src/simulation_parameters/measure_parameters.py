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

# init measurement types specific to TRAN analysis
_TRAN_ONLY_TYPES = {
    "DUTY",
    "FOUR",
    "FREQ",
    "OFF_TIME",
    "ON_TIME",
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
    # WHEN clause support
    when_variable: str = ""
    when_condition: str = ""
    # second variable for ERR1/ERR2
    variable2: str = ""
    # TRIG-TARG qualifiers
    trig_variable: str = ""
    trig_condition: str = ""
    trig_td: str = ""
    trig_rise: str = ""
    trig_fall: str = ""
    trig_cross: str = ""
    trig_at_val: str = ""
    targ_variable: str = ""
    targ_condition: str = ""
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
        # determine measure type
        measure_type_upper = tokens[3].upper()
        # check for TRIG keyword (special case)
        if measure_type_upper == "TRIG":
            # set measure type
            measure_type = "TRIG"
        # check for other keywords
        elif measure_type_upper in _ALLOWED_MEASURE_TYPES:
            # set measure type
            measure_type = measure_type_upper
        # handle unknown measure type
        else:
            # return none
            return None
        # init variable
        variable = ""
        # init common qualifiers
        from_val = ""
        to_val = ""
        td_val = ""
        rise_val = ""
        fall_val = ""
        cross_val = ""
        minval = ""
        default_val = ""
        precision = ""
        print_val = ""
        # init type-specific qualifiers
        at_val = ""
        on_val = ""
        off_val = ""
        rfc_level = ""
        output = ""
        # init WHEN clause fields
        when_variable = ""
        when_condition = ""
        # init second variable for ERR1/ERR2
        variable2 = ""
        # init TRIG-TARG qualifiers
        trig_variable = ""
        trig_condition = ""
        trig_td = ""
        trig_rise = ""
        trig_fall = ""
        trig_cross = ""
        trig_at_val = ""
        targ_variable = ""
        targ_condition = ""
        targ_td = ""
        targ_rise = ""
        targ_fall = ""
        targ_cross = ""
        targ_at_val = ""
        # init ERROR-specific qualifiers
        error_file = ""
        indepvarcol = ""
        depvarcol = ""
        comp_function = ""
        # init FOUR-specific qualifiers
        numfreq = ""
        gridsize = ""
        # handle TRIG-TARG syntax
        if measure_type == "TRIG":
            # parse TRIG clause
            idx = 4
            # check for AT form (AT=<value>)
            if idx < len(tokens) and tokens[idx].upper().startswith("AT="):
                # set trig at value
                trig_at_val = tokens[idx].split("=", 1)[1] if "=" in tokens[idx] else ""
                # advance index
                idx += 1
            # check for variable form
            elif idx < len(tokens):
                # split variable and condition
                trig_token = tokens[idx]
                # check for equals sign in token
                if "=" in trig_token:
                    # split at first equals
                    parts = trig_token.split("=", 1)
                    # set trig variable
                    trig_variable = parts[0]
                    # set trig condition
                    trig_condition = "=" + parts[1]
                # handle simple variable
                else:
                    # set trig variable
                    trig_variable = trig_token
                # advance index
                idx += 1
                # parse TRIG qualifiers
                while idx < len(tokens):
                    # normalize token
                    token_upper = tokens[idx].upper()
                    # check for TD
                    if token_upper.startswith("TD="):
                        # set trig td
                        trig_td = tokens[idx].split("=", 1)[1] if "=" in tokens[idx] else ""
                        # advance index
                        idx += 1
                    # check for RISE
                    elif token_upper.startswith("RISE="):
                        # set trig rise
                        trig_rise = tokens[idx].split("=", 1)[1] if "=" in tokens[idx] else ""
                        # advance index
                        idx += 1
                    # check for FALL
                    elif token_upper.startswith("FALL="):
                        # set trig fall
                        trig_fall = tokens[idx].split("=", 1)[1] if "=" in tokens[idx] else ""
                        # advance index
                        idx += 1
                    # check for CROSS
                    elif token_upper.startswith("CROSS="):
                        # set trig cross
                        trig_cross = tokens[idx].split("=", 1)[1] if "=" in tokens[idx] else ""
                        # advance index
                        idx += 1
                    # break on TARG
                    elif token_upper == "TARG":
                        # break loop
                        break
                    # handle unknown token
                    else:
                        # log warning
                        logger.warning("Ignoring unknown TRIG qualifier '%s'", tokens[idx])
                        # advance index
                        idx += 1
            # parse TARG clause
            if idx < len(tokens) and tokens[idx].upper() == "TARG":
                # advance index
                idx += 1
                # check for AT form (AT=<value>)
                if idx < len(tokens) and tokens[idx].upper().startswith("AT="):
                    # set targ at value
                    targ_at_val = tokens[idx].split("=", 1)[1] if "=" in tokens[idx] else ""
                    # advance index
                    idx += 1
                # check for variable form
                elif idx < len(tokens):
                    # split variable and condition
                    targ_token = tokens[idx]
                    # check for equals sign in token
                    if "=" in targ_token:
                        # split at first equals
                        parts = targ_token.split("=", 1)
                        # set targ variable
                        targ_variable = parts[0]
                        # set targ condition
                        targ_condition = "=" + parts[1]
                    # handle simple variable
                    else:
                        # set targ variable
                        targ_variable = targ_token
                    # advance index
                    idx += 1
                    # parse TARG qualifiers
                    while idx < len(tokens):
                        # normalize token
                        token_upper = tokens[idx].upper()
                        # check for TD
                        if token_upper.startswith("TD="):
                            # set targ td
                            targ_td = tokens[idx].split("=", 1)[1] if "=" in tokens[idx] else ""
                            # advance index
                            idx += 1
                        # check for RISE
                        elif token_upper.startswith("RISE="):
                            # set targ rise
                            targ_rise = tokens[idx].split("=", 1)[1] if "=" in tokens[idx] else ""
                            # advance index
                            idx += 1
                        # check for FALL
                        elif token_upper.startswith("FALL="):
                            # set targ fall
                            targ_fall = tokens[idx].split("=", 1)[1] if "=" in tokens[idx] else ""
                            # advance index
                            idx += 1
                        # check for CROSS
                        elif token_upper.startswith("CROSS="):
                            # set targ cross
                            targ_cross = tokens[idx].split("=", 1)[1] if "=" in tokens[idx] else ""
                            # advance index
                            idx += 1
                        # handle unknown token
                        else:
                            # log warning
                            logger.warning("Ignoring unknown TARG qualifier '%s'", tokens[idx])
                            # advance index
                            idx += 1
            # set variable to empty for TRIG-TARG
            variable = ""
        # handle standard measurement types
        else:
            # check if variable is present
            if len(tokens) >= 5:
                # check for WHEN keyword (WHEN measure type has no variable)
                if measure_type == "WHEN":
                    # set variable to empty for WHEN measure type
                    variable = ""
                    # parse WHEN condition from position 4
                    when_token = tokens[4]
                    # check for equals sign in token
                    if "=" in when_token:
                        # split variable and condition
                        parts = when_token.split("=", 1)
                        # set when variable
                        when_variable = parts[0]
                        # set when condition
                        when_condition = "=" + parts[1]
                # handle normal variable
                else:
                    # set variable
                    variable = tokens[4]
            # handle ERR1/ERR2 two-variable syntax
            if measure_type in ("ERR1", "ERR2"):
                # check for second variable
                if len(tokens) >= 6:
                    # set second variable
                    variable2 = tokens[5]
            # iterate remaining tokens
            for i in range(5, len(tokens)):
                # get token
                token = tokens[i]
                # check for WHEN keyword
                if token.upper() == "WHEN" and i + 1 < len(tokens):
                    # set when variable
                    when_variable = tokens[i + 1]
                    # check for condition in same token
                    if "=" in when_variable:
                        # split variable and condition
                        parts = when_variable.split("=", 1)
                        # set when variable
                        when_variable = parts[0]
                        # set when condition without space
                        when_condition = "=" + parts[1]
                    # check if next token starts with equals (separate tokens)
                    elif i + 2 < len(tokens) and tokens[i + 2].startswith("="):
                        # set when condition from next token
                        when_condition = tokens[i + 2]
                        # advance index by 3
                        i += 2
                    # advance index by 2
                    i += 1
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
                        # set from value
                        from_val = val
                    # map TO
                    elif key_upper == "TO":
                        # set to value
                        to_val = val
                    # map TD
                    elif key_upper == "TD":
                        # validate TD support
                        if measure_type in _TD_SUPPORTED_TYPES:
                            # set td value
                            td_val = val
                        # handle unsupported TD
                        else:
                            # log warning
                            logger.warning("Ignoring TD qualifier for measure type '%s'", measure_type)
                    # map RISE
                    elif key_upper == "RISE":
                        # validate RISE support
                        if measure_type in _RFC_SUPPORTED_TYPES:
                            # set rise value
                            rise_val = val
                        # handle unsupported RISE
                        else:
                            # log warning
                            logger.warning("Ignoring RISE qualifier for measure type '%s'", measure_type)
                    # map FALL
                    elif key_upper == "FALL":
                        # validate FALL support
                        if measure_type in _RFC_SUPPORTED_TYPES:
                            # set fall value
                            fall_val = val
                        # handle unsupported FALL
                        else:
                            # log warning
                            logger.warning("Ignoring FALL qualifier for measure type '%s'", measure_type)
                    # map CROSS
                    elif key_upper == "CROSS":
                        # validate CROSS support
                        if measure_type in _RFC_SUPPORTED_TYPES:
                            # set cross value
                            cross_val = val
                        # handle unsupported CROSS
                        else:
                            # log warning
                            logger.warning("Ignoring CROSS qualifier for measure type '%s'", measure_type)
                    # map MINVAL
                    elif key_upper == "MINVAL":
                        # set minval
                        minval = val
                    # map DEFAULT_VAL
                    elif key_upper == "DEFAULT_VAL":
                        # set default val
                        default_val = val
                    # map PRECISION
                    elif key_upper == "PRECISION":
                        # set precision
                        precision = val
                    # map PRINT
                    elif key_upper == "PRINT":
                        # set print val
                        print_val = val
                    # map AT
                    elif key_upper == "AT":
                        # validate AT support
                        if measure_type in _AT_SUPPORTED_TYPES:
                            # set at value
                            at_val = val
                        # handle unsupported AT
                        else:
                            # log warning
                            logger.warning("Ignoring AT qualifier for measure type '%s'", measure_type)
                    # map ON
                    elif key_upper == "ON":
                        # validate ON support
                        if measure_type in _ON_OFF_SUPPORTED_TYPES:
                            # set on value
                            on_val = val
                        # handle unsupported ON
                        else:
                            # log warning
                            logger.warning("Ignoring ON qualifier for measure type '%s'", measure_type)
                    # map OFF
                    elif key_upper == "OFF":
                        # validate OFF support
                        if measure_type in _ON_OFF_SUPPORTED_TYPES:
                            # set off value
                            off_val = val
                        # handle unsupported OFF
                        else:
                            # log warning
                            logger.warning("Ignoring OFF qualifier for measure type '%s'", measure_type)
                    # map RFC_LEVEL
                    elif key_upper == "RFC_LEVEL":
                        # validate RFC_LEVEL support
                        if measure_type in _RFC_LEVEL_SUPPORTED_TYPES:
                            # set rfc level
                            rfc_level = val
                        # handle unsupported RFC_LEVEL
                        else:
                            # log warning
                            logger.warning("Ignoring RFC_LEVEL qualifier for measure type '%s'", measure_type)
                    # map OUTPUT
                    elif key_upper == "OUTPUT":
                        # validate OUTPUT support
                        if measure_type in _OUTPUT_SUPPORTED_TYPES:
                            # set output
                            output = val
                        # handle unsupported OUTPUT
                        else:
                            # log warning
                            logger.warning("Ignoring OUTPUT qualifier for measure type '%s'", measure_type)
                    # map FILE (ERROR-specific)
                    elif key_upper == "FILE":
                        # validate ERROR type
                        if measure_type == "ERROR":
                            # set error file
                            error_file = val
                        # handle unsupported FILE
                        else:
                            # log warning
                            logger.warning("Ignoring FILE qualifier for non-ERROR measure type '%s'", measure_type)
                    # map INDEPVARCOL (ERROR-specific)
                    elif key_upper == "INDEPVARCOL":
                        # validate ERROR type
                        if measure_type == "ERROR":
                            # set indepvarcol
                            indepvarcol = val
                        # handle unsupported INDEPVARCOL
                        else:
                            # log warning
                            logger.warning("Ignoring INDEPVARCOL qualifier for non-ERROR measure type '%s'", measure_type)
                    # map DEPVARCOL (ERROR-specific)
                    elif key_upper == "DEPVARCOL":
                        # validate ERROR type
                        if measure_type == "ERROR":
                            # set depvarcol
                            depvarcol = val
                        # handle unsupported DEPVARCOL
                        else:
                            # log warning
                            logger.warning("Ignoring DEPVARCOL qualifier for non-ERROR measure type '%s'", measure_type)
                    # map COMP_FUNCTION (ERROR-specific)
                    elif key_upper == "COMP_FUNCTION":
                        # validate ERROR type
                        if measure_type == "ERROR":
                            # set comp function
                            comp_function = val
                        # handle unsupported COMP_FUNCTION
                        else:
                            # log warning
                            logger.warning("Ignoring COMP_FUNCTION qualifier for non-ERROR measure type '%s'", measure_type)
                    # map NUMFREQ (FOUR-specific)
                    elif key_upper == "NUMFREQ":
                        # validate FOUR type
                        if measure_type == "FOUR":
                            # set numfreq
                            numfreq = val
                        # handle unsupported NUMFREQ
                        else:
                            # log warning
                            logger.warning("Ignoring NUMFREQ qualifier for non-FOUR measure type '%s'", measure_type)
                    # map GRIDSIZE (FOUR-specific)
                    elif key_upper == "GRIDSIZE":
                        # validate FOUR type
                        if measure_type == "FOUR":
                            # set gridsize
                            gridsize = val
                        # handle unsupported GRIDSIZE
                        else:
                            # log warning
                            logger.warning("Ignoring GRIDSIZE qualifier for non-FOUR measure type '%s'", measure_type)
                    # handle unknown option
                    else:
                        # log warning
                        logger.warning("Ignoring unknown .MEASURE option '%s'", key)
        # return model
        return cls(
            analysis_type=analysis_type,
            result_name=result_name,
            measure_type=measure_type,
            variable=variable,
            from_val=from_val,
            to_val=to_val,
            td_val=td_val,
            rise_val=rise_val,
            fall_val=fall_val,
            cross_val=cross_val,
            minval=minval,
            default_val=default_val,
            precision=precision,
            print_val=print_val,
            at_val=at_val,
            on_val=on_val,
            off_val=off_val,
            rfc_level=rfc_level,
            output=output,
            when_variable=when_variable,
            when_condition=when_condition,
            variable2=variable2,
            trig_variable=trig_variable,
            trig_condition=trig_condition,
            trig_td=trig_td,
            trig_rise=trig_rise,
            trig_fall=trig_fall,
            trig_cross=trig_cross,
            trig_at_val=trig_at_val,
            targ_variable=targ_variable,
            targ_condition=targ_condition,
            targ_td=targ_td,
            targ_rise=targ_rise,
            targ_fall=targ_fall,
            targ_cross=targ_cross,
            targ_at_val=targ_at_val,
            error_file=error_file,
            indepvarcol=indepvarcol,
            depvarcol=depvarcol,
            comp_function=comp_function,
            numfreq=numfreq,
            gridsize=gridsize,
        )

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
                # handle variable without condition
                else:
                    # append trig variable
                    tokens.append(self.trig_variable)
                # append trig qualifiers
                if self.trig_td:
                    # append TD with equals
                    tokens.append(f"TD={self.trig_td}")
                if self.trig_rise:
                    # append RISE with equals
                    tokens.append(f"RISE={self.trig_rise}")
                if self.trig_fall:
                    # append FALL with equals
                    tokens.append(f"FALL={self.trig_fall}")
                if self.trig_cross:
                    # append CROSS with equals
                    tokens.append(f"CROSS={self.trig_cross}")
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
                # handle variable without condition
                else:
                    # append targ variable
                    tokens.append(self.targ_variable)
                # append targ qualifiers
                if self.targ_td:
                    # append TD with equals
                    tokens.append(f"TD={self.targ_td}")
                if self.targ_rise:
                    # append RISE with equals
                    tokens.append(f"RISE={self.targ_rise}")
                if self.targ_fall:
                    # append FALL with equals
                    tokens.append(f"FALL={self.targ_fall}")
                if self.targ_cross:
                    # append CROSS with equals
                    tokens.append(f"CROSS={self.targ_cross}")
        # handle standard measurement types
        else:
            # append measure type
            tokens.append(self.measure_type)
            # append variable
            if self.variable:
                # append variable
                tokens.append(self.variable)
            # append second variable for ERR1/ERR2
            if self.variable2:
                # append second variable
                tokens.append(self.variable2)
            # append WHEN clause (except for WHEN measure type)
            if self.when_variable and self.measure_type != "WHEN":
                # append WHEN keyword
                tokens.append("WHEN")
                # check for condition
                if self.when_condition:
                    # append combined variable and condition
                    tokens.append(self.when_variable + self.when_condition)
                # handle variable without condition
                else:
                    # append when variable
                    tokens.append(self.when_variable)
            # handle WHEN measure type specifically
            elif self.measure_type == "WHEN" and self.when_variable:
                # append when variable and condition combined
                if self.when_condition:
                    # append combined variable and condition
                    tokens.append(self.when_variable + self.when_condition)
                # handle variable without condition
                else:
                    # append variable
                    tokens.append(self.when_variable)
            # append FROM
            if self.from_val:
                # append FROM
                tokens.append(f"FROM={self.from_val}")
            # append TO
            if self.to_val:
                # append TO
                tokens.append(f"TO={self.to_val}")
            # append TD
            if self.td_val:
                # append TD
                tokens.append(f"TD={self.td_val}")
            # append RISE
            if self.rise_val:
                # append RISE
                tokens.append(f"RISE={self.rise_val}")
            # append FALL
            if self.fall_val:
                # append FALL
                tokens.append(f"FALL={self.fall_val}")
            # append CROSS
            if self.cross_val:
                # append CROSS
                tokens.append(f"CROSS={self.cross_val}")
            # append MINVAL
            if self.minval:
                # append MINVAL
                tokens.append(f"MINVAL={self.minval}")
            # append DEFAULT_VAL
            if self.default_val:
                # append DEFAULT_VAL
                tokens.append(f"DEFAULT_VAL={self.default_val}")
            # append PRECISION
            if self.precision:
                # append PRECISION
                tokens.append(f"PRECISION={self.precision}")
            # append PRINT
            if self.print_val:
                # append PRINT
                tokens.append(f"PRINT={self.print_val}")
            # append AT
            if self.at_val:
                # append AT
                tokens.append(f"AT={self.at_val}")
            # append ON
            if self.on_val:
                # append ON
                tokens.append(f"ON={self.on_val}")
            # append OFF
            if self.off_val:
                # append OFF
                tokens.append(f"OFF={self.off_val}")
            # append RFC_LEVEL
            if self.rfc_level:
                # append RFC_LEVEL
                tokens.append(f"RFC_LEVEL={self.rfc_level}")
            # append OUTPUT
            if self.output:
                # append OUTPUT
                tokens.append(f"OUTPUT={self.output}")
            # append ERROR-specific qualifiers
            if self.error_file:
                # append FILE
                tokens.append(f"FILE={self.error_file}")
            if self.indepvarcol:
                # append INDEPVARCOL
                tokens.append(f"INDEPVARCOL={self.indepvarcol}")
            if self.depvarcol:
                # append DEPVARCOL
                tokens.append(f"DEPVARCOL={self.depvarcol}")
            if self.comp_function:
                # append COMP_FUNCTION
                tokens.append(f"COMP_FUNCTION={self.comp_function}")
            # append FOUR-specific qualifiers
            if self.numfreq:
                # append NUMFREQ
                tokens.append(f"NUMFREQ={self.numfreq}")
            if self.gridsize:
                # append GRIDSIZE
                tokens.append(f"GRIDSIZE={self.gridsize}")
        # return joined statement
        return " ".join(tokens)
