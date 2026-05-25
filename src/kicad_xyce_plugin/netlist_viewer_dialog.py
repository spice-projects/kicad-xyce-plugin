from PySide6.QtCore import Qt
from PySide6.QtWidgets import QDialog, QPushButton, QTextEdit, QVBoxLayout, QWidget


class NetlistViewerDialog(QDialog):

    def __init__(self, parent: QWidget, netlist: str = ""):
        super().__init__(parent)
        # set modal
        self.setWindowModality(Qt.ApplicationModal)
        # set window title
        self.setWindowTitle("Netlist Preview")
        # create vertical layout
        self._layout = QVBoxLayout(self)
        # create text editor
        self._text_edit = QTextEdit(self)
        # make editor read-only
        self._text_edit.setReadOnly(True)
        # set font to monospaced
        self._text_edit.setFontFamily("Courier")
        # set text content
        self._text_edit.setText(netlist)
        # add editor to layout
        self._layout.addWidget(self._text_edit)
        # create close button
        self._close_button = QPushButton("Close", self)
        # connect clicked signal to accept
        self._close_button.clicked.connect(self.accept)
        # add button to layout
        self._layout.addWidget(self._close_button)
        # set initial dialog size
        self.resize(600, 400)
