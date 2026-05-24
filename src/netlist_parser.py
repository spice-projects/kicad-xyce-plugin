from dataclasses import dataclass
from dataclasses import field


_Y_PREFIXES = ("YMEMRISTOR", "YPDE", "YACC", "YLIN")


_NODE_COUNTS: dict[str, int] = {
    "B": 2,
    "C": 2,
    "D": 2,
    "E": 4,
    "F": 2,
    "G": 4,
    "H": 2,
    "I": 2,
    "J": 3,
    "K": 0,
    "L": 2,
    "M": 4,
    "O": 4,
    "P": 2,
    "Q": 3,
    "R": 2,
    "S": 4,
    "T": 4,
    "U": 2,
    "V": 2,
    "W": 2,
    "Z": 3,
}


_Y_NODE_COUNTS: dict[str, int] = {
    "YMEMRISTOR": 2,
    "YLIN": 2,
    "YACC": 3,
    "YPDE": 0,
}

_SIMULATION_DIRECTIVES = (
    # bias point analysis and its output/setup companions
    ".OP", ".PRINT", ".SAVE", ".NODESET",
    # DC sweep analysis
    ".DC",
    # transient analysis and its post-processing companions
    ".TRAN", ".FFT", ".FOUR",
    # AC frequency-domain analysis and its S/Y/Z-parameter output spec
    ".AC", ".LIN",
    # harmonic balance analysis
    ".HB",
    # noise analysis
    ".NOISE",
    # measure output and sensitivity output
    ".MEASURE", ".MEAS", ".SENS",
    # initial condition / bias point setup
    ".IC", ".DCVOLT",
)


@dataclass
class Device:
    # device name
    name: str
    # device type letter
    type_letter: str
    # device nodes
    nodes: list[str]


@dataclass
class SubcircuitDefinition:
    # subcircuit name
    name: str
    # subcircuit ports
    ports: list[str]
    # list of devices
    devices: list[Device] = field(default_factory=list)


@dataclass
class NetlistTopology:
    # netlist title
    title: str
    # top-level devices
    devices: list[Device]
    # top-level nodes
    nodes: set[str]
    # subcircuit definitions
    subcircuit_definitions: dict[str, SubcircuitDefinition]
    # global nodes
    global_nodes: set[str]
    # simulation directives
    directives: list[str] = field(default_factory=list)


def _join_continuation_lines(raw_lines: list[str]) -> list[str]:
    # build the result list of joined logical lines
    joined: list[str] = []
    # process each physical line
    for line in raw_lines:
        # check for continuation character
        if line.lstrip().startswith("+"):
            # check for preceding lines
            if joined:
                # append to the previous line
                joined[-1] = joined[-1] + " " + line.lstrip()[1:].strip()
        # else case
        else:
            # append new line
            joined.append(line)
    # return the joined lines
    return joined


def _strip_inline_comment(line: str) -> str:
    # find comment position
    idx = line.find(";")
    # return stripped string
    if idx >= 0:
        # return part before semicolon
        return line[:idx]
    # return original line
    return line


def _get_type_letter(upper_name: str) -> str:
    # iterate prefixes
    for prefix in _Y_PREFIXES:
        # check for prefix
        if upper_name.startswith(prefix):
            # return prefix
            return prefix
    # return first letter
    return upper_name[0]


def _extract_x_nodes(fields: list[str]) -> list[str]:
    # parameter index
    params_idx: int | None = None
    # iterate fields
    for i, f in enumerate(fields):
        # get uppercase version
        upper_f = f.upper()
        # check for parameters
        if upper_f == "PARAMS:" or upper_f.startswith("PARAMS:"):
            # store index
            params_idx = i
            # break loop
            break
    # slice the relevant part
    relevant = fields[:params_idx] if params_idx is not None else fields
    # check length
    if len(relevant) <= 1:
        # return empty list
        return []
    # return nodes
    return relevant[:-1]


def _extract_nodes(type_letter: str, tokens: list[str]) -> list[str]:
    # get fields
    fields = tokens[1:]
    # check for subcircuit
    if type_letter == "X":
        # extract nodes
        return _extract_x_nodes(fields)
    # check for y-type
    if type_letter in _Y_NODE_COUNTS:
        # get count
        count = _Y_NODE_COUNTS[type_letter]
        # return slice
        return fields[:count] if count > 0 else []
    # check for standard type
    if type_letter in _NODE_COUNTS:
        # get count
        count = _NODE_COUNTS[type_letter]
        # return slice
        return fields[:count]
    # return empty
    return []


def parse_netlist(text: str) -> tuple[str, NetlistTopology]:
    # get logical lines
    logical_lines = _join_continuation_lines(text.splitlines())
    # strip inline comments
    logical_lines = [_strip_inline_comment(line) for line in logical_lines]
    # title
    title: str | None = None
    # initialize lists
    top_level_devices: list[Device] = []
    # initialize node set
    top_level_nodes: set[str] = set()
    # initialize subcircuit dict
    subcircuit_definitions: dict[str, SubcircuitDefinition] = {}
    # initialize global nodes
    global_nodes: set[str] = set()
    # initialize directives
    directives: list[str] = []
    # initialize subcircuit
    current_subckt: SubcircuitDefinition | None = None
    # sanitized netlist
    netlist: list[str] = []
    # flag for the first non-blank line — in SPICE the first line is always the title
    first_line = True
    # iterate logical lines
    for line in logical_lines:
        # strip line
        stripped = line.strip()
        # skip blank lines (permitted before and after the title)
        if not stripped:
            # append line as is
            netlist.append(stripped)
            # continue loop
            continue
        # first non-blank line is always the title in SPICE format
        if first_line:
            # mark title consumed
            first_line = False
            # handle explicit .TITLE directive on the title line
            toks = stripped.split()
            if toks[0].upper() == ".TITLE":
                # extract title text after the keyword
                title = " ".join(toks[1:]) if len(toks) >= 2 else ""
                netlist.append(f".TITLE {title}" if title else ".TITLE")
            else:
                # entire first line is the title
                title = stripped
                netlist.append(stripped)
            # continue to next line
            continue
        # skip comment lines
        if stripped.startswith("*"):
            # append line as is
            netlist.append(stripped)
            # continue loop
            continue
        # split into tokens
        tokens = stripped.split()
        # get upper token
        first_upper = tokens[0].upper()
        # check for end
        if first_upper == ".END":
            # append line as is
            netlist.append(".END")
            # stop parsing
            break
        # check for directives
        if stripped.startswith("."):
            # title
            if first_upper == ".TITLE":
                # check for title
                if len(tokens) >= 2:
                    # set title
                    title = " ".join(tokens[1:])
                # append line as is
                netlist.append(f"{first_upper} {' '.join(tokens[1:])}")
                # continue loop
                continue
            # simulation type and output directives — stripped from sanitized netlist
            if first_upper in _SIMULATION_DIRECTIVES:
                # add to directives
                directives.append(stripped)
                # continue loop
                continue
            # option packages that are managed separately from the sanitized netlist
            if first_upper == ".OPTIONS" and len(tokens) > 1 and tokens[1].upper() in ("HBINT", "NONLIN-HB", "LINSOL-HB", "DEVICE", "TIMEINT", "NONLIN", "LINSOL",):
                # add to directives
                directives.append(stripped)
                # continue loop
                continue
            # .PREPROCESS REPLACEGROUND
            if first_upper == ".PREPROCESS" and len(tokens) > 2 and tokens[1].upper() == "REPLACEGROUND":
                # add to directives
                directives.append(stripped)
                # continue loop
                continue
            # check subcircuit
            if first_upper == ".SUBCKT":
                # append line as is
                netlist.append(f"{first_upper} {' '.join(tokens[1:])}")
                # check length
                if len(tokens) >= 2:
                    # get subcircuit name
                    subckt_name = tokens[1].upper()
                    # initialize ports
                    ports: list[str] = []
                    # iterate tokens
                    for t in tokens[2:]:
                        # check for params
                        if t.upper() == "PARAMS:" or t.upper().startswith("PARAMS:"):
                            # stop
                            break
                        # add port
                        ports.append(t.upper())
                    # create definition
                    current_subckt = SubcircuitDefinition(name=subckt_name, ports=ports)
                    # add to dict
                    subcircuit_definitions[subckt_name] = current_subckt
                # continue loop
                continue
            # check ends
            if first_upper == ".ENDS":
                # end current subcircuit
                netlist.append(".ENDS")
                # reset subcircuit
                current_subckt = None
                # continue loop
                continue
            # check global
            if first_upper == ".GLOBAL":
                # append line as is
                netlist.append(f"{first_upper} {' '.join(tokens[1:])}")
                # iterate tokens
                for t in tokens[1:]:
                    # add to globals
                    global_nodes.add(t.upper())
                # continue loop
                continue
            # other directives
            netlist.append(f"{first_upper} {' '.join(tokens[1:])}")
            # continue loop
            continue
        # append line
        netlist.append(stripped)
        # get upper name
        upper_name = tokens[0].upper()
        # get type letter
        type_letter = _get_type_letter(upper_name)
        # get raw nodes
        raw_nodes = _extract_nodes(type_letter, tokens)
        # normalize nodes
        node_names = [n.upper() for n in raw_nodes]
        # create device
        device = Device(name=upper_name, type_letter=type_letter, nodes=node_names)
        # add device
        if current_subckt is not None:
            # append to subcircuit
            current_subckt.devices.append(device)
        else:
            # append to top level
            top_level_devices.append(device)
            # add to nodes
            for node in node_names:
                # add node
                top_level_nodes.add(node)
        # handle global nodes
        for node in node_names:
            # check prefix
            if node.startswith("$G"):
                # add to globals
                global_nodes.add(node)
    # return result
    return f"{'\n'.join(netlist)}\n", NetlistTopology(title=title, devices=top_level_devices, nodes=top_level_nodes, subcircuit_definitions=subcircuit_definitions, global_nodes=global_nodes, directives=directives)
