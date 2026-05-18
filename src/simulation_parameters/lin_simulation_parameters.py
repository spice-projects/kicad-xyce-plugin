from __future__ import annotations

from dataclasses import dataclass

from netlist_parser import NetlistTopology


@dataclass(frozen=True)
class LinSimulationParameters:
    """S/Y/Z-parameter (LIN) analysis.

    .LIN drives its own internal sweep via an embedded .AC directive.
    Both .AC and .LIN are emitted from to_xyce_directives so that the class
    is fully self-contained.  Because of this, LinSimulationParameters MUST
    be registered before AcSimulationParameters in ALL_SIMULATION_PARAMETERS_TYPES
    so the dispatch logic resolves .LIN netlists to this class and not to
    AcSimulationParameters.
    """

    # .LIN keyword arguments
    sparcalc: bool = True
    format: str = "TOUCHSTONE2"
    lintype: str = "S"
    dataformat: str = "RI"
    file: str = ""
    width: str = ""
    precision: str = ""
    # embedded AC sweep fields
    sweep_mode: str = "LIN"
    points: str = ""
    start: str = ""
    end: str = ""
    data_table_name: str = ""
    replace_ground: bool = True

    # --- helpers ---

    @staticmethod
    def _parse_ac(tokens: list[str], current: dict) -> None:  # type: ignore[type-arg]
        """Parse .AC directive tokens into the *current* dict in-place."""
        if len(tokens) < 2:
            return
        second = tokens[1].upper()
        if second.startswith("DATA="):
            current["sweep_mode"] = "DATA"
            current["data_table_name"] = second.split("=", 1)[1]
            return
        if second in ("DEC", "OCT"):
            current["sweep_mode"] = second
            if len(tokens) >= 5:
                current["points"] = tokens[2]
                current["start"] = tokens[3]
                current["end"] = tokens[4]
            return
        current["sweep_mode"] = "LIN"
        if second == "LIN":
            if len(tokens) >= 5:
                current["points"] = tokens[2]
                current["start"] = tokens[3]
                current["end"] = tokens[4]
        else:
            if len(tokens) >= 4:
                current["points"] = tokens[1]
                current["start"] = tokens[2]
                current["end"] = tokens[3]

    @staticmethod
    def _parse_lin(tokens: list[str], current: dict) -> None:  # type: ignore[type-arg]
        """Parse .LIN keyword=value pairs into the *current* dict in-place."""
        for token in tokens[1:]:
            upper = token.upper()
            if "=" not in upper:
                continue
            key, _, val = token.partition("=")
            key_upper = key.upper()
            if key_upper == "SPARCALC":
                current["sparcalc"] = val.upper() in ("1", "TRUE", "YES")
            elif key_upper == "FORMAT":
                current["format"] = val.upper()
            elif key_upper == "TYPE":
                current["lintype"] = val.upper()
            elif key_upper == "DATAFORMAT":
                current["dataformat"] = val.upper()
            elif key_upper == "FILE":
                current["file"] = val
            elif key_upper == "WIDTH":
                current["width"] = val
            elif key_upper == "PRECISION":
                current["precision"] = val

    # --- class methods ---

    @classmethod
    def from_xyce_directives(cls, directives: list[str]) -> "LinSimulationParameters" | None:
        # init defaults
        current: dict = {  # type: ignore[type-arg]
            "sparcalc": True,
            "format": "TOUCHSTONE2",
            "lintype": "S",
            "dataformat": "RI",
            "file": "",
            "width": "",
            "precision": "",
            "sweep_mode": "LIN",
            "points": "",
            "start": "",
            "end": "",
            "data_table_name": "",
            "replace_ground": False,
        }
        found = False
        for directive in directives:
            tokens = directive.split()
            if not tokens:
                continue
            cmd = tokens[0].upper()
            if cmd == ".PREPROCESS" and len(tokens) > 2 and tokens[1].upper() == "REPLACEGROUND":
                current["replace_ground"] = tokens[2].upper() == "TRUE"
                continue
            if cmd == ".AC":
                cls._parse_ac(tokens, current)
                continue
            if cmd == ".LIN":
                found = True
                cls._parse_lin(tokens, current)
                continue
        return cls(**current) if found else None

    def to_xyce_directives(self, topology: NetlistTopology | None = None) -> list[str]:
        directives: list[str] = []
        # prepend replaceground preprocessing when enabled
        if self.replace_ground:
            directives.append(".PREPROCESS REPLACEGROUND TRUE")
        # build the embedded AC sweep directive
        if self.sweep_mode == "DATA":
            directives.append(f".AC DATA={self.data_table_name}")
        elif self.sweep_mode in ("DEC", "OCT"):
            directives.append(f".AC {self.sweep_mode} {self.points} {self.start} {self.end}")
        else:
            directives.append(f".AC LIN {self.points} {self.start} {self.end}")
        # build the .LIN directive with optional keyword arguments
        lin_parts = [".LIN"]
        if not self.sparcalc:
            lin_parts.append("SPARCALC=0")
        if self.format != "TOUCHSTONE2":
            lin_parts.append(f"FORMAT={self.format}")
        if self.lintype != "S":
            lin_parts.append(f"TYPE={self.lintype}")
        if self.dataformat != "RI":
            lin_parts.append(f"DATAFORMAT={self.dataformat}")
        if self.file:
            lin_parts.append(f"FILE={self.file}")
        if self.width:
            lin_parts.append(f"WIDTH={self.width}")
        if self.precision:
            lin_parts.append(f"PRECISION={self.precision}")
        directives.append(" ".join(lin_parts))
        return directives
