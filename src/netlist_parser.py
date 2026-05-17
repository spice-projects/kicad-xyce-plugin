from dataclasses import dataclass
from dataclasses import field


# multi-character y-device type prefixes
_Y_PREFIXES = ("YMEMRISTOR", "YPDE", "YACC", "YLIN")


# fixed node counts for single-letter device types
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


# fixed node counts for multi-character y-type device prefixes
_Y_NODE_COUNTS: dict[str, int] = {
    "YMEMRISTOR": 2,
    "YLIN": 2,
    "YACC": 3,
    "YPDE": 0,
}


# device class
@dataclass
class Device:
    # device name
    name: str
    # device type letter
    type_letter: str
    # device nodes
    nodes: list[str]


# subcircuit definition class
@dataclass
class SubcircuitDefinition:
    # subcircuit name
    name: str
    # subcircuit ports
    ports: list[str]
    # list of devices
    devices: list[Device] = field(default_factory=list)


# netlist topology class
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


# joins continuation lines
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


# strips inline comments
def _strip_inline_comment(line: str) -> str:
    # find comment position
    idx = line.find(";")
    # return stripped string
    if idx >= 0:
        # return part before semicolon
        return line[:idx]
    # return original line
    return line


# gets type letter from name
def _get_type_letter(upper_name: str) -> str:
    # iterate prefixes
    for prefix in _Y_PREFIXES:
        # check for prefix
        if upper_name.startswith(prefix):
            # return prefix
            return prefix
    # return first letter
    return upper_name[0]


# extracts x-device nodes
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


# extracts nodes from tokens
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


# parses netlist
def parse_netlist(text: str) -> NetlistTopology:
    # get logical lines
    logical_lines = _join_continuation_lines(text.splitlines())
    # strip inline comments
    logical_lines = [_strip_inline_comment(line) for line in logical_lines]
    # set title
    title = logical_lines[0].strip() if logical_lines else ""
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
    # iterate logical lines
    for line in logical_lines[1:]:
        # strip line
        stripped = line.strip()
        # skip empty or comments
        if not stripped or stripped.startswith("*"):
            # continue loop
            continue
        # split into tokens
        tokens = stripped.split()
        # get upper token
        first_upper = tokens[0].upper()
        # check for end
        if first_upper == ".END":
            # stop parsing
            break
        # check for directives
        if stripped.startswith("."):
            # check directive types
            if first_upper in (".OP", ".PRINT", ".SAVE", ".NODESET"):
                # add to directives
                directives.append(stripped)
            # check subcircuit
            if first_upper == ".SUBCKT":
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
            # check ends
            elif first_upper == ".ENDS":
                # reset subcircuit
                current_subckt = None
            # check global
            elif first_upper == ".GLOBAL":
                # iterate tokens
                for t in tokens[1:]:
                    # add to globals
                    global_nodes.add(t.upper())
            # continue loop
            continue
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
        # else case
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
    return NetlistTopology(
        title=title,
        devices=top_level_devices,
        nodes=top_level_nodes,
        subcircuit_definitions=subcircuit_definitions,
        global_nodes=global_nodes,
        directives=directives,
    )
