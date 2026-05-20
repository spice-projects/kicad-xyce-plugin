from simulation_parameters import AcSimulationParameters, PrintParameters


class TestAcSimulationParameters:

    def test_lin_directive_default(self):
        # arrange
        params = AcSimulationParameters(sweep_mode="LIN", points="100", start="1", end="1MEG", replace_ground=False)
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".AC LIN 100 1 1MEG"]

    def test_dec_directive(self):
        # arrange
        params = AcSimulationParameters(sweep_mode="DEC", points="10", start="1k", end="100MEG", replace_ground=False)
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".AC DEC 10 1k 100MEG"]

    def test_oct_directive(self):
        # arrange
        params = AcSimulationParameters(sweep_mode="OCT", points="5", start="1", end="1MEG", replace_ground=False)
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".AC OCT 5 1 1MEG"]

    def test_data_directive(self):
        # arrange
        params = AcSimulationParameters(sweep_mode="DATA", data_table_name="myTable", replace_ground=False)
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".AC DATA=myTable"]

    def test_replace_ground(self):
        # arrange
        params = AcSimulationParameters(sweep_mode="LIN", points="10", start="1", end="1MEG", replace_ground=True)
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".PREPROCESS REPLACEGROUND TRUE", ".AC LIN 10 1 1MEG"]


class TestAcFromXyceDirectives:

    def test_parses_lin_sweep(self):
        # arrange / act
        params = AcSimulationParameters.from_xyce_directives([".AC LIN 100 1 1MEG"])
        # assert
        assert params is not None
        assert params.sweep_mode == "LIN"
        assert params.points == "100"
        assert params.start == "1"
        assert params.end == "1MEG"

    def test_parses_dec_sweep(self):
        # arrange / act
        params = AcSimulationParameters.from_xyce_directives([".AC DEC 10 1k 100MEG"])
        # assert
        assert params is not None
        assert params.sweep_mode == "DEC"
        assert params.points == "10"
        assert params.start == "1k"
        assert params.end == "100MEG"

    def test_parses_oct_sweep(self):
        # arrange / act
        params = AcSimulationParameters.from_xyce_directives([".AC OCT 5 1 1MEG"])
        # assert
        assert params is not None
        assert params.sweep_mode == "OCT"

    def test_parses_data_sweep(self):
        # arrange / act
        params = AcSimulationParameters.from_xyce_directives([".AC DATA=myTable"])
        # assert
        assert params is not None
        assert params.sweep_mode == "DATA"
        assert params.data_table_name == "myTable"

    def test_parses_implicit_lin_sweep(self):
        # arrange / act (no LIN keyword)
        params = AcSimulationParameters.from_xyce_directives([".AC 100 1 1MEG"])
        # assert
        assert params is not None
        assert params.sweep_mode == "LIN"
        assert params.points == "100"
        assert params.start == "1"
        assert params.end == "1MEG"

    def test_parses_replaceground_true(self):
        # arrange / act
        params = AcSimulationParameters.from_xyce_directives([".AC LIN 10 1 1MEG", ".PREPROCESS REPLACEGROUND TRUE"])
        # assert
        assert params is not None
        assert params.replace_ground is True

    def test_empty_directives_returns_none(self):
        # arrange / act
        params = AcSimulationParameters.from_xyce_directives([])
        # assert
        assert params is None

    def test_non_ac_directives_return_none(self):
        # arrange / act
        params = AcSimulationParameters.from_xyce_directives([".TRAN 1ns 1ms"])
        # assert
        assert params is None

    def test_serializes_print_ac_directive(self):
        # arrange
        print_params = PrintParameters(print_type="AC", output_variables=("V(OUT)",))
        params = AcSimulationParameters(sweep_mode="LIN", points="10", start="1", end="1MEG", replace_ground=False, print_parameters=print_params)
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".AC LIN 10 1 1MEG", ".PRINT AC V(OUT)"]

    def test_parses_print_ac_directive(self):
        # arrange / act
        params = AcSimulationParameters.from_xyce_directives([".AC LIN 10 1 1MEG", ".PRINT AC V(OUT) I(V1)"])
        # assert
        assert params is not None
        assert params.print_parameters is not None
        assert params.print_parameters.print_type == "AC"
        assert params.print_parameters.output_variables == ("V(OUT)", "I(V1)")

    def test_ignores_non_ac_print_directive(self):
        # arrange / act
        params = AcSimulationParameters.from_xyce_directives([".AC LIN 10 1 1MEG", ".PRINT TRAN V(OUT)"])
        # assert
        assert params is not None
        assert params.print_parameters is None


class TestAcFromXyceDirectivesMeasure:

    def test_parses_single_measure_directive(self):
        # arrange / act
        params = AcSimulationParameters.from_xyce_directives([".AC LIN 10 1 1MEG", ".MEASURE AC gain_at_1k FIND V(OUT) AT=1k"])
        # assert
        assert len(params.measure_parameters) == 1
        assert params.measure_parameters[0].result_name == "gain_at_1k"
        assert params.measure_parameters[0].measure_type == "FIND"
        assert params.measure_parameters[0].analysis_type == "AC"
        assert params.measure_parameters[0].variable == "V(OUT)"

    def test_parses_multiple_measure_directives(self):
        # arrange / act
        params = AcSimulationParameters.from_xyce_directives([".AC LIN 10 1 1MEG", ".MEASURE AC gain_at_1k FIND V(OUT) AT=1k", ".MEASURE AC bandwidth WHEN V(OUT)=0.707 CROSS=1"])
        # assert
        assert len(params.measure_parameters) == 2
        assert params.measure_parameters[0].result_name == "gain_at_1k"
        assert params.measure_parameters[1].result_name == "bandwidth"

    def test_ignores_non_ac_measure_directive(self):
        # arrange / act
        params = AcSimulationParameters.from_xyce_directives([".AC LIN 10 1 1MEG", ".MEASURE TRAN avg_out AVG V(OUT)"])
        # assert
        assert len(params.measure_parameters) == 0


class TestAcToXyceDirectivesMeasure:

    def test_emits_single_measure_directive(self):
        # arrange
        from simulation_parameters import MeasureEntry
        measure = MeasureEntry(result_name="gain_at_1k", measure_type="FIND", analysis_type="AC", variable="V(OUT)", at_val="1k")
        params = AcSimulationParameters(sweep_mode="LIN", points="10", start="1", end="1MEG", replace_ground=False, measure_parameters=(measure,))
        # act
        directives = params.to_xyce_directives()
        # assert
        assert any(d.startswith(".MEASURE AC gain_at_1k FIND V(OUT) AT=1k") for d in directives)

    def test_emits_multiple_measure_directives(self):
        # arrange
        from simulation_parameters import MeasureEntry
        measure1 = MeasureEntry(result_name="gain_at_1k", measure_type="FIND", analysis_type="AC", variable="V(OUT)", at_val="1k")
        measure2 = MeasureEntry(result_name="bandwidth", measure_type="WHEN", analysis_type="AC", variable="", when_variable="V(OUT)", when_condition="=0.707", cross_val="1")
        params = AcSimulationParameters(sweep_mode="LIN", points="10", start="1", end="1MEG", replace_ground=False, measure_parameters=(measure1, measure2))
        # act
        directives = params.to_xyce_directives()
        # assert
        assert any(d.startswith(".MEASURE AC gain_at_1k FIND V(OUT) AT=1k") for d in directives)
        assert any(d.startswith(".MEASURE AC bandwidth WHEN V(OUT)=0.707 CROSS=1") for d in directives)

    def test_measure_round_trip(self):
        # arrange
        from simulation_parameters import MeasureEntry
        measure = MeasureEntry(result_name="gain_at_1k", measure_type="FIND", analysis_type="AC", variable="V(OUT)", at_val="1k")
        params = AcSimulationParameters(sweep_mode="LIN", points="10", start="1", end="1MEG", replace_ground=False, measure_parameters=(measure,))
        # act
        directives = params.to_xyce_directives()
        reparsed = AcSimulationParameters.from_xyce_directives(directives)
        # assert
        assert len(reparsed.measure_parameters) == 1
        assert reparsed.measure_parameters[0].result_name == "gain_at_1k"
        assert reparsed.measure_parameters[0].measure_type == "FIND"
        assert reparsed.measure_parameters[0].analysis_type == "AC"
        assert reparsed.measure_parameters[0].variable == "V(OUT)"
        assert reparsed.measure_parameters[0].at_val == "1k"


class TestReferenceGuideExamples:
    # reference guide examples from xyce_rg.txt section 2.1.1 (lines 774-780)

    def test_reference_guide_example_lin_sweep(self):
        # arrange - .AC LIN 101 100Hz 200Hz
        directive = ".AC LIN 101 100Hz 200Hz"
        # act
        params = AcSimulationParameters.from_xyce_directives([directive])
        # assert
        assert params is not None
        assert params.sweep_mode == "LIN"
        assert params.points == "101"
        assert params.start == "100Hz"
        assert params.end == "200Hz"
        # verify the directive contains the expected ac line
        directives = params.to_xyce_directives()
        assert ".AC LIN 101 100Hz 200Hz" in directives

    def test_reference_guide_example_oct_sweep(self):
        # arrange - .AC OCT 10 1kHz 16kHz
        directive = ".AC OCT 10 1kHz 16kHz"
        # act
        params = AcSimulationParameters.from_xyce_directives([directive])
        # assert
        assert params is not None
        assert params.sweep_mode == "OCT"
        assert params.points == "10"
        assert params.start == "1kHz"
        assert params.end == "16kHz"
        # verify the directive contains the expected ac line
        directives = params.to_xyce_directives()
        assert ".AC OCT 10 1kHz 16kHz" in directives

    def test_reference_guide_example_dec_sweep(self):
        # arrange - .AC DEC 20 1MEG 100MEG
        directive = ".AC DEC 20 1MEG 100MEG"
        # act
        params = AcSimulationParameters.from_xyce_directives([directive])
        # assert
        assert params is not None
        assert params.sweep_mode == "DEC"
        assert params.points == "20"
        assert params.start == "1MEG"
        assert params.end == "100MEG"
        # verify the directive contains the expected ac line
        directives = params.to_xyce_directives()
        assert ".AC DEC 20 1MEG 100MEG" in directives

    def test_reference_guide_example_data_sweep(self):
        # arrange - .AC DATA=<table name>
        directive = ".AC DATA=myTable"
        # act
        params = AcSimulationParameters.from_xyce_directives([directive])
        # assert
        assert params is not None
        assert params.sweep_mode == "DATA"
        assert params.data_table_name == "myTable"
        # verify the directive contains the expected ac line
        directives = params.to_xyce_directives()
        assert ".AC DATA=myTable" in directives
