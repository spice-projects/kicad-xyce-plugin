from kicad_xyce_plugin.simulation_parameters import MeasureEntry


class TestMeasureEntry:

    def test_minimal_avg_measure(self):
        # arrange
        statement = ".MEASURE TRAN avgAll AVG V(1)"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.analysis_type == "TRAN"
        assert params.result_name == "avgAll"
        assert params.measure_type == "AVG"
        assert params.variable == "V(1)"
        assert params.to_xyce_statement() == ".MEASURE TRAN avgAll AVG V(1)"

    def test_minimal_max_measure(self):
        # arrange
        statement = ".MEASURE TRAN maxV1 MAX V(1)"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.analysis_type == "TRAN"
        assert params.result_name == "maxV1"
        assert params.measure_type == "MAX"
        assert params.variable == "V(1)"
        assert params.to_xyce_statement() == ".MEASURE TRAN maxV1 MAX V(1)"

    def test_minimal_min_measure(self):
        # arrange
        statement = ".MEASURE TRAN minV1 MIN V(1)"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.analysis_type == "TRAN"
        assert params.result_name == "minV1"
        assert params.measure_type == "MIN"
        assert params.variable == "V(1)"
        assert params.to_xyce_statement() == ".MEASURE TRAN minV1 MIN V(1)"

    def test_minimal_rms_measure(self):
        # arrange
        statement = ".MEASURE TRAN rmsV1 RMS V(1)"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.analysis_type == "TRAN"
        assert params.result_name == "rmsV1"
        assert params.measure_type == "RMS"
        assert params.variable == "V(1)"
        assert params.to_xyce_statement() == ".MEASURE TRAN rmsV1 RMS V(1)"

    def test_minimal_pp_measure(self):
        # arrange
        statement = ".MEASURE TRAN ppV1 PP V(1)"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.analysis_type == "TRAN"
        assert params.result_name == "ppV1"
        assert params.measure_type == "PP"
        assert params.variable == "V(1)"
        assert params.to_xyce_statement() == ".MEASURE TRAN ppV1 PP V(1)"

    def test_minimal_integ_measure(self):
        # arrange
        statement = ".MEASURE TRAN integV1 INTEG V(1)"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.analysis_type == "TRAN"
        assert params.result_name == "integV1"
        assert params.measure_type == "INTEG"
        assert params.variable == "V(1)"
        assert params.to_xyce_statement() == ".MEASURE TRAN integV1 INTEG V(1)"

    def test_minimal_eqn_measure(self):
        # arrange
        statement = ".MEASURE TRAN eqn1 EQN V(1)"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.analysis_type == "TRAN"
        assert params.result_name == "eqn1"
        assert params.measure_type == "EQN"
        assert params.variable == "V(1)"
        assert params.to_xyce_statement() == ".MEASURE TRAN eqn1 EQN V(1)"

    def test_avg_with_from_to(self):
        # arrange
        statement = ".MEASURE TRAN avgAll AVG V(1) FROM=1m TO=5m"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.from_val == "1m"
        assert params.to_val == "5m"
        assert params.to_xyce_statement() == ".MEASURE TRAN avgAll AVG V(1) FROM=1m TO=5m"

    def test_avg_with_td(self):
        # arrange
        statement = ".MEASURE TRAN avgAll AVG V(1) TD=1m"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.td_val == "1m"
        assert params.to_xyce_statement() == ".MEASURE TRAN avgAll AVG V(1) TD=1m"

    def test_max_with_rise_fall_cross(self):
        # arrange
        statement = ".MEASURE TRAN maxV1 MAX V(1) RISE=2 FALL=1 CROSS=3"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.rise_val == "2"
        assert params.fall_val == "1"
        assert params.cross_val == "3"
        assert params.to_xyce_statement() == ".MEASURE TRAN maxV1 MAX V(1) RISE=2 FALL=1 CROSS=3"

    def test_measure_with_minval(self):
        # arrange
        statement = ".MEASURE TRAN maxV1 MAX V(1) MINVAL=0.1"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.minval == "0.1"
        assert params.to_xyce_statement() == ".MEASURE TRAN maxV1 MAX V(1) MINVAL=0.1"

    def test_measure_with_default_val(self):
        # arrange
        statement = ".MEASURE TRAN maxV1 MAX V(1) DEFAULT_VAL=0"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.default_val == "0"
        assert params.to_xyce_statement() == ".MEASURE TRAN maxV1 MAX V(1) DEFAULT_VAL=0"

    def test_measure_with_precision(self):
        # arrange
        statement = ".MEASURE TRAN maxV1 MAX V(1) PRECISION=6"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.precision == "6"
        assert params.to_xyce_statement() == ".MEASURE TRAN maxV1 MAX V(1) PRECISION=6"

    def test_measure_with_print(self):
        # arrange
        statement = ".MEASURE TRAN maxV1 MAX V(1) PRINT=1"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.print_val == "1"
        assert params.to_xyce_statement() == ".MEASURE TRAN maxV1 MAX V(1) PRINT=1"

    def test_deriv_at_measure(self):
        # arrange
        statement = ".MEASURE TRAN deriv1 DERIV V(1) AT=5"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "DERIV"
        assert params.at_val == "5"
        assert params.to_xyce_statement() == ".MEASURE TRAN deriv1 DERIV V(1) AT=5"

    def test_deriv_when_measure(self):
        # arrange
        statement = ".MEASURE TRAN deriv1 DERIV V(1) WHEN V(2)=0.75"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "DERIV"
        assert params.variable == "V(1)"
        assert params.when_variable == "V(2)"
        assert params.when_condition == "=0.75"
        assert params.to_xyce_statement() == ".MEASURE TRAN deriv1 DERIV V(1) WHEN V(2)=0.75"

    def test_find_at_measure(self):
        # arrange
        statement = ".MEASURE TRAN find1 FIND V(1) AT=5"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "FIND"
        assert params.at_val == "5"
        assert params.to_xyce_statement() == ".MEASURE TRAN find1 FIND V(1) AT=5"

    def test_find_when_measure(self):
        # arrange
        statement = ".MEASURE TRAN find1 FIND V(1) WHEN V(2)=0.75"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "FIND"
        assert params.variable == "V(1)"
        assert params.when_variable == "V(2)"
        assert params.when_condition == "=0.75"
        assert params.to_xyce_statement() == ".MEASURE TRAN find1 FIND V(1) WHEN V(2)=0.75"

    def test_when_measure(self):
        # arrange
        statement = ".MEASURE TRAN when1 WHEN V(1)=0.75"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "WHEN"
        # WHEN measure type uses when_variable instead of variable
        assert params.when_variable == "V(1)"
        assert params.when_condition == "=0.75"
        assert params.to_xyce_statement() == ".MEASURE TRAN when1 WHEN V(1)=0.75"

    def test_duty_measure(self):
        # arrange
        statement = ".MEASURE TRAN dutyAll DUTY V(1) ON=0.75 OFF=0.25"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "DUTY"
        assert params.on_val == "0.75"
        assert params.off_val == "0.25"
        assert params.to_xyce_statement() == ".MEASURE TRAN dutyAll DUTY V(1) ON=0.75 OFF=0.25"

    def test_freq_measure(self):
        # arrange
        statement = ".MEASURE TRAN freq1 FREQ V(1) ON=0.75 OFF=0.25"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "FREQ"
        assert params.on_val == "0.75"
        assert params.off_val == "0.25"
        assert params.to_xyce_statement() == ".MEASURE TRAN freq1 FREQ V(1) ON=0.75 OFF=0.25"

    def test_on_time_measure(self):
        # arrange
        statement = ".MEASURE TRAN onTime1 ON_TIME V(1) ON=0.75"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "ON_TIME"
        assert params.on_val == "0.75"
        assert params.to_xyce_statement() == ".MEASURE TRAN onTime1 ON_TIME V(1) ON=0.75"

    def test_off_time_measure(self):
        # arrange
        statement = ".MEASURE TRAN offTime1 OFF_TIME V(1) OFF=0.25"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "OFF_TIME"
        assert params.off_val == "0.25"
        assert params.to_xyce_statement() == ".MEASURE TRAN offTime1 OFF_TIME V(1) OFF=0.25"

    def test_four_measure(self):
        # arrange
        statement = ".MEASURE TRAN four1 FOUR V(1) AT=1k NUMFREQ=10 GRIDSIZE=200"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "FOUR"
        assert params.at_val == "1k"
        assert params.numfreq == "10"
        assert params.gridsize == "200"
        assert params.to_xyce_statement() == ".MEASURE TRAN four1 FOUR V(1) AT=1k NUMFREQ=10 GRIDSIZE=200"

    def test_err1_measure(self):
        # arrange
        statement = ".MEASURE TRAN err1 ERR1 V(1) V(2)"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "ERR1"
        assert params.variable == "V(1)"
        assert params.variable2 == "V(2)"
        assert params.to_xyce_statement() == ".MEASURE TRAN err1 ERR1 V(1) V(2)"

    def test_err2_measure(self):
        # arrange
        statement = ".MEASURE TRAN err2 ERR2 V(1) V(2)"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "ERR2"
        assert params.variable == "V(1)"
        assert params.variable2 == "V(2)"
        assert params.to_xyce_statement() == ".MEASURE TRAN err2 ERR2 V(1) V(2)"

    def test_error_measure(self):
        # arrange
        statement = ".MEASURE TRAN error1 ERROR V(1) FILE=data.csv INDEPVARCOL=1 DEPVARCOL=2 COMP_FUNCTION=L2"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "ERROR"
        assert params.error_file == "data.csv"
        assert params.indepvarcol == "1"
        assert params.depvarcol == "2"
        assert params.comp_function == "L2"
        assert params.to_xyce_statement() == ".MEASURE TRAN error1 ERROR V(1) FILE=data.csv INDEPVARCOL=1 DEPVARCOL=2 COMP_FUNCTION=L2"

    def test_trig_targ_at_at(self):
        # arrange
        statement = ".MEASURE TRAN trig1 TRIG AT=2ms TARG AT=8ms"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "TRIG"
        assert params.trig_at_val == "2ms"
        assert params.targ_at_val == "8ms"
        assert params.to_xyce_statement() == ".MEASURE TRAN trig1 TRIG AT=2ms TARG AT=8ms"

    def test_trig_targ_variable_variable(self):
        # arrange
        statement = ".MEASURE TRAN trig1 TRIG V(1)=0.2 TARG V(2)=0.8"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "TRIG"
        assert params.trig_variable == "V(1)"
        assert params.trig_condition == "=0.2"
        assert params.targ_variable == "V(2)"
        assert params.targ_condition == "=0.8"
        # The serializer reconstructs the condition properly
        assert params.to_xyce_statement() == ".MEASURE TRAN trig1 TRIG V(1)=0.2 TARG V(2)=0.8"

    def test_trig_targ_with_qualifiers(self):
        # arrange
        statement = ".MEASURE TRAN trig1 TRIG V(1)=0.2 RISE=1 TD=1ms TARG V(2)=0.8 FALL=2 TD=5ms"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "TRIG"
        assert params.trig_variable == "V(1)"
        assert params.trig_condition == "=0.2"
        assert params.trig_rise == "1"
        assert params.trig_td == "1ms"
        assert params.targ_variable == "V(2)"
        assert params.targ_condition == "=0.8"
        assert params.targ_fall == "2"
        assert params.targ_td == "5ms"
        # The serializer reorders qualifiers, but the semantic meaning is preserved
        regenerated = params.to_xyce_statement()
        assert "TRIG V(1)=0.2" in regenerated
        assert "TD=1ms" in regenerated
        assert "RISE=1" in regenerated
        assert "TARG V(2)=0.8" in regenerated
        assert "TD=5ms" in regenerated
        assert "FALL=2" in regenerated

    def test_ac_analysis_measure(self):
        # arrange
        statement = ".MEASURE AC maxV1 MAX V(1)"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.analysis_type == "AC"
        assert params.measure_type == "MAX"
        assert params.variable == "V(1)"
        assert params.to_xyce_statement() == ".MEASURE AC maxV1 MAX V(1)"

    def test_dc_analysis_measure(self):
        # arrange
        statement = ".MEASURE DC maxV1 MAX V(1)"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.analysis_type == "DC"
        assert params.measure_type == "MAX"
        assert params.variable == "V(1)"
        assert params.to_xyce_statement() == ".MEASURE DC maxV1 MAX V(1)"

    def test_noise_analysis_measure(self):
        # arrange
        statement = ".MEASURE NOISE maxonoise MAX ONOISE"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.analysis_type == "NOISE"
        assert params.measure_type == "MAX"
        assert params.variable == "ONOISE"
        assert params.to_xyce_statement() == ".MEASURE NOISE maxonoise MAX ONOISE"

    def test_tran_cont_measure(self):
        # arrange
        statement = ".MEASURE TRAN_CONT deriv1 DERIV V(1) AT=5"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.analysis_type == "TRAN_CONT"
        assert params.measure_type == "DERIV"
        assert params.at_val == "5"
        assert params.to_xyce_statement() == ".MEASURE TRAN_CONT deriv1 DERIV V(1) AT=5"

    def test_ac_cont_measure(self):
        # arrange
        statement = ".MEASURE AC_CONT find1 FIND V(1) AT=5"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.analysis_type == "AC_CONT"
        assert params.measure_type == "FIND"
        assert params.at_val == "5"
        assert params.to_xyce_statement() == ".MEASURE AC_CONT find1 FIND V(1) AT=5"

    def test_meas_alias(self):
        # arrange
        statement = ".MEAS TRAN maxV1 MAX V(1)"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.analysis_type == "TRAN"
        assert params.measure_type == "MAX"
        assert params.variable == "V(1)"
        assert params.to_xyce_statement() == ".MEASURE TRAN maxV1 MAX V(1)"

    def test_complex_variable_expression(self):
        # arrange
        statement = ".MEASURE TRAN avgPower AVG {V(1)*I(R1)}"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.variable == "{V(1)*I(R1)}"
        assert params.to_xyce_statement() == ".MEASURE TRAN avgPower AVG {V(1)*I(R1)}"

    def test_max_with_output_qualifier(self):
        # arrange
        statement = ".MEASURE TRAN maxV1 MAX V(1) OUTPUT=output.txt"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.output == "output.txt"
        assert params.to_xyce_statement() == ".MEASURE TRAN maxV1 MAX V(1) OUTPUT=output.txt"

    def test_max_with_rfc_level_qualifier(self):
        # arrange
        statement = ".MEASURE TRAN maxV1 MAX V(1) RFC_LEVEL=0.5"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.rfc_level == "0.5"
        assert params.to_xyce_statement() == ".MEASURE TRAN maxV1 MAX V(1) RFC_LEVEL=0.5"

    def test_invalid_measure_statement(self):
        # arrange
        statement = ".TRAN 1u 1m"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is None

    def test_invalid_analysis_type(self):
        # arrange
        statement = ".MEASURE INVALID maxV1 MAX V(1)"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is None

    def test_invalid_measure_type(self):
        # arrange
        statement = ".MEASURE TRAN maxV1 INVALID V(1)"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is None

    def test_too_short_statement(self):
        # arrange
        statement = ".MEASURE TRAN"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is None

    def test_round_trip_avg_measure(self):
        # arrange
        original = ".MEASURE TRAN avgAll AVG V(1) FROM=1m TO=5m TD=100u MINVAL=0.1 DEFAULT_VAL=0 PRECISION=6"
        # act
        params = MeasureEntry.from_xyce_statement(original)
        regenerated = params.to_xyce_statement() if params else None
        # assert
        assert params is not None
        assert regenerated == original

    def test_round_trip_trig_targ(self):
        # arrange
        original = ".MEASURE TRAN trig1 TRIG V(1)=0.2 RISE=1 TD=1ms TARG V(2)=0.8 FALL=2 TD=5ms"
        # act
        params = MeasureEntry.from_xyce_statement(original)
        regenerated = params.to_xyce_statement() if params else None
        # assert
        assert params is not None
        # The serializer reorders qualifiers, but the semantic meaning is preserved
        assert "TRIG V(1)=0.2" in regenerated
        assert "TD=1ms" in regenerated
        assert "RISE=1" in regenerated
        assert "TARG V(2)=0.8" in regenerated
        assert "TD=5ms" in regenerated
        assert "FALL=2" in regenerated

    def test_round_trip_complex_measure(self):
        # arrange
        original = ".MEASURE TRAN maxV1 MAX V(1) FROM=1m TO=5m RISE=2 FALL=1 CROSS=3 MINVAL=0.1 DEFAULT_VAL=0 PRECISION=6 PRINT=1 RFC_LEVEL=0.5 OUTPUT=output.txt"
        # act
        params = MeasureEntry.from_xyce_statement(original)
        regenerated = params.to_xyce_statement() if params else None
        # assert
        assert params is not None
        assert regenerated == original

    def test_dc_cont_measure(self):
        # arrange
        statement = ".MEASURE DC_CONT deriv1 DERIV V(1) WHEN V(2)=0.75"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.analysis_type == "DC_CONT"
        assert params.measure_type == "DERIV"
        assert params.when_variable == "V(2)"
        assert params.when_condition == "=0.75"
        assert params.to_xyce_statement() == ".MEASURE DC_CONT deriv1 DERIV V(1) WHEN V(2)=0.75"

    def test_noise_cont_measure(self):
        # arrange
        statement = ".MEASURE NOISE_CONT find1 FIND ONOISE WHEN V(2)=1 RISE=2"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.analysis_type == "NOISE_CONT"
        assert params.measure_type == "FIND"
        assert params.when_variable == "V(2)"
        assert params.when_condition == "=1"
        assert params.to_xyce_statement() == ".MEASURE NOISE_CONT find1 FIND ONOISE WHEN V(2)=1 RISE=2"

    def test_cont_trig_targ_at_at(self):
        # arrange
        statement = ".MEASURE TRAN_CONT trig1 TRIG AT=2ms TARG AT=8ms"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.analysis_type == "TRAN_CONT"
        assert params.measure_type == "TRIG"
        assert params.trig_at_val == "2ms"
        assert params.targ_at_val == "8ms"
        assert params.to_xyce_statement() == ".MEASURE TRAN_CONT trig1 TRIG AT=2ms TARG AT=8ms"

    # Reference guide examples from Xyce_RG.txt section 2.1.18
    def test_reference_guide_example_when_with_minval(self):
        # arrange - .MEASURE TRAN hit1_75 WHEN V(1)=0.75 MINVAL=0.02
        statement = ".MEASURE TRAN hit1_75 WHEN V(1)=0.75 MINVAL=0.02"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.result_name == "hit1_75"
        assert params.measure_type == "WHEN"
        assert params.when_variable == "V(1)"
        assert params.when_condition == "=0.75"
        assert params.minval == "0.02"
        assert params.to_xyce_statement() == ".MEASURE TRAN hit1_75 WHEN V(1)=0.75 MINVAL=0.02"

    def test_reference_guide_example_when_with_minval_rise(self):
        # arrange - .MEASURE TRAN hit2_75 WHEN V(1)=0.75 MINVAL=0.08 RISE=2
        statement = ".MEASURE TRAN hit2_75 WHEN V(1)=0.75 MINVAL=0.08 RISE=2"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.result_name == "hit2_75"
        assert params.measure_type == "WHEN"
        assert params.when_variable == "V(1)"
        assert params.when_condition == "=0.75"
        assert params.minval == "0.08"
        assert params.rise_val == "2"
        # Serializer reorders qualifiers, check for component presence
        regenerated = params.to_xyce_statement()
        assert "WHEN V(1)=0.75" in regenerated
        assert "MINVAL=0.08" in regenerated
        assert "RISE=2" in regenerated

    def test_reference_guide_example_ac_real_part(self):
        # arrange - .MEASURE AC maxV1R MAX VR(1)
        statement = ".MEASURE AC maxV1R MAX VR(1)"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.analysis_type == "AC"
        assert params.result_name == "maxV1R"
        assert params.measure_type == "MAX"
        assert params.variable == "VR(1)"
        assert params.to_xyce_statement() == ".MEASURE AC maxV1R MAX VR(1)"

    # Additional coverage for missing parser paths
    def test_when_with_minval_and_rise_fall_cross(self):
        # arrange - test WHEN with all RFC qualifiers and MINVAL
        statement = ".MEASURE TRAN when1 WHEN V(1)=0.75 MINVAL=0.02 RISE=2 FALL=1 CROSS=3"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "WHEN"
        assert params.when_variable == "V(1)"
        assert params.when_condition == "=0.75"
        assert params.minval == "0.02"
        assert params.rise_val == "2"
        assert params.fall_val == "1"
        assert params.cross_val == "3"
        # Serializer reorders qualifiers, check for component presence
        regenerated = params.to_xyce_statement()
        assert "WHEN V(1)=0.75" in regenerated
        assert "MINVAL=0.02" in regenerated
        assert "RISE=2" in regenerated
        assert "FALL=1" in regenerated
        assert "CROSS=3" in regenerated

    def test_trig_variable_without_condition(self):
        # arrange - TRIG with variable but no condition
        statement = ".MEASURE TRAN trig1 TRIG V(1) TARG V(2)=0.8"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "TRIG"
        assert params.trig_variable == "V(1)"
        assert params.trig_condition == ""
        assert params.targ_variable == "V(2)"
        assert params.targ_condition == "=0.8"
        assert params.to_xyce_statement() == ".MEASURE TRAN trig1 TRIG V(1) TARG V(2)=0.8"

    def test_targ_variable_without_condition(self):
        # arrange - TARG with variable but no condition
        statement = ".MEASURE TRAN trig1 TRIG V(1)=0.2 TARG V(2)"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "TRIG"
        assert params.trig_variable == "V(1)"
        assert params.trig_condition == "=0.2"
        assert params.targ_variable == "V(2)"
        assert params.targ_condition == ""
        assert params.to_xyce_statement() == ".MEASURE TRAN trig1 TRIG V(1)=0.2 TARG V(2)"

    def test_find_when_with_all_qualifiers(self):
        # arrange - FIND WHEN with all qualifiers
        statement = ".MEASURE TRAN find1 FIND V(1) WHEN V(2)=0.75 FROM=1m TO=5m TD=1ms RISE=2 FALL=1 CROSS=3 MINVAL=0.1 DEFAULT_VAL=0"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "FIND"
        assert params.variable == "V(1)"
        assert params.when_variable == "V(2)"
        assert params.when_condition == "=0.75"
        assert params.from_val == "1m"
        assert params.to_val == "5m"
        assert params.td_val == "1ms"
        assert params.rise_val == "2"
        assert params.fall_val == "1"
        assert params.cross_val == "3"
        assert params.minval == "0.1"
        assert params.default_val == "0"
        # Serializer order may vary, check key components
        regenerated = params.to_xyce_statement()
        assert "FIND V(1)" in regenerated
        assert "WHEN V(2)=0.75" in regenerated
        assert "FROM=1m" in regenerated
        assert "TO=5m" in regenerated
        assert "TD=1ms" in regenerated
        assert "RISE=2" in regenerated
        assert "FALL=1" in regenerated
        assert "CROSS=3" in regenerated
        assert "MINVAL=0.1" in regenerated
        assert "DEFAULT_VAL=0" in regenerated

    def test_deriv_when_with_all_qualifiers(self):
        # arrange - DERIV WHEN with all qualifiers
        statement = ".MEASURE TRAN deriv1 DERIV V(1) WHEN V(2)=0.75 FROM=1m TO=5m TD=1ms RISE=2 FALL=1 CROSS=3 MINVAL=0.1"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "DERIV"
        assert params.variable == "V(1)"
        assert params.when_variable == "V(2)"
        assert params.when_condition == "=0.75"
        assert params.from_val == "1m"
        assert params.to_val == "5m"
        assert params.td_val == "1ms"
        assert params.rise_val == "2"
        assert params.fall_val == "1"
        assert params.cross_val == "3"
        assert params.minval == "0.1"
        # Serializer order may vary, check key components
        regenerated = params.to_xyce_statement()
        assert "DERIV V(1)" in regenerated
        assert "WHEN V(2)=0.75" in regenerated
        assert "FROM=1m" in regenerated
        assert "TO=5m" in regenerated
        assert "TD=1ms" in regenerated
        assert "RISE=2" in regenerated
        assert "FALL=1" in regenerated
        assert "CROSS=3" in regenerated
        assert "MINVAL=0.1" in regenerated

    def test_err1_with_all_qualifiers(self):
        # arrange - ERR1 with all qualifiers
        statement = ".MEASURE TRAN err1 ERR1 V(1) V(2) FROM=1m TO=5m MINVAL=0.1"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "ERR1"
        assert params.variable == "V(1)"
        assert params.variable2 == "V(2)"
        assert params.from_val == "1m"
        assert params.to_val == "5m"
        assert params.minval == "0.1"
        # Serializer order may vary, check key components
        regenerated = params.to_xyce_statement()
        assert "ERR1 V(1) V(2)" in regenerated
        assert "FROM=1m" in regenerated
        assert "TO=5m" in regenerated
        assert "MINVAL=0.1" in regenerated

    def test_error_without_comp_function(self):
        # arrange - ERROR without COMP_FUNCTION
        statement = ".MEASURE TRAN error1 ERROR V(1) FILE=data.csv INDEPVARCOL=1 DEPVARCOL=2"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "ERROR"
        assert params.error_file == "data.csv"
        assert params.indepvarcol == "1"
        assert params.depvarcol == "2"
        assert params.comp_function == ""
        assert params.to_xyce_statement() == ".MEASURE TRAN error1 ERROR V(1) FILE=data.csv INDEPVARCOL=1 DEPVARCOL=2"

    def test_error_with_only_file(self):
        # arrange - ERROR with only FILE parameter
        statement = ".MEASURE TRAN error1 ERROR V(1) FILE=data.csv"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "ERROR"
        assert params.error_file == "data.csv"
        assert params.indepvarcol == ""
        assert params.depvarcol == ""
        assert params.comp_function == ""
        assert params.to_xyce_statement() == ".MEASURE TRAN error1 ERROR V(1) FILE=data.csv"

    def test_four_without_optional_params(self):
        # arrange - FOUR without NUMFREQ and GRIDSIZE
        statement = ".MEASURE TRAN four1 FOUR V(1) AT=1k"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "FOUR"
        assert params.at_val == "1k"
        assert params.numfreq == ""
        assert params.gridsize == ""
        assert params.to_xyce_statement() == ".MEASURE TRAN four1 FOUR V(1) AT=1k"

    def test_four_with_from_to_td(self):
        # arrange - FOUR with measurement window qualifiers
        statement = ".MEASURE TRAN four1 FOUR V(1) AT=1k FROM=1m TO=5m TD=100u"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "FOUR"
        assert params.at_val == "1k"
        assert params.from_val == "1m"
        assert params.to_val == "5m"
        # TD is not supported for FOUR, so it will be ignored
        assert params.td_val == ""
        # Serializer will not include TD for FOUR
        regenerated = params.to_xyce_statement()
        assert "FOUR V(1)" in regenerated
        assert "AT=1k" in regenerated
        assert "FROM=1m" in regenerated
        assert "TO=5m" in regenerated
        assert "TD" not in regenerated

    def test_avg_with_min_thresh_max_thresh(self):
        # arrange - AVG with MIN_THRESH and MAX_THRESH (TRAN-specific)
        statement = ".MEASURE TRAN avgAll AVG V(1) MIN_THRESH=0.1 MAX_THRESH=0.9"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "AVG"
        assert params.variable == "V(1)"
        assert params.min_thresh == "0.1"
        assert params.max_thresh == "0.9"
        assert params.to_xyce_statement() == ".MEASURE TRAN avgAll AVG V(1) MIN_THRESH=0.1 MAX_THRESH=0.9"

    def test_deriv_at_with_minval(self):
        # arrange - DERIV AT with MINVAL
        statement = ".MEASURE TRAN deriv1 DERIV V(1) AT=5 MINVAL=0.1"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "DERIV"
        assert params.at_val == "5"
        assert params.minval == "0.1"
        # Serializer reorders qualifiers, check for component presence
        regenerated = params.to_xyce_statement()
        assert "DERIV V(1)" in regenerated
        assert "AT=5" in regenerated
        assert "MINVAL=0.1" in regenerated

    def test_find_at_with_minval(self):
        # arrange - FIND AT with MINVAL
        statement = ".MEASURE TRAN find1 FIND V(1) AT=5 MINVAL=0.1"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "FIND"
        assert params.at_val == "5"
        assert params.minval == "0.1"
        # Serializer reorders qualifiers, check for component presence
        regenerated = params.to_xyce_statement()
        assert "FIND V(1)" in regenerated
        assert "AT=5" in regenerated
        assert "MINVAL=0.1" in regenerated

    def test_when_with_default_val(self):
        # arrange - WHEN with DEFAULT_VAL
        statement = ".MEASURE TRAN when1 WHEN V(1)=0.75 DEFAULT_VAL=0"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "WHEN"
        assert params.when_variable == "V(1)"
        assert params.when_condition == "=0.75"
        assert params.default_val == "0"
        assert params.to_xyce_statement() == ".MEASURE TRAN when1 WHEN V(1)=0.75 DEFAULT_VAL=0"

    def test_when_with_precision(self):
        # arrange - WHEN with PRECISION
        statement = ".MEASURE TRAN when1 WHEN V(1)=0.75 PRECISION=6"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "WHEN"
        assert params.when_variable == "V(1)"
        assert params.when_condition == "=0.75"
        assert params.precision == "6"
        assert params.to_xyce_statement() == ".MEASURE TRAN when1 WHEN V(1)=0.75 PRECISION=6"

    def test_when_with_print(self):
        # arrange - WHEN with PRINT
        statement = ".MEASURE TRAN when1 WHEN V(1)=0.75 PRINT=1"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "WHEN"
        assert params.when_variable == "V(1)"
        assert params.when_condition == "=0.75"
        assert params.print_val == "1"
        assert params.to_xyce_statement() == ".MEASURE TRAN when1 WHEN V(1)=0.75 PRINT=1"

    # additional coverage tests for missing lines
    def test_trig_with_rise_qualifier(self):
        # arrange - TRIG with RISE qualifier
        statement = ".MEASURE TRAN trig1 TRIG V(1)=0.2 RISE=1 TARG V(2)=0.8"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "TRIG"
        assert params.trig_variable == "V(1)"
        assert params.trig_condition == "=0.2"
        assert params.trig_rise == "1"
        assert params.targ_variable == "V(2)"
        assert params.targ_condition == "=0.8"
        regenerated = params.to_xyce_statement()
        assert "TRIG V(1)=0.2" in regenerated
        assert "RISE=1" in regenerated
        assert "TARG V(2)=0.8" in regenerated

    def test_trig_with_cross_qualifier(self):
        # arrange - TRIG with CROSS qualifier
        statement = ".MEASURE TRAN trig1 TRIG V(1)=0.2 CROSS=2 TARG V(2)=0.8"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "TRIG"
        assert params.trig_variable == "V(1)"
        assert params.trig_condition == "=0.2"
        assert params.trig_cross == "2"
        assert params.targ_variable == "V(2)"
        assert params.targ_condition == "=0.8"
        regenerated = params.to_xyce_statement()
        assert "TRIG V(1)=0.2" in regenerated
        assert "CROSS=2" in regenerated
        assert "TARG V(2)=0.8" in regenerated

    def test_trig_with_unknown_qualifier(self):
        # arrange - TRIG with unknown qualifier
        statement = ".MEASURE TRAN trig1 TRIG V(1)=0.2 UNKNOWN=1 TARG V(2)=0.8"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "TRIG"
        assert params.trig_variable == "V(1)"
        assert params.trig_condition == "=0.2"
        # unknown qualifier should be ignored
        regenerated = params.to_xyce_statement()
        assert "TRIG V(1)=0.2" in regenerated
        assert "TARG V(2)=0.8" in regenerated
        assert "UNKNOWN" not in regenerated

    def test_targ_with_rise_qualifier(self):
        # arrange - TARG with RISE qualifier
        statement = ".MEASURE TRAN trig1 TRIG V(1)=0.2 TARG V(2)=0.8 RISE=1"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "TRIG"
        assert params.trig_variable == "V(1)"
        assert params.trig_condition == "=0.2"
        assert params.targ_variable == "V(2)"
        assert params.targ_condition == "=0.8"
        assert params.targ_rise == "1"
        regenerated = params.to_xyce_statement()
        assert "TRIG V(1)=0.2" in regenerated
        assert "TARG V(2)=0.8" in regenerated
        assert "RISE=1" in regenerated

    def test_targ_with_cross_qualifier(self):
        # arrange - TARG with CROSS qualifier
        statement = ".MEASURE TRAN trig1 TRIG V(1)=0.2 TARG V(2)=0.8 CROSS=2"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "TRIG"
        assert params.trig_variable == "V(1)"
        assert params.trig_condition == "=0.2"
        assert params.targ_variable == "V(2)"
        assert params.targ_condition == "=0.8"
        assert params.targ_cross == "2"
        regenerated = params.to_xyce_statement()
        assert "TRIG V(1)=0.2" in regenerated
        assert "TARG V(2)=0.8" in regenerated
        assert "CROSS=2" in regenerated

    def test_targ_with_unknown_qualifier(self):
        # arrange - TARG with unknown qualifier
        statement = ".MEASURE TRAN trig1 TRIG V(1)=0.2 TARG V(2)=0.8 UNKNOWN=1"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "TRIG"
        assert params.trig_variable == "V(1)"
        assert params.trig_condition == "=0.2"
        assert params.targ_variable == "V(2)"
        assert params.targ_condition == "=0.8"
        # unknown qualifier should be ignored
        regenerated = params.to_xyce_statement()
        assert "TRIG V(1)=0.2" in regenerated
        assert "TARG V(2)=0.8" in regenerated
        assert "UNKNOWN" not in regenerated

    def test_when_without_condition(self):
        # arrange - WHEN without condition
        statement = ".MEASURE TRAN when1 WHEN V(1)"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "WHEN"
        assert params.when_variable == "V(1)"
        assert params.when_condition == ""

    def test_unsupported_rise_qualifier(self):
        # arrange - AVG with RISE (not supported for AVG)
        statement = ".MEASURE TRAN avgAll AVG V(1) RISE=2"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "AVG"
        # RISE should be ignored since AVG doesn't support it
        assert params.rise_val == ""
        regenerated = params.to_xyce_statement()
        assert "RISE" not in regenerated

    def test_unsupported_fall_qualifier(self):
        # arrange - AVG with FALL (not supported for AVG)
        statement = ".MEASURE TRAN avgAll AVG V(1) FALL=1"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "AVG"
        # FALL should be ignored since AVG doesn't support it
        assert params.fall_val == ""
        regenerated = params.to_xyce_statement()
        assert "FALL" not in regenerated

    def test_unsupported_cross_qualifier(self):
        # arrange - AVG with CROSS (not supported for AVG)
        statement = ".MEASURE TRAN avgAll AVG V(1) CROSS=3"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "AVG"
        # CROSS should be ignored since AVG doesn't support it
        assert params.cross_val == ""
        regenerated = params.to_xyce_statement()
        assert "CROSS" not in regenerated

    def test_unsupported_at_qualifier(self):
        # arrange - AVG with AT (not supported for AVG)
        statement = ".MEASURE TRAN avgAll AVG V(1) AT=5"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "AVG"
        # AT should be ignored since AVG doesn't support it
        assert params.at_val == ""
        regenerated = params.to_xyce_statement()
        assert "AT" not in regenerated

    def test_unsupported_on_qualifier(self):
        # arrange - AVG with ON (not supported for AVG)
        statement = ".MEASURE TRAN avgAll AVG V(1) ON=0.75"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "AVG"
        # ON should be ignored since AVG doesn't support it
        assert params.on_val == ""
        regenerated = params.to_xyce_statement()
        assert "ON" not in regenerated

    def test_unsupported_off_qualifier(self):
        # arrange - AVG with OFF (not supported for AVG)
        statement = ".MEASURE TRAN avgAll AVG V(1) OFF=0.25"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "AVG"
        # OFF should be ignored since AVG doesn't support it
        assert params.off_val == ""
        regenerated = params.to_xyce_statement()
        assert "OFF" not in regenerated

    def test_unsupported_rfc_level_qualifier(self):
        # arrange - AVG with RFC_LEVEL (not supported for AVG)
        statement = ".MEASURE TRAN avgAll AVG V(1) RFC_LEVEL=0.5"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "AVG"
        # RFC_LEVEL should be ignored since AVG doesn't support it
        assert params.rfc_level == ""
        regenerated = params.to_xyce_statement()
        assert "RFC_LEVEL" not in regenerated

    def test_unsupported_output_qualifier(self):
        # arrange - AVG with OUTPUT (not supported for AVG)
        statement = ".MEASURE TRAN avgAll AVG V(1) OUTPUT=output.txt"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "AVG"
        # OUTPUT should be ignored since AVG doesn't support it
        assert params.output == ""
        regenerated = params.to_xyce_statement()
        assert "OUTPUT" not in regenerated

    def test_unsupported_file_qualifier(self):
        # arrange - AVG with FILE (ERROR-specific)
        statement = ".MEASURE TRAN avgAll AVG V(1) FILE=data.csv"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "AVG"
        # FILE should be ignored since AVG is not ERROR type
        assert params.error_file == ""
        regenerated = params.to_xyce_statement()
        assert "FILE" not in regenerated

    def test_unsupported_indepvarcol_qualifier(self):
        # arrange - AVG with INDEPVARCOL (ERROR-specific)
        statement = ".MEASURE TRAN avgAll AVG V(1) INDEPVARCOL=1"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "AVG"
        # INDEPVARCOL should be ignored since AVG is not ERROR type
        assert params.indepvarcol == ""
        regenerated = params.to_xyce_statement()
        assert "INDEPVARCOL" not in regenerated

    def test_unsupported_depvarcol_qualifier(self):
        # arrange - AVG with DEPVARCOL (ERROR-specific)
        statement = ".MEASURE TRAN avgAll AVG V(1) DEPVARCOL=2"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "AVG"
        # DEPVARCOL should be ignored since AVG is not ERROR type
        assert params.depvarcol == ""
        regenerated = params.to_xyce_statement()
        assert "DEPVARCOL" not in regenerated

    def test_unsupported_comp_function_qualifier(self):
        # arrange - AVG with COMP_FUNCTION (ERROR-specific)
        statement = ".MEASURE TRAN avgAll AVG V(1) COMP_FUNCTION=L2"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "AVG"
        # COMP_FUNCTION should be ignored since AVG is not ERROR type
        assert params.comp_function == ""
        regenerated = params.to_xyce_statement()
        assert "COMP_FUNCTION" not in regenerated

    def test_unsupported_numfreq_qualifier(self):
        # arrange - AVG with NUMFREQ (FOUR-specific)
        statement = ".MEASURE TRAN avgAll AVG V(1) NUMFREQ=10"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "AVG"
        # NUMFREQ should be ignored since AVG is not FOUR type
        assert params.numfreq == ""
        regenerated = params.to_xyce_statement()
        assert "NUMFREQ" not in regenerated

    def test_unsupported_gridsize_qualifier(self):
        # arrange - AVG with GRIDSIZE (FOUR-specific)
        statement = ".MEASURE TRAN avgAll AVG V(1) GRIDSIZE=200"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "AVG"
        # GRIDSIZE should be ignored since AVG is not FOUR type
        assert params.gridsize == ""
        regenerated = params.to_xyce_statement()
        assert "GRIDSIZE" not in regenerated

    def test_unknown_option(self):
        # arrange - AVG with unknown option
        statement = ".MEASURE TRAN avgAll AVG V(1) UNKNOWN_OPTION=value"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "AVG"
        # unknown option should be ignored
        regenerated = params.to_xyce_statement()
        assert "UNKNOWN_OPTION" not in regenerated

    # New FFT measure tests
    def test_fft_enob_measure(self):
        # arrange
        statement = ".MEASURE TRAN my_enob FFT ENOB V(OUT) BINSIZ=10.0"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "ENOB"
        assert params.variable == "V(OUT)"
        assert params.binsiz == "10.0"
        assert params.to_xyce_statement() == ".MEASURE TRAN my_enob FFT ENOB V(OUT) BINSIZ=10.0"

    def test_fft_sfdr_measure(self):
        # arrange
        statement = ".MEASURE TRAN my_sfdr FFT SFDR V(OUT) BINSIZ=10.0 MAXFREQ=1e6 MINFREQ=100"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "SFDR"
        assert params.binsiz == "10.0"
        assert params.maxfreq == "1e6"
        assert params.minfreq == "100"
        regenerated = params.to_xyce_statement()
        assert "FFT SFDR" in regenerated
        assert "BINSIZ=10.0" in regenerated
        assert "MAXFREQ=1e6" in regenerated
        assert "MINFREQ=100" in regenerated

    def test_fft_thd_measure(self):
        # arrange
        statement = ".MEASURE TRAN my_thd FFT THD V(OUT) NBHARM=10 MAXFREQ=1e6 MINFREQ=100"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "THD"
        assert params.nbharm == "10"
        assert params.maxfreq == "1e6"
        assert params.minfreq == "100"
        regenerated = params.to_xyce_statement()
        assert "FFT THD" in regenerated
        assert "NBHARM=10" in regenerated
        assert "MAXFREQ=1e6" in regenerated
        assert "MINFREQ=100" in regenerated

    def test_fft_unsupported_qualifiers(self):
        # arrange - FFT ENOB with TD (unsupported for FFT)
        statement = ".MEASURE TRAN my_enob FFT ENOB V(OUT) TD=1ms"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.td_val == ""
        assert "TD=" not in params.to_xyce_statement()

    # Alias tests
    def test_derivative_alias(self):
        # arrange
        statement = ".MEASURE TRAN d1 DERIVATIVE V(1) AT=5"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "DERIV"
        assert params.to_xyce_statement() == ".MEASURE TRAN d1 DERIV V(1) AT=5"

    def test_integral_alias(self):
        # arrange
        statement = ".MEASURE TRAN i1 INTEGRAL V(1)"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "INTEG"
        assert params.to_xyce_statement() == ".MEASURE TRAN i1 INTEG V(1)"

    def test_param_alias(self):
        # arrange
        statement = ".MEASURE TRAN p1 PARAM {V(1)*2}"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.measure_type == "EQN"
        assert params.variable == "{V(1)*2}"
        assert params.to_xyce_statement() == ".MEASURE TRAN p1 EQN {V(1)*2}"

    # TRIG-TARG advanced tests
    def test_trig_targ_with_val_qualifier(self):
        # arrange
        statement = ".MEASURE TRAN trig1 TRIG V(1) VAL=0.2 TARG V(2) VAL=0.8"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.trig_variable == "V(1)"
        assert params.trig_val == "0.2"
        assert params.targ_variable == "V(2)"
        assert params.targ_val == "0.8"
        assert params.to_xyce_statement() == ".MEASURE TRAN trig1 TRIG V(1) VAL=0.2 TARG V(2) VAL=0.8"

    def test_trig_targ_with_frac_max(self):
        # arrange
        statement = ".MEASURE TRAN trig1 TRIG V(1) FRAC_MAX=0.5 TARG V(2) FRAC_MAX=0.9"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.trig_frac_max == "0.5"
        assert params.targ_frac_max == "0.9"
        regenerated = params.to_xyce_statement()
        assert "TRIG V(1) FRAC_MAX=0.5" in regenerated
        assert "TARG V(2) FRAC_MAX=0.9" in regenerated

    # HSPICE compatibility tests
    def test_goal_weight_qualifiers(self):
        # arrange
        statement = ".MEASURE TRAN m1 MAX V(1) GOAL=5.0 WEIGHT=2.0"
        # act
        params = MeasureEntry.from_xyce_statement(statement)
        # assert
        assert params is not None
        assert params.goal == "5.0"
        assert params.weight == "2.0"
        regenerated = params.to_xyce_statement()
        assert "GOAL=5.0" in regenerated
        assert "WEIGHT=2.0" in regenerated
