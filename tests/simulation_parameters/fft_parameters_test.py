from simulation_parameters import FftParameters


class TestFftParameters:

    def test_minimal_fft_statement(self):
        # arrange
        statement = ".FFT V(OUT)"
        # act
        params = FftParameters.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.output_variable == "V(OUT)"
        assert params.to_xyce_statement() == ".FFT V(OUT)"

    def test_full_fft_statement(self):
        # arrange
        statement = ".FFT V(OUT) NP=1024 WINDOW=HANN ALFA=1.0 FORMAT=NORM START=0 STOP=10m FREQ=1k FMIN=0 FMAX=10k"
        # act
        params = FftParameters.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.output_variable == "V(OUT)"
        assert params.np == "1024"
        assert params.window == "HANN"
        assert params.alfa == "1.0"
        assert params.fft_format == "NORM"
        assert params.start == "0"
        assert params.stop == "10m"
        assert params.freq == "1k"
        assert params.fmin == "0"
        assert params.fmax == "10k"

    def test_synonyms_from_to(self):
        # arrange
        statement = ".FFT V(OUT) FROM=1m TO=5m"
        # act
        params = FftParameters.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.start == "1m"
        assert params.stop == "5m"
        assert params.to_xyce_statement() == ".FFT V(OUT) START=1m STOP=5m"

    def test_synonym_triangular(self):
        # arrange
        statement = ".FFT V(OUT) WINDOW=TRIANGULAR"
        # act
        params = FftParameters.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.window == "TRIANGULAR"
        assert params.to_xyce_statement() == ".FFT V(OUT) WINDOW=TRIANGULAR"

    def test_invalid_fft_statement(self):
        # arrange
        statement = ".TRAN 1u 1m"
        # act
        params = FftParameters.from_xyce_statement(statement)
        # assert
        assert params is None

    def test_handles_complex_output_variable(self):
        # arrange
        statement = ".FFT {V(OUT)*I(R1)} WINDOW=RECT"
        # act
        params = FftParameters.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.output_variable == "{V(OUT)*I(R1)}"
