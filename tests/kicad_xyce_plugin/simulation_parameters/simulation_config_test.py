from kicad_xyce_plugin.simulation_parameters import OptionParameters, SimulationConfig, SensParameter, TransientSimulationParameters, StepParameters


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
        config = SimulationConfig(analysis=analysis, step=step)
        # act
        directives = config.to_xyce_directives()
        # assert
        assert ".TRAN 1u 1m" in directives
        assert ".STEP R1 1k 10k 1k" in directives

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
        config = SimulationConfig(analysis=analysis, step=step, options=options)
        # act
        directives = config.to_xyce_directives()
        # assert
        assert ".OPTIONS DEVICE TEMP=25" in directives
        assert ".OPTIONS TIMEINT RELTOL=1e-3" in directives
        assert ".TRAN 1u 1m" in directives
        assert ".STEP R1 1k 10k 1k" in directives
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
        config = SimulationConfig(analysis=analysis, step=step)
        # act
        directives = config.to_xyce_directives()
        # assert
        assert ".TRAN 1u 1m" in directives
        assert ".STEP R1 1k 10k 1k" in directives
        assert ".SENS objfunc={V(2)} param=R1:R" in directives
        assert ".OPTIONS SENSITIVITY direct=1 adjoint=0" in directives
