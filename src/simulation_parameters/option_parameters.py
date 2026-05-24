from __future__ import annotations

from dataclasses import dataclass
from dataclasses import field

from netlist_parser import NetlistTopology


def _parse_option_tokens(tokens: list[str]) -> dict[str, str]:
    # parse a series of option tokens into a normalized dictionary
    options: dict[str, str] = {}
    for token in tokens:
        # skip empty tokens produced by extra whitespace
        if not token:
            continue
        # split key/value pairs and normalize keys to uppercase
        if "=" in token:
            key, value = token.split("=", 1)
            options[key.upper()] = value
            continue
        # support flag-style options without an explicit value
        options[token.upper()] = ""
    return options


@dataclass(frozen=True)
class OptionParameters:
    # top-level device options applied across simulations
    device: dict[str, str] = field(default_factory=dict)
    # transient integration control parameters
    timeint: dict[str, str] = field(default_factory=dict)
    # generic nonlinear solver parameters
    nonlin: dict[str, str] = field(default_factory=dict)
    # generic linear solver parameters
    linsol: dict[str, str] = field(default_factory=dict)

    @classmethod
    def from_xyce_directives(cls, directives: list[str]) -> "OptionParameters":
        # init option groups
        device: dict[str, str] = {}
        timeint: dict[str, str] = {}
        nonlin: dict[str, str] = {}
        linsol: dict[str, str] = {}
        # parse each directive looking for supported option packages
        for directive in directives:
            # break directive into tokens
            tokens = directive.split()
            # skip empty directives
            if not tokens:
                continue
            # handle only .OPTIONS directives
            if tokens[0].upper() != ".OPTIONS" or len(tokens) <= 1:
                continue
            # normalize the package name
            package = tokens[1].upper()
            if package == "DEVICE":
                device = _parse_option_tokens(tokens[2:])
                continue
            if package == "TIMEINT":
                timeint = _parse_option_tokens(tokens[2:])
                continue
            if package == "NONLIN":
                nonlin = _parse_option_tokens(tokens[2:])
                continue
            if package == "LINSOL":
                linsol = _parse_option_tokens(tokens[2:])
                continue
        return cls(device=device, timeint=timeint, nonlin=nonlin, linsol=linsol)

    def to_xyce_directives(self, topology: NetlistTopology | None = None) -> list[str]:
        # serialize configured option blocks in a deterministic order
        directives: list[str] = []
        if self.device:
            directives.append(".OPTIONS DEVICE " + " ".join(f"{k}={v}" if v else k for k, v in self.device.items()))
        if self.timeint:
            directives.append(".OPTIONS TIMEINT " + " ".join(f"{k}={v}" if v else k for k, v in self.timeint.items()))
        if self.nonlin:
            directives.append(".OPTIONS NONLIN " + " ".join(f"{k}={v}" if v else k for k, v in self.nonlin.items()))
        if self.linsol:
            directives.append(".OPTIONS LINSOL " + " ".join(f"{k}={v}" if v else k for k, v in self.linsol.items()))
        return directives
