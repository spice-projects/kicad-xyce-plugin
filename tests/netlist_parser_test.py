from netlist_parser import Device, NetlistTopology, parse_netlist


class TestNetlistParser:

    def test_title_is_first_line(self):
        # arrange
        netlist = "My Circuit Title\nR1 NET1 NET2 1k\n.END\n"
        # act
        topology = parse_netlist(netlist)
        # assert
        assert topology.title == "My Circuit Title"

    def test_title_line_treated_as_comment_not_device(self):
        # arrange
        netlist = "* KiCad schematic\nR1 A B 1k\n.END\n"
        # act
        topology = parse_netlist(netlist)
        # assert — title captured even when it starts with *; R1 is the only device
        assert topology.title == "* KiCad schematic"
        assert len(topology.devices) == 1

    def test_parsing_stops_at_end_directive(self):
        # arrange — R2 appears after .END and must be ignored
        netlist = "Title\nR1 A B 1k\n.END\nR2 C D 2k\n"
        # act
        topology = parse_netlist(netlist)
        # assert
        assert len(topology.devices) == 1
        assert topology.devices[0].name == "R1"

    def test_empty_netlist_returns_empty_topology(self):
        # arrange
        netlist = "Title\n.END\n"
        # act
        topology = parse_netlist(netlist)
        # assert
        assert topology.title == "Title"
        assert topology.devices == []
        assert topology.nodes == set()

    def test_star_comment_lines_are_skipped(self):
        # arrange
        netlist = "Title\n* this is a comment\nR1 A B 1k\n.END\n"
        # act
        topology = parse_netlist(netlist)
        # assert
        assert len(topology.devices) == 1

    def test_inline_semicolon_comment_stripped(self):
        # arrange — inline comment must not affect node extraction
        netlist = "Title\nR1 NET1 NET2 1k ; bypass resistor\n.END\n"
        # act
        topology = parse_netlist(netlist)
        # assert
        assert topology.devices[0].nodes == ["NET1", "NET2"]

    def test_inline_comment_on_directive_stripped(self):
        # arrange
        netlist = "Title\n.GLOBAL VDD ; power rail\n.END\n"
        # act
        topology = parse_netlist(netlist)
        # assert
        assert "VDD" in topology.global_nodes

    def test_continuation_line_joined_to_previous(self):
        # arrange — device line is split across two physical lines with '+'
        netlist = "Title\nR1 NET1\n+ NET2 1k\n.END\n"
        # act
        topology = parse_netlist(netlist)
        # assert
        assert len(topology.devices) == 1
        assert topology.devices[0].nodes == ["NET1", "NET2"]

    def test_continuation_line_with_leading_whitespace(self):
        # arrange — the '+' can be preceded by whitespace per SPICE spec
        netlist = "Title\nR1 NET1\n   + NET2 1k\n.END\n"
        # act
        topology = parse_netlist(netlist)
        # assert
        assert topology.devices[0].nodes == ["NET1", "NET2"]

    def test_multiple_continuation_lines(self):
        # arrange — three continuation lines for a single X device
        netlist = "Title\nX1 IN\n+ OUT\n+ VCC GND\n+ opamp\n.END\n"
        # act
        topology = parse_netlist(netlist)
        # assert
        assert len(topology.devices) == 1
        assert topology.devices[0].nodes == ["IN", "OUT", "VCC", "GND"]

    def test_lowercase_device_letter_recognised(self):
        # arrange
        netlist = "Title\nr1 net1 net2 1k\n.END\n"
        # act
        topology = parse_netlist(netlist)
        # assert
        assert topology.devices[0].type_letter == "R"

    def test_device_name_normalised_to_uppercase(self):
        # arrange
        netlist = "Title\nr1 a b 1k\n.END\n"
        # act
        topology = parse_netlist(netlist)
        # assert
        assert topology.devices[0].name == "R1"

    def test_node_names_normalised_to_uppercase(self):
        # arrange
        netlist = "Title\nR1 net1 gnd 1k\n.END\n"
        # act
        topology = parse_netlist(netlist)
        # assert
        assert "NET1" in topology.nodes
        assert "GND" in topology.nodes

    def test_directive_keywords_case_insensitive(self):
        # arrange — lowercase .global and .end
        netlist = "Title\n.global VDD\n.end\n"
        # act
        topology = parse_netlist(netlist)
        # assert
        assert "VDD" in topology.global_nodes

    def test_resistor_type_letter(self):
        # arrange
        netlist = "Title\nR1 NET1 NET2 10k\n.END\n"
        # act
        topology = parse_netlist(netlist)
        # assert
        assert topology.devices[0].type_letter == "R"

    def test_resistor_nodes(self):
        # arrange
        netlist = "Title\nR1 NET1 NET2 10k\n.END\n"
        # act
        topology = parse_netlist(netlist)
        # assert
        assert topology.devices[0].nodes == ["NET1", "NET2"]

    def test_resistor_ground_node_zero(self):
        # arrange — ground is always named '0'
        netlist = "Title\nR1 VCC 0 1k\n.END\n"
        # act
        topology = parse_netlist(netlist)
        # assert
        assert topology.devices[0].nodes == ["VCC", "0"]
        assert "0" in topology.nodes

    def test_capacitor_nodes(self):
        # arrange
        netlist = "Title\nC1 VCC GND 100n\n.END\n"
        # act
        topology = parse_netlist(netlist)
        # assert
        assert topology.devices[0].type_letter == "C"
        assert topology.devices[0].nodes == ["VCC", "GND"]

    def test_inductor_nodes(self):
        # arrange
        netlist = "Title\nL1 IN OUT 10u\n.END\n"
        # act
        topology = parse_netlist(netlist)
        # assert
        assert topology.devices[0].type_letter == "L"
        assert topology.devices[0].nodes == ["IN", "OUT"]

    def test_diode_nodes(self):
        # arrange
        netlist = "Title\nD1 ANODE CATHODE 1N4148\n.END\n"
        # act
        topology = parse_netlist(netlist)
        # assert
        assert topology.devices[0].type_letter == "D"
        assert topology.devices[0].nodes == ["ANODE", "CATHODE"]

    def test_bjt_nodes(self):
        # arrange
        netlist = "Title\nQ1 COLL BASE EMIT NPN\n.END\n"
        # act
        topology = parse_netlist(netlist)
        # assert
        assert topology.devices[0].type_letter == "Q"
        assert topology.devices[0].nodes == ["COLL", "BASE", "EMIT"]

    def test_mosfet_nodes(self):
        # arrange
        netlist = "Title\nM1 DRAIN GATE SOURCE BULK NMOS\n.END\n"
        # act
        topology = parse_netlist(netlist)
        # assert
        assert topology.devices[0].type_letter == "M"
        assert topology.devices[0].nodes == ["DRAIN", "GATE", "SOURCE", "BULK"]

    def test_voltage_source_nodes(self):
        # arrange
        netlist = "Title\nV1 VCC GND DC 5\n.END\n"
        # act
        topology = parse_netlist(netlist)
        # assert
        assert topology.devices[0].type_letter == "V"
        assert topology.devices[0].nodes == ["VCC", "GND"]

    def test_current_source_nodes(self):
        # arrange
        netlist = "Title\nI1 NET1 NET2 DC 1m\n.END\n"
        # act
        topology = parse_netlist(netlist)
        # assert
        assert topology.devices[0].type_letter == "I"
        assert topology.devices[0].nodes == ["NET1", "NET2"]

    def test_vcvs_nodes(self):
        # arrange — E device: + - +ctrl -ctrl
        netlist = "Title\nE1 VOUT GND VIN GND 10\n.END\n"
        # act
        topology = parse_netlist(netlist)
        # assert
        assert topology.devices[0].type_letter == "E"
        assert topology.devices[0].nodes == ["VOUT", "GND", "VIN", "GND"]

    def test_cccs_nodes(self):
        # arrange — F device: + - (controlling source name follows, not a node)
        netlist = "Title\nF1 NET1 NET2 VSENSE 5\n.END\n"
        # act
        topology = parse_netlist(netlist)
        # assert
        assert topology.devices[0].type_letter == "F"
        assert topology.devices[0].nodes == ["NET1", "NET2"]

    def test_vccs_nodes(self):
        # arrange — G device: + - +ctrl -ctrl
        netlist = "Title\nG1 IOUT GND VIN GND 0.01\n.END\n"
        # act
        topology = parse_netlist(netlist)
        # assert
        assert topology.devices[0].type_letter == "G"
        assert topology.devices[0].nodes == ["IOUT", "GND", "VIN", "GND"]

    def test_ccvs_nodes(self):
        # arrange — H device: + - (controlling source name follows, not a node)
        netlist = "Title\nH1 VOUT GND ISENSE 100\n.END\n"
        # act
        topology = parse_netlist(netlist)
        # assert
        assert topology.devices[0].type_letter == "H"
        assert topology.devices[0].nodes == ["VOUT", "GND"]

    def test_nonlinear_dependent_source_nodes(self):
        # arrange — B device: + -
        netlist = "Title\nB1 VOUT GND V={V(IN)*2}\n.END\n"
        # act
        topology = parse_netlist(netlist)
        # assert
        assert topology.devices[0].type_letter == "B"
        assert topology.devices[0].nodes == ["VOUT", "GND"]

    def test_subcircuit_instance_nodes(self):
        # arrange
        netlist = "Title\nX1 IN OUT VCC GND opamp\n.END\n"
        # act
        topology = parse_netlist(netlist)
        # assert
        assert topology.devices[0].type_letter == "X"
        assert topology.devices[0].nodes == ["IN", "OUT", "VCC", "GND"]

    def test_subcircuit_instance_with_params_keyword(self):
        # arrange — PARAMS: keyword separates node list from plugin.parameter assignments
        netlist = "Title\nX1 IN OUT VCC GND opamp PARAMS: gain=100\n.END\n"
        # act
        topology = parse_netlist(netlist)
        # assert
        assert topology.devices[0].nodes == ["IN", "OUT", "VCC", "GND"]

    def test_subcircuit_instance_single_port(self):
        # arrange — only one token before subckt name means zero nodes
        netlist = "Title\nX1 myblock\n.END\n"
        # act
        topology = parse_netlist(netlist)
        # assert
        assert topology.devices[0].nodes == []

    def test_mutual_inductor_has_no_nodes(self):
        # arrange
        netlist = "Title\nK1 L1 L2 0.99\n.END\n"
        # act
        topology = parse_netlist(netlist)
        # assert
        assert topology.devices[0].type_letter == "K"
        assert topology.devices[0].nodes == []

    def test_jfet_nodes(self):
        # arrange
        netlist = "Title\nJ1 DRAIN GATE SOURCE NJF\n.END\n"
        # act
        topology = parse_netlist(netlist)
        # assert
        assert topology.devices[0].type_letter == "J"
        assert topology.devices[0].nodes == ["DRAIN", "GATE", "SOURCE"]

    def test_mesfet_nodes(self):
        # arrange
        netlist = "Title\nZ1 DRAIN GATE SOURCE GAASFET\n.END\n"
        # act
        topology = parse_netlist(netlist)
        # assert
        assert topology.devices[0].type_letter == "Z"
        assert topology.devices[0].nodes == ["DRAIN", "GATE", "SOURCE"]

    def test_ideal_transmission_line_nodes(self):
        # arrange — T device: A+ A- B+ B-
        netlist = "Title\nT1 A_P A_N B_P B_N Z0=50 TD=1n\n.END\n"
        # act
        topology = parse_netlist(netlist)
        # assert
        assert topology.devices[0].type_letter == "T"
        assert topology.devices[0].nodes == ["A_P", "A_N", "B_P", "B_N"]

    def test_lossy_transmission_line_nodes(self):
        # arrange — O device: A+ A- B+ B-
        netlist = "Title\nO1 A_P A_N B_P B_N LTRA\n.END\n"
        # act
        topology = parse_netlist(netlist)
        # assert
        assert topology.devices[0].type_letter == "O"
        assert topology.devices[0].nodes == ["A_P", "A_N", "B_P", "B_N"]

    def test_voltage_controlled_switch_nodes(self):
        # arrange — S device: +switch -switch +ctrl -ctrl
        netlist = "Title\nS1 SW_P SW_N CTRL_P CTRL_N SMOD\n.END\n"
        # act
        topology = parse_netlist(netlist)
        # assert
        assert topology.devices[0].type_letter == "S"
        assert topology.devices[0].nodes == ["SW_P", "SW_N", "CTRL_P", "CTRL_N"]

    def test_current_controlled_switch_nodes(self):
        # arrange — W device: +switch -switch (controlling source name follows)
        netlist = "Title\nW1 SW_P SW_N VSENSE WMOD\n.END\n"
        # act
        topology = parse_netlist(netlist)
        # assert
        assert topology.devices[0].type_letter == "W"
        assert topology.devices[0].nodes == ["SW_P", "SW_N"]

    def test_port_device_nodes(self):
        # arrange — P device: + -
        netlist = "Title\nP1 RF_P GND 1\n.END\n"
        # act
        topology = parse_netlist(netlist)
        # assert
        assert topology.devices[0].type_letter == "P"
        assert topology.devices[0].nodes == ["RF_P", "GND"]

    def test_ylin_device_nodes(self):
        # arrange — YLIN: + -
        netlist = "Title\nYLIN1 NET1 NET2 lin_model\n.END\n"
        # act
        topology = parse_netlist(netlist)
        # assert
        assert topology.devices[0].type_letter == "YLIN"
        assert topology.devices[0].nodes == ["NET1", "NET2"]

    def test_ymemristor_device_nodes(self):
        # arrange — YMEMRISTOR: + -
        netlist = "Title\nYMEMRISTOR1 NET1 NET2 memr_model\n.END\n"
        # act
        topology = parse_netlist(netlist)
        # assert
        assert topology.devices[0].type_letter == "YMEMRISTOR"
        assert topology.devices[0].nodes == ["NET1", "NET2"]

    def test_yacc_device_nodes(self):
        # arrange — YACC: acceleration velocity displacement
        netlist = "Title\nYACC1 ACC VEL DISP yacc_model\n.END\n"
        # act
        topology = parse_netlist(netlist)
        # assert
        assert topology.devices[0].type_letter == "YACC"
        assert topology.devices[0].nodes == ["ACC", "VEL", "DISP"]

    def test_ypde_device_recognised(self):
        # arrange — YPDE: variable node count; recognised type, zero nodes extracted
        netlist = "Title\nYPDE1 NET1 NET2 pde_model\n.END\n"
        # act
        topology = parse_netlist(netlist)
        # assert
        assert topology.devices[0].type_letter == "YPDE"

    def test_global_nodes_from_directive(self):
        # arrange
        netlist = "Title\n.GLOBAL VDD VSS\n.END\n"
        # act
        topology = parse_netlist(netlist)
        # assert
        assert "VDD" in topology.global_nodes
        assert "VSS" in topology.global_nodes

    def test_global_node_dollar_g_prefix_top_level(self):
        # arrange — $G-prefixed node referenced in a top-level device
        netlist = "Title\nR1 $G_VDD GND 1k\n.END\n"
        # act
        topology = parse_netlist(netlist)
        # assert
        assert "$G_VDD" in topology.global_nodes

    def test_global_node_dollar_g_prefix_inside_subcircuit(self):
        # arrange — $G-prefixed node referenced inside a .SUBCKT block
        netlist = "Title\n.SUBCKT myblock IN OUT\nR1 IN $G_VDD 1k\n.ENDS\n.END\n"
        # act
        topology = parse_netlist(netlist)
        # assert
        assert "$G_VDD" in topology.global_nodes

    def test_global_directive_and_dollar_g_coexist(self):
        # arrange — both sources of global nodes present in the same netlist
        netlist = "Title\n.GLOBAL VDD\nR1 $G_REF GND 1k\n.END\n"
        # act
        topology = parse_netlist(netlist)
        # assert
        assert "VDD" in topology.global_nodes
        assert "$G_REF" in topology.global_nodes

    def test_subcircuit_definition_recorded(self):
        # arrange
        netlist = "Title\n.SUBCKT opamp IN OUT VCC GND\nR1 IN OUT 1k\n.ENDS\n.END\n"
        # act
        topology = parse_netlist(netlist)
        # assert
        assert "OPAMP" in topology.subcircuit_definitions

    def test_subcircuit_definition_ports(self):
        # arrange
        netlist = "Title\n.SUBCKT opamp IN OUT VCC GND\n.ENDS\n.END\n"
        # act
        topology = parse_netlist(netlist)
        # assert
        subckt = topology.subcircuit_definitions["OPAMP"]
        assert subckt.ports == ["IN", "OUT", "VCC", "GND"]

    def test_subcircuit_definition_with_params_keyword(self):
        # arrange — PARAMS: keyword terminates the port list
        netlist = "Title\n.SUBCKT myblock IN OUT PARAMS: gain=1\n.ENDS\n.END\n"
        # act
        topology = parse_netlist(netlist)
        # assert
        subckt = topology.subcircuit_definitions["MYBLOCK"]
        assert subckt.ports == ["IN", "OUT"]

    def test_subcircuit_devices_separate_from_top_level(self):
        # arrange — R1 inside subcircuit must not appear in top-level device list
        netlist = "Title\n.SUBCKT myblock IN OUT\nR1 IN OUT 1k\n.ENDS\nR2 A B 2k\n.END\n"
        # act
        topology = parse_netlist(netlist)
        # assert
        top_names = [d.name for d in topology.devices]
        assert "R1" not in top_names
        assert "R2" in top_names

    def test_subcircuit_internal_devices_stored_in_definition(self):
        # arrange
        netlist = "Title\n.SUBCKT myblock IN OUT\nC1 IN OUT 10p\n.ENDS\n.END\n"
        # act
        topology = parse_netlist(netlist)
        # assert
        subckt = topology.subcircuit_definitions["MYBLOCK"]
        assert len(subckt.devices) == 1
        assert subckt.devices[0].name == "C1"

    def test_subcircuit_nodes_not_in_top_level_nodes(self):
        # arrange — INTERNAL_NODE must not appear in top-level nodes set
        netlist = "Title\n.SUBCKT myblock IN OUT\nR1 IN INTERNAL_NODE 1k\nR2 INTERNAL_NODE OUT 1k\n.ENDS\n.END\n"
        # act
        topology = parse_netlist(netlist)
        # assert
        assert "INTERNAL_NODE" not in topology.nodes

    def test_nodes_set_contains_all_unique_top_level_nodes(self):
        # arrange
        netlist = "Title\nR1 A B 1k\nR2 B C 2k\nR3 C A 3k\n.END\n"
        # act
        topology = parse_netlist(netlist)
        # assert
        assert topology.nodes == {"A", "B", "C"}

    def test_duplicate_node_names_stored_only_once(self):
        # arrange — VCC and GND both appear in two devices
        netlist = "Title\nR1 VCC GND 1k\nC1 VCC GND 100n\n.END\n"
        # act
        topology = parse_netlist(netlist)
        # assert — set deduplication keeps exactly two unique node names
        assert len(topology.nodes) == 2

    def test_nodes_set_excludes_k_device_references(self):
        # arrange
        netlist = "Title\nL1 A B 1u\nL2 C D 1u\nK1 L1 L2 0.5\n.END\n"
        # act
        topology = parse_netlist(netlist)
        # assert — K device tokens reference inductor names, not circuit nodes
        assert "L1" not in topology.nodes
        assert "L2" not in topology.nodes

    def test_complete_kicad_netlist(self):
        # arrange — representative circuit with multiple device types
        netlist = ("* KiCad Schematic Netlist Version 4\n"
                   "* Generated by Eeschema\n"
                   "R1 NET1 GND 10k\n"
                   "C1 VCC GND 100n\n"
                   "L1 IN NET1 1u\n"
                   "D1 NET1 VCC 1N4148\n"
                   "Q1 NET2 NET1 GND NPN\n"
                   "M1 NET3 NET2 GND GND NMOS\n"
                   "V1 VCC GND DC 5\n"
                   "X1 IN NET1 VCC GND opamp\n"
                   ".GLOBAL VCC GND\n"
                   ".END\n")
        # act
        topology = parse_netlist(netlist)
        # assert
        assert topology.title == "* KiCad Schematic Netlist Version 4"
        # 8 devices: R1, C1, L1, D1, Q1, M1, V1, X1
        assert len(topology.devices) == 8
        assert "NET1" in topology.nodes
        assert "GND" in topology.nodes
        assert "VCC" in topology.nodes
        assert "VCC" in topology.global_nodes
        assert "GND" in topology.global_nodes

    def test_multiple_device_names_collected(self):
        # arrange
        netlist = "Title\nR1 A B 1k\nR2 B C 2k\nR3 C D 3k\n.END\n"
        # act
        topology = parse_netlist(netlist)
        # assert
        names = [d.name for d in topology.devices]
        assert "R1" in names
        assert "R2" in names
        assert "R3" in names

    def test_return_type_is_netlist_topology(self):
        # arrange
        netlist = "Title\n.END\n"
        # act
        result = parse_netlist(netlist)
        # assert
        assert isinstance(result, NetlistTopology)

    def test_device_is_dataclass_instance(self):
        # arrange
        netlist = "Title\nR1 A B 1k\n.END\n"
        # act
        topology = parse_netlist(netlist)
        # assert
        assert isinstance(topology.devices[0], Device)

    def test_directive_extraction(self):
        # arrange
        netlist = "Title\n.OP\n.PRINT DC V(1)\n.SAVE TYPE=IC\n.NODESET V(2)=5\n.END\n"
        # act
        topology = parse_netlist(netlist)
        # assert
        assert ".OP" in topology.directives
        assert ".PRINT DC V(1)" in topology.directives
        assert ".SAVE TYPE=IC" in topology.directives
        assert ".NODESET V(2)=5" in topology.directives
        assert len(topology.directives) == 4

    def test_directive_extraction_ignores_non_simulation_directives(self):
        # arrange
        netlist = "Title\n.MODEL M1 NMOS\n.SUBCKT S1 A B\n.ENDS\n.END\n"
        # act
        topology = parse_netlist(netlist)
        # assert
        assert len(topology.directives) == 0
