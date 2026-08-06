from kicad_xyce_plugin.simulation_parameters import OptionParameters


class TestOptionParameters:

    def test_parse_option_directives(self):
        # arrange
        directives = [
            ".OPTIONS DEVICE TEMP=25 GMIN=1e-12",
            ".OPTIONS TIMEINT RELTOL=1e-3 ABSTOL=1e-12",
            ".OPTIONS NONLIN MAXSTEP=10",
            ".OPTIONS LINSOL TYPE=AZTECOO",
            ".OPTIONS FFT FFT_ACCURATE=1 FFTOUT=1 FFT_MODE=0",
        ]
        # act
        params = OptionParameters.from_xyce_directives(directives)
        # assert
        assert params.device == {"TEMP": "25", "GMIN": "1e-12"}
        assert params.timeint == {"RELTOL": "1e-3", "ABSTOL": "1e-12"}
        assert params.nonlin == {"MAXSTEP": "10"}
        assert params.linsol == {"TYPE": "AZTECOO"}
        assert params.fft == {"FFT_ACCURATE": "1", "FFTOUT": "1", "FFT_MODE": "0"}

    def test_parse_fft_options_with_mixed_case(self):
        # arrange
        directives = [".OPTIONS fft FFT_ACCURATE=0 fFtOut=1 FFT_MODE=1"]
        # act
        params = OptionParameters.from_xyce_directives(directives)
        # assert
        assert params.fft == {"FFT_ACCURATE": "0", "FFTOUT": "1", "FFT_MODE": "1"}

    def test_generate_directives(self):
        # arrange
        params = OptionParameters(
            device={"TEMP": "25"},
            timeint={"RELTOL": "1e-3"},
            nonlin={"MAXSTEP": "10"},
            linsol={"TYPE": "AZTECOO"},
            fft={"FFT_ACCURATE": "0", "FFTOUT": "1"},
        )
        # act
        directives = params.to_xyce_directives()
        # assert
        assert ".OPTIONS DEVICE TEMP=25" in directives
        assert ".OPTIONS TIMEINT RELTOL=1e-3" in directives
        assert ".OPTIONS NONLIN MAXSTEP=10" in directives
        assert ".OPTIONS LINSOL TYPE=AZTECOO" in directives
        assert ".OPTIONS FFT FFT_ACCURATE=0 FFTOUT=1" in directives

    def test_round_trip_directives(self):
        # arrange
        directives = [
            ".OPTIONS DEVICE TEMP=25",
            ".OPTIONS NONLIN MAXSTEP=10",
            ".OPTIONS FFT FFTOUT=1",
        ]
        # act
        params = OptionParameters.from_xyce_directives(directives)
        round_trip = params.to_xyce_directives()
        # assert
        assert round_trip == [
            ".OPTIONS DEVICE TEMP=25",
            ".OPTIONS NONLIN MAXSTEP=10",
            ".OPTIONS FFT FFTOUT=1",
        ]
