from __future__ import annotations

import logging
from dataclasses import dataclass


# init module logger
logger = logging.getLogger(__name__)

# init allowed window values from the reference guide
_ALLOWED_WINDOW_VALUES = {
    "RECT",
    "RECTANGULAR",
    "BART",
    "BARTLETT",
    "BARTLETTHANN",
    "BLACK",
    "BLACKMAN",
    "HAMM",
    "HAMMING",
    "HANN",
    "HANNING",
    "HARRIS",
    "BLACKMANHARRIS",
    "NUTTALL",
    "COSINE2",
    "COSINE4",
    "HALFCYCLESINE",
    "HALFCYCLESINE3",
    "HALFCYCLESINE6",
    "TRIANGULAR",
}

# init allowed format values from the reference guide
_ALLOWED_FORMAT_VALUES = {
    "NORM",
    "UNORM",
}


def _tokenize_fft_statement(fft_statement: str) -> list[str]:
    # init token list
    tokens: list[str] = []
    # init current token buffer
    current_chars: list[str] = []
    # init brace nesting depth
    brace_depth = 0
    # iterate characters
    for char in fft_statement:
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
class FftParameters:

    output_variable: str
    np: str = ""
    window: str = ""
    alfa: str = ""
    fft_format: str = ""
    start: str = ""
    stop: str = ""
    freq: str = ""
    fmin: str = ""
    fmax: str = ""

    @classmethod
    def from_xyce_statement(cls, fft_statement: str) -> "FftParameters" | None:
        # parse tokens
        tokens = _tokenize_fft_statement(fft_statement)
        # reject non-fft statements
        if len(tokens) < 2 or tokens[0].upper() != ".FFT":
            # return none
            return None
        # required positional output variable
        output_variable = tokens[1]
        # init options
        np = ""
        # init window
        window = ""
        # init alfa
        alfa = ""
        # init format
        fft_format = ""
        # init start
        start = ""
        # init stop
        stop = ""
        # init freq
        freq = ""
        # init fmin
        fmin = ""
        # init fmax
        fmax = ""
        # iterate remaining tokens
        for token in tokens[2:]:
            # check for equals sign
            if "=" in token:
                # split key and value
                key, val = token.split("=", 1)
                # normalize key
                key = key.upper()
                # map np
                if key == "NP":
                    # set np
                    np = val
                # map window
                elif key == "WINDOW":
                    # normalize window
                    norm_window = val.upper()
                    # validate window
                    if norm_window in _ALLOWED_WINDOW_VALUES:
                        # set window
                        window = norm_window
                    # handle invalid window
                    else:
                        # log warning
                        logger.warning("Ignoring invalid .FFT WINDOW value '%s'", val)
                # map alfa
                elif key == "ALFA":
                    # set alfa
                    alfa = val
                # map format
                elif key == "FORMAT":
                    # normalize format
                    norm_format = val.upper()
                    # validate format
                    if norm_format in _ALLOWED_FORMAT_VALUES:
                        # set format
                        fft_format = norm_format
                    # handle invalid format
                    else:
                        # log warning
                        logger.warning("Ignoring invalid .FFT FORMAT value '%s'", val)
                # map start
                elif key in ("START", "FROM"):
                    # set start
                    start = val
                # map stop
                elif key in ("STOP", "TO"):
                    # set stop
                    stop = val
                # map freq
                elif key == "FREQ":
                    # set freq
                    freq = val
                # map fmin
                elif key == "FMIN":
                    # set fmin
                    fmin = val
                # map fmax
                elif key == "FMAX":
                    # set fmax
                    fmax = val
                # handle unknown option
                else:
                    # log warning
                    logger.warning("Ignoring unknown .FFT option '%s'", key)
            # handle unexpected token
            else:
                # log warning
                logger.warning("Ignoring unexpected .FFT token '%s'", token)
        # return model
        return cls(output_variable=output_variable, np=np, window=window, alfa=alfa, fft_format=fft_format, start=start, stop=stop, freq=freq, fmin=fmin, fmax=fmax)

    def to_xyce_statement(self) -> str:
        # init tokens
        tokens = [".FFT", self.output_variable]
        # append np
        if self.np:
            # append np token
            tokens.append(f"NP={self.np}")
        # append window
        if self.window:
            # append window token
            tokens.append(f"WINDOW={self.window}")
        # append alfa
        if self.alfa:
            # append alfa token
            tokens.append(f"ALFA={self.alfa}")
        # append format
        if self.fft_format:
            # append format token
            tokens.append(f"FORMAT={self.fft_format}")
        # append start
        if self.start:
            # append start token
            tokens.append(f"START={self.start}")
        # append stop
        if self.stop:
            # append stop token
            tokens.append(f"STOP={self.stop}")
        # append freq
        if self.freq:
            # append freq token
            tokens.append(f"FREQ={self.freq}")
        # append fmin
        if self.fmin:
            # append fmin token
            tokens.append(f"FMIN={self.fmin}")
        # append fmax
        if self.fmax:
            # append fmax token
            tokens.append(f"FMAX={self.fmax}")
        # return joined statement
        return " ".join(tokens)
