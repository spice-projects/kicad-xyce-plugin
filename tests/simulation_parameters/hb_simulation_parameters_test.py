from simulation_parameters import HbSimulationParameters, PrintParameters


class TestHbSimulationParameters:

    def test_single_frequency_directive(self):
        # arrange
        params = HbSimulationParameters(frequencies=("1MEG",), replace_ground=False)
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".HB 1MEG"]

    def test_multiple_frequencies_directive(self):
        # arrange
        params = HbSimulationParameters(frequencies=("1MEG", "2MEG", "500K"), replace_ground=False)
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".HB 1MEG 2MEG 500K"]

    def test_replace_ground(self):
        # arrange
        params = HbSimulationParameters(frequencies=("1MEG",), replace_ground=True)
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".PREPROCESS REPLACEGROUND TRUE", ".HB 1MEG"]


class TestHbFromXyceDirectives:

    def test_parses_single_frequency(self):
        # arrange / act
        params = HbSimulationParameters.from_xyce_directives([".HB 1MEG"])
        # assert
        assert params is not None
        assert params.frequencies == ("1MEG",)

    def test_parses_multiple_frequencies(self):
        # arrange / act
        params = HbSimulationParameters.from_xyce_directives([".HB 1MEG 2MEG 500K"])
        # assert
        assert params is not None
        assert params.frequencies == ("1MEG", "2MEG", "500K")

    def test_parses_replaceground_true(self):
        # arrange / act
        params = HbSimulationParameters.from_xyce_directives([".HB 1MEG", ".PREPROCESS REPLACEGROUND TRUE"])
        # assert
        assert params is not None
        assert params.replace_ground is True

    def test_empty_directives_returns_none(self):
        # arrange / act
        params = HbSimulationParameters.from_xyce_directives([])
        # assert
        assert params is None

    def test_non_hb_directives_return_none(self):
        # arrange / act
        params = HbSimulationParameters.from_xyce_directives([".TRAN 1ns 1ms"])
        # assert
        assert params is None

    def test_serializes_print_hb_directive(self):
        # arrange
        print_params = PrintParameters(print_type="HB", output_variables=("V(1)", "I(V1)"))
        params = HbSimulationParameters(frequencies=("1MEG",), replace_ground=False, print_parameters=print_params)
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".HB 1MEG", ".PRINT HB V(1) I(V1)"]

    def test_parses_print_hb_directive(self):
        # arrange / act
        params = HbSimulationParameters.from_xyce_directives([".HB 1MEG", ".PRINT HB V(1) I(V1)"])
        # assert
        assert params is not None
        assert params.print_parameters is not None
        assert params.print_parameters.print_type == "HB"
        assert params.print_parameters.output_variables == ("V(1)", "I(V1)")

    def test_ignores_non_hb_print_directive(self):
        # arrange / act
        params = HbSimulationParameters.from_xyce_directives([".HB 1MEG", ".PRINT TRAN V(1)"])
        # assert
        assert params is not None
        assert params.print_parameters is None
