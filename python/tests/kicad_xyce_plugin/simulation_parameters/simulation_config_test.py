from kicad_xyce_plugin.simulation_parameters import DataBlock, OptionParameters, SimulationConfig, SensParameter, TransientSimulationParameters, StepParameters


class TestSimulationConfig:

    def test_parse_analysis_and_step(self):
        # arrange
        directives = [
            ".TRAN 1u 1m",
            ".STEP R1 1k 10k 1k"
        ]

        # act
        config = SimulationConfig.from_xyce_directives(directives)

        # assert
        assert isinstance(config.analysis, TransientSimulationParameters)
        assert config.analysis.initial_step_value == "1u"
        assert config.step.enabled is True
        assert config.step.variable == "R1"

    def test_parse_only_analysis(self):
        # arrange
        directives = [".TRAN 1u 1m"]

        # act
        config = SimulationConfig.from_xyce_directives(directives)

        # assert
        assert isinstance(config.analysis, TransientSimulationParameters)
        assert config.step.enabled is False

    def test_parse_nothing(self):
        # arrange
        directives = ["* just a comment"]

        # act
        config = SimulationConfig.from_xyce_directives(directives)

        # assert
        assert config.analysis is None
        assert config.step.enabled is False

    def test_generate_directives(self):
        # arrange
        analysis = TransientSimulationParameters("1u", "1m", "", "", "", tuple())
        step = StepParameters(sweep_mode="LIN", variable="R1", start="1k", stop="10k", step="1k", enabled=True)
        config = SimulationConfig(analysis=analysis, steps=(step,))

        # act
        directives = config.to_xyce_directives()

        # assert
        assert ".TRAN 1u 1m" in directives
        assert ".STEP LIN R1 1k 10k 1k" in directives

    def test_parse_directives_with_options(self):
        # arrange
        directives = [
            ".OPTIONS DEVICE TEMP=25",
            ".TRAN 1u 1m",
        ]

        # act
        config = SimulationConfig.from_xyce_directives(directives)

        # assert
        assert config.options.device == {"TEMP": "25"}
        assert isinstance(config.analysis, TransientSimulationParameters)

    def test_generate_directives_with_options(self):
        # arrange
        options = OptionParameters(device={"TEMP": "25"}, timeint={"RELTOL": "1e-3"})
        sensitivity = SensParameter("DC", "objfunc", ("V(2)",), ("R1:R",), True, False, None)
        analysis = TransientSimulationParameters("1u", "1m", "", "", "", tuple(), replace_ground=True, print_parameters=None, fft_parameters=tuple(), four_parameters=tuple(), measure_parameters=tuple(), sensitivity=sensitivity)
        step = StepParameters(sweep_mode="LIN", variable="R1", start="1k", stop="10k", step="1k", enabled=True)
        config = SimulationConfig(analysis=analysis, steps=(step,), options=options)

        # act
        directives = config.to_xyce_directives()

        # assert
        assert ".OPTIONS DEVICE TEMP=25" in directives
        assert ".OPTIONS TIMEINT RELTOL=1e-3" in directives
        assert ".TRAN 1u 1m" in directives
        assert ".STEP LIN R1 1k 10k 1k" in directives
        assert ".SENS objfunc={V(2)} param=R1:R" in directives
        assert ".OPTIONS SENSITIVITY direct=1 adjoint=0" in directives

    def test_parse_transient_with_sensitivity(self):
        # arrange
        directives = [
            ".TRAN 1u 1m",
            ".SENS objfunc={V(2)} param=R1:R",
            ".OPTIONS SENSITIVITY direct=1 adjoint=0",
        ]

        # act
        config = SimulationConfig.from_xyce_directives(directives)

        # assert
        assert isinstance(config.analysis, TransientSimulationParameters)
        assert config.analysis.sensitivity is not None
        assert config.analysis.sensitivity.objective_mode == "objfunc"
        assert config.analysis.sensitivity.objective_values == ("V(2)",)
        assert config.analysis.sensitivity.parameter_list == ("R1:R",)
        assert config.analysis.sensitivity.direct is True
        assert config.analysis.sensitivity.adjoint is False

    def test_generate_directives_with_sensitivity(self):
        # arrange
        sensitivity = SensParameter("DC", "objfunc", ("V(2)",), ("R1:R",), True, False, None)
        analysis = TransientSimulationParameters("1u", "1m", "", "", "", tuple(), replace_ground=True, print_parameters=None, fft_parameters=tuple(), four_parameters=tuple(), measure_parameters=tuple(), sensitivity=sensitivity)
        step = StepParameters(sweep_mode="LIN", variable="R1", start="1k", stop="10k", step="1k", enabled=True)
        config = SimulationConfig(analysis=analysis, steps=(step,))

        # act
        directives = config.to_xyce_directives()

        # assert
        assert ".TRAN 1u 1m" in directives
        assert ".STEP LIN R1 1k 10k 1k" in directives
        assert ".SENS objfunc={V(2)} param=R1:R" in directives
        assert ".OPTIONS SENSITIVITY direct=1 adjoint=0" in directives

    def test_parse_multiple_step_directives(self):
        # arrange - nested loops: inner loop first, outer last
        directives = [
            ".TRAN 1u 1m",
            ".STEP R1 1k 10k 1k",
            ".STEP TEMP 0 100 10",
        ]

        # act
        config = SimulationConfig.from_xyce_directives(directives)

        # assert
        assert len(config.steps) == 2
        assert config.steps[0].variable == "R1"
        assert config.steps[1].variable == "TEMP"

    def test_generate_multiple_step_directives(self):
        # arrange
        analysis = TransientSimulationParameters("1u", "1m", "", "", "", tuple())
        step_inner = StepParameters(sweep_mode="LIN", variable="R1", start="1k", stop="10k", step="1k", enabled=True)
        step_outer = StepParameters(sweep_mode="LIN", variable="TEMP", start="0", stop="100", step="10", enabled=True)
        config = SimulationConfig(analysis=analysis, steps=(step_inner, step_outer))

        # act
        directives = config.to_xyce_directives()

        # assert - both steps emitted and in correct order
        assert ".STEP LIN R1 1k 10k 1k" in directives
        assert ".STEP LIN TEMP 0 100 10" in directives
        r1_idx = directives.index(".STEP LIN R1 1k 10k 1k")
        temp_idx = directives.index(".STEP LIN TEMP 0 100 10")
        assert r1_idx < temp_idx

    def test_parse_data_block(self):
        # arrange - .DATA block flattened by _join_continuation_lines
        directives = [
            ".TRAN 1u 1m",
            ".STEP DATA=myTable",
            ".DATA myTable r1 r2 1.0 2.0 3.0 4.0",
            ".ENDDATA",
        ]

        # act
        config = SimulationConfig.from_xyce_directives(directives)

        # assert
        assert len(config.data_blocks) == 1
        assert config.data_blocks[0].name == "myTable"
        assert config.data_blocks[0].parameters == ("r1", "r2")
        assert config.data_blocks[0].records == (("1.0", "2.0"), ("3.0", "4.0"))

    def test_generate_data_block(self):
        # arrange
        analysis = TransientSimulationParameters("1u", "1m", "", "", "", tuple())
        step = StepParameters(sweep_mode="DATA", data_table_name="myTable", enabled=True)
        block = DataBlock(name="myTable", parameters=("r1", "r2"), records=(("1.0", "2.0"), ("3.0", "4.0")))
        config = SimulationConfig(analysis=analysis, steps=(step,), data_blocks=(block,))

        # act
        directives = config.to_xyce_directives()

        # assert
        assert ".STEP DATA=myTable" in directives
        assert ".DATA myTable" in directives
        assert "+ r1 r2" in directives
        assert "+ 1.0 2.0" in directives
        assert ".ENDDATA" in directives
