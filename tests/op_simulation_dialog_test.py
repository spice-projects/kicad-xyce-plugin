from unittest import TestCase

from plugin.simulation_dialog import OpSimulationParameters


class TestOpSimulationParameters(TestCase):

    def test_op_directive(self):
        # arrange
        params = OpSimulationParameters()
        # act
        directive = params.to_xyce_directive()
        # assert
        self.assertEqual(directive, ".OP")

    def test_to_json(self):
        # arrange
        params = OpSimulationParameters()
        # act
        json_str = params.to_json()
        # assert
        self.assertEqual(json_str, "{}")

    def test_from_json(self):
        # arrange
        json_str = "{}"
        # act
        params = OpSimulationParameters.from_json(json_str)
        # assert
        self.assertIsInstance(params, OpSimulationParameters)
