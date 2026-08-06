from __future__ import annotations

from collections.abc import Sequence


def build_final_netlist(netlist: str, directives: Sequence[str], passthrough_directives: Sequence[str] = ()) -> str:
    # merge managed directives with verbatim pass-through directives in the final emission order
    combined_directives = [*directives, *passthrough_directives]
    # preserve the original netlist when there is nothing to insert
    if not combined_directives:
        return netlist
    # join the directives exactly as they should appear in the output netlist
    directive_block = "\n".join(combined_directives)
    # insert the directive block immediately before the terminating .END line
    return netlist.replace(".END\n", f"\n{directive_block}\n\n.END\n")
