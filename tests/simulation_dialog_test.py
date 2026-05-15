from unittest import TestCase
from unittest.mock import MagicMock, patch
from plugin.simulation_dialog import SimulationDialog, OpSimulationParameters, TransientSimulationParameters

class TestSimulationDialog(TestCase):

    def test_on_submit_op_sets_result(self):
        # arrange
        with patch("plugin.simulation_dialog.SimulationDialog._setup_ui"):
            dialog = SimulationDialog()
            dialog._result = None
            dialog.accept = MagicMock()
            # act
            dialog._on_submit_op()
            # assert
            self.assertIsInstance(dialog._result, OpSimulationParameters)
            dialog.accept.assert_called_once()

    def test_on_submit_transient_sets_result(self):
        # arrange
        with patch("plugin.simulation_dialog.SimulationDialog._setup_ui"):
            dialog = SimulationDialog()
            dialog._result = None
            dialog.accept = MagicMock()
            dialog._root = MagicMock()
            # act
            dialog._on_submit_transient("1u", "1m", "0", "", "", False, "")
            # assert
            self.assertIsInstance(dialog._result, TransientSimulationParameters)
            dialog.accept.assert_called_once()
