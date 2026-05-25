from kicad_xyce_plugin.simulation_parameters import LinSimulationParameters, PrintParameters


class TestLinSimulationParameters:

    def test_minimal_default_directives(self):
        # arrange
        params = LinSimulationParameters(sweep_mode="LIN", points="100", start="1", end="1MEG", replace_ground=False)
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".AC LIN 100 1 1MEG", ".LIN"]

    def test_dec_sweep_directives(self):
        # arrange
        params = LinSimulationParameters(sweep_mode="DEC", points="10", start="1k", end="10MEG", replace_ground=False)
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".AC DEC 10 1k 10MEG", ".LIN"]

    def test_data_sweep_directives(self):
        # arrange
        params = LinSimulationParameters(sweep_mode="DATA", data_table_name="myTable", replace_ground=False)
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".AC DATA=myTable", ".LIN"]

    def test_lin_keyword_arguments(self):
        # arrange
        params = LinSimulationParameters(sweep_mode="LIN", points="100", start="1", end="1MEG", format="TOUCHSTONE", lintype="Y", dataformat="MA", file="output.s2p",)
        # act
        directives = params.to_xyce_directives()
        # assert
        assert ".AC LIN 100 1 1MEG" in directives
        lin_line = next(d for d in directives if d.startswith(".LIN"))
        assert "FORMAT=TOUCHSTONE" in lin_line
        assert "TYPE=Y" in lin_line
        assert "DATAFORMAT=MA" in lin_line
        assert "FILE=output.s2p" in lin_line

    def test_sparcalc_false(self):
        # arrange
        params = LinSimulationParameters(sweep_mode="LIN", points="10", start="1", end="1MEG", sparcalc=False)
        # act
        directives = params.to_xyce_directives()
        # assert
        lin_line = next(d for d in directives if d.startswith(".LIN"))
        assert "SPARCALC=0" in lin_line

    def test_replace_ground(self):
        # arrange
        params = LinSimulationParameters(sweep_mode="LIN", points="10", start="1", end="1MEG", replace_ground=True)
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives[0] == ".PREPROCESS REPLACEGROUND TRUE"
        assert ".AC LIN 10 1 1MEG" in directives
        assert any(d.startswith(".LIN") for d in directives)

    def test_width_and_precision_directives(self):
        # arrange
        params = LinSimulationParameters(sweep_mode="LIN", points="10", start="1", end="1MEG", replace_ground=False, width="80", precision="6")
        # act
        directives = params.to_xyce_directives()
        # assert
        lin_line = next(d for d in directives if d.startswith(".LIN"))
        assert "WIDTH=80" in lin_line
        assert "PRECISION=6" in lin_line


class TestLinFromXyceDirectives:

    def test_parses_lin_and_ac(self):
        # arrange / act
        params = LinSimulationParameters.from_xyce_directives([".AC LIN 100 1 1MEG", ".LIN"])
        # assert
        assert params is not None
        assert params.sweep_mode == "LIN"
        assert params.points == "100"
        assert params.start == "1"
        assert params.end == "1MEG"

    def test_parses_lin_keyword_args(self):
        # arrange / act
        params = LinSimulationParameters.from_xyce_directives([".AC LIN 100 1 1MEG", ".LIN FORMAT=TOUCHSTONE TYPE=Y DATAFORMAT=MA FILE=output.s2p",])
        # assert
        assert params is not None
        assert params.format == "TOUCHSTONE"
        assert params.lintype == "Y"
        assert params.dataformat == "MA"
        assert params.file == "output.s2p"

    def test_parses_sparcalc_false(self):
        # arrange / act
        params = LinSimulationParameters.from_xyce_directives([".AC LIN 10 1 1MEG", ".LIN SPARCALC=0"])
        # assert
        assert params is not None
        assert params.sparcalc is False

    def test_parses_replaceground_true(self):
        # arrange / act
        params = LinSimulationParameters.from_xyce_directives([
            ".AC LIN 10 1 1MEG",
            ".LIN",
            ".PREPROCESS REPLACEGROUND TRUE",
        ])
        # assert
        assert params is not None
        assert params.replace_ground is True

    def test_empty_directives_returns_none(self):
        # arrange / act
        params = LinSimulationParameters.from_xyce_directives([])
        # assert
        assert params is None

    def test_ac_only_returns_none(self):
        # arrange / act — no .LIN directive, should not match LinSimulationParameters
        params = LinSimulationParameters.from_xyce_directives([".AC LIN 100 1 1MEG"])
        # assert
        assert params is None

    def test_non_lin_directives_return_none(self):
        # arrange / act
        params = LinSimulationParameters.from_xyce_directives([".TRAN 1ns 1ms"])
        # assert
        assert params is None

    def test_serializes_print_ac_directive(self):
        # arrange
        print_params = PrintParameters(print_type="AC", output_variables=("V(OUT)",))
        params = LinSimulationParameters(sweep_mode="LIN", points="10", start="1", end="1MEG", replace_ground=False, print_parameters=print_params)
        # act
        directives = params.to_xyce_directives()
        # assert
        assert ".PRINT AC V(OUT)" in directives

    def test_parses_print_ac_directive(self):
        # arrange / act
        params = LinSimulationParameters.from_xyce_directives([".AC LIN 10 1 1MEG", ".LIN", ".PRINT AC V(OUT) I(V1)"])
        # assert
        assert params is not None
        assert params.print_parameters is not None
        assert params.print_parameters.print_type == "AC"
        assert params.print_parameters.output_variables == ("V(OUT)", "I(V1)")

    def test_ignores_empty_directive(self):
        # arrange / act — an empty string in the list should be silently skipped
        params = LinSimulationParameters.from_xyce_directives(["", ".AC LIN 10 1 1MEG", ".LIN"])
        # assert
        assert params is not None
        assert params.sweep_mode == "LIN"

    def test_ac_alone_does_not_crash(self):
        # arrange / act — .AC with no extra tokens should be skipped without error
        params = LinSimulationParameters.from_xyce_directives([".AC", ".LIN"])
        # assert
        assert params is not None
        assert params.sweep_mode == "LIN"

    def test_parses_ac_data_sweep(self):
        # arrange / act
        params = LinSimulationParameters.from_xyce_directives([".AC DATA=myTable", ".LIN"])
        # assert
        assert params is not None
        assert params.sweep_mode == "DATA"
        assert params.data_table_name == "myTable"

    def test_parses_ac_dec_sweep(self):
        # arrange / act
        params = LinSimulationParameters.from_xyce_directives([".AC DEC 10 1k 10MEG", ".LIN"])
        # assert
        assert params is not None
        assert params.sweep_mode == "DEC"
        assert params.points == "10"
        assert params.start == "1k"
        assert params.end == "10MEG"

    def test_parses_ac_implicit_lin(self):
        # arrange / act (no LIN keyword in .AC)
        params = LinSimulationParameters.from_xyce_directives([".AC 100 1 1MEG", ".LIN"])
        # assert
        assert params is not None
        assert params.sweep_mode == "LIN"
        assert params.points == "100"
        assert params.start == "1"
        assert params.end == "1MEG"

    def test_parses_lin_token_without_equals(self):
        # arrange / act — token with no '=' in .LIN should be silently skipped
        params = LinSimulationParameters.from_xyce_directives([".AC LIN 10 1 1MEG", ".LIN NOEQUALS FORMAT=TS"])
        # assert
        assert params is not None
        assert params.format == "TS"

    def test_parses_lin_width_and_precision(self):
        # arrange / act
        params = LinSimulationParameters.from_xyce_directives([".AC LIN 10 1 1MEG", ".LIN WIDTH=80 PRECISION=6"])
        # assert
        assert params is not None
        assert params.width == "80"
        assert params.precision == "6"

    def test_ignores_non_ac_print_directive(self):
        # arrange / act
        params = LinSimulationParameters.from_xyce_directives([".AC LIN 10 1 1MEG", ".LIN", ".PRINT TRAN V(OUT)"])
        # assert
        assert params is not None
        assert params.print_parameters is None


class TestReferenceGuideExamples:
    # reference guide examples from xyce_rg.txt section 2.1.17 (lines 1811-1812)

    def test_reference_guide_example_minimal(self):
        # arrange - .LIN
        directive = ".LIN"
        # act
        params = LinSimulationParameters.from_xyce_directives([".AC LIN 100 1 1MEG", directive])
        # assert
        assert params is not None
        assert params.sweep_mode == "LIN"
        assert params.points == "100"
        assert params.start == "1"
        assert params.end == "1MEG"
        # minimal .lin should emit with defaults
        directives = params.to_xyce_directives()
        assert ".LIN" in directives

    def test_reference_guide_example_with_options(self):
        # arrange - .LIN FORMAT=TOUCHSTONE DATAFORMAT=MA FILE=foo
        directive = ".LIN FORMAT=TOUCHSTONE DATAFORMAT=MA FILE=foo"
        # act
        params = LinSimulationParameters.from_xyce_directives([".AC LIN 100 1 1MEG", directive])
        # assert
        assert params is not None
        assert params.format == "TOUCHSTONE"
        assert params.dataformat == "MA"
        assert params.file == "foo"
        regenerated = params.to_xyce_directives()
        lin_line = next(d for d in regenerated if d.startswith(".LIN"))
        assert "FORMAT=TOUCHSTONE" in lin_line
        assert "DATAFORMAT=MA" in lin_line
        assert "FILE=foo" in lin_line
