from PySide6.QtCore import Qt
from PySide6.QtWidgets import QDialog, QVBoxLayout, QTextEdit, QPushButton, QWidget


class NetlistViewerDialog(QDialog):

    def __init__(self, parent: QWidget, netlist: str = ""):
        # init parent
        super().__init__(parent)
        # set modal
        self.setWindowModality(Qt.ApplicationModal)
        # set title
        self.setWindowTitle("Netlist Preview")
        # create layout
        self._layout = QVBoxLayout(self)
        # create text editor
        self._text_edit = QTextEdit(self)
        # make it read-only
        self._text_edit.setReadOnly(True)
        # set font
        self._text_edit.setFontFamily("Courier")
        # set content
        self._text_edit.setText(netlist)
        # add to layout
        self._layout.addWidget(self._text_edit)
        # create close button
        self._close_button = QPushButton("Close", self)
        # connect close
        self._close_button.clicked.connect(self.accept)
        # add to layout
        self._layout.addWidget(self._close_button)
        # set size
        self.resize(600, 400)
