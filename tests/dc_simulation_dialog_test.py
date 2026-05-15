from unittest import TestCase

from plugin.simulation_dialog import _parse_list_values, DCSimulationParameters


def _lin(primary_variable="VIN", start="0", stop="5", step="0.1", secondary_variable="", secondary_start="", secondary_stop="", secondary_step="") -> DCSimulationParameters:
    return DCSimulationParameters("LIN", primary_variable, start, stop, step, "", tuple(), "", secondary_variable, secondary_start, secondary_stop, secondary_step, "")


def _dec(primary_variable="VIN", start="1", stop="100", points="10", secondary_variable="", secondary_start="", secondary_stop="", secondary_points="") -> DCSimulationParameters:
    return DCSimulationParameters("DEC", primary_variable, start, stop, "", points, tuple(), "", secondary_variable, secondary_start, secondary_stop, "", secondary_points)


def _oct(primary_variable="VIN", start="0.125", stop="64", points="2", secondary_variable="", secondary_start="", secondary_stop="", secondary_points="") -> DCSimulationParameters:
    return DCSimulationParameters("OCT", primary_variable, start, stop, "", points, tuple(), "", secondary_variable, secondary_start, secondary_stop, "", secondary_points)


def _list(primary_variable="TEMP", list_values=("10", "15", "27")) -> DCSimulationParameters:
    return DCSimulationParameters("LIST", primary_variable, "", "", "", "", list_values, "", "", "", "", "", "")


def _data(data_table_name="resistorValues") -> DCSimulationParameters:
    return DCSimulationParameters("DATA", "", "", "", "", "", tuple(), data_table_name, "", "", "", "", "")


class TestDCSimulationParametersLin(TestCase):

    def test_lin_basic_directive(self):
        # arrange
        params = _lin(primary_variable="VIN", start="-10", stop="15", step="1")
        # act
        directive = params.to_xyce_directive()
        # assert
        self.assertEqual(directive, ".DC VIN -10 15 1")

    def test_lin_fractional_step(self):
        # arrange
        params = _lin(primary_variable="R1", start="0", stop="3.5", step="0.05")
        # act
        directive = params.to_xyce_directive()
        # assert
        self.assertEqual(directive, ".DC R1 0 3.5 0.05")

    def test_lin_negative_step_allowed(self):
        # arrange — descending sweep; step is negative (warn but do not block)
        params = _lin(primary_variable="VIN", start="5", stop="0", step="-0.1")
        # act
        directive = params.to_xyce_directive()
        # assert
        self.assertEqual(directive, ".DC VIN 5 0 -0.1")

    def test_lin_with_secondary_sweep(self):
        # arrange
        params = _lin(primary_variable="R1", start="0", stop="3.5", step="0.05", secondary_variable="C1", secondary_start="0", secondary_stop="3.5", secondary_step="0.5")
        # act
        directive = params.to_xyce_directive()
        # assert
        self.assertEqual(directive, ".DC R1 0 3.5 0.05 C1 0 3.5 0.5")

    def test_lin_no_secondary_when_variable_empty(self):
        # arrange — secondary_variable is empty so no secondary tokens emitted
        params = _lin(primary_variable="VIN", start="0", stop="5", step="0.1", secondary_variable="")
        # act
        directive = params.to_xyce_directive()
        # assert — no secondary tokens present
        self.assertNotIn("  ", directive)
        self.assertEqual(directive, ".DC VIN 0 5 0.1")


class TestDCSimulationParametersDec(TestCase):

    def test_dec_basic_directive(self):
        # arrange
        params = _dec(primary_variable="VIN", start="1", stop="100", points="2")
        # act
        directive = params.to_xyce_directive()
        # assert
        self.assertEqual(directive, ".DC DEC VIN 1 100 2")

    def test_dec_with_secondary_sweep(self):
        # arrange
        params = _dec(primary_variable="VIN", start="1", stop="100", points="2", secondary_variable="R1", secondary_start="1", secondary_stop="10", secondary_points="3")
        # act
        directive = params.to_xyce_directive()
        # assert
        self.assertEqual(directive, ".DC DEC VIN 1 100 2 R1 1 10 3")

    def test_dec_no_secondary_when_variable_empty(self):
        # arrange
        params = _dec(primary_variable="VIN", start="1", stop="100", points="5")
        # act
        directive = params.to_xyce_directive()
        # assert — mode keyword present, no secondary tokens
        self.assertEqual(directive, ".DC DEC VIN 1 100 5")


class TestDCSimulationParametersOct(TestCase):

    def test_oct_basic_directive(self):
        # arrange
        params = _oct(primary_variable="VIN", start="0.125", stop="64", points="2")
        # act
        directive = params.to_xyce_directive()
        # assert
        self.assertEqual(directive, ".DC OCT VIN 0.125 64 2")

    def test_oct_with_secondary_sweep(self):
        # arrange
        params = _oct(primary_variable="VIN", start="0.125", stop="64", points="2", secondary_variable="R1", secondary_start="1", secondary_stop="10", secondary_points="4")
        # act
        directive = params.to_xyce_directive()
        # assert
        self.assertEqual(directive, ".DC OCT VIN 0.125 64 2 R1 1 10 4")

    def test_oct_no_secondary_when_variable_empty(self):
        # arrange
        params = _oct(primary_variable="VIN", start="0.125", stop="64", points="2")
        # act
        directive = params.to_xyce_directive()
        # assert
        self.assertEqual(directive, ".DC OCT VIN 0.125 64 2")


class TestDCSimulationParametersList(TestCase):

    def test_list_single_value(self):
        # arrange
        params = _list(primary_variable="TEMP", list_values=("27",))
        # act
        directive = params.to_xyce_directive()
        # assert
        self.assertEqual(directive, ".DC TEMP LIST 27")

    def test_list_multiple_values(self):
        # arrange
        params = _list(primary_variable="TEMP", list_values=("10", "15", "18", "27", "33"))
        # act
        directive = params.to_xyce_directive()
        # assert
        self.assertEqual(directive, ".DC TEMP LIST 10 15 18 27 33")

    def test_list_variable_name_used(self):
        # arrange
        params = _list(primary_variable="VCC", list_values=("3.3", "5.0"))
        # act
        directive = params.to_xyce_directive()
        # assert
        self.assertEqual(directive, ".DC VCC LIST 3.3 5.0")


class TestDCSimulationParametersData(TestCase):

    def test_data_directive(self):
        # arrange
        params = _data(data_table_name="resistorValues")
        # act
        directive = params.to_xyce_directive()
        # assert
        self.assertEqual(directive, ".DC DATA=resistorValues")

    def test_data_table_name_used_verbatim(self):
        # arrange
        params = _data(data_table_name="myCustomTable")
        # act
        directive = params.to_xyce_directive()
        # assert
        self.assertEqual(directive, ".DC DATA=myCustomTable")


class TestParseListValues(TestCase):

    def test_empty_string_returns_empty_tuple(self):
        # arrange / act
        result = _parse_list_values("")
        # assert
        self.assertEqual(result, tuple())

    def test_single_value(self):
        # arrange / act
        result = _parse_list_values("27")
        # assert
        self.assertEqual(result, ("27",))

    def test_space_separated_values(self):
        # arrange / act
        result = _parse_list_values("10 15 27 33")
        # assert
        self.assertEqual(result, ("10", "15", "27", "33"))

    def test_comma_separated_values(self):
        # arrange / act
        result = _parse_list_values("10,15,27,33")
        # assert
        self.assertEqual(result, ("10", "15", "27", "33"))

    def test_mixed_separator_values(self):
        # arrange / act
        result = _parse_list_values("10, 15 27,33")
        # assert
        self.assertEqual(result, ("10", "15", "27", "33"))

    def test_leading_and_trailing_whitespace_ignored(self):
        # arrange / act
        result = _parse_list_values("  10 20  ")
        # assert
        self.assertEqual(result, ("10", "20"))

    def test_result_is_tuple(self):
        # arrange / act
        result = _parse_list_values("1 2 3")
        # assert — immutable sequence required by frozen dataclass
        self.assertIsInstance(result, tuple)
