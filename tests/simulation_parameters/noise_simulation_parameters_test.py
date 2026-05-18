from simulation_parameters import NoiseSimulationParameters


class TestNoiseSimulationParameters:

    def test_lin_directive(self):
        # arrange
        params = NoiseSimulationParameters(output_node="5", source_name="V1", sweep_mode="LIN", points="100", start="1", end="1MEG")
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".NOISE V(5) V1 LIN 100 1 1MEG"]

    def test_with_ref_node(self):
        # arrange
        params = NoiseSimulationParameters(output_node="5", ref_node="3", source_name="V1", sweep_mode="LIN", points="100", start="1", end="1MEG")
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".NOISE V(5,3) V1 LIN 100 1 1MEG"]

    def test_dec_directive(self):
        # arrange
        params = NoiseSimulationParameters(output_node="out", source_name="Vin", sweep_mode="DEC", points="10", start="1k", end="100MEG")
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".NOISE V(out) Vin DEC 10 1k 100MEG"]

    def test_data_directive(self):
        # arrange
        params = NoiseSimulationParameters(output_node="out", source_name="Vin", sweep_mode="DATA", data_table_name="myTable")
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".NOISE V(out) Vin DATA=myTable"]

    def test_replace_ground(self):
        # arrange
        params = NoiseSimulationParameters(output_node="5", source_name="V1", sweep_mode="LIN", points="10", start="1", end="1MEG", replace_ground=True)
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".PREPROCESS REPLACEGROUND TRUE", ".NOISE V(5) V1 LIN 10 1 1MEG"]


class TestNoiseFromXyceDirectives:

    def test_parses_lin_sweep(self):
        # arrange / act
        params = NoiseSimulationParameters.from_xyce_directives([".NOISE V(5) V1 LIN 100 1 1MEG"])
        # assert
        assert params is not None
        assert params.output_node == "5"
        assert params.ref_node == ""
        assert params.source_name == "V1"
        assert params.sweep_mode == "LIN"
        assert params.points == "100"
        assert params.start == "1"
        assert params.end == "1MEG"

    def test_parses_output_with_ref_node(self):
        # arrange / act
        params = NoiseSimulationParameters.from_xyce_directives([".NOISE V(5,3) V1 LIN 100 1 1MEG"])
        # assert
        assert params is not None
        assert params.output_node == "5"
        assert params.ref_node == "3"

    def test_parses_dec_sweep(self):
        # arrange / act
        params = NoiseSimulationParameters.from_xyce_directives([".NOISE V(out) Vin DEC 10 1k 100MEG"])
        # assert
        assert params is not None
        assert params.sweep_mode == "DEC"
        assert params.points == "10"
        assert params.start == "1k"
        assert params.end == "100MEG"

    def test_parses_data_sweep(self):
        # arrange / act
        params = NoiseSimulationParameters.from_xyce_directives([".NOISE V(out) Vin DATA=myTable"])
        # assert
        assert params is not None
        assert params.sweep_mode == "DATA"
        assert params.data_table_name == "myTable"

    def test_parses_replaceground_true(self):
        # arrange / act
        params = NoiseSimulationParameters.from_xyce_directives([".NOISE V(5) V1 LIN 10 1 1MEG", ".PREPROCESS REPLACEGROUND TRUE"])
        # assert
        assert params is not None
        assert params.replace_ground is True

    def test_empty_directives_returns_none(self):
        # arrange / act
        params = NoiseSimulationParameters.from_xyce_directives([])
        # assert
        assert params is None

    def test_non_noise_directives_return_none(self):
        # arrange / act
        params = NoiseSimulationParameters.from_xyce_directives([".TRAN 1ns 1ms"])
        # assert
        assert params is None
