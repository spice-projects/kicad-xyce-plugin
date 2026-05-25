from pathlib import Path

from kicad_xyce_plugin.simulation_parameters import PrintParameters


FIXTURES_DIR = Path(__file__).parent / "test-suite"


class TestPrintParameters:

    def test_non_print_statement_returns_none(self):
        # arrange
        statement = ".TRAN 1u 1m"
        # act
        print_parameters = PrintParameters.from_xyce_statement(statement)
        # assert
        assert print_parameters is None

    def test_minimal_print_statement(self):
        # arrange
        statement = ".PRINT TRAN"
        # act
        print_parameters = PrintParameters.from_xyce_statement(statement)
        # assert
        assert print_parameters is not None
        assert print_parameters.print_type == "TRAN"
        assert print_parameters.print_format == ""
        assert print_parameters.print_file == ""
        assert print_parameters.output_variables == ()
        assert print_parameters.extra_options == ()

    def test_parses_format_file_and_variables(self):
        # arrange
        statement = ".print tran FORMAT=RAW FILE=waves.raw V(OUT) I(V1)"
        # act
        print_parameters = PrintParameters.from_xyce_statement(statement)
        # assert
        assert print_parameters.print_type == "TRAN"
        assert print_parameters.print_format == "RAW"
        assert print_parameters.print_file == "waves.raw"
        assert print_parameters.output_variables == ("V(OUT)", "I(V1)")

    def test_parses_expression_with_spaces(self):
        # arrange
        statement = ".PRINT TRAN FORMAT=RAW V(OUT) {V(OUT) * I(V1)}"
        # act
        print_parameters = PrintParameters.from_xyce_statement(statement)
        # assert
        assert print_parameters.output_variables == ("V(OUT)", "{V(OUT) * I(V1)}")

    def test_parses_generic_options(self):
        # arrange
        statement = ".PRINT TRAN WIDTH=20 PRECISION=12 V(OUT)"
        # act
        print_parameters = PrintParameters.from_xyce_statement(statement)
        # assert
        assert print_parameters.extra_options == ("WIDTH=20", "PRECISION=12")
        assert print_parameters.output_variables == ("V(OUT)",)

    def test_ignores_sample_options_for_non_sample_print_type_with_warning(self, caplog):
        # arrange
        statement = ".PRINT TRAN OUTPUT_SAMPLE_STATS=TRUE OUTPUT_ALL_SAMPLES=TRUE V(OUT)"
        # act
        with caplog.at_level("WARNING"):
            print_parameters = PrintParameters.from_xyce_statement(statement)
        # assert
        assert print_parameters.extra_options == ()
        assert print_parameters.output_variables == ("V(OUT)",)
        assert "unsupported .PRINT option 'OUTPUT_SAMPLE_STATS'" in caplog.text
        assert "unsupported .PRINT option 'OUTPUT_ALL_SAMPLES'" in caplog.text

    def test_accepts_sample_options_for_es_print_type(self):
        # arrange
        statement = ".PRINT ES OUTPUT_SAMPLE_STATS=TRUE OUTPUT_ALL_SAMPLES=FALSE V(OUT)"
        # act
        print_parameters = PrintParameters.from_xyce_statement(statement)
        # assert
        assert print_parameters.extra_options == ("OUTPUT_SAMPLE_STATS=TRUE", "OUTPUT_ALL_SAMPLES=FALSE")
        assert print_parameters.output_variables == ("V(OUT)",)

    def test_ignores_invalid_format_value_with_warning(self, caplog):
        # arrange
        statement = ".PRINT TRAN FORMAT=INVALID V(OUT)"
        # act
        with caplog.at_level("WARNING"):
            print_parameters = PrintParameters.from_xyce_statement(statement)
        # assert
        assert print_parameters.print_format == ""
        assert print_parameters.output_variables == ("V(OUT)",)
        assert "Ignoring invalid FORMAT value 'INVALID'" in caplog.text

    def test_serializes_statement(self):
        # arrange
        print_parameters = PrintParameters(print_type="TRAN", print_format="RAW", print_file="waves.raw", output_variables=("V(OUT)", "ID(M1)", "{V(OUT)*I(V1)}"), extra_options=("WIDTH=20",))
        # act
        statement = print_parameters.to_xyce_statement()
        # assert
        assert statement == ".PRINT TRAN FORMAT=RAW FILE=waves.raw WIDTH=20 V(OUT) ID(M1) {V(OUT)*I(V1)}"

    def test_round_trip(self):
        # arrange
        statement = ".PRINT TRAN FORMAT=RAW FILE=waves.raw WIDTH=20 V(OUT) {V(OUT) * I(V1)}"
        # act
        parsed = PrintParameters.from_xyce_statement(statement)
        serialized = parsed.to_xyce_statement()
        reparsed = PrintParameters.from_xyce_statement(serialized)
        # assert
        assert reparsed == parsed

    def test_parses_all_directives_from_test_suite(self):
        # arrange
        test_suite_path = FIXTURES_DIR / "print-directive.txt"
        # read raw lines from the test suite file
        lines = test_suite_path.read_text().splitlines()
        # build directive list by joining continuation lines
        directives = []
        # init current directive buffer
        current = None
        for line in lines:
            # strip surrounding whitespace
            stripped = line.strip()
            # skip empty lines and flush any buffered directive
            if not stripped:
                # flush current directive when present
                if current is not None:
                    # append buffered directive
                    directives.append(current)
                    # reset buffer
                    current = None
                # next
                continue
            # join continuation line to current directive
            if stripped.startswith("+"):
                # append continuation content to current directive
                if current is not None:
                    # merge continuation
                    current = current + " " + stripped[1:].strip()
                # next
                continue
            # flush current directive when present
            if current is not None:
                # append buffered directive
                directives.append(current)
            # start new directive
            current = stripped
        # flush trailing directive
        if current is not None:
            # append final directive
            directives.append(current)
        # act and assert
        for directive in directives:
            # parse the directive
            result = PrintParameters.from_xyce_statement(directive)
            # assert result is not none
            assert result is not None


class TestWildcardNormalization:

    def test_w_star_normalizes_to_p_star(self):
        # arrange
        statement = ".PRINT TRAN W(*)"
        # act
        print_parameters = PrintParameters.from_xyce_statement(statement)
        # assert — W(*) is a PSpice alias for P(*) and must be stored as P(*)
        assert print_parameters.output_variables == ("P(*)",)

    def test_w_named_instance_normalizes_to_p_form(self):
        # arrange
        statement = ".PRINT DC W(R1)"
        # act
        print_parameters = PrintParameters.from_xyce_statement(statement)
        # assert — named W(...) also maps to P(...)
        assert print_parameters.output_variables == ("P(R1)",)

    def test_w_star_in_mixed_wildcard_list_normalizes(self):
        # arrange
        statement = ".PRINT TRAN V(*) I(*) W(*)"
        # act
        print_parameters = PrintParameters.from_xyce_statement(statement)
        # assert — W(*) becomes P(*); other wildcards are unchanged
        assert print_parameters.output_variables == ("V(*)", "I(*)", "P(*)")

    def test_normalized_p_star_survives_round_trip(self):
        # arrange — parse a statement that contains W(*) before serialization
        statement = ".PRINT TRAN V(*) W(*)"
        # act
        parsed = PrintParameters.from_xyce_statement(statement)
        serialized = parsed.to_xyce_statement()
        reparsed = PrintParameters.from_xyce_statement(serialized)
        # assert — W(*) is gone; round-trip contains P(*) only
        assert "W(*)" not in serialized
        assert reparsed.output_variables == ("V(*)", "P(*)")
