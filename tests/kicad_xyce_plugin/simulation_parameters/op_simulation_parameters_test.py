from unittest.mock import MagicMock

from kicad_xyce_plugin.simulation_parameters import IcEntry, NodesetEntry, OpSimulationParameters, PrintParameters


class TestPrintParametersWildcards:

    def test_print_parameters_emits_print_dc_directive(self):
        # arrange — OP analysis always uses .PRINT DC regardless of context
        print_params = PrintParameters(print_type="DC", output_variables=("V(*)",))
        params = OpSimulationParameters(replace_ground=False, print_parameters=print_params)
        # act
        directives = params.to_xyce_directives()
        # assert — emitted directive is .PRINT DC, never .PRINT TRAN or .PRINT OP
        assert any(d.startswith(".PRINT DC") for d in directives)
        assert not any(d.startswith(".PRINT TRAN") for d in directives)

    def test_generic_wildcards_via_print_parameters(self):
        # arrange — the three universal wildcards valid for .PRINT DC
        wildcards = ("V(*)", "I(*)", "P(*)")
        print_params = PrintParameters(print_type="DC", output_variables=wildcards)
        params = OpSimulationParameters(replace_ground=False, print_parameters=print_params)
        # act
        directives = params.to_xyce_directives()
        # assert — each wildcard token appears in the .PRINT DC line
        print_line = next(d for d in directives if d.startswith(".PRINT DC"))
        for wildcard in wildcards:
            assert wildcard in print_line

    def test_bjt_lead_wildcards_via_print_parameters(self):
        # arrange — BJT lead current wildcards: IB, IC, IE, IS
        wildcards = ("IB(*)", "IC(*)", "IE(*)", "IS(*)")
        print_params = PrintParameters(print_type="DC", output_variables=wildcards)
        params = OpSimulationParameters(replace_ground=False, print_parameters=print_params)
        # act
        directives = params.to_xyce_directives()
        # assert — each BJT wildcard appears in the emitted .PRINT DC line
        print_line = next(d for d in directives if d.startswith(".PRINT DC"))
        for wildcard in wildcards:
            assert wildcard in print_line

    def test_fet_lead_wildcards_via_print_parameters(self):
        # arrange — FET lead current wildcards: IB, ID, IG, IS
        wildcards = ("IB(*)", "ID(*)", "IG(*)", "IS(*)")
        print_params = PrintParameters(print_type="DC", output_variables=wildcards)
        params = OpSimulationParameters(replace_ground=False, print_parameters=print_params)
        # act
        directives = params.to_xyce_directives()
        # assert — each FET wildcard appears in the emitted .PRINT DC line
        print_line = next(d for d in directives if d.startswith(".PRINT DC"))
        for wildcard in wildcards:
            assert wildcard in print_line

    def test_w_star_normalizes_to_p_star_on_parse(self):
        # arrange — netlist contains PSpice-style W(*) in a .PRINT DC directive
        directives = [".OP", ".PRINT DC W(*)"]
        # act
        params = OpSimulationParameters.from_xyce_directives(directives)
        # assert — W(*) is stored as P(*) at parse time; no W(*) survives
        assert params.print_parameters is not None
        assert "P(*)" in params.print_parameters.output_variables
        assert "W(*)" not in params.print_parameters.output_variables

    def test_print_parameters_round_trip(self):
        # arrange — wildcards spanning generic, BJT, and FET sets
        wildcards = ("V(*)", "I(*)", "P(*)", "IB(*)", "IC(*)", "IE(*)", "IS(*)", "ID(*)", "IG(*)")
        print_params = PrintParameters(print_type="DC", output_variables=wildcards)
        params = OpSimulationParameters(replace_ground=False, print_parameters=print_params)
        # act
        directives = params.to_xyce_directives()
        reparsed = OpSimulationParameters.from_xyce_directives([".OP"] + [d for d in directives if d.startswith(".PRINT")])
        # assert — all wildcards survive a full serialize/parse cycle with the correct type
        assert reparsed.print_parameters is not None
        assert reparsed.print_parameters.print_type == "DC"
        assert reparsed.print_parameters.output_variables == wildcards

    def test_print_parameters_takes_priority_over_legacy_fields(self):
        # arrange — both legacy and new fields are set; print_parameters must win
        print_params = PrintParameters(print_type="DC", output_variables=("V(*)",))
        params = OpSimulationParameters(print_dc_enabled=True, print_dc_specific_variables=("I(R1)",), replace_ground=False, print_parameters=print_params)
        # act
        directives = params.to_xyce_directives()
        print_line = next(d for d in directives if d.startswith(".PRINT DC"))
        # assert — only V(*) from print_parameters appears; I(R1) from legacy is not emitted
        assert "V(*)" in print_line
        assert "I(R1)" not in print_line


class TestOpSimulationParameters:

    def test_op_directive_default(self):
        # arrange
        params = OpSimulationParameters(replace_ground=False)
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".OP"]

    def test_print_dc_directive(self):
        # arrange
        params = OpSimulationParameters(print_dc_enabled=True, print_dc_specific_variables=("V(1)", "I(V1)"), replace_ground=False)
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".OP", ".PRINT DC V(1) I(V1)"]

    def test_save_directive(self):
        # arrange
        params = OpSimulationParameters(save_enabled=True, save_type="IC", save_file="test.ic", replace_ground=False)
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".OP", ".SAVE TYPE=IC FILE=test.ic"]

    def test_nodeset_directive(self):
        # arrange
        entries = (NodesetEntry(node="out", voltage="1.2"),)
        params = OpSimulationParameters(nodeset_entries=entries, replace_ground=False)
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
        params = OpSimulationParameters(print_dc_enabled=True, print_dc_format="CSV", replace_ground=False)
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".OP", ".PRINT DC FORMAT=CSV"]

    def test_print_dc_with_file(self):
        # arrange
        params = OpSimulationParameters(print_dc_enabled=True, print_dc_file="output.csv", replace_ground=False)
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".OP", ".PRINT DC FILE=output.csv"]

    def test_print_dc_deduplicates_variables(self):
        # arrange
        params = OpSimulationParameters(print_dc_enabled=True, print_dc_specific_variables=("V(1)", "V(1)", "I(R1)"), replace_ground=False)
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".OP", ".PRINT DC V(1) I(R1)"]

    def test_save_without_file(self):
        # arrange
        params = OpSimulationParameters(save_enabled=True, replace_ground=False)
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".OP", ".SAVE TYPE=NODESET"]

    def test_topology_without_all_nodes_or_currents(self):
        # arrange
        params = OpSimulationParameters(print_dc_enabled=True, print_dc_specific_variables=("V(1)",), replace_ground=False)
        topology = MagicMock()
        # act
        directives = params.to_xyce_directives(topology=topology)
        # assert
        assert directives == [".OP", ".PRINT DC V(1)"]

    def test_ic_directive(self):
        # arrange
        entries = (IcEntry(node="out", voltage="1.0"), IcEntry(node="in", voltage="0"))
        params = OpSimulationParameters(ic_entries=entries, replace_ground=False)
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


class TestReferenceGuideExamples:
    # reference guide examples from xyce_rg.txt section 2.1.24 (lines 3784)

    def test_reference_guide_example_basic(self):
        # arrange - .OP
        directive = ".OP"
        # act
        params = OpSimulationParameters.from_xyce_directives([directive])
        # assert
        assert params is not None
        directives = params.to_xyce_directives()
        assert ".OP" in directives

    def test_reference_guide_example_round_trip(self):
        # arrange - .OP (simple directive with no arguments)
        directive = ".OP"
        # act
        params = OpSimulationParameters.from_xyce_directives([directive])
        regenerated = params.to_xyce_directives()
        reparsed = OpSimulationParameters.from_xyce_directives(regenerated)
        # assert
        assert reparsed is not None
        regenerated2 = reparsed.to_xyce_directives()
        assert ".OP" in regenerated2
