from kicad_xyce_plugin.simulation_parameters import StepParameters


class TestStepParametersParsing:

    def test_parse_linear_step_implicit(self):
        # arrange
        directives = [".STEP R1 1k 10k 1k"]

        # act
        p = StepParameters.from_xyce_directives(directives)

        # assert
        assert p.enabled is True
        assert p.sweep_mode == "LIN"
        assert p.variable == "R1"
        assert p.start == "1k"
        assert p.stop == "10k"
        assert p.step == "1k"

    def test_parse_linear_step_explicit(self):
        # arrange
        directives = [".STEP LIN R1 1k 10k 1k"]

        # act
        p = StepParameters.from_xyce_directives(directives)

        # assert
        assert p.enabled is True
        assert p.sweep_mode == "LIN"
        assert p.variable == "R1"
        assert p.start == "1k"
        assert p.stop == "10k"
        assert p.step == "1k"

    def test_parse_dec_step(self):
        # arrange
        directives = [".STEP DEC R1 100 1MEG 10"]

        # act
        p = StepParameters.from_xyce_directives(directives)

        # assert
        assert p.enabled is True
        assert p.sweep_mode == "DEC"
        assert p.variable == "R1"
        assert p.start == "100"
        assert p.stop == "1MEG"
        assert p.points == "10"

    def test_parse_list_step(self):
        # arrange
        directives = [".STEP R1 LIST 1k 2k 5k"]

        # act
        p = StepParameters.from_xyce_directives(directives)

        # assert
        assert p.enabled is True
        assert p.sweep_mode == "LIST"
        assert p.variable == "R1"
        assert p.list_values == ("1k", "2k", "5k")

    def test_parse_data_step(self):
        # arrange
        directives = [".STEP DATA=myTable"]

        # act
        p = StepParameters.from_xyce_directives(directives)

        # assert
        assert p.enabled is True
        assert p.sweep_mode == "DATA"
        assert p.data_table_name == "myTable"

    def test_parse_data_step_with_spaces(self):
        # arrange
        directives = [".STEP DATA = myTable"]

        # act
        p = StepParameters.from_xyce_directives(directives)

        # assert
        assert p.enabled is True
        assert p.sweep_mode == "DATA"
        assert p.data_table_name == "myTable"

    def test_parsing_returns_disabled_when_no_directive(self):
        # arrange
        directives = [".TRAN 1u 1m"]

        # act
        p = StepParameters.from_xyce_directives(directives)

        # assert
        assert p.enabled is False

    def test_all_from_xyce_directives_returns_multiple_steps(self):
        # arrange - nested loops: inner first, outer last
        directives = [
            ".TRAN 1u 1m",
            ".STEP R1 1k 10k 1k",
            ".STEP TEMP 0 100 10",
        ]

        # act
        steps = StepParameters.all_from_xyce_directives(directives)

        # assert
        assert len(steps) == 2
        assert steps[0].variable == "R1"
        assert steps[1].variable == "TEMP"

    def test_all_from_xyce_directives_returns_empty_tuple_when_no_step(self):
        # arrange
        directives = [".TRAN 1u 1m"]

        # act
        steps = StepParameters.all_from_xyce_directives(directives)

        # assert
        assert steps == ()

    def test_all_from_xyce_directives_preserves_order(self):
        # arrange - three steps in declaration order
        directives = [
            ".STEP DEC R1 1 1k 5",
            ".STEP R2 LIST 100 200 300",
            ".STEP TEMP 0 100 25",
        ]

        # act
        steps = StepParameters.all_from_xyce_directives(directives)

        # assert
        assert len(steps) == 3
        assert steps[0].sweep_mode == "DEC"
        assert steps[1].sweep_mode == "LIST"
        assert steps[2].sweep_mode == "LIN"


class TestStepParametersGeneration:

    def test_generates_linear_step_with_explicit_lin_keyword(self):
        # arrange
        p = StepParameters(sweep_mode="LIN", variable="R1", start="1k", stop="10k", step="1k", enabled=True)

        # act
        directives = p.to_xyce_directives()

        # assert - LIN keyword is always explicit per project convention
        assert directives == [".STEP LIN R1 1k 10k 1k"]

    def test_generates_dec_step(self):
        # arrange
        p = StepParameters(sweep_mode="DEC", variable="R1", start="100", stop="1MEG", points="10", enabled=True)

        # act
        directives = p.to_xyce_directives()

        # assert
        assert directives == [".STEP DEC R1 100 1MEG 10"]

    def test_generates_list_step(self):
        # arrange
        p = StepParameters(sweep_mode="LIST", variable="R1", list_values=("1k", "2k", "5k"), enabled=True)

        # act
        directives = p.to_xyce_directives()

        # assert
        assert directives == [".STEP R1 LIST 1k 2k 5k"]

    def test_generates_data_step(self):
        # arrange
        p = StepParameters(sweep_mode="DATA", data_table_name="myTable", enabled=True)

        # act
        directives = p.to_xyce_directives()

        # assert
        assert directives == [".STEP DATA=myTable"]

    def test_generates_nothing_when_disabled(self):
        # arrange
        p = StepParameters(enabled=False)

        # act
        directives = p.to_xyce_directives()

        # assert
        assert directives == []
