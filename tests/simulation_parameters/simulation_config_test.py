from simulation_parameters import SimulationConfig, TransientSimulationParameters, StepParameters


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
