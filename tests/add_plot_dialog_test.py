import sys
from unittest.mock import MagicMock

import numpy as np
from PySide6.QtQuick import QQuickView
from PySide6.QtWidgets import QApplication, QDialog

from add_plot_dialog import AddPlotDialog
from expression.expression import Expression
from expression.expression_manager import ExpressionManager

_app = QApplication.instance() or QApplication(sys.argv)


def _make_dialog(expressions=None, selected=None):
    # build an AddPlotDialog bypassing __init__ so no Qt objects are created
    e1 = Expression("V(R1)", np.array([1.0]), "V")
    e2 = Expression("I(L1)", np.array([0.5]), "A")
    e3 = Expression("V(R2)", np.array([2.0]), "V")
    if expressions is None:
        expressions = [e1, e2, e3]
    manager = ExpressionManager(expressions)
    if selected is None:
        selected = [e1]
    dialog = AddPlotDialog.__new__(AddPlotDialog)
    QDialog.__init__(dialog)
    dialog._expressions_manager = manager
    dialog._selected_expressions = set(selected)
    dialog._allow_custom_expressions = True
    dialog._expression_filter = lambda e: True
    dialog._qml_view = MagicMock()
    dialog._qml_view.rootObject.return_value = MagicMock()
    return dialog, e1, e2, e3


class TestAddPlotDialogInitialization:

    def test_initialize_passes_variable_type_to_qml(self):
        # arrange
        e1 = Expression("V1", np.array([1.0]), "V", variable_type="voltage")
        dialog, _, _, _ = _make_dialog(expressions=[e1])
        dialog._qml_view = MagicMock()
        root = MagicMock()
        dialog._qml_view.rootObject.return_value = root
        # act
        dialog._on_qml_ready(QQuickView.Status.Ready)
        # assert
        root.initialize.assert_called_with([["V1", False, "voltage"]])

    def test_selected_expressions_returns_set_type(self):
        # arrange
        dialog, _e1, _e2, _e3 = _make_dialog()
        # act
        result = dialog.selected_expressions
        # assert
        assert isinstance(result, set)

    def test_selected_expressions_empty_when_none_selected(self):
        # arrange
        dialog, _e1, _e2, _e3 = _make_dialog(selected=[])
        # act
        result = dialog.selected_expressions
        # assert
        assert result == set()


class TestAddPlotDialogOnSelectionChanged:

    def test_select_adds_expression(self):
        # arrange — default selected set contains only e1
        dialog, e1, e2, _e3 = _make_dialog()
        # act — select e2 which was not previously selected
        dialog._on_selection_changed("I(L1)", True)
        # assert — e2 is now in the selected set
        assert e2 in dialog._selected_expressions

    def test_deselect_removes_expression(self):
        # arrange — start with both e1 and e2 selected
        dialog, e1, e2, _e3 = _make_dialog()
        dialog._selected_expressions.add(e2)
        # act — deselect e1
        dialog._on_selection_changed("V(R1)", False)
        # assert — e1 no longer in selected set
        assert e1 not in dialog._selected_expressions

    def test_deselect_preserves_other_expressions(self):
        # arrange — start with both e1 and e2 selected
        dialog, _e1, e2, _e3 = _make_dialog()
        dialog._selected_expressions.add(e2)
        # act — deselect e1
        dialog._on_selection_changed("V(R1)", False)
        # assert — e2 still selected
        assert e2 in dialog._selected_expressions

    def test_select_unknown_expression_does_not_crash(self):
        # arrange
        dialog, _e1, _e2, _e3 = _make_dialog()
        # act — name not found in manager; should silently do nothing
        dialog._on_selection_changed("NonExistent(X)", True)
        # assert — no exception; selected set unchanged from initial [e1]
        assert len(dialog._selected_expressions) == 1

    def test_select_same_expression_twice_no_duplicates(self):
        # arrange — default selected set contains only e1
        dialog, _e1, _e2, _e3 = _make_dialog()
        # act — select e1 again (already in selected set)
        dialog._on_selection_changed("V(R1)", True)
        # assert — set semantics; still exactly one entry
        assert len(dialog._selected_expressions) == 1


class TestAddPlotDialogOnCustomExpressionRequested:

    def test_blank_input_ignored(self):
        # arrange
        dialog, _e1, _e2, _e3 = _make_dialog(selected=[])
        root = dialog._qml_view.rootObject.return_value
        # act
        dialog._on_custom_expression_requested("   ")
        # assert — nothing added, no QML call
        assert len(dialog._selected_expressions) == 0
        root.addExpression.assert_not_called()

    def test_empty_string_ignored(self):
        # arrange
        dialog, _e1, _e2, _e3 = _make_dialog(selected=[])
        root = dialog._qml_view.rootObject.return_value
        # act
        dialog._on_custom_expression_requested("")
        # assert
        assert len(dialog._selected_expressions) == 0
        root.addExpression.assert_not_called()

    def test_valid_expression_added_to_selected(self):
        # arrange
        dialog, e1, _e2, _e3 = _make_dialog(selected=[])
        # act — "V(R1)" is a known expression in the manager
        dialog._on_custom_expression_requested("V(R1)")
        # assert — expression was resolved and added to the selected set
        assert len(dialog._selected_expressions) == 1

    def test_valid_expression_calls_add_expression_on_qml(self):
        # arrange
        dialog, e1, _e2, _e3 = _make_dialog(selected=[])
        root = dialog._qml_view.rootObject.return_value
        # act
        dialog._on_custom_expression_requested("V(R1)")
        # assert — QML list is updated
        root.addExpression.assert_called_once()

    def test_invalid_expression_calls_set_expression_error(self):
        # arrange
        dialog, _e1, _e2, _e3 = _make_dialog(selected=[])
        root = dialog._qml_view.rootObject.return_value
        # act — expression that cannot be evaluated (unknown variable)
        dialog._on_custom_expression_requested("V(NonExistentNode)")
        # assert — error displayed in QML
        root.setExpressionError.assert_called_once()

    def test_invalid_expression_not_added_to_selected(self):
        # arrange
        dialog, _e1, _e2, _e3 = _make_dialog(selected=[])
        # act
        dialog._on_custom_expression_requested("V(NonExistentNode)")
        # assert
        assert len(dialog._selected_expressions) == 0
