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

    def test_harmonics_directive(self):
        # arrange
        params = HbSimulationParameters(frequencies=("1MEG", "2MEG"), harmonics=(15, 12), replace_ground=False)
        # act
        directives = params.to_xyce_directives()
        # assert
        assert ".HB 1MEG 2MEG" in directives
        assert ".OPTIONS HBINT NUMFREQ=15,12" in directives

    def test_tahb_directive(self):
        # arrange
        params = HbSimulationParameters(frequencies=("1MEG",), tahb=2, replace_ground=False)
        # act
        directives = params.to_xyce_directives()
        # assert
        assert ".OPTIONS HBINT TAHB=2" in directives

    def test_selectharms_directive(self):
        # arrange
        params = HbSimulationParameters(frequencies=("1MEG",), selectharms="box", replace_ground=False)
        # act
        directives = params.to_xyce_directives()
        # assert
        assert ".OPTIONS HBINT SELECTHARMS=box" in directives

    def test_startup_periods_directive(self):
        # arrange
        params = HbSimulationParameters(frequencies=("1MEG",), startup_periods=5, replace_ground=False)
        # act
        directives = params.to_xyce_directives()
        # assert
        assert ".OPTIONS HBINT STARTUPPERIODS=5" in directives

    def test_combined_hbint_options(self):
        # arrange
        params = HbSimulationParameters(frequencies=("1MEG",), harmonics=(10,), tahb=1, selectharms="hybrid", startup_periods=0, replace_ground=False)
        # act
        directives = params.to_xyce_directives()
        # assert
        assert ".OPTIONS HBINT NUMFREQ=10 TAHB=1 SELECTHARMS=hybrid STARTUPPERIODS=0" in directives


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

    def test_parses_hbint_options(self):
        # arrange / act
        params = HbSimulationParameters.from_xyce_directives([".HB 1MEG", ".OPTIONS HBINT NUMFREQ=15 TAHB=2 SELECTHARMS=box STARTUPPERIODS=5"])
        # assert
        assert params is not None
        assert params.harmonics == (15,)
        assert params.tahb == 2
        assert params.selectharms == "box"
        assert params.startup_periods == 5

    def test_parses_nonlin_and_linsol_hb_options(self):
        # arrange / act
        params = HbSimulationParameters.from_xyce_directives([".HB 1MEG", ".OPTIONS NONLIN-HB ABSTOL=1e-9", ".OPTIONS LINSOL-HB TYPE=AZTECOO"])
        # assert
        assert params is not None
        assert params.nonlin_options == {"ABSTOL": "1e-9"}
        assert params.linsol_options == {"TYPE": "AZTECOO"}

    def test_serializes_nonlin_and_linsol_hb_options(self):
        # arrange
        params = HbSimulationParameters(frequencies=("1MEG",), nonlin_options={"ABSTOL": "1e-9"}, linsol_options={"TYPE": "AZTECOO"}, replace_ground=False)
        # act
        directives = params.to_xyce_directives()
        # assert
        assert ".OPTIONS NONLIN-HB ABSTOL=1e-9" in directives
        assert ".OPTIONS LINSOL-HB TYPE=AZTECOO" in directives

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


class TestReferenceGuideExamples:
    # reference guide examples from xyce_rg.txt section 2.1.13 (lines 1622-1623)

    def test_reference_guide_example_single_frequency(self):
        # arrange - .HB 1e4
        directive = ".HB 1e4"
        # act
        params = HbSimulationParameters.from_xyce_directives([directive])
        # assert
        assert params is not None
        assert params.frequencies == ("1e4",)
        # verify the directive contains the expected hb line
        directives = params.to_xyce_directives()
        assert ".HB 1e4" in directives

    def test_reference_guide_example_multiple_frequencies(self):
        # arrange - .hb 1e4 2e2
        directive = ".hb 1e4 2e2"
        # act
        params = HbSimulationParameters.from_xyce_directives([directive])
        # assert
        assert params is not None
        assert params.frequencies == ("1e4", "2e2")
        # verify the directive contains the expected hb line
        directives = params.to_xyce_directives()
        assert ".HB 1e4 2e2" in directives
