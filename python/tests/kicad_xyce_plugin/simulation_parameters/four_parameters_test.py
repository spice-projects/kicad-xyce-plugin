from kicad_xyce_plugin.simulation_parameters import FourParameters, TransientSimulationParameters


class TestToXyceDirectivesFour:

    def test_emits_multiple_four_directives(self):
        # arrange
        four1 = FourParameters(fundamental_frequency="1k", output_variables=("V(OUT)",))
        four2 = FourParameters(fundamental_frequency="2k", output_variables=("V(IN)", "I(R1)"))
        params = TransientSimulationParameters("1u", "1m", four_parameters=(four1, four2), replace_ground=False)
        # act
        directives = params.to_xyce_directives()
        # assert
        assert len(directives) == 3
        assert directives[0] == ".TRAN 1u 1m"
        assert directives[1] == ".FOUR 1k V(OUT)"
        assert directives[2] == ".FOUR 2k V(IN) I(R1)"


class TestFromXyceDirectivesFour:

    def test_parses_single_four_directive(self):
        # arrange / act
        params = TransientSimulationParameters.from_xyce_directives([".TRAN 1u 1m", ".FOUR 1k V(OUT)"])
        # assert
        assert len(params.four_parameters) == 1
        assert params.four_parameters[0].fundamental_frequency == "1k"
        assert params.four_parameters[0].output_variables == ("V(OUT)",)

    def test_parses_multiple_four_directives(self):
        # arrange / act
        params = TransientSimulationParameters.from_xyce_directives([".TRAN 1u 1m", ".FOUR 1k V(1)", ".FOUR 2k V(2) I(1)"])
        # assert
        assert len(params.four_parameters) == 2
        assert params.four_parameters[0].fundamental_frequency == "1k"
        assert params.four_parameters[0].output_variables == ("V(1)",)
        assert params.four_parameters[1].fundamental_frequency == "2k"
        assert params.four_parameters[1].output_variables == ("V(2)", "I(1)")


class TestFromXyceDirectivesFourRoundTrip:

    def test_round_trip_with_four_parameters(self):
        # arrange
        four = FourParameters(fundamental_frequency="1k", output_variables=("V(OUT)",))
        original = TransientSimulationParameters("1u", "1m", four_parameters=(four,))
        # act
        parsed = TransientSimulationParameters.from_xyce_directives(original.to_xyce_directives())
        # assert
        assert len(parsed.four_parameters) == 1
        assert parsed.four_parameters[0].fundamental_frequency == "1k"
        assert parsed.four_parameters[0].output_variables == ("V(OUT)",)
