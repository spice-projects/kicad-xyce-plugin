import logging
from pathlib import Path
from typing import Callable

from PySide6.QtCore import QUrl, Qt, Slot
from PySide6.QtGui import QColor
from PySide6.QtQuick import QQuickView
from PySide6.QtWidgets import QDialog, QVBoxLayout, QWidget

from .expression import Expression, ExpressionManager

logger = logging.getLogger(__name__)

_QML_FILE = Path(__file__).parent / "add_plot_dialog.qml"
_BG = "#1a1b1e"


class AddPlotDialog(QDialog):

    def __init__(self, parent: QWidget, expressions_manager: ExpressionManager, selected_expressions: list[Expression], allow_custom_expressions: bool = True, expression_filter: Callable[[Expression], bool] = lambda expression: True):
        super().__init__(parent)
        # store expressions for index mapping when reading back the selection
        self._expressions_manager = expressions_manager
        self._selected_expressions: set[Expression] = set(selected_expressions)
        self._allow_custom_expressions = allow_custom_expressions
        self._expression_filter = expression_filter
        # window setup
        self.setWindowTitle("Add Plot")
        self.setWindowModality(Qt.WindowModality.WindowModal)
        self.resize(560, 480)
        # create the QML view — inject expression names before loading so QML can bind to them immediately
        self._qml_view = QQuickView()
        self._qml_view.statusChanged.connect(self._on_qml_ready)
        self._qml_view.setResizeMode(QQuickView.ResizeMode.SizeRootObjectToView)
        self._qml_view.setColor(QColor(_BG))
        self._qml_view.setSource(QUrl.fromLocalFile(str(_QML_FILE)))
        # embed the QML view into the dialog
        container = QWidget.createWindowContainer(self._qml_view, self)
        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.addWidget(container)

    @Slot(QQuickView.Status)
    def _on_qml_ready(self, status: QQuickView.Status):
        # only proceed once QML has finished loading successfully
        if status != QQuickView.Status.Ready:
            return
        # connect QML dialog signals to Python accept / reject
        root = self._qml_view.rootObject()
        # properties
        root.setProperty("allowCustomExpressions", self._allow_custom_expressions)
        # signals
        root.dialogAccepted.connect(self.accept)
        root.dialogRejected.connect(self.reject)
        root.selectionChanged.connect(self._on_selection_changed)
        root.customExpressionRequested.connect(self._on_custom_expression_requested)
        # initialize view
        root.initialize([[v.name, v in self._selected_expressions, v.variable_type or "Misc"] for v in self._expressions_manager.expressions if self._expression_filter(v)])

    @Slot(str, bool)
    def _on_selection_changed(self, expression: str, selected: bool):
        # log information
        logger.debug("User %s expression: %s", "selected" if selected else "deselected", expression)
        # handle selection change from QML — update the selected expression list
        expression = next((v for v in self._expressions_manager.expressions if v.name == expression), None)
        # toggle expression in selected list based on selection state
        if expression is not None:
            # if selected, add to selected expressions if not already there; if deselected, remove from selected expressions if present
            if selected:
                # append expression to selected set
                self._selected_expressions.add(expression)
                # exit
                return
            # remove expression from selected set
            self._selected_expressions.remove(expression)

    @Slot(str)
    def _on_custom_expression_requested(self, text: str):
        # ignore blank input
        text = text.strip()
        if not text:
            return
        # evaluate the expression through the manager; returns None on failure
        expression = self._expressions_manager.evaluate(text, text)
        root = self._qml_view.rootObject()
        if expression is None:
            # set error state on the input field in QML; the user can correct and resubmit without closing the dialog
            root.setExpressionError("Invalid expression")
            # exit
            return
        # log information
        logger.debug("User added custom expression: %s", text)
        # add to selected set
        self._selected_expressions.add(expression)
        # update QML list and selection state; addExpression handles deduplication
        root.addExpression(expression.name, True)

    @property
    def selected_expressions(self) -> set[Expression]:
        return self._selected_expressions
