from unittest.mock import MagicMock

from simulation_parameters import IcEntry, NodesetEntry, OpSimulationParameters


class TestOpSimulationParameters:

    def test_op_directive_default(self):
        # arrange
        params = OpSimulationParameters()
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".OP"]

    def test_print_dc_directive(self):
        # arrange
        params = OpSimulationParameters(print_dc_enabled=True, print_dc_specific_variables=("V(1)", "I(V1)"))
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".OP", ".PRINT DC V(1) I(V1)"]

    def test_save_directive(self):
        # arrange
        params = OpSimulationParameters(save_enabled=True, save_type="IC", save_file="test.ic")
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".OP", ".SAVE TYPE=IC FILE=test.ic"]

    def test_nodeset_directive(self):
        # arrange
        entries = (NodesetEntry(node="out", voltage="1.2"),)
        params = OpSimulationParameters(nodeset_entries=entries)
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".OP", ".NODESET V(out)=1.2"]

    def test_dynamic_resolution(self):
        # arrange
        params = OpSimulationParameters(print_dc_enabled=True, print_dc_all_nodes=True, print_dc_all_currents=True)
        topology = MagicMock()
        topology.nodes = ["1", "2"]
        topology.devices = [MagicMock(name="R1"), MagicMock(name="V1")]
        topology.devices[0].name = "R1"
        topology.devices[1].name = "V1"
        # act
        directives = params.to_xyce_directives(topology=topology)
        # assert
        assert ".PRINT DC V(1) V(2) I(R1) I(V1)" in directives

    def test_replace_ground(self):
        # arrange
        params = OpSimulationParameters(replace_ground=True)
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".PREPROCESS REPLACEGROUND TRUE", ".OP"]

    def test_print_dc_with_format(self):
        # arrange
        params = OpSimulationParameters(print_dc_enabled=True, print_dc_format="CSV")
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".OP", ".PRINT DC FORMAT=CSV"]

    def test_print_dc_with_file(self):
        # arrange
        params = OpSimulationParameters(print_dc_enabled=True, print_dc_file="output.csv")
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".OP", ".PRINT DC FILE=output.csv"]

    def test_print_dc_deduplicates_variables(self):
        # arrange
        params = OpSimulationParameters(print_dc_enabled=True, print_dc_specific_variables=("V(1)", "V(1)", "I(R1)"))
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".OP", ".PRINT DC V(1) I(R1)"]

    def test_save_without_file(self):
        # arrange
        params = OpSimulationParameters(save_enabled=True)
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".OP", ".SAVE TYPE=NODESET"]

    def test_topology_without_all_nodes_or_currents(self):
        # arrange
        params = OpSimulationParameters(print_dc_enabled=True, print_dc_specific_variables=("V(1)",))
        topology = MagicMock()
        # act
        directives = params.to_xyce_directives(topology=topology)
        # assert
        assert directives == [".OP", ".PRINT DC V(1)"]

    def test_ic_directive(self):
        # arrange
        entries = (IcEntry(node="out", voltage="1.0"), IcEntry(node="in", voltage="0"))
        params = OpSimulationParameters(ic_entries=entries)
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".OP", ".IC V(out)=1.0 V(in)=0"]


class TestFromXyceDirectives:

    def test_empty_directives(self):
        # arrange / act
        params = OpSimulationParameters.from_xyce_directives([])
        # assert — no .OP directive means None is returned
        assert params is None

    def test_parses_print_dc_with_variables(self):
        # arrange / act
        params = OpSimulationParameters.from_xyce_directives([".OP", ".PRINT DC V(1) I(R1)"])
        # assert
        assert params.print_dc_enabled is True
        assert params.print_dc_specific_variables == ("V(1)", "I(R1)")

    def test_parses_save(self):
        # arrange / act
        params = OpSimulationParameters.from_xyce_directives([".OP", ".SAVE"])
        # assert
        assert params.save_enabled is True

    def test_parses_nodeset(self):
        # arrange / act
        params = OpSimulationParameters.from_xyce_directives([".OP", ".NODESET V(out)=1.2 V(in)=0.5"])
        # assert
        assert params.nodeset_entries == (NodesetEntry(node="out", voltage="1.2"), NodesetEntry(node="in", voltage="0.5"))

    def test_nodeset_ignores_pair_without_equals(self):
        # arrange / act
        params = OpSimulationParameters.from_xyce_directives([".OP", ".NODESET INVALID"])
        # assert
        assert params.nodeset_entries == ()

    def test_nodeset_ignores_invalid_node_format(self):
        # arrange / act
        params = OpSimulationParameters.from_xyce_directives([".OP", ".NODESET out=1.2"])
        # assert
        assert params.nodeset_entries == ()

    def test_parses_replaceground_true(self):
        # arrange / act
        params = OpSimulationParameters.from_xyce_directives([".OP", ".PREPROCESS REPLACEGROUND TRUE"])
        # assert
        assert params.replace_ground is True

    def test_parses_replaceground_false(self):
        # arrange / act
        params = OpSimulationParameters.from_xyce_directives([".OP", ".PREPROCESS REPLACEGROUND FALSE"])
        # assert
        assert params.replace_ground is False

    def test_ignores_unknown_directive(self):
        # arrange / act
        params = OpSimulationParameters.from_xyce_directives([".TRAN 1ns 1ms"])
        # assert — no .OP directive means None is returned
        assert params is None

    def test_ignores_print_non_dc(self):
        # arrange / act
        params = OpSimulationParameters.from_xyce_directives([".OP", ".PRINT AC V(1)"])
        # assert
        assert params.print_dc_enabled is False

    def test_parses_ic_v_node_form(self):
        # arrange / act
        params = OpSimulationParameters.from_xyce_directives([".OP", ".IC V(out)=1.0 V(in)=0"])
        # assert
        assert params is not None
        assert params.ic_entries == (IcEntry(node="out", voltage="1.0"), IcEntry(node="in", voltage="0"))

    def test_parses_ic_node_val_form(self):
        # arrange / act
        params = OpSimulationParameters.from_xyce_directives([".OP", ".IC out 1.0"])
        # assert
        assert params is not None
        assert params.ic_entries == (IcEntry(node="out", voltage="1.0"),)

    def test_parses_dcvolt(self):
        # arrange / act
        params = OpSimulationParameters.from_xyce_directives([".OP", ".DCVOLT V(out)=2.5"])
        # assert
        assert params is not None
        assert params.ic_entries == (IcEntry(node="out", voltage="2.5"),)

    def test_ignores_print_without_type(self):
        # arrange / act
        params = OpSimulationParameters.from_xyce_directives([".OP", ".PRINT"])
        # assert
        assert params.print_dc_enabled is False
