from kicad_xyce_plugin.simulation_parameters import OptionParameters


class TestOptionParameters:

    def test_parse_option_directives(self):
        # arrange
        directives = [
            ".OPTIONS DEVICE TEMP=25 GMIN=1e-12",
            ".OPTIONS TIMEINT RELTOL=1e-3 ABSTOL=1e-12",
            ".OPTIONS NONLIN MAXSTEP=10",
            ".OPTIONS LINSOL TYPE=AZTECOO",
        ]
        # act
        params = OptionParameters.from_xyce_directives(directives)
        # assert
        assert params.device == {"TEMP": "25", "GMIN": "1e-12"}
        assert params.timeint == {"RELTOL": "1e-3", "ABSTOL": "1e-12"}
        assert params.nonlin == {"MAXSTEP": "10"}
        assert params.linsol == {"TYPE": "AZTECOO"}

    def test_generate_directives(self):
        # arrange
        params = OptionParameters(
            device={"TEMP": "25"},
            timeint={"RELTOL": "1e-3"},
            nonlin={"MAXSTEP": "10"},
            linsol={"TYPE": "AZTECOO"},
        )
        # act
        directives = params.to_xyce_directives()
        # assert
        assert ".OPTIONS DEVICE TEMP=25" in directives
        assert ".OPTIONS TIMEINT RELTOL=1e-3" in directives
        assert ".OPTIONS NONLIN MAXSTEP=10" in directives
        assert ".OPTIONS LINSOL TYPE=AZTECOO" in directives

    def test_round_trip_directives(self):
        # arrange
        directives = [
            ".OPTIONS DEVICE TEMP=25",
            ".OPTIONS NONLIN MAXSTEP=10",
        ]
        # act
        params = OptionParameters.from_xyce_directives(directives)
        round_trip = params.to_xyce_directives()
        # assert
        assert round_trip == [
            ".OPTIONS DEVICE TEMP=25",
            ".OPTIONS NONLIN MAXSTEP=10",
        ]
