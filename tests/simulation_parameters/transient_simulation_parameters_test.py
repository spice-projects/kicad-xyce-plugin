from simulation_parameters import TransientSchedulePoint, TransientSimulationParameters


class TestToXyceDirectivesBasic:

    def test_minimal_directive(self):
        # arrange
        params = TransientSimulationParameters("1u", "1m")
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".TRAN 1u 1m"]

    def test_start_time_is_included_when_provided(self):
        # arrange
        params = TransientSimulationParameters("1n", "10u", start_time_value="100n")
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".TRAN 1n 10u 100n"]

    def test_start_time_defaults_to_zero_when_only_step_ceiling_given(self):
        # arrange
        params = TransientSimulationParameters("1u", "1m", step_ceiling_value="5u")
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".TRAN 1u 1m 0 5u"]

    def test_step_ceiling_is_included_when_provided(self):
        # arrange
        params = TransientSimulationParameters("1u", "1m", start_time_value="0", step_ceiling_value="10u")
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".TRAN 1u 1m 0 10u"]

    def test_start_time_without_step_ceiling_does_not_append_blank(self):
        # arrange
        params = TransientSimulationParameters("1u", "1m", start_time_value="500n")
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".TRAN 1u 1m 500n"]

    def test_op_keyword_noop_is_appended(self):
        # arrange
        params = TransientSimulationParameters("1u", "1m", op_keyword="NOOP")
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".TRAN 1u 1m NOOP"]

    def test_op_keyword_uic_is_appended(self):
        # arrange
        params = TransientSimulationParameters("1u", "1m", op_keyword="UIC")
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".TRAN 1u 1m UIC"]

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
        assert directives == [".TRAN 1u 1m 0 5u UIC"]


class TestToXyceDirectivesSchedule:

    def test_single_schedule_point_produces_schedule_clause(self):
        # arrange
        params = TransientSimulationParameters("1n", "5u", schedule_points=(TransientSchedulePoint("1u", "10n"),))
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".TRAN 1n 5u {schedule(1u, 10n)}"]

    def test_multiple_schedule_points_are_flattened(self):
        # arrange
        params = TransientSimulationParameters("1n", "20u", schedule_points=(TransientSchedulePoint("1u", "10n"), TransientSchedulePoint("10u", "100n")))
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".TRAN 1n 20u {schedule(1u, 10n, 10u, 100n)}"]

    def test_schedule_combined_with_start_and_step_ceiling(self):
        # arrange
        params = TransientSimulationParameters("1n", "10u", start_time_value="0", step_ceiling_value="200n", schedule_points=(TransientSchedulePoint("5u", "50n"),))
        # act
        directives = params.to_xyce_directives()
        # assert
        assert directives == [".TRAN 1n 10u 0 200n {schedule(5u, 50n)}"]

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
        assert params.replace_ground is False


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
