from pathlib import Path

from simulation_parameters import FftParameters, PrintParameters, TransientSchedulePoint, TransientSimulationParameters


FIXTURES_DIR = Path(__file__).parent / "test-suite"


class TestFromXyceDirectivesTestSuite:

    def test_parses_all_directives_from_test_suite(self):
        # arrange
        test_suite_path = FIXTURES_DIR / "tran-directive.txt"
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
            result = TransientSimulationParameters.from_xyce_directives([directive])
            # assert result is not none
            assert result is not None


class TestToXyceDirectivesBasic:

    def test_minimal_directive(self):
        # arrange
        params = TransientSimulationParameters("1u", "1m")
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".PREPROCESS REPLACEGROUND TRUE", ".TRAN 1u 1m"]

    def test_start_time_is_included_when_provided(self):
        # arrange
        params = TransientSimulationParameters("1n", "10u", start_time_value="100n")
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".PREPROCESS REPLACEGROUND TRUE", ".TRAN 1n 10u 100n"]

    def test_start_time_defaults_to_zero_when_only_step_ceiling_given(self):
        # arrange
        params = TransientSimulationParameters("1u", "1m", step_ceiling_value="5u")
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".PREPROCESS REPLACEGROUND TRUE", ".TRAN 1u 1m 0 5u"]

    def test_step_ceiling_is_included_when_provided(self):
        # arrange
        params = TransientSimulationParameters("1u", "1m", start_time_value="0", step_ceiling_value="10u")
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".PREPROCESS REPLACEGROUND TRUE", ".TRAN 1u 1m 0 10u"]

    def test_start_time_without_step_ceiling_does_not_append_blank(self):
        # arrange
        params = TransientSimulationParameters("1u", "1m", start_time_value="500n")
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".PREPROCESS REPLACEGROUND TRUE", ".TRAN 1u 1m 500n"]

    def test_op_keyword_noop_is_appended(self):
        # arrange
        params = TransientSimulationParameters("1u", "1m", op_keyword="NOOP")
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".PREPROCESS REPLACEGROUND TRUE", ".TRAN 1u 1m NOOP"]

    def test_op_keyword_uic_is_appended(self):
        # arrange
        params = TransientSimulationParameters("1u", "1m", op_keyword="UIC")
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".PREPROCESS REPLACEGROUND TRUE", ".TRAN 1u 1m UIC"]

    def test_empty_op_keyword_is_not_appended(self):
        # arrange
        params = TransientSimulationParameters("1u", "1m")
        # act
        directives = params.to_xyce_directives()
        # assert
        assert "NOOP" not in directives[0]
        assert "UIC" not in directives[0]

    def test_start_time_step_ceiling_and_op_keyword_combined(self):
        # arrange
        params = TransientSimulationParameters("1u", "1m", start_time_value="0", step_ceiling_value="5u", op_keyword="UIC")
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".PREPROCESS REPLACEGROUND TRUE", ".TRAN 1u 1m 0 5u UIC"]

    def test_print_parameters_is_appended_when_configured(self):
        # arrange
        print_parameters = PrintParameters(print_type="TRAN", print_format="RAW", print_file="out.raw", output_variables=("V(OUT)", "I(V1)"))
        params = TransientSimulationParameters("1u", "1m", print_parameters=print_parameters)
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".PREPROCESS REPLACEGROUND TRUE", ".TRAN 1u 1m", ".PRINT TRAN FORMAT=RAW FILE=out.raw V(OUT) I(V1)"]


class TestToXyceDirectivesSchedule:

    def test_single_schedule_point_produces_schedule_clause(self):
        # arrange
        params = TransientSimulationParameters("1n", "5u", schedule_points=(TransientSchedulePoint("1u", "10n"),))
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".PREPROCESS REPLACEGROUND TRUE", ".TRAN 1n 5u {schedule(1u, 10n)}"]

    def test_multiple_schedule_points_are_flattened(self):
        # arrange
        params = TransientSimulationParameters("1n", "20u", schedule_points=(TransientSchedulePoint("1u", "10n"), TransientSchedulePoint("10u", "100n")))
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".PREPROCESS REPLACEGROUND TRUE", ".TRAN 1n 20u {schedule(1u, 10n, 10u, 100n)}"]

    def test_schedule_combined_with_start_and_step_ceiling(self):
        # arrange
        params = TransientSimulationParameters("1n", "10u", start_time_value="0", step_ceiling_value="200n", schedule_points=(TransientSchedulePoint("5u", "50n"),))
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".PREPROCESS REPLACEGROUND TRUE", ".TRAN 1n 10u 0 200n {schedule(5u, 50n)}"]

    def test_empty_schedule_points_produces_no_schedule_clause(self):
        # arrange
        params = TransientSimulationParameters("1u", "1m")
        # act
        directives = params.to_xyce_directives()
        # assert
        assert "schedule" not in directives[0]


class TestToXyceDirectivesReplaceGround:

    def test_replace_ground_false_emits_no_preprocess(self):
        # arrange
        params = TransientSimulationParameters("1u", "1m", replace_ground=False)
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".TRAN 1u 1m"]

    def test_replace_ground_true_prepends_preprocess_directive(self):
        # arrange
        params = TransientSimulationParameters("1u", "1m", replace_ground=True)
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".PREPROCESS REPLACEGROUND TRUE", ".TRAN 1u 1m"]

    def test_replace_ground_with_all_options_combined(self):
        # arrange
        params = TransientSimulationParameters("1u", "1m", start_time_value="0", step_ceiling_value="5u", op_keyword="NOOP", schedule_points=(TransientSchedulePoint("500u", "1u"),), replace_ground=True)
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".PREPROCESS REPLACEGROUND TRUE", ".TRAN 1u 1m 0 5u NOOP {schedule(500u, 1u)}"]

    def test_topology_argument_is_accepted_and_ignored(self):
        # arrange — topology is accepted by the signature but unused for transient
        params = TransientSimulationParameters("1u", "1m", replace_ground=True)
        # act
        directives = params.to_xyce_directives(topology=None)
        # assert
        assert directives[0] == ".PREPROCESS REPLACEGROUND TRUE"


class TestFromXyceDirectivesBasic:

    def test_empty_directives_returns_none(self):
        # arrange / act
        params = TransientSimulationParameters.from_xyce_directives([])
        # assert — no .TRAN directive means None is returned
        assert params is None

    def test_blank_directive_string_is_skipped(self):
        # arrange / act
        params = TransientSimulationParameters.from_xyce_directives([""])
        # assert — blank lines do not crash and return None
        assert params is None

    def test_non_tran_directives_are_ignored(self):
        # arrange / act
        params = TransientSimulationParameters.from_xyce_directives([".OP", ".DC VIN 0 5 1"])
        # assert — no .TRAN directive means None is returned
        assert params is None

    def test_minimal_tran_directive(self):
        # arrange / act
        params = TransientSimulationParameters.from_xyce_directives([".TRAN 1us 100ms"])
        # assert
        assert params.initial_step_value == "1us"
        assert params.final_time_value == "100ms"
        assert params.start_time_value == ""
        assert params.step_ceiling_value == ""
        assert params.op_keyword == ""

    def test_tran_with_start_time(self):
        # arrange / act
        params = TransientSimulationParameters.from_xyce_directives([".TRAN 1ms 100ms 0ms"])
        # assert
        assert params.initial_step_value == "1ms"
        assert params.final_time_value == "100ms"
        assert params.start_time_value == "0ms"
        assert params.step_ceiling_value == ""

    def test_tran_with_start_time_and_step_ceiling(self):
        # arrange / act
        params = TransientSimulationParameters.from_xyce_directives([".TRAN 1ms 100ms 0ms .1ms"])
        # assert
        assert params.initial_step_value == "1ms"
        assert params.final_time_value == "100ms"
        assert params.start_time_value == "0ms"
        assert params.step_ceiling_value == ".1ms"

    def test_tran_with_noop_keyword(self):
        # arrange / act
        params = TransientSimulationParameters.from_xyce_directives([".TRAN 1u 1m NOOP"])
        # assert
        assert params.op_keyword == "NOOP"
        assert params.start_time_value == ""

    def test_tran_with_uic_keyword(self):
        # arrange / act
        params = TransientSimulationParameters.from_xyce_directives([".TRAN 1u 1m UIC"])
        # assert
        assert params.op_keyword == "UIC"

    def test_tran_with_noop_after_positionals(self):
        # arrange / act
        params = TransientSimulationParameters.from_xyce_directives([".TRAN 1u 1m 0 5u NOOP"])
        # assert
        assert params.start_time_value == "0"
        assert params.step_ceiling_value == "5u"
        assert params.op_keyword == "NOOP"

    def test_tran_lowercase_noop_is_normalised(self):
        # arrange / act
        params = TransientSimulationParameters.from_xyce_directives([".TRAN 1u 1m noop"])
        # assert
        assert params.op_keyword == "NOOP"

    def test_tran_lowercase_uic_is_normalised(self):
        # arrange / act
        params = TransientSimulationParameters.from_xyce_directives([".TRAN 1u 1m uic"])
        # assert
        assert params.op_keyword == "UIC"

    def test_parses_print_tran_directive(self):
        # arrange / act
        params = TransientSimulationParameters.from_xyce_directives([".TRAN 1u 1m", ".PRINT TRAN FORMAT=RAW FILE=tran.raw V(OUT) I(V1)"])
        # assert
        assert params.print_parameters is not None
        assert params.print_parameters.print_type == "TRAN"
        assert params.print_parameters.print_format == "RAW"
        assert params.print_parameters.print_file == "tran.raw"
        assert params.print_parameters.output_variables == ("V(OUT)", "I(V1)")

    def test_ignores_non_transient_print_directive(self):
        # arrange / act
        params = TransientSimulationParameters.from_xyce_directives([".TRAN 1u 1m", ".PRINT DC V(OUT)"])
        # assert
        assert params.print_parameters is None

    def test_parses_print_tran_expression_with_spaces(self):
        # arrange / act
        params = TransientSimulationParameters.from_xyce_directives([".TRAN 1u 1m", ".PRINT TRAN FORMAT=RAW V(OUT) {V(OUT) * I(V1)}"])
        # assert
        assert params.print_parameters is not None
        assert params.print_parameters.output_variables == ("V(OUT)", "{V(OUT) * I(V1)}")


class TestFromXyceDirectivesFft:

    def test_parses_single_fft_directive(self):
        # arrange / act
        params = TransientSimulationParameters.from_xyce_directives([".TRAN 1u 1m", ".FFT V(OUT) WINDOW=HANN"])
        # assert
        assert len(params.fft_parameters) == 1
        assert params.fft_parameters[0].output_variable == "V(OUT)"
        assert params.fft_parameters[0].window == "HANN"

    def test_parses_multiple_fft_directives(self):
        # arrange / act
        params = TransientSimulationParameters.from_xyce_directives([".TRAN 1u 1m", ".FFT V(1)", ".FFT V(2) WINDOW=RECT"])
        # assert
        assert len(params.fft_parameters) == 2
        assert params.fft_parameters[0].output_variable == "V(1)"
        assert params.fft_parameters[1].output_variable == "V(2)"
        assert params.fft_parameters[1].window == "RECT"


class TestFromXyceDirectivesSchedule:

    def test_single_schedule_point_is_parsed(self):
        # arrange / act
        params = TransientSimulationParameters.from_xyce_directives([".TRAN 0 2.0e-3 {schedule( 0.5e-3, 0, 1.0e-3, 1.0e-6 )}"])
        # assert
        assert len(params.schedule_points) == 2
        assert params.schedule_points[0] == TransientSchedulePoint("0.5e-3", "0")
        assert params.schedule_points[1] == TransientSchedulePoint("1.0e-3", "1.0e-6")

    def test_schedule_with_three_pairs(self):
        # arrange / act — example straight from the reference guide
        params = TransientSimulationParameters.from_xyce_directives([".TRAN 0 2.0e-3 {schedule( 0.5e-3, 0, 1.0e-3, 1.0e-6, 2.0e-3, 0 )}"])
        # assert
        assert len(params.schedule_points) == 3
        assert params.schedule_points[2] == TransientSchedulePoint("2.0e-3", "0")

    def test_schedule_does_not_affect_positional_args(self):
        # arrange / act
        params = TransientSimulationParameters.from_xyce_directives([".TRAN 1n 10u 0 200n {schedule( 5u, 50n )}"])
        # assert
        assert params.initial_step_value == "1n"
        assert params.final_time_value == "10u"
        assert params.start_time_value == "0"
        assert params.step_ceiling_value == "200n"
        assert len(params.schedule_points) == 1

    def test_no_schedule_clause_leaves_schedule_points_empty(self):
        # arrange / act
        params = TransientSimulationParameters.from_xyce_directives([".TRAN 1u 1m"])
        # assert
        assert params.schedule_points == ()


class TestFromXyceDirectivesEdgeCases:

    def test_bare_tran_with_no_arguments_leaves_step_and_time_empty(self):
        # arrange / act — ".TRAN" alone: len(tokens) < 2, both guards are False
        params = TransientSimulationParameters.from_xyce_directives([".TRAN"])
        # assert
        assert params.initial_step_value == ""
        assert params.final_time_value == ""

    def test_tran_with_only_initial_step_leaves_final_time_empty(self):
        # arrange / act — ".TRAN 1u": len(tokens) == 2, first guard True, second False
        params = TransientSimulationParameters.from_xyce_directives([".TRAN 1u"])
        # assert
        assert params.initial_step_value == "1u"
        assert params.final_time_value == ""


class TestFromXyceDirectivesReplaceGround:

    def test_replaceground_true_is_parsed(self):
        # arrange / act
        params = TransientSimulationParameters.from_xyce_directives([".PREPROCESS REPLACEGROUND TRUE", ".TRAN 1u 1m"])
        # assert
        assert params.replace_ground is True

    def test_replaceground_false_is_parsed(self):
        # arrange / act
        params = TransientSimulationParameters.from_xyce_directives([".PREPROCESS REPLACEGROUND FALSE", ".TRAN 1u 1m"])
        # assert
        assert params.replace_ground is False

    def test_replaceground_lowercase_true_is_parsed(self):
        # arrange / act
        params = TransientSimulationParameters.from_xyce_directives([".PREPROCESS REPLACEGROUND true", ".TRAN 1u 1m"])
        # assert
        assert params.replace_ground is True

    def test_replaceground_absent_defaults_to_false(self):
        # arrange / act
        params = TransientSimulationParameters.from_xyce_directives([".TRAN 1u 1m"])
        # assert
        assert params.replace_ground is True


class TestPrintWildcards:

    def test_generic_wildcards_round_trip(self):
        # arrange — the three universal wildcards valid for every .PRINT TRAN statement
        wildcards = ("V(*)", "I(*)", "P(*)")
        print_params = PrintParameters(print_type="TRAN", output_variables=wildcards)
        params = TransientSimulationParameters("1u", "1m", replace_ground=False, print_parameters=print_params)
        # act
        directives = params.to_xyce_directives()
        reparsed = TransientSimulationParameters.from_xyce_directives(directives)
        # assert — wildcards survive a full serialize/parse cycle
        assert reparsed.print_parameters is not None
        assert reparsed.print_parameters.print_type == "TRAN"
        assert reparsed.print_parameters.output_variables == wildcards

    def test_bjt_lead_wildcards_round_trip(self):
        # arrange — BJT lead current wildcards: IB, IC, IE, IS
        wildcards = ("IB(*)", "IC(*)", "IE(*)", "IS(*)")
        print_params = PrintParameters(print_type="TRAN", output_variables=wildcards)
        params = TransientSimulationParameters("1u", "1m", replace_ground=False, print_parameters=print_params)
        # act
        directives = params.to_xyce_directives()
        reparsed = TransientSimulationParameters.from_xyce_directives(directives)
        # assert — all four BJT lead wildcards survive the cycle unchanged
        assert reparsed.print_parameters is not None
        assert reparsed.print_parameters.output_variables == wildcards

    def test_fet_lead_wildcards_round_trip(self):
        # arrange — FET lead current wildcards: IB, ID, IG, IS
        wildcards = ("IB(*)", "ID(*)", "IG(*)", "IS(*)")
        print_params = PrintParameters(print_type="TRAN", output_variables=wildcards)
        params = TransientSimulationParameters("1u", "1m", replace_ground=False, print_parameters=print_params)
        # act
        directives = params.to_xyce_directives()
        reparsed = TransientSimulationParameters.from_xyce_directives(directives)
        # assert — all four FET lead wildcards survive the cycle unchanged
        assert reparsed.print_parameters is not None
        assert reparsed.print_parameters.output_variables == wildcards

    def test_w_star_normalizes_to_p_star_on_parse(self):
        # arrange — netlist contains PSpice-style W(*) power wildcard
        directives = [".TRAN 1u 1m", ".PRINT TRAN W(*)"]
        # act
        params = TransientSimulationParameters.from_xyce_directives(directives)
        # assert — W(*) is stored as P(*) at parse time; no W(*) survives
        assert params.print_parameters is not None
        assert "P(*)" in params.print_parameters.output_variables
        assert "W(*)" not in params.print_parameters.output_variables

    def test_print_directive_uses_tran_not_dc_type(self):
        # arrange — print_parameters with print_type="TRAN"
        print_params = PrintParameters(print_type="TRAN", output_variables=("V(*)",))
        params = TransientSimulationParameters("1u", "1m", replace_ground=False, print_parameters=print_params)
        # act
        directives = params.to_xyce_directives()
        # assert — emitted directive is .PRINT TRAN, never .PRINT DC
        assert any(d.startswith(".PRINT TRAN") for d in directives)
        assert not any(d.startswith(".PRINT DC") for d in directives)


class TestToXyceDirectivesFft:

    def test_emits_multiple_fft_directives(self):
        # arrange
        fft1 = FftParameters(output_variable="V(1)")
        fft2 = FftParameters(output_variable="V(2)", window="BLACKMAN")
        params = TransientSimulationParameters("1u", "1m", fft_parameters=(fft1, fft2), replace_ground=False)
        # act
        directives = params.to_xyce_directives()
        # assert
        assert len(directives) == 3
        assert directives[0] == ".TRAN 1u 1m"
        assert directives[1] == ".FFT V(1)"
        assert directives[2] == ".FFT V(2) WINDOW=BLACKMAN"


class TestFromXyceDirectivesRoundTrip:

    def test_minimal_round_trip(self):
        # arrange
        original = TransientSimulationParameters("1u", "1m")
        # act
        parsed = TransientSimulationParameters.from_xyce_directives(original.to_xyce_directives())
        # assert
        assert parsed.initial_step_value == "1u"
        assert parsed.final_time_value == "1m"

    def test_full_round_trip(self):
        # arrange
        original = TransientSimulationParameters("1u", "1m", start_time_value="0", step_ceiling_value="5u", op_keyword="NOOP", schedule_points=(TransientSchedulePoint("500u", "1u"),), replace_ground=True)
        # act
        parsed = TransientSimulationParameters.from_xyce_directives(original.to_xyce_directives())
        # assert
        assert parsed.initial_step_value == "1u"
        assert parsed.final_time_value == "1m"
        assert parsed.start_time_value == "0"
        assert parsed.step_ceiling_value == "5u"
        assert parsed.op_keyword == "NOOP"
        assert parsed.replace_ground is True
        assert len(parsed.schedule_points) == 1
        assert parsed.schedule_points[0] == TransientSchedulePoint("500u", "1u")

    def test_round_trip_with_print_parameters(self):
        # arrange
        print_parameters = PrintParameters(print_type="TRAN", print_format="RAW", print_file="waves.raw", output_variables=("V(OUT)", "ID(M1)", "{V(OUT)*I(V1)}"))
        original = TransientSimulationParameters("1u", "1m", print_parameters=print_parameters)
        # act
        parsed = TransientSimulationParameters.from_xyce_directives(original.to_xyce_directives())
        # assert
        assert parsed.print_parameters is not None
        assert parsed.print_parameters.print_type == "TRAN"
        assert parsed.print_parameters.print_format == "RAW"
        assert parsed.print_parameters.print_file == "waves.raw"
        assert parsed.print_parameters.output_variables == ("V(OUT)", "ID(M1)", "{V(OUT)*I(V1)}")

    def test_round_trip_with_fft_parameters(self):
        # arrange
        fft = FftParameters(output_variable="V(OUT)", window="HANN", np="1024")
        original = TransientSimulationParameters("1u", "1m", fft_parameters=(fft,))
        # act
        parsed = TransientSimulationParameters.from_xyce_directives(original.to_xyce_directives())
        # assert
        assert len(parsed.fft_parameters) == 1
        assert parsed.fft_parameters[0].output_variable == "V(OUT)"
        assert parsed.fft_parameters[0].window == "HANN"
        assert parsed.fft_parameters[0].np == "1024"


class TestTransientSchedulePoint:

    def test_stores_time_and_step(self):
        # arrange / act
        point = TransientSchedulePoint(time_value="1u", max_time_step_value="10n")
        # assert
        assert point.time_value == "1u"
        assert point.max_time_step_value == "10n"

    def test_equality(self):
        # arrange
        a = TransientSchedulePoint("1u", "10n")
        b = TransientSchedulePoint("1u", "10n")
        # assert
        assert a == b

    def test_inequality_on_time(self):
        # arrange
        a = TransientSchedulePoint("1u", "10n")
        b = TransientSchedulePoint("2u", "10n")
        # assert
        assert a != b
