from simulation_parameters import DeviceNoiseOperator, NoiseSimulationParameters, PrintParameters


class TestDeviceNoiseOperator:

    def test_create_device_noise_operator(self):
        # arrange
        operator = DeviceNoiseOperator(device_name="R1", operator_type="DNI", noise_source="")
        # act/assert
        assert operator.device_name == "R1"
        assert operator.operator_type == "DNI"
        assert operator.noise_source == ""

    def test_create_device_noise_operator_with_noise_source(self):
        # arrange
        operator = DeviceNoiseOperator(device_name="Q2", operator_type="DNO", noise_source="FLICKER")
        # act/assert
        assert operator.device_name == "Q2"
        assert operator.operator_type == "DNO"
        assert operator.noise_source == "FLICKER"


class TestNoiseSimulationParameters:

    def test_lin_directive(self):
        # arrange
        params = NoiseSimulationParameters(output_node="5", source_name="V1", sweep_mode="LIN", points="100", start="1", end="1MEG", replace_ground=False)
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".NOISE V(5) V1 LIN 100 1 1MEG"]

    def test_with_ref_node(self):
        # arrange
        params = NoiseSimulationParameters(output_node="5", ref_node="3", source_name="V1", sweep_mode="LIN", points="100", start="1", end="1MEG", replace_ground=False)
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".NOISE V(5,3) V1 LIN 100 1 1MEG"]

    def test_dec_directive(self):
        # arrange
        params = NoiseSimulationParameters(output_node="out", source_name="Vin", sweep_mode="DEC", points="10", start="1k", end="100MEG", replace_ground=False)
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".NOISE V(out) Vin DEC 10 1k 100MEG"]

    def test_data_directive(self):
        # arrange
        params = NoiseSimulationParameters(output_node="out", source_name="Vin", sweep_mode="DATA", data_table_name="myTable", replace_ground=False)
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

    def test_serializes_print_noise_directive(self):
        # arrange
        print_params = PrintParameters(print_type="NOISE", output_variables=("V(OUT)",))
        params = NoiseSimulationParameters(output_node="OUT", source_name="V1", sweep_mode="LIN", points="10", start="1", end="1MEG", replace_ground=False, print_parameters=print_params)
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".NOISE V(OUT) V1 LIN 10 1 1MEG", ".PRINT NOISE V(OUT)"]

    def test_parses_print_noise_directive(self):
        # arrange / act
        params = NoiseSimulationParameters.from_xyce_directives([".NOISE V(5) V1 LIN 10 1 1MEG", ".PRINT NOISE V(OUT)"])
        # assert
        assert params is not None
        assert params.print_parameters is not None
        assert params.print_parameters.print_type == "NOISE"
        assert params.print_parameters.output_variables == ("V(OUT)",)

    def test_ignores_non_noise_print_directive(self):
        # arrange / act
        params = NoiseSimulationParameters.from_xyce_directives([".NOISE V(5) V1 LIN 10 1 1MEG", ".PRINT TRAN V(OUT)"])
        # assert
        assert params is not None
        assert params.print_parameters is None

    def test_ignores_empty_directive(self):
        # arrange / act — an empty string in the list should be silently skipped
        params = NoiseSimulationParameters.from_xyce_directives(["", ".NOISE V(5) V1 LIN 10 1 1MEG"])
        # assert
        assert params is not None
        assert params.output_node == "5"

    def test_parses_bare_output_node(self):
        # arrange / act — output token not wrapped in V(), triggers the fallback path
        params = NoiseSimulationParameters.from_xyce_directives([".NOISE 5 V1 LIN 10 1 1MEG"])
        # assert
        assert params is not None
        assert params.output_node == "5"
        assert params.ref_node == ""

    def test_parses_noise_directive_alone(self):
        # arrange / act — .NOISE with no output node or source should still set found
        params = NoiseSimulationParameters.from_xyce_directives([".NOISE"])
        # assert
        assert params is not None
        assert params.output_node == ""

    def test_parses_noise_without_sweep_type(self):
        # arrange / act — .NOISE with output and source but no sweep parameters
        params = NoiseSimulationParameters.from_xyce_directives([".NOISE V(5) V1"])
        # assert
        assert params is not None
        assert params.output_node == "5"
        assert params.source_name == "V1"
        assert params.sweep_mode == "LIN"

    def test_parses_implicit_lin_sweep(self):
        # arrange / act (no LIN keyword in .NOISE)
        params = NoiseSimulationParameters.from_xyce_directives([".NOISE V(5) V1 100 1 1MEG"])
        # assert
        assert params is not None
        assert params.sweep_mode == "LIN"
        assert params.points == "100"
        assert params.start == "1"
        assert params.end == "1MEG"

    def test_parses_device_noise_operators(self):
        # arrange / act
        params = NoiseSimulationParameters.from_xyce_directives([".NOISE V(5) V1 LIN 10 1 1MEG", ".PRINT NOISE INOISE ONOISE DNI(R1) DNO(R2)"])
        # assert
        assert params is not None
        assert len(params.device_noise_operators) == 2
        assert params.device_noise_operators[0].device_name == "R1"
        assert params.device_noise_operators[0].operator_type == "DNI"
        assert params.device_noise_operators[1].device_name == "R2"
        assert params.device_noise_operators[1].operator_type == "DNO"

    def test_parses_device_noise_operators_with_noise_source(self):
        # arrange / act
        params = NoiseSimulationParameters.from_xyce_directives([".NOISE V(5) V1 LIN 10 1 1MEG", ".PRINT NOISE DNI(Q2,FLICKER) DNO(Q2,THERMAL)"])
        # assert
        assert params is not None
        assert len(params.device_noise_operators) == 2
        assert params.device_noise_operators[0].device_name == "Q2"
        assert params.device_noise_operators[0].operator_type == "DNI"
        assert params.device_noise_operators[0].noise_source == "FLICKER"
        assert params.device_noise_operators[1].device_name == "Q2"
        assert params.device_noise_operators[1].operator_type == "DNO"
        assert params.device_noise_operators[1].noise_source == "THERMAL"

    def test_serializes_device_noise_operators(self):
        # arrange
        device_operators = (DeviceNoiseOperator(device_name="R1", operator_type="DNI", noise_source=""), DeviceNoiseOperator(device_name="R2", operator_type="DNO", noise_source=""))
        print_params = PrintParameters(print_type="NOISE", output_variables=("INOISE", "ONOISE"))
        params = NoiseSimulationParameters(output_node="5", source_name="V1", sweep_mode="LIN", points="10", start="1", end="1MEG", replace_ground=False, print_parameters=print_params, device_noise_operators=device_operators)
        # act
        directives = params.to_xyce_directives()
        # assert
        assert ".PRINT NOISE INOISE ONOISE DNI(R1) DNO(R2)" in directives

    def test_serializes_device_noise_operators_with_noise_source(self):
        # arrange
        device_operators = (DeviceNoiseOperator(device_name="Q2", operator_type="DNI", noise_source="FLICKER"),)
        print_params = PrintParameters(print_type="NOISE", output_variables=())
        params = NoiseSimulationParameters(output_node="5", source_name="V1", sweep_mode="LIN", points="10", start="1", end="1MEG", replace_ground=False, print_parameters=print_params, device_noise_operators=device_operators)
        # act
        directives = params.to_xyce_directives()
        # assert
        assert ".PRINT NOISE DNI(Q2,FLICKER)" in directives

    def test_omits_print_when_no_output_variables(self):
        # arrange
        print_params = PrintParameters(print_type="NOISE", output_variables=())
        params = NoiseSimulationParameters(output_node="5", source_name="V1", sweep_mode="LIN", points="10", start="1", end="1MEG", replace_ground=False, print_parameters=print_params, device_noise_operators=())
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".NOISE V(5) V1 LIN 10 1 1MEG"]
