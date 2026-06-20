from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

from ..netlist_parser import NetlistTopology

from .print_parameters import PrintParameters


@dataclass(frozen=True)
class NodesetEntry:
    node: str
    voltage: str


@dataclass(frozen=True)
class IcEntry:
    node: str
    voltage: str


@dataclass(frozen=True)
class OpSimulationParameters:

    print_dc_enabled: bool = False
    print_dc_all_nodes: bool = False
    print_dc_all_currents: bool = False
    print_dc_specific_variables: tuple[str, ...] = ()
    print_dc_format: str = ""
    print_dc_file: str = ""
    save_enabled: bool = False
    save_type: str = "NODESET"
    save_file: str = ""
    nodeset_entries: tuple[NodesetEntry, ...] = ()
    ic_entries: tuple[IcEntry, ...] = ()
    replace_ground: bool = True
    print_parameters: PrintParameters | None = None

    @classmethod
    def from_xyce_directives(cls, directives: list[str]) -> "OpSimulationParameters" | None:
        # preprocess replaceground flag
        replace_ground = True
        # init flag
        print_dc_enabled = False
        # init list
        print_dc_vars = []
        # init parsed print parameters object
        print_parameters_parsed: PrintParameters | None = None
        # init flag
        save_enabled = False
        # init list
        nodeset_entries = []
        # init list
        ic_entries = []
        # flag indicating whether a valid directive was found
        found = False
        # parse directives
        for directive in directives:
            # get tokens
            tokens = directive.split()
            # check command
            cmd = tokens[0].upper()
            # check .OP directive
            if cmd == ".OP":
                # set flag
                found = True
                # next
                continue
            # handle print dc
            if cmd == ".PRINT" and len(tokens) > 1 and tokens[1].upper() == "DC":
                # set enabled
                print_dc_enabled = True
                # parse via PrintParameters for structured wildcard/format access
                print_parameters_parsed = PrintParameters.from_xyce_statement(directive)
                # also collect raw vars for backward-compatible list
                print_dc_vars.extend(tokens[2:])
                # next
                continue
            # handle save
            if cmd == ".SAVE":
                # set enabled
                save_enabled = True
                # next
                continue
            # handle nodeset
            if cmd == ".NODESET":
                # process pairs
                for pair in tokens[1:]:
                    # check if pair valid
                    if "=" in pair:
                        # split node and voltage
                        node_part, voltage = pair.split("=", 1)
                        # validate
                        if node_part.startswith("V(") and node_part.endswith(")"):
                            # append entry
                            nodeset_entries.append(NodesetEntry(node=node_part[2:-1], voltage=voltage))
                # next
                continue
            # handle initial conditions (.IC and .DCVOLT use the same format)
            if cmd in (".IC", ".DCVOLT"):
                # iterate tokens looking for V(node)=val or node val pairs
                remaining = tokens[1:]
                i = 0
                while i < len(remaining):
                    token = remaining[i]
                    if "=" in token:
                        # V(node)=val form
                        lhs, voltage = token.split("=", 1)
                        if lhs.startswith("V(") and lhs.endswith(")"):
                            node = lhs[2:-1]
                        else:
                            node = lhs
                        ic_entries.append(IcEntry(node=node, voltage=voltage))
                        i += 1
                    elif i + 1 < len(remaining):
                        # node val pair form
                        ic_entries.append(IcEntry(node=token, voltage=remaining[i + 1]))
                        i += 2
                    else:
                        i += 1
                # next
                continue
            # preprocess replaceground
            if cmd == ".PREPROCESS" and len(tokens) > 2 and tokens[1].upper() == "REPLACEGROUND":
                # set flag based on value
                replace_ground = tokens[2].upper() == "TRUE"
        # return instance if a valid directive was found
        return cls(print_dc_enabled=print_dc_enabled, print_dc_specific_variables=tuple(print_dc_vars), save_enabled=save_enabled, nodeset_entries=tuple(nodeset_entries), ic_entries=tuple(ic_entries), replace_ground=replace_ground, print_parameters=print_parameters_parsed) if found else None

    def to_xyce_directives(self, topology: NetlistTopology | None = None) -> list[str]:
        # start lines
        lines = [".OP"]
        # use print_parameters when set (newer wildcard approach)
        if self.print_parameters is not None:
            # emit the statement directly using the structured print parameters
            lines.append(self.print_parameters.to_xyce_statement())
        # check enabled
        elif self.print_dc_enabled:
            # start tokens
            tokens = [".PRINT DC"]
            # check format
            if self.print_dc_format:
                # append format
                tokens.append(f"FORMAT={self.print_dc_format}")
            # check file
            if self.print_dc_file:
                # append file
                tokens.append(f"FILE={self.print_dc_file}")
            # get custom vars
            vars = list(self.print_dc_specific_variables)
            # check topology
            if topology:
                # check all nodes
                if self.print_dc_all_nodes:
                    # iterate nodes
                    for node in topology.nodes:
                        # append node
                        vars.append(f"V({node})")
                # check all currents
                if self.print_dc_all_currents:
                    # iterate devices
                    for dev in topology.devices:
                        # append current
                        vars.append(f"I({dev.name})")
            # add unique
            tokens.extend(dict.fromkeys(vars))
            # join tokens
            lines.append(" ".join(tokens))
        # check save enabled
        if self.save_enabled:
            # build tokens
            tokens = [".SAVE"]
            # append type
            tokens.append(f"TYPE={self.save_type}")
            # check file
            if self.save_file:
                # append file
                tokens.append(f"FILE={self.save_file}")
            # join tokens
            lines.append(" ".join(tokens))
        # check nodeset entries
        if self.nodeset_entries:
            # format pairs
            pairs = " ".join(f"V({e.node})={e.voltage}" for e in self.nodeset_entries)
            # append directive
            lines.append(f".NODESET {pairs}")
        # check initial condition entries
        if self.ic_entries:
            # format pairs
            pairs = " ".join(f"V({e.node})={e.voltage}" for e in self.ic_entries)
            # append directive
            lines.append(f".IC {pairs}")
        # return directives
        return ([".PREPROCESS REPLACEGROUND TRUE"] if self.replace_ground else []) + lines

    def raw_output_file_path(self, working_directory: Path, netlist_file_path: Path) -> Path | None:
        # check raw format is selected in print parameters and a print file is specified
        if self.print_parameters is None or self.print_parameters.print_format != "RAW":
            return None
        # return the output file path specified in the print parameters if available
        if self.print_parameters.print_file:
            return working_directory / self.print_parameters.print_file
        # otherwise, create raw file from netlist file, adding the ".raw" suffix
        return netlist_file_path.with_suffix(netlist_file_path.suffix + ".raw")
