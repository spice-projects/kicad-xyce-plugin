from __future__ import annotations

import logging

from dataclasses import dataclass

logger = logging.getLogger(__name__)

# init allowed generic print option keys
_COMMON_OPTION_KEYS = {
    "FORMAT",
    "FILE",
    "WIDTH",
    "PRECISION",
    "FILTER",
    "DELIMITER",
    "TIMESCALEFACTOR",
}

# init allowed sampling-specific print option keys
_SAMPLE_OPTION_KEYS = {
    "OUTPUT_SAMPLE_STATS",
    "OUTPUT_ALL_SAMPLES",
}

# init known print types with sample-specific options
_SAMPLE_PRINT_TYPES = {"ES", "PCE", "SAMPLING", "TRANADJOINT"}

# init allowed format values from the reference guide
_ALLOWED_FORMAT_VALUES = {
    "STD",
    "NOINDEX",
    "PROBE",
    "TECPLOT",
    "RAW",
    "CSV",
    "GNUPLOT",
    "SPLOT",
}


def _tokenize_print_statement(print_statement: str) -> list[str]:
    # init token list
    tokens: list[str] = []
    # init current token buffer
    current_chars: list[str] = []
    # init brace nesting depth
    brace_depth = 0
    # init quote state
    quote_char = ""
    # iterate characters
    for char in print_statement:
        # check active quote
        if quote_char:
            # append char
            current_chars.append(char)
            # check quote close
            if char == quote_char:
                # reset quote state
                quote_char = ""
            # next
            continue
        # check quote open
        if char in ('"', "'"):
            # append char
            current_chars.append(char)
            # set quote char
            quote_char = char
            # next
            continue
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


def _split_option_token(token: str) -> tuple[str, str] | None:
    # check equals sign exists
    if "=" not in token:
        # return none
        return None
    # split key and value
    option_key, option_value = token.split("=", 1)
    # strip key text
    stripped_key = option_key.strip()
    # validate key exists
    if not stripped_key:
        # return none
        return None
    # validate key starts with alpha or underscore
    if not (stripped_key[0].isalpha() or stripped_key[0] == "_"):
        # return none
        return None
    # validate remaining key chars
    for char in stripped_key[1:]:
        # check valid key char
        if not (char.isalnum() or char == "_"):
            # return none
            return None
    # return normalized key and raw value
    return stripped_key.upper(), option_value


def _is_supported_option_for_print_type(print_type: str, option_key: str) -> bool:
    # check common options
    if option_key in _COMMON_OPTION_KEYS:
        # supported
        return True
    # check sample-only options
    if option_key in _SAMPLE_OPTION_KEYS and print_type in _SAMPLE_PRINT_TYPES:
        # supported
        return True
    # unsupported
    return False


@dataclass(frozen=True)
class PrintParameters:
    print_type: str
    print_format: str = ""
    print_file: str = ""
    output_variables: tuple[str, ...] = ()
    extra_options: tuple[str, ...] = ()

    @classmethod
    def from_xyce_statement(cls, print_statement: str) -> "PrintParameters" | None:
        # parse tokens
        tokens = _tokenize_print_statement(print_statement)
        # reject non-print statements
        if len(tokens) < 2 or tokens[0].upper() != ".PRINT":
            # return none
            return None
        # parse print type
        print_type = tokens[1].upper()
        # init format value
        print_format = ""
        # init file value
        print_file = ""
        # init extra options
        extra_options: list[str] = []
        # init output variable list
        output_variables: list[str] = []
        # init section flag
        in_output_variables = False
        # iterate remaining tokens
        for token in tokens[2:]:
            # parse option token when still in option section
            option_pair = _split_option_token(token) if not in_output_variables else None
            # check option token
            if option_pair is not None:
                # unpack option pair
                option_key, option_value = option_pair
                # validate option key against print type
                if not _is_supported_option_for_print_type(print_type, option_key):
                    # log and ignore unsupported option
                    logger.warning("Ignoring unsupported .PRINT option '%s' for print type '%s'", option_key, print_type)
                    # next
                    continue
                # map format option
                if option_key == "FORMAT":
                    # normalize format candidate
                    normalized_format = option_value.upper()
                    # validate format value
                    if normalized_format not in _ALLOWED_FORMAT_VALUES:
                        # log and ignore invalid format
                        logger.warning("Ignoring invalid FORMAT value '%s' for print type '%s'", option_value, print_type)
                        # next
                        continue
                    # store format
                    print_format = normalized_format
                    # next
                    continue
                # map file option
                if option_key == "FILE":
                    # store file
                    print_file = option_value
                    # next
                    continue
                # append generic option token
                extra_options.append(token)
                # next
                continue
            # mark output-variable section
            in_output_variables = True
            # normalize W(...) to P(...) — W is the PSpice alias for P and round-trips as P
            if token.upper().startswith("W("):
                token = "P(" + token[2:]
            # append output variable
            output_variables.append(token)
        # return model
        return cls(print_type=print_type, print_format=print_format, print_file=print_file, output_variables=tuple(output_variables), extra_options=tuple(extra_options))

    def to_xyce_statement(self) -> str:
        # init token list
        tokens = [".PRINT", self.print_type]
        # append format option
        if self.print_format:
            tokens.append(f"FORMAT={self.print_format}")
        # append file option
        if self.print_file:
            tokens.append(f"FILE={self.print_file}")
        # append extra options
        tokens.extend(self.extra_options)
        # append output variables
        tokens.extend(self.output_variables)
        # return joined statement
        return " ".join(tokens)
