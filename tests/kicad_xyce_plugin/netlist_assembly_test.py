from kicad_xyce_plugin.netlist_assembly import build_final_netlist


class TestNetlistAssembly:

    def test_appends_passthrough_step_directives_before_end(self):
        # arrange
        netlist = "Title\nR1 A B 1k\n.END\n"
        directives = [".TRAN 1u 1m"]
        passthrough = [".STEP V303 LIST 450 470"]
        # act
        final_netlist = build_final_netlist(netlist, directives, passthrough)
        # assert
        assert final_netlist == "Title\nR1 A B 1k\n\n.TRAN 1u 1m\n.STEP V303 LIST 450 470\n\n.END\n"

    def test_returns_original_netlist_when_no_directives_exist(self):
        # arrange
        netlist = "Title\nR1 A B 1k\n.END\n"
        # act
        final_netlist = build_final_netlist(netlist, [], [])
        # assert
        assert final_netlist == netlist
