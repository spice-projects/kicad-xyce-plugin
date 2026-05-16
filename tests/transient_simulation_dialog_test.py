from unittest import TestCase

from plugin.simulation_dialog import TransientSchedulePoint, TransientSimulationParameters


def _tran(initial_step="1u", final_time="1m", start_time="", step_ceiling="", op_keyword="", schedule_points=tuple()) -> TransientSimulationParameters:
    return TransientSimulationParameters(initial_step, final_time, start_time, step_ceiling, op_keyword, schedule_points)


class TestTransientSimulationParametersBasic(TestCase):

    def test_minimal_directive(self):
        # arrange
        params = _tran(initial_step="1u", final_time="1m")
        # act
        directive = params.to_xyce_directive()
        # assert
        self.assertEqual(directive, ".TRAN 1u 1m")

    def test_start_time_is_included_when_provided(self):
        # arrange
        params = _tran(initial_step="1n", final_time="10u", start_time="100n")
        # act
        directive = params.to_xyce_directive()
        # assert
        self.assertEqual(directive, ".TRAN 1n 10u 100n")

    def test_start_time_defaults_to_zero_when_only_step_ceiling_given(self):
        # arrange — step ceiling provided but start time omitted
        params = _tran(initial_step="1u", final_time="1m", start_time="", step_ceiling="5u")
        # act
        directive = params.to_xyce_directive()
        # assert
        self.assertEqual(directive, ".TRAN 1u 1m 0 5u")

    def test_step_ceiling_is_included_when_provided(self):
        # arrange
        params = _tran(initial_step="1u", final_time="1m", start_time="0", step_ceiling="10u")
        # act
        directive = params.to_xyce_directive()
        # assert
        self.assertEqual(directive, ".TRAN 1u 1m 0 10u")

    def test_start_time_without_step_ceiling_does_not_append_blank(self):
        # arrange — only start time given, no step ceiling
        params = _tran(initial_step="1u", final_time="1m", start_time="500n", step_ceiling="")
        # act
        directive = params.to_xyce_directive()
        # assert
        self.assertEqual(directive, ".TRAN 1u 1m 500n")

    def test_op_keyword_noop_is_appended(self):
        # arrange
        params = _tran(initial_step="1u", final_time="1m", op_keyword="NOOP")
        # act
        directive = params.to_xyce_directive()
        # assert
        self.assertEqual(directive, ".TRAN 1u 1m NOOP")

    def test_op_keyword_uic_is_appended(self):
        # arrange
        params = _tran(initial_step="1u", final_time="1m", op_keyword="UIC")
        # act
        directive = params.to_xyce_directive()
        # assert
        self.assertEqual(directive, ".TRAN 1u 1m UIC")

    def test_empty_op_keyword_is_not_appended(self):
        # arrange
        params = _tran(initial_step="1u", final_time="1m", op_keyword="")
        # act
        directive = params.to_xyce_directive()
        # assert
        self.assertNotIn("NOOP", directive)
        self.assertNotIn("UIC", directive)

    def test_start_time_step_ceiling_and_op_keyword_combined(self):
        # arrange
        params = _tran(initial_step="1u", final_time="1m", start_time="0", step_ceiling="5u", op_keyword="UIC")
        # act
        directive = params.to_xyce_directive()
        # assert
        self.assertEqual(directive, ".TRAN 1u 1m 0 5u UIC")


class TestTransientSimulationParametersSchedule(TestCase):

    def test_single_schedule_point_produces_schedule_clause(self):
        # arrange
        points = (TransientSchedulePoint("1u", "10n"),)
        params = _tran(initial_step="1n", final_time="5u", schedule_points=points)
        # act
        directive = params.to_xyce_directive()
        # assert
        self.assertEqual(directive, ".TRAN 1n 5u {schedule(1u, 10n)}")

    def test_multiple_schedule_points_are_flattened(self):
        # arrange
        points = (
            TransientSchedulePoint("1u", "10n"),
            TransientSchedulePoint("10u", "100n"),
        )
        params = _tran(initial_step="1n", final_time="20u", schedule_points=points)
        # act
        directive = params.to_xyce_directive()
        # assert
        self.assertEqual(directive, ".TRAN 1n 20u {schedule(1u, 10n, 10u, 100n)}")

    def test_schedule_combined_with_start_and_step_ceiling(self):
        # arrange
        points = (TransientSchedulePoint("5u", "50n"),)
        params = _tran(initial_step="1n", final_time="10u", start_time="0", step_ceiling="200n", schedule_points=points)
        # act
        directive = params.to_xyce_directive()
        # assert
        self.assertEqual(directive, ".TRAN 1n 10u 0 200n {schedule(5u, 50n)}")

    def test_empty_schedule_points_produces_no_schedule_clause(self):
        # arrange
        params = _tran(initial_step="1u", final_time="1m", schedule_points=tuple())
        # act
        directive = params.to_xyce_directive()
        # assert
        self.assertNotIn("schedule", directive)


class TestTransientSimulationParametersJson(TestCase):

    def test_to_json_round_trips(self):
        # arrange
        params = _tran(initial_step="2u", final_time="2m", start_time="0", step_ceiling="10u", op_keyword="NOOP")
        # act
        json_str = params.to_json()
        result = TransientSimulationParameters.from_json(json_str)
        # assert
        self.assertEqual(result, params)

    def test_from_json_restores_schedule_points(self):
        # arrange
        points = (TransientSchedulePoint("1u", "10n"),)
        params = _tran(initial_step="1n", final_time="5u", schedule_points=points)
        json_str = params.to_json()
        # act
        result = TransientSimulationParameters.from_json(json_str)
        # assert
        self.assertEqual(len(result.schedule_points), 1)
        self.assertEqual(result.schedule_points[0].time_value, "1u")
        self.assertEqual(result.schedule_points[0].max_time_step_value, "10n")


class TestTransientSchedulePoint(TestCase):

    def test_schedule_point_stores_time_and_step(self):
        # arrange / act
        point = TransientSchedulePoint(time_value="1u", max_time_step_value="10n")
        # assert
        self.assertEqual(point.time_value, "1u")
        self.assertEqual(point.max_time_step_value, "10n")
