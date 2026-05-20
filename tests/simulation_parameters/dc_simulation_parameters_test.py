from simulation_parameters import DCSimulationParameters, PrintParameters


class TestPrintWildcards:

    def test_generic_wildcards_round_trip(self):
        # arrange — the three universal wildcards valid for every .PRINT DC statement
        wildcards = ("V(*)", "I(*)", "P(*)")
        print_params = PrintParameters(print_type="DC", output_variables=wildcards)
        params = DCSimulationParameters("LIN", "VIN", "0", "5", "0.1", replace_ground=False, print_parameters=print_params)
        # act
        directives = params.to_xyce_directives()
        reparsed = DCSimulationParameters.from_xyce_directives(directives)
        # assert — wildcards survive a full serialize/parse cycle
        assert reparsed.print_parameters is not None
        assert reparsed.print_parameters.print_type == "DC"
        assert reparsed.print_parameters.output_variables == wildcards

    def test_bjt_lead_wildcards_round_trip(self):
        # arrange — BJT lead current wildcards: IB, IC, IE, IS
        wildcards = ("IB(*)", "IC(*)", "IE(*)", "IS(*)")
        print_params = PrintParameters(print_type="DC", output_variables=wildcards)
        params = DCSimulationParameters("LIN", "VIN", "0", "5", "0.1", replace_ground=False, print_parameters=print_params)
        # act
        directives = params.to_xyce_directives()
        reparsed = DCSimulationParameters.from_xyce_directives(directives)
        # assert — all four BJT lead wildcards survive the cycle unchanged
        assert reparsed.print_parameters is not None
        assert reparsed.print_parameters.output_variables == wildcards

    def test_fet_lead_wildcards_round_trip(self):
        # arrange — FET lead current wildcards: IB, ID, IG, IS
        wildcards = ("IB(*)", "ID(*)", "IG(*)", "IS(*)")
        print_params = PrintParameters(print_type="DC", output_variables=wildcards)
        params = DCSimulationParameters("LIN", "VIN", "0", "5", "0.1", replace_ground=False, print_parameters=print_params)
        # act
        directives = params.to_xyce_directives()
        reparsed = DCSimulationParameters.from_xyce_directives(directives)
        # assert — all four FET lead wildcards survive the cycle unchanged
        assert reparsed.print_parameters is not None
        assert reparsed.print_parameters.output_variables == wildcards

    def test_w_star_normalizes_to_p_star_on_parse(self):
        # arrange — netlist contains PSpice-style W(*) power wildcard
        directives = [".DC VIN 0 5 0.1", ".PRINT DC W(*)"]
        # act
        params = DCSimulationParameters.from_xyce_directives(directives)
        # assert — W(*) is stored as P(*) at parse time; no W(*) survives
        assert params.print_parameters is not None
        assert "P(*)" in params.print_parameters.output_variables
        assert "W(*)" not in params.print_parameters.output_variables

    def test_print_directive_uses_dc_not_tran_type(self):
        # arrange — print_parameters with print_type="DC"
        print_params = PrintParameters(print_type="DC", output_variables=("V(*)",))
        params = DCSimulationParameters("LIN", "VIN", "0", "5", "0.1", replace_ground=False, print_parameters=print_params)
        # act
        directives = params.to_xyce_directives()
        # assert — emitted directive is .PRINT DC, never .PRINT TRAN
        assert any(d.startswith(".PRINT DC") for d in directives)
        assert not any(d.startswith(".PRINT TRAN") for d in directives)


class TestToXyceDirectivesLin:

    def test_lin_basic(self):
        # arrange
        params = DCSimulationParameters("LIN", "VIN", "-10", "15", "1")
        # act / assert
        assert params.to_xyce_directives() == [".PREPROCESS REPLACEGROUND TRUE", ".DC VIN -10 15 1"]

    def test_lin_fractional_step(self):
        # arrange
        params = DCSimulationParameters("LIN", "R1", "0", "3.5", "0.05")
        # act / assert
        assert params.to_xyce_directives() == [".PREPROCESS REPLACEGROUND TRUE", ".DC R1 0 3.5 0.05"]

    def test_lin_negative_step(self):
        # arrange
        params = DCSimulationParameters("LIN", "VIN", "5", "0", "-0.1")
        # act / assert
        assert params.to_xyce_directives() == [".PREPROCESS REPLACEGROUND TRUE", ".DC VIN 5 0 -0.1"]

    def test_lin_with_secondary_sweep(self):
        # arrange
        params = DCSimulationParameters("LIN", "R1", "0", "3.5", "0.05", secondary_variable="C1", secondary_start="0", secondary_stop="3.5", secondary_step="0.5")
        # act / assert
        assert params.to_xyce_directives() == [".PREPROCESS REPLACEGROUND TRUE", ".DC R1 0 3.5 0.05 C1 0 3.5 0.5"]

    def test_lin_no_secondary_when_variable_empty(self):
        # arrange
        params = DCSimulationParameters("LIN", "VIN", "0", "5", "0.1")
        # act / assert
        assert params.to_xyce_directives() == [".PREPROCESS REPLACEGROUND TRUE", ".DC VIN 0 5 0.1"]

    def test_lin_with_replace_ground(self):
        # arrange
        params = DCSimulationParameters("LIN", "VIN", "0", "5", "0.1", replace_ground=False)
        # act / assert
        assert params.to_xyce_directives() == [".DC VIN 0 5 0.1"]


class TestToXyceDirectivesDec:

    def test_dec_basic(self):
        # arrange
        params = DCSimulationParameters("DEC", "VIN", "1", "100", points="2")
        # act / assert
        assert params.to_xyce_directives() == [".PREPROCESS REPLACEGROUND TRUE", ".DC DEC VIN 1 100 2"]

    def test_dec_with_secondary_sweep(self):
        # arrange
        params = DCSimulationParameters("DEC", "VIN", "1", "100", points="2", secondary_variable="R1", secondary_start="1", secondary_stop="10", secondary_points="3")
        # act / assert
        assert params.to_xyce_directives() == [".PREPROCESS REPLACEGROUND TRUE", ".DC DEC VIN 1 100 2 R1 1 10 3"]

    def test_dec_no_secondary_when_variable_empty(self):
        # arrange
        params = DCSimulationParameters("DEC", "VIN", "1", "100", points="5")
        # act / assert
        assert params.to_xyce_directives() == [".PREPROCESS REPLACEGROUND TRUE", ".DC DEC VIN 1 100 5"]


class TestToXyceDirectivesOct:

    def test_oct_basic(self):
        # arrange
        params = DCSimulationParameters("OCT", "VIN", "0.125", "64", points="2")
        # act / assert
        assert params.to_xyce_directives() == [".PREPROCESS REPLACEGROUND TRUE", ".DC OCT VIN 0.125 64 2"]

    def test_oct_with_secondary_sweep(self):
        # arrange
        params = DCSimulationParameters("OCT", "VIN", "0.125", "64", points="2", secondary_variable="R1", secondary_start="1", secondary_stop="10", secondary_points="4")
        # act / assert
        assert params.to_xyce_directives() == [".PREPROCESS REPLACEGROUND TRUE", ".DC OCT VIN 0.125 64 2 R1 1 10 4"]

    def test_oct_no_secondary_when_variable_empty(self):
        # arrange
        params = DCSimulationParameters("OCT", "VIN", "0.125", "64", points="2")
        # act / assert
        assert params.to_xyce_directives() == [".PREPROCESS REPLACEGROUND TRUE", ".DC OCT VIN 0.125 64 2"]

    def test_oct_with_replace_ground(self):
        # arrange
        params = DCSimulationParameters("OCT", "VIN", "0.125", "64", points="2", replace_ground=False)
        # act / assert
        assert params.to_xyce_directives() == [".DC OCT VIN 0.125 64 2"]


class TestToXyceDirectivesList:

    def test_list_single_value(self):
        # arrange
        params = DCSimulationParameters("LIST", "TEMP", list_values=("27",))
        # act / assert
        assert params.to_xyce_directives() == [".PREPROCESS REPLACEGROUND TRUE", ".DC TEMP LIST 27"]

    def test_list_multiple_values(self):
        # arrange
        params = DCSimulationParameters("LIST", "TEMP", list_values=("10", "15", "18", "27", "33"))
        # act / assert
        assert params.to_xyce_directives() == [".PREPROCESS REPLACEGROUND TRUE", ".DC TEMP LIST 10 15 18 27 33"]

    def test_list_with_replace_ground(self):
        # arrange
        params = DCSimulationParameters("LIST", "VCC", list_values=("3.3", "5.0"), replace_ground=False)
        # act / assert
        assert params.to_xyce_directives() == [".DC VCC LIST 3.3 5.0"]


class TestToXyceDirectivesData:

    def test_data_directive(self):
        # arrange
        params = DCSimulationParameters("DATA", data_table_name="resistorValues")
        # act / assert
        assert params.to_xyce_directives() == [".PREPROCESS REPLACEGROUND TRUE", ".DC DATA=resistorValues"]

    def test_data_table_name_used_verbatim(self):
        # arrange
        params = DCSimulationParameters("DATA", data_table_name="myCustomTable")
        # act / assert
        assert params.to_xyce_directives() == [".PREPROCESS REPLACEGROUND TRUE", ".DC DATA=myCustomTable"]

    def test_data_with_replace_ground(self):
        # arrange
        params = DCSimulationParameters("DATA", data_table_name="myTable", replace_ground=False)
        # act / assert
        assert params.to_xyce_directives() == [".DC DATA=myTable"]


class TestFromXyceDirectives:

    def test_empty_directives_returns_none(self):
        # arrange / act
        params = DCSimulationParameters.from_xyce_directives([])
        # assert — no .DC directive means no match
        assert params is None

    def test_blank_directive_string_is_skipped(self):
        # arrange / act
        params = DCSimulationParameters.from_xyce_directives([""])
        # assert — blank lines do not crash and return None
        assert params is None

    def test_bare_dc_with_no_arguments_is_skipped(self):
        # arrange / act — ".DC" alone has no sweep spec; defaults should be unchanged
        params = DCSimulationParameters.from_xyce_directives([".DC"])
        # assert
        assert params.sweep_mode == "LIN"
        assert params.primary_variable == ""

    def test_non_dc_directives_are_ignored(self):
        # arrange / act
        params = DCSimulationParameters.from_xyce_directives([".TRAN 1ns 100ns", ".OP"])
        # assert — no .DC directive means None is returned
        assert params is None

    def test_replace_ground_true(self):
        # arrange / act — PREPROCESS without .DC returns None
        params = DCSimulationParameters.from_xyce_directives([".PREPROCESS REPLACEGROUND TRUE"])
        # assert
        assert params is None

    def test_replace_ground_false(self):
        # arrange / act — PREPROCESS without .DC returns None
        params = DCSimulationParameters.from_xyce_directives([".PREPROCESS REPLACEGROUND FALSE"])
        # assert
        assert params is None

    def test_replace_ground_true_with_dc(self):
        # arrange / act
        params = DCSimulationParameters.from_xyce_directives([".PREPROCESS REPLACEGROUND TRUE", ".DC VIN 0 5 1"])
        # assert
        assert params.replace_ground is True

    def test_replace_ground_false_with_dc(self):
        # arrange / act
        params = DCSimulationParameters.from_xyce_directives([".PREPROCESS REPLACEGROUND FALSE", ".DC VIN 0 5 1"])
        # assert
        assert params.replace_ground is False

    def test_lin_implicit(self):
        # arrange / act
        params = DCSimulationParameters.from_xyce_directives([".DC VIN -10 15 1"])
        # assert
        assert params.sweep_mode == "LIN"
        assert params.primary_variable == "VIN"
        assert params.start == "-10"
        assert params.stop == "15"
        assert params.step == "1"

    def test_lin_implicit_with_secondary(self):
        # arrange / act
        params = DCSimulationParameters.from_xyce_directives([".DC R1 0 3.5 0.05 C1 0 3.5 0.5"])
        # assert
        assert params.sweep_mode == "LIN"
        assert params.primary_variable == "R1"
        assert params.secondary_variable == "C1"
        assert params.secondary_start == "0"
        assert params.secondary_stop == "3.5"
        assert params.secondary_step == "0.5"

    def test_lin_explicit(self):
        # arrange / act
        params = DCSimulationParameters.from_xyce_directives([".DC LIN V1 5 25 5"])
        # assert
        assert params.sweep_mode == "LIN"
        assert params.primary_variable == "V1"
        assert params.start == "5"
        assert params.stop == "25"
        assert params.step == "5"

    def test_lin_explicit_with_secondary(self):
        # arrange / act
        params = DCSimulationParameters.from_xyce_directives([".DC LIN R1 0 3.5 0.05 C1 0 3.5 0.5"])
        # assert
        assert params.sweep_mode == "LIN"
        assert params.primary_variable == "R1"
        assert params.secondary_variable == "C1"
        assert params.secondary_start == "0"
        assert params.secondary_stop == "3.5"
        assert params.secondary_step == "0.5"

    def test_dec(self):
        # arrange / act
        params = DCSimulationParameters.from_xyce_directives([".DC DEC VIN 1 100 2"])
        # assert
        assert params.sweep_mode == "DEC"
        assert params.primary_variable == "VIN"
        assert params.start == "1"
        assert params.stop == "100"
        assert params.points == "2"

    def test_dec_with_secondary(self):
        # arrange / act
        params = DCSimulationParameters.from_xyce_directives([".DC DEC R1 100 10000 3 DEC VGS 0.001 1.0 2"])
        # assert
        assert params.sweep_mode == "DEC"
        assert params.primary_variable == "R1"
        assert params.secondary_variable == "VGS"
        assert params.secondary_start == "0.001"
        assert params.secondary_stop == "1.0"
        assert params.secondary_points == "2"

    def test_oct(self):
        # arrange / act
        params = DCSimulationParameters.from_xyce_directives([".DC OCT VIN 0.125 64 2"])
        # assert
        assert params.sweep_mode == "OCT"
        assert params.primary_variable == "VIN"
        assert params.start == "0.125"
        assert params.stop == "64"
        assert params.points == "2"

    def test_oct_with_secondary(self):
        # arrange / act
        params = DCSimulationParameters.from_xyce_directives([".DC OCT R1 0.015625 512 3 OCT C1 512 4096 1"])
        # assert
        assert params.sweep_mode == "OCT"
        assert params.primary_variable == "R1"
        assert params.secondary_variable == "C1"
        assert params.secondary_start == "512"
        assert params.secondary_stop == "4096"
        assert params.secondary_points == "1"

    def test_list(self):
        # arrange / act
        params = DCSimulationParameters.from_xyce_directives([".DC VIN LIST 1.0 2.0 5.0"])
        # assert
        assert params.sweep_mode == "LIST"
        assert params.primary_variable == "VIN"
        assert params.list_values == ("1.0", "2.0", "5.0")

    def test_data(self):
        # arrange / act
        params = DCSimulationParameters.from_xyce_directives([".DC DATA=resistorValues"])
        # assert
        assert params.sweep_mode == "DATA"
        assert params.data_table_name == "resistorValues"

    def test_replace_ground_combined_with_dc_directive(self):
        # arrange / act
        params = DCSimulationParameters.from_xyce_directives([".PREPROCESS REPLACEGROUND TRUE", ".DC VIN 0 5 0.1"])
        # assert — both directives parsed correctly
        assert params.replace_ground is True
        assert params.sweep_mode == "LIN"
        assert params.primary_variable == "VIN"

    def test_serializes_print_dc_directive(self):
        # arrange
        print_params = PrintParameters(print_type="DC", output_variables=("V(OUT)",))
        params = DCSimulationParameters("LIN", "VIN", "0", "5", "0.1", replace_ground=False, print_parameters=print_params)
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".DC VIN 0 5 0.1", ".PRINT DC V(OUT)"]

    def test_parses_print_dc_directive(self):
        # arrange / act
        params = DCSimulationParameters.from_xyce_directives([".DC VIN 0 5 0.1", ".PRINT DC V(OUT) I(V1)"])
        # assert
        assert params is not None
        assert params.print_parameters is not None
        assert params.print_parameters.print_type == "DC"
        assert params.print_parameters.output_variables == ("V(OUT)", "I(V1)")

    def test_ignores_non_dc_print_directive(self):
        # arrange / act
        params = DCSimulationParameters.from_xyce_directives([".DC VIN 0 5 0.1", ".PRINT TRAN V(OUT)"])
        # assert
        assert params is not None
        assert params.print_parameters is None


class TestDCFromXyceDirectivesMeasure:

    def test_parses_single_measure_directive(self):
        # arrange / act
        params = DCSimulationParameters.from_xyce_directives([".DC VIN 0 5 0.1", ".MEASURE DC vout_at_2v FIND V(OUT) WHEN VIN=2"])
        # assert
        assert len(params.measure_parameters) == 1
        assert params.measure_parameters[0].result_name == "vout_at_2v"
        assert params.measure_parameters[0].measure_type == "FIND"
        assert params.measure_parameters[0].analysis_type == "DC"
        assert params.measure_parameters[0].variable == "V(OUT)"

    def test_parses_multiple_measure_directives(self):
        # arrange / act
        params = DCSimulationParameters.from_xyce_directives([".DC VIN 0 5 0.1", ".MEASURE DC vout_at_2v FIND V(OUT) WHEN VIN=2", ".MEASURE DC vout_at_4v FIND V(OUT) WHEN VIN=4"])
        # assert
        assert len(params.measure_parameters) == 2
        assert params.measure_parameters[0].result_name == "vout_at_2v"
        assert params.measure_parameters[1].result_name == "vout_at_4v"

    def test_ignores_non_dc_measure_directive(self):
        # arrange / act
        params = DCSimulationParameters.from_xyce_directives([".DC VIN 0 5 0.1", ".MEASURE TRAN avg_out AVG V(OUT)"])
        # assert
        assert len(params.measure_parameters) == 0


class TestDCToXyceDirectivesMeasure:

    def test_emits_single_measure_directive(self):
        # arrange
        from simulation_parameters import MeasureEntry
        measure = MeasureEntry(result_name="vout_at_2v", measure_type="FIND", analysis_type="DC", variable="V(OUT)", when_variable="VIN", when_condition="=2")
        params = DCSimulationParameters("LIN", "VIN", "0", "5", "0.1", replace_ground=False, measure_parameters=(measure,))
        # act
        directives = params.to_xyce_directives()
        # assert
        assert any(d.startswith(".MEASURE DC vout_at_2v FIND V(OUT) WHEN VIN=2") for d in directives)

    def test_emits_multiple_measure_directives(self):
        # arrange
        from simulation_parameters import MeasureEntry
        measure1 = MeasureEntry(result_name="vout_at_2v", measure_type="FIND", analysis_type="DC", variable="V(OUT)", when_variable="VIN", when_condition="=2")
        measure2 = MeasureEntry(result_name="vout_at_4v", measure_type="FIND", analysis_type="DC", variable="V(OUT)", when_variable="VIN", when_condition="=4")
        params = DCSimulationParameters("LIN", "VIN", "0", "5", "0.1", replace_ground=False, measure_parameters=(measure1, measure2))
        # act
        directives = params.to_xyce_directives()
        # assert
        assert any(d.startswith(".MEASURE DC vout_at_2v FIND V(OUT) WHEN VIN=2") for d in directives)
        assert any(d.startswith(".MEASURE DC vout_at_4v FIND V(OUT) WHEN VIN=4") for d in directives)

    def test_measure_round_trip(self):
        # arrange
        from simulation_parameters import MeasureEntry
        measure = MeasureEntry(result_name="vout_at_2v", measure_type="FIND", analysis_type="DC", variable="V(OUT)", when_variable="VIN", when_condition="=2")
        params = DCSimulationParameters("LIN", "VIN", "0", "5", "0.1", replace_ground=False, measure_parameters=(measure,))
        # act
        directives = params.to_xyce_directives()
        reparsed = DCSimulationParameters.from_xyce_directives(directives)
        # assert
        assert len(reparsed.measure_parameters) == 1
        assert reparsed.measure_parameters[0].result_name == "vout_at_2v"
        assert reparsed.measure_parameters[0].measure_type == "FIND"
        assert reparsed.measure_parameters[0].analysis_type == "DC"


class TestReferenceGuideExamples:
    # reference guide examples from xyce_rg.txt section 2.1.3 (lines 904-984)

    def test_reference_guide_example_lin_sweep(self):
        # arrange - .DC LIN V1 5 25 5
        directive = ".DC LIN V1 5 25 5"
        # act
        params = DCSimulationParameters.from_xyce_directives([directive])
        # assert
        assert params is not None
        assert params.sweep_mode == "LIN"
        assert params.primary_variable == "V1"
        assert params.start == "5"
        assert params.stop == "25"
        assert params.step == "5"
        # verify the directive contains the expected dc line
        directives = params.to_xyce_directives()
        dc_line = next(d for d in directives if d.startswith(".DC"))
        assert "V1 5 25 5" in dc_line

    def test_reference_guide_example_lin_implicit(self):
        # arrange - .DC VIN -10 15 1
        directive = ".DC VIN -10 15 1"
        # act
        params = DCSimulationParameters.from_xyce_directives([directive])
        # assert
        assert params is not None
        assert params.sweep_mode == "LIN"
        assert params.primary_variable == "VIN"
        assert params.start == "-10"
        assert params.stop == "15"
        assert params.step == "1"
        # verify the directive contains the expected dc line
        directives = params.to_xyce_directives()
        assert ".DC VIN -10 15 1" in directives

    def test_reference_guide_example_lin_with_secondary(self):
        # arrange - .DC R1 0 3.5 0.05 C1 0 3.5 0.5
        directive = ".DC R1 0 3.5 0.05 C1 0 3.5 0.5"
        # act
        params = DCSimulationParameters.from_xyce_directives([directive])
        # assert
        assert params is not None
        assert params.sweep_mode == "LIN"
        assert params.primary_variable == "R1"
        assert params.secondary_variable == "C1"
        # verify the directive contains the expected dc line
        directives = params.to_xyce_directives()
        assert ".DC R1 0 3.5 0.05 C1 0 3.5 0.5" in directives

    def test_reference_guide_example_dec_sweep(self):
        # arrange - .DC DEC VIN 1 100 2
        directive = ".DC DEC VIN 1 100 2"
        # act
        params = DCSimulationParameters.from_xyce_directives([directive])
        # assert
        assert params is not None
        assert params.sweep_mode == "DEC"
        assert params.primary_variable == "VIN"
        assert params.start == "1"
        assert params.stop == "100"
        assert params.points == "2"
        # verify the directive contains the expected dc line
        directives = params.to_xyce_directives()
        assert ".DC DEC VIN 1 100 2" in directives

    def test_reference_guide_example_dec_with_secondary(self):
        # arrange - .DC DEC R1 100 10000 3 DEC VGS 0.001 1.0 2
        directive = ".DC DEC R1 100 10000 3 DEC VGS 0.001 1.0 2"
        # act
        params = DCSimulationParameters.from_xyce_directives([directive])
        # assert
        assert params is not None
        assert params.sweep_mode == "DEC"
        assert params.primary_variable == "R1"
        assert params.secondary_variable == "VGS"
        # verify the directive contains the expected dc line
        directives = params.to_xyce_directives()
        dc_line = next(d for d in directives if d.startswith(".DC"))
        assert "R1 100 10000 3" in dc_line
        assert "VGS 0.001 1.0 2" in dc_line

    def test_reference_guide_example_oct_sweep(self):
        # arrange - .DC OCT VIN 0.125 64 2
        directive = ".DC OCT VIN 0.125 64 2"
        # act
        params = DCSimulationParameters.from_xyce_directives([directive])
        # assert
        assert params is not None
        assert params.sweep_mode == "OCT"
        assert params.primary_variable == "VIN"
        assert params.start == "0.125"
        assert params.stop == "64"
        assert params.points == "2"
        # verify the directive contains the expected dc line
        directives = params.to_xyce_directives()
        assert ".DC OCT VIN 0.125 64 2" in directives

    def test_reference_guide_example_oct_with_secondary(self):
        # arrange - .DC OCT R1 0.015625 512 3 OCT C1 512 4096 1
        directive = ".DC OCT R1 0.015625 512 3 OCT C1 512 4096 1"
        # act
        params = DCSimulationParameters.from_xyce_directives([directive])
        # assert
        assert params is not None
        assert params.sweep_mode == "OCT"
        assert params.primary_variable == "R1"
        assert params.secondary_variable == "C1"
        # verify the directive contains the expected dc line
        directives = params.to_xyce_directives()
        dc_line = next(d for d in directives if d.startswith(".DC"))
        assert "R1 0.015625 512 3" in dc_line
        assert "C1 512 4096 1" in dc_line

    def test_reference_guide_example_list_sweep(self):
        # arrange - .DC VIN LIST 1.0 2.0 5.0 6.0 10.0
        directive = ".DC VIN LIST 1.0 2.0 5.0 6.0 10.0"
        # act
        params = DCSimulationParameters.from_xyce_directives([directive])
        # assert
        assert params is not None
        assert params.sweep_mode == "LIST"
        assert params.primary_variable == "VIN"
        assert params.list_values == ("1.0", "2.0", "5.0", "6.0", "10.0")
        # verify the directive contains the expected dc line
        directives = params.to_xyce_directives()
        assert ".DC VIN LIST 1.0 2.0 5.0 6.0 10.0" in directives

    def test_reference_guide_example_data_sweep(self):
        # arrange - .DC DATA=resistorValues
        directive = ".DC DATA=resistorValues"
        # act
        params = DCSimulationParameters.from_xyce_directives([directive])
        # assert
        assert params is not None
        assert params.sweep_mode == "DATA"
        assert params.data_table_name == "resistorValues"
        # verify the directive contains the expected dc line
        directives = params.to_xyce_directives()
        assert ".DC DATA=resistorValues" in directives
