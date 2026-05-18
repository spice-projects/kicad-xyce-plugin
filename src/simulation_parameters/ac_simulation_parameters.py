from __future__ import annotations

from dataclasses import dataclass

from netlist_parser import NetlistTopology


@dataclass(frozen=True)
class AcSimulationParameters:

    sweep_mode: str
    points: str = ""
    start: str = ""
    end: str = ""
    data_table_name: str = ""
    replace_ground: bool = True

    @classmethod
    def from_xyce_directives(cls, directives: list[str]) -> "AcSimulationParameters" | None:
        # init defaults
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
            # skip non-AC directives
            if cmd != ".AC":
                continue
            # flag indicating a valid AC directive was found
            found = True
            if len(tokens) < 2:
                continue
            second = tokens[1].upper()
            # handle DATA sweep: .AC DATA=<tablename>
            if second.startswith("DATA="):
                # set sweep mode and data table name
                sweep_mode = "DATA"
                data_table_name = second.split("=", 1)[1]
                # next
                continue
            # detect decade or octave log sweep: .AC DEC|OCT <points> <start> <end>
            if second in ("DEC", "OCT"):
                sweep_mode = second
                if len(tokens) >= 5:
                    points = tokens[2]
                    start = tokens[3]
                    end = tokens[4]
                # next
                continue
            # linear sweep: .AC [LIN] <points> <start> <end>
            sweep_mode = "LIN"
            if second == "LIN":
                # explicit LIN keyword
                if len(tokens) >= 5:
                    points = tokens[2]
                    start = tokens[3]
                    end = tokens[4]
            else:
                # implicit LIN
                if len(tokens) >= 4:
                    points = tokens[1]
                    start = tokens[2]
                    end = tokens[3]
        # return instance if a valid directive was found
        return cls(sweep_mode=sweep_mode, points=points, start=start, end=end, data_table_name=data_table_name, replace_ground=replace_ground) if found else None

    def to_xyce_directives(self, topology: NetlistTopology | None = None) -> list[str]:
        # prepend replaceground preprocessing when enabled
        preprocess = [".PREPROCESS REPLACEGROUND TRUE"] if self.replace_ground else []
        # build the core AC directive
        if self.sweep_mode == "DATA":
            return preprocess + [f".AC DATA={self.data_table_name}"]
        if self.sweep_mode in ("DEC", "OCT"):
            return preprocess + [f".AC {self.sweep_mode} {self.points} {self.start} {self.end}"]
        # LIN sweep (explicit)
        return preprocess + [f".AC LIN {self.points} {self.start} {self.end}"]
