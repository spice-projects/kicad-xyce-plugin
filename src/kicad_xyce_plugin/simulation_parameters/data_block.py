from __future__ import annotations

import re
from dataclasses import dataclass


@dataclass(frozen=True)
class DataBlock:

    name: str
    parameters: tuple[str, ...]
    records: tuple[tuple[str, ...], ...]

    @classmethod
    def from_xyce_directives(cls, directives: list[str]) -> tuple["DataBlock", ...]:
        # init results list
        results: list[DataBlock] = []
        # parse active data block variables
        active_name = ""
        # active parameter names list
        active_params: list[str] = []
        # active data values list
        active_values: list[str] = []
        # iterate all directives to parse data blocks
        for directive in directives:
            # tokenize the directive
            tokens = directive.split()
            # skip empty directives
            if not tokens:
                # continue loop
                continue
            # get upper command
            cmd = tokens[0].upper()
            # check for .DATA start
            if cmd == ".DATA":
                # extract table name
                active_name = tokens[1] if len(tokens) > 1 else ""
                # process the rest of the tokens as potential headers and values
                active_params = []
                # init active values
                active_values = []
                # iterate over subsequent tokens to separate parameters from values
                for token in tokens[2:]:
                    # check if token is a number
                    if cls._is_number(token):
                        # add to values
                        active_values.append(token)
                    # else case
                    else:
                        # add to parameters
                        active_params.append(token)
            # handle spice continuation lines (+) inside an active .DATA block
            elif cmd == "+" and active_name:
                # classify each continuation token as a parameter name or value
                for token in tokens[1:]:
                    # check if token is a number
                    if cls._is_number(token):
                        # add to values
                        active_values.append(token)
                    # else case
                    else:
                        # add to parameters
                        active_params.append(token)
            # check for .ENDDATA
            elif cmd == ".ENDDATA":
                # if there is an active table, group the values into rows
                if active_name:
                    # number of columns
                    num_cols = len(active_params)
                    # list of records
                    records: list[tuple[str, ...]] = []
                    # group values if there are columns
                    if num_cols > 0:
                        # group values by columns count
                        for i in range(0, len(active_values), num_cols):
                            # append record row
                            records.append(tuple(active_values[i:i + num_cols]))
                    # append the data block to results
                    results.append(cls(name=active_name, parameters=tuple(active_params), records=tuple(records)))
                    # reset active block
                    active_name = ""
        # return tuple of data blocks
        return tuple(results)

    def to_xyce_directives(self) -> list[str]:
        # init directive lines
        lines: list[str] = []
        # append the start directive
        lines.append(f".DATA {self.name}")
        # build parameters line
        param_str = " ".join(self.parameters)
        # append parameters line with continuation char
        lines.append(f"+ {param_str}")
        # iterate all records to output them
        for record in self.records:
            # build record line
            record_str = " ".join(record)
            # append record line with continuation char
            lines.append(f"+ {record_str}")
        # append the end directive
        lines.append(".ENDDATA")
        # return the directive list
        return lines

    @staticmethod
    def _is_number(s: str) -> bool:
        # normalize to lowercase
        normalized = s.lower()
        # try standard float conversion
        try:
            # check if it is float
            float(normalized)
            # return true
            return True
        # catch conversion error
        except ValueError:
            # pass to check suffix
            pass
        # define spice scale factor suffixes
        suffixes = ("meg", "mil", "f", "p", "n", "u", "m", "k", "g", "t")
        # iterate suffixes to see if string ends with any
        for suffix in suffixes:
            # check suffix match
            if normalized.endswith(suffix):
                # try
                try:
                    # extract the value portion without the suffix
                    val = normalized[:-len(suffix)]
                    # check if the value portion is a float
                    float(val)
                    # return true
                    return True
                # catch conversion error
                except ValueError:
                    # pass to next suffix
                    pass
        # return false
        return False
