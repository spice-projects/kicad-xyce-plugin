from __future__ import annotations

import re
from dataclasses import dataclass

from netlist_parser import NetlistTopology


def _parse_output_node(token: str) -> tuple[str, str]:
    """Parse V(out) or V(out,ref) into (output_node, ref_node)."""
    # match V(node) or V(node,ref)
    m = re.fullmatch(r"[Vv]\(([^,)]+)(?:,([^)]+))?\)", token)
    if m:
        return m.group(1).strip(), (m.group(2) or "").strip()
    # fallback — return the token itself as the output node
    return token, ""


@dataclass(frozen=True)
class NoiseSimulationParameters:

    output_node: str
    ref_node: str = ""
    source_name: str = ""
    sweep_mode: str = "LIN"
    points: str = ""
    start: str = ""
    end: str = ""
    data_table_name: str = ""
    replace_ground: bool = True

    @classmethod
    def from_xyce_directives(cls, directives: list[str]) -> "NoiseSimulationParameters" | None:
        # init defaults
        output_node = ""
        ref_node = ""
        source_name = ""
        sweep_mode = "LIN"
        points = ""
        start = ""
        end = ""
        data_table_name = ""
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
            # skip non-NOISE directives
            if cmd != ".NOISE":
                continue
            # flag indicating a valid NOISE directive was found
            found = True
            if len(tokens) < 3:
                continue
            # parse output node: tokens[1] is V(out) or V(out,ref)
            output_node, ref_node = _parse_output_node(tokens[1])
            # parse source name: tokens[2]
            source_name = tokens[2]
            if len(tokens) < 4:
                continue
            sweep_token = tokens[3].upper()
            # handle DATA sweep: .NOISE V(...) SRC DATA=<tablename>
            if sweep_token.startswith("DATA="):
                sweep_mode = "DATA"
                data_table_name = sweep_token.split("=", 1)[1]
                continue
            # detect log sweep type
            if sweep_token in ("DEC", "OCT"):
                sweep_mode = sweep_token
                if len(tokens) >= 7:
                    points = tokens[4]
                    start = tokens[5]
                    end = tokens[6]
                continue
            # linear sweep
            sweep_mode = "LIN"
            if sweep_token == "LIN":
                if len(tokens) >= 7:
                    points = tokens[4]
                    start = tokens[5]
                    end = tokens[6]
            else:
                # implicit LIN (no keyword)
                if len(tokens) >= 6:
                    points = tokens[3]
                    start = tokens[4]
                    end = tokens[5]
        # return instance if a valid directive was found
        return cls(output_node=output_node, ref_node=ref_node, source_name=source_name, sweep_mode=sweep_mode, points=points, start=start, end=end, data_table_name=data_table_name, replace_ground=replace_ground) if found else None

    def to_xyce_directives(self, topology: NetlistTopology | None = None) -> list[str]:
        # prepend replaceground preprocessing when enabled
        preprocess = [".PREPROCESS REPLACEGROUND TRUE"] if self.replace_ground else []
        # build the V(out[,ref]) token
        if self.ref_node:
            out_token = f"V({self.output_node},{self.ref_node})"
        else:
            out_token = f"V({self.output_node})"
        # build the sweep portion
        if self.sweep_mode == "DATA":
            sweep = f"DATA={self.data_table_name}"
        elif self.sweep_mode in ("DEC", "OCT"):
            sweep = f"{self.sweep_mode} {self.points} {self.start} {self.end}"
        else:
            sweep = f"LIN {self.points} {self.start} {self.end}"
        return preprocess + [f".NOISE {out_token} {self.source_name} {sweep}"]
