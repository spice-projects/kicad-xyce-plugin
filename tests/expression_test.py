from unittest import TestCase

import numpy as np

from expression import Expression


class TestExpression(TestCase):

    def test_name(self):
        # arrange
        expr = Expression("V(R1)", np.array([1.0, 2.0]), "V")
        # act
        name = expr.name
        # assert
        self.assertEqual(name, "V(R1)")

    def test_data(self):
        # arrange
        data = np.array([1.0, 2.0, 3.0])
        expr = Expression("V(R1)", data, "V")
        # act
        result = expr.data
        # assert
        np.testing.assert_array_equal(result, data)

    def test_unit(self):
        # arrange
        expr = Expression("V(R1)", np.array([1.0]), "V")
        # act
        unit = expr.unit
        # assert
        self.assertEqual(unit, "V")

    def test_source_default_is_none(self):
        # arrange
        expr = Expression("V(R1)", np.array([1.0]), "V")
        # act
        source = expr.source
        # assert
        self.assertIsNone(source)

    def test_source_stored(self):
        # arrange
        expr = Expression("V(R1)", np.array([1.0]), "V", source="V(R1)")
        # act
        source = expr.source
        # assert
        self.assertEqual(source, "V(R1)")

    def test_complex_false_for_real_data(self):
        # arrange
        expr = Expression("V(R1)", np.array([1.0, 2.0]), "V")
        # act
        result = expr.complex
        # assert
        self.assertFalse(result)

    def test_complex_true_for_complex_data(self):
        # arrange
        expr = Expression("V(R1)", np.array([1+2j, 3+4j], dtype=np.complex128), "V")
        # act
        result = expr.complex
        # assert
        self.assertTrue(result)

    def test_values_caches_result(self):
        # arrange
        expr = Expression("V(R1)", np.array([1.0, 2.0]), "V")
        # act
        first = expr.data
        second = expr.data
        # assert — same object returned on repeated calls
        self.assertIs(first, second)

    def test_values_returns_data_when_already_contiguous(self):
        # arrange
        data = np.ascontiguousarray([1.0, 2.0, 3.0])
        expr = Expression("V(R1)", data, "V")
        # act
        result = expr.data
        # assert — no copy made; same underlying buffer
        self.assertIs(result, data)

    def test_variable_type_default_is_none(self):
        # arrange
        expr = Expression("V(R1)", np.array([1.0]), "V")
        # act
        variable_type = expr.variable_type
        # assert
        self.assertIsNone(variable_type)

    def test_variable_type_stored_when_provided(self):
        # arrange
        expr = Expression("V(R1)", np.array([1.0]), "V", variable_type="voltage")
        # act
        variable_type = expr.variable_type
        # assert
        self.assertEqual(variable_type, "voltage")

    def test_variable_type_with_frequency(self):
        # arrange
        expr = Expression("Frequency", np.array([1e3, 1e4]), "Hz", variable_type="frequency")
        # act / assert
        self.assertEqual(expr.variable_type, "frequency")

    def test_variable_type_with_current(self):
        # arrange
        expr = Expression("I(R1)", np.array([0.1, 0.2]), "A", variable_type="current")
        # act / assert
        self.assertEqual(expr.variable_type, "current")

    def test_variable_type_with_time(self):
        # arrange
        expr = Expression("Time", np.array([0.0, 1e-6, 2e-6]), "s", variable_type="time")
        # act / assert
        self.assertEqual(expr.variable_type, "time")

    def test_variable_type_with_power(self):
        # arrange
        expr = Expression("P(R1)", np.array([1.0, 2.0]), "W", variable_type="power")
        # act / assert
        self.assertEqual(expr.variable_type, "power")

    def test_variable_type_with_parameter(self):
        # arrange
        expr = Expression("L1_value", np.array([1e-6]), "", variable_type="parameter")
        # act / assert
        self.assertEqual(expr.variable_type, "parameter")

    def test_variable_type_with_phase(self):
        # arrange
        expr = Expression("Phase", np.array([0.0, 45.0, 90.0]), "°", variable_type="phase")
        # act / assert
        self.assertEqual(expr.variable_type, "phase")

    def test_variable_type_with_complex_data(self):
        # arrange — complex data with frequency type
        expr = Expression("V(out)", np.array([1+2j, 3+4j], dtype=np.complex128), "V", variable_type="voltage")
        # act / assert
        self.assertEqual(expr.variable_type, "voltage")
        self.assertTrue(expr.complex)

    def test_variable_type_with_source(self):
        # arrange — variable_type and source both provided
        expr = Expression("V(R1)", np.array([1.0]), "V", source="V(R1)", variable_type="voltage")
        # act / assert
        self.assertEqual(expr.variable_type, "voltage")
        self.assertEqual(expr.source, "V(R1)")
