def is_network_parameter_probe_name(function_name: str) -> bool:
    # network-parameter probes are S/Z/Y/H followed by two digits
    if len(function_name) != 3:
        return False
    # must start with S/Z/Y/H
    if function_name[0] not in ("s", "z", "y", "h"):
        return False
    # then two digits
    return function_name[1].isdigit() and function_name[2].isdigit()
