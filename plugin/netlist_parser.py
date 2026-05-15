from dataclasses import dataclass, field


# multi-character Y-device type prefixes — must be checked before single-letter fallback
_Y_PREFIXES = ("YMEMRISTOR", "YPDE", "YACC", "YLIN")

# fixed node counts for single-letter device types; value = number of nodes after device name
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

# fixed node counts for multi-character Y-type device prefixes
_Y_NODE_COUNTS: dict[str, int] = {
    "YMEMRISTOR": 2,
    "YLIN": 2,
    "YACC": 3,
    "YPDE": 0,
}


@dataclass
class Device:

    name: str
    type_letter: str
    nodes: list[str]


@dataclass
class SubcircuitDefinition:

    name: str
    ports: list[str]
    devices: list[Device] = field(default_factory=list)


@dataclass
class NetlistTopology:

    title: str
    devices: list[Device]
    nodes: set[str]
    subcircuit_definitions: dict[str, SubcircuitDefinition]
    global_nodes: set[str]


def _join_continuation_lines(raw_lines: list[str]) -> list[str]:
    # build the result list of joined logical lines
    joined: list[str] = []
    # process each physical line in order
    for line in raw_lines:
        # a line whose first non-whitespace char is '+' continues the previous logical line
        if line.lstrip().startswith("+"):
            # only merge when there is a preceding logical line to attach to
            if joined:
                # strip the leading '+' and append the rest to the previous line
                joined[-1] = joined[-1] + " " + line.lstrip()[1:].strip()
        else:
            # start a new logical line
            joined.append(line)
    # return the assembled list of logical lines
    return joined


def _strip_inline_comment(line: str) -> str:
    # find the position of the first semicolon
    idx = line.find(";")
    # return only the portion before the semicolon when an inline comment is present
    if idx >= 0:
        return line[:idx]
    # return the line unchanged when no inline comment is present
    return line


def _get_type_letter(upper_name: str) -> str:
    # check multi-character Y-type prefixes before falling back to the first character
    for prefix in _Y_PREFIXES:
        # return immediately on the first matching Y-type prefix
        if upper_name.startswith(prefix):
            return prefix
    # use the single first character as the type letter for all other device types
    return upper_name[0]


def _extract_x_nodes(fields: list[str]) -> list[str]:
    # locate the PARAMS: keyword that marks the end of the node list
    params_idx: int | None = None
    # scan each field token for the PARAMS: boundary
    for i, f in enumerate(fields):
        # normalise to uppercase for case-insensitive comparison
        upper_f = f.upper()
        # record the position and stop scanning when the boundary is found
        if upper_f == "PARAMS:" or upper_f.startswith("PARAMS:"):
            params_idx = i
            break
    # keep only the tokens before PARAMS: when the keyword was found
    relevant = fields[:params_idx] if params_idx is not None else fields
    # return empty list when there are not enough tokens to hold a node
    if len(relevant) <= 1:
        return []
    # the last token is the subcircuit reference name, not a node
    return relevant[:-1]


def _extract_nodes(type_letter: str, tokens: list[str]) -> list[str]:
    # strip the device name token; remaining fields contain nodes and parameters
    fields = tokens[1:]
    # subcircuit instances use their own variable-length parsing rule
    if type_letter == "X":
        return _extract_x_nodes(fields)
    # Y-type devices use the multi-character prefix node count table
    if type_letter in _Y_NODE_COUNTS:
        # look up the fixed node count for this Y-type prefix
        count = _Y_NODE_COUNTS[type_letter]
        # return empty list for variable-count Y types with no fixed count
        return fields[:count] if count > 0 else []
    # all other device types use the single-letter node count table
    if type_letter in _NODE_COUNTS:
        # look up the fixed node count for this device type letter
        count = _NODE_COUNTS[type_letter]
        # return the first N field tokens which represent the node names
        return fields[:count]
    # unrecognised device type: return no nodes
    return []


def parse_netlist(text: str) -> NetlistTopology:
    # join continuation lines before any other processing
    logical_lines = _join_continuation_lines(text.splitlines())
    # strip inline ';' comments from every logical line
    logical_lines = [_strip_inline_comment(line) for line in logical_lines]
    # the very first logical line is always the netlist title
    title = logical_lines[0].strip() if logical_lines else ""
    # accumulate top-level devices and node names separately from subcircuit contents
    top_level_devices: list[Device] = []
    top_level_nodes: set[str] = set()
    subcircuit_definitions: dict[str, SubcircuitDefinition] = {}
    global_nodes: set[str] = set()
    # track the currently open .SUBCKT context; None means top-level scope
    current_subckt: SubcircuitDefinition | None = None
    # process all logical lines that follow the title
    for line in logical_lines[1:]:
        # strip leading and trailing whitespace for uniform processing
        stripped = line.strip()
        # skip blank lines and full-line comment lines starting with '*'
        if not stripped or stripped.startswith("*"):
            continue
        # split into tokens for keyword and field extraction
        tokens = stripped.split()
        # normalise the first token for case-insensitive keyword comparisons
        first_upper = tokens[0].upper()
        # stop parsing when the .END directive is reached
        if first_upper == ".END":
            break
        # dispatch directive lines separately from device element lines
        if stripped.startswith("."):
            # open a new subcircuit definition block on .SUBCKT
            if first_upper == ".SUBCKT":
                # require at least the subcircuit name token to proceed
                if len(tokens) >= 2:
                    # normalise the subcircuit name to uppercase
                    subckt_name = tokens[1].upper()
                    # collect port node names up to any PARAMS: keyword
                    ports: list[str] = []
                    # iterate the remaining tokens looking for port names
                    for t in tokens[2:]:
                        # stop collecting ports at the PARAMS: boundary
                        if t.upper() == "PARAMS:" or t.upper().startswith("PARAMS:"):
                            break
                        # add the normalised port name to the list
                        ports.append(t.upper())
                    # create the definition and record it under the open context
                    current_subckt = SubcircuitDefinition(name=subckt_name, ports=ports)
                    # register the definition by name for later lookup
                    subcircuit_definitions[subckt_name] = current_subckt
            # close the currently open subcircuit definition block on .ENDS
            elif first_upper == ".ENDS":
                current_subckt = None
            # register explicitly declared global node names on .GLOBAL
            elif first_upper == ".GLOBAL":
                # add each declared node name to the global node set
                for t in tokens[1:]:
                    global_nodes.add(t.upper())
            continue
        # normalise the device name to uppercase for consistent storage
        upper_name = tokens[0].upper()
        # determine the device type letter from the uppercased name
        type_letter = _get_type_letter(upper_name)
        # extract the raw node name tokens for this device type
        raw_nodes = _extract_nodes(type_letter, tokens)
        # normalise node names to uppercase for case-insensitive comparison
        node_names = [n.upper() for n in raw_nodes]
        # construct the device instance with normalised fields
        device = Device(name=upper_name, type_letter=type_letter, nodes=node_names)
        # add device to the open subcircuit or to the top-level list
        if current_subckt is not None:
            # device belongs to the currently open subcircuit definition
            current_subckt.devices.append(device)
        else:
            # device is a top-level element; add it to the top-level device list
            top_level_devices.append(device)
            # register each node name in the top-level node set
            for node in node_names:
                top_level_nodes.add(node)
        # register any $G-prefixed node as global regardless of nesting scope
        for node in node_names:
            # only process nodes that carry the global $G prefix
            if node.startswith("$G"):
                global_nodes.add(node)
    return NetlistTopology(title=title, devices=top_level_devices, nodes=top_level_nodes, subcircuit_definitions=subcircuit_definitions, global_nodes=global_nodes)
