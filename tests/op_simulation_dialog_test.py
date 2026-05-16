from unittest import TestCase
from unittest.mock import MagicMock

from simulation_dialog import NodesetEntry
from simulation_dialog import OpSimulationParameters


class TestOpSimulationParameters(TestCase):

    def test_op_directive_default(self):
        # arrange
        params = OpSimulationParameters()
        # act
        directive = params.to_xyce_directive()
        # assert
        self.assertEqual(directive, ".OP")

    def test_print_dc_directive(self):
        # arrange
        params = OpSimulationParameters(
            print_dc_enabled=True,
            print_dc_specific_variables=("V(1)", "I(V1)")
        )
        # act
        directive = params.to_xyce_directive()
        # assert
        self.assertEqual(directive, ".OP\n.PRINT DC V(1) I(V1)")

    def test_save_directive(self):
        # arrange
        params = OpSimulationParameters(
            save_enabled=True,
            save_type="IC",
            save_file="test.ic"
        )
        # act
        directive = params.to_xyce_directive()
        # assert
        self.assertEqual(directive, ".OP\n.SAVE TYPE=IC FILE=test.ic")

    def test_nodeset_directive(self):
        # arrange
        entries = (NodesetEntry(node="out", voltage="1.2"),)
        params = OpSimulationParameters(nodeset_entries=entries)
        # act
        directive = params.to_xyce_directive()
        # assert
        self.assertEqual(directive, ".OP\n.NODESET V(out)=1.2")

    def test_dynamic_resolution(self):
        # arrange
        params = OpSimulationParameters(
            print_dc_enabled=True,
            print_dc_all_nodes=True,
            print_dc_all_currents=True
        )
        topology = MagicMock()
        topology.nodes = ["1", "2"]
        topology.devices = [MagicMock(name="R1"), MagicMock(name="V1")]
        topology.devices[0].name = "R1"
        topology.devices[1].name = "V1"
        # act
        directive = params.to_xyce_directive(topology=topology)
        # assert
        self.assertIn(".PRINT DC V(1) V(2) I(R1) I(V1)", directive)
