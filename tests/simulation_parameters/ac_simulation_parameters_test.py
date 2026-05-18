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
