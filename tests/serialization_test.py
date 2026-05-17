from simulation_dialog import DCSimulationParameters, NodesetEntry, OpSimulationParameters, TransientSchedulePoint, TransientSimulationParameters


class TestSimulationParameters:

    def test_op_parameters_serialization(self):
        # arrange
        params = OpSimulationParameters()
        # act
        directive = params.to_xyce_directive()
        # assert
        assert ".OP" in directive
        # arrange
        params = OpSimulationParameters(print_dc_enabled=True, print_dc_specific_variables=("V(2)", "I(R1)"))
        # act
        directive = params.to_xyce_directive()
        # assert
        assert ".PRINT DC" in directive
        assert "V(2)" in directive
        assert "I(R1)" in directive
        assert ".OP" in directive
        # arrange
        params = OpSimulationParameters(save_enabled=True, save_type="NODESET", save_file="save.out")
        # act
        directive = params.to_xyce_directive()
        # assert
        assert ".SAVE TYPE=NODESET FILE=save.out" in directive
        # arrange
        params = OpSimulationParameters(nodeset_entries=(NodesetEntry(node="2", voltage="5V"),))
        # act
        directive = params.to_xyce_directive()
        # assert
        assert ".NODESET V(2)=5V" in directive

    def test_op_parameters_deserialization(self):
        # arrange
        directives = [
            ".PRINT DC V(2) I(R1)",
            ".SAVE TYPE=NODESET FILE=save.out",
            ".NODESET V(2)=5V V(3)=2.5V"
        ]
        # act
        params = OpSimulationParameters.from_directives(directives)
        # assert
        assert params.print_dc_enabled
        assert "V(2)" in params.print_dc_specific_variables
        assert "I(R1)" in params.print_dc_specific_variables
        assert params.save_enabled
        assert len(params.nodeset_entries) == 2
        assert params.nodeset_entries[0].node == "2"
        assert params.nodeset_entries[0].voltage == "5V"
        assert params.nodeset_entries[1].node == "3"
        assert params.nodeset_entries[1].voltage == "2.5V"

    def test_transient_parameters_serialization(self):
        # arrange
        params = TransientSimulationParameters(
            initial_step_value="1u",
            final_time_value="1m",
            start_time_value="0",
            step_ceiling_value="10u",
            op_keyword="UIC",
            schedule_points=(TransientSchedulePoint("0.5m", "1u"),)
        )
        # act
        directive = params.to_xyce_directive()
        # assert
        assert directive.startswith(".TRAN 1u 1m 0 10u UIC")
        assert "{schedule(0.5m, 1u)}" in directive

    def test_dc_parameters_serialization(self):
        # arrange
        params = DCSimulationParameters(
            sweep_mode="LIN",
            primary_variable="VIN",
            start="0",
            stop="5",
            step="0.1",
            points="",
            list_values=(),
            data_table_name="",
            secondary_variable="I1",
            secondary_start="0",
            secondary_stop="1",
            secondary_step="0.2",
            secondary_points=""
        )
        # act
        directive = params.to_xyce_directive()
        # assert
        assert directive == ".DC VIN 0 5 0.1 I1 0 1 0.2"
        # arrange
        params = DCSimulationParameters(
            sweep_mode="LIST",
            primary_variable="V1",
            start="",
            stop="",
            step="",
            points="",
            list_values=("1", "2", "3"),
            data_table_name="",
            secondary_variable="",
            secondary_start="",
            secondary_stop="",
            secondary_step="",
            secondary_points=""
        )
        # act
        directive = params.to_xyce_directive()
        # assert
        assert directive == ".DC V1 LIST 1 2 3"
