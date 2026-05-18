from __future__ import annotations

from dataclasses import dataclass

from netlist_parser import NetlistTopology


@dataclass(frozen=True)
class HbSimulationParameters:

    frequencies: tuple[str, ...]
    replace_ground: bool = True

    @classmethod
    def from_xyce_directives(cls, directives: list[str]) -> "HbSimulationParameters" | None:
        # init defaults
        frequencies: list[str] = []
        replace_ground = True
        # flag indicating whether a valid directive was found
        found = False
        # parse directives
        for directive in directives:
            # tokenize the directive
            tokens = directive.split()
            # skip empty directives
            if not tokens:
                continue
            cmd = tokens[0].upper()
            # handle preprocess replaceground
            if cmd == ".PREPROCESS" and len(tokens) > 2 and tokens[1].upper() == "REPLACEGROUND":
                # set replace_ground based on the third token
                replace_ground = tokens[2].upper() == "TRUE"
                # next
                continue
            # skip non-HB directives
            if cmd != ".HB":
                continue
            # flag indicating a valid HB directive was found
            found = True
            # collect all fundamental frequencies from remaining tokens
            frequencies = tokens[1:]
        # return instance if a valid directive was found
        return cls(frequencies=tuple(frequencies), replace_ground=replace_ground) if found else None

    def to_xyce_directives(self, topology: NetlistTopology | None = None) -> list[str]:
        # prepend replaceground preprocessing when enabled
        preprocess = [".PREPROCESS REPLACEGROUND TRUE"] if self.replace_ground else []
        # build the HB directive with space-separated fundamental frequencies
        return preprocess + [".HB " + " ".join(self.frequencies)]
