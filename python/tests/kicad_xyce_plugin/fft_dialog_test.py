import importlib
import sys
import types
from unittest.mock import MagicMock

import numpy as np
import pytest

from kicad_xyce_plugin.expression import Expression
from kicad_xyce_plugin.fft import FftOutput, WindowFunction

_fft_dialog_module = None


class QUrl:

    @staticmethod
    def fromLocalFile(path):
        return path


class MockSignal:
    def __init__(self):
        self._slots = []

    def connect(self, slot):
        self._slots.append(slot)


class MockQQuickView:
    class Status:
        Ready = 1

    class ResizeMode:
        SizeRootObjectToView = 1

    def __init__(self):
        self.statusChanged = MockSignal()

    def setResizeMode(self, mode):
        pass

    def setColor(self, color):
        pass

    def setSource(self, source):
        pass

    def rootObject(self):
        return None


class MockQDialog:
    def __init__(self, parent=None):
        pass

    def accept(self):
        pass

    def reject(self):
        pass

    def setWindowTitle(self, title):
        pass

    def setWindowModality(self, modality):
        pass

    def resize(self, width, height):
        pass

    def setMinimumHeight(self, height):
        pass
# QVBoxLayout stub


class MockQVBoxLayout:
    def __init__(self, parent=None):
        pass

    def setContentsMargins(self, left, top, right, bottom):
        pass

    def addWidget(self, widget):
        pass
# QWidget stub


class MockQWidget:
    @staticmethod
    def createWindowContainer(view, parent=None):
        return MagicMock()


def _create_mock_qt_modules():
    # build the QtCore mock module
    qtcore = types.ModuleType("PySide6.QtCore")
    # set up Qt with a WindowModality namespace
    qtcore.Qt = types.SimpleNamespace(WindowModality=types.SimpleNamespace(WindowModal=0))
    # slot decorator passes through unchanged
    qtcore.Slot = lambda *args, **kwargs: (lambda f: f)
    # attach QUrl to qtcore
    qtcore.QUrl = QUrl
    # build the QtGui mock module
    qtgui = types.ModuleType("PySide6.QtGui")
    # QColor passthrough
    qtgui.QColor = lambda value: value
    # build the QtQuick mock module
    qtquick = types.ModuleType("PySide6.QtQuick")
    # attach QQuickView stub to qtquick
    qtquick.QQuickView = MockQQuickView
    # build the QtWidgets mock module
    qtwidgets = types.ModuleType("PySide6.QtWidgets")
    # attach widget stubs to qtwidgets
    qtwidgets.QDialog = MockQDialog
    qtwidgets.QVBoxLayout = MockQVBoxLayout
    qtwidgets.QWidget = MockQWidget
    # build the top-level PySide6 mock
    module = types.ModuleType("PySide6")
    return {
        "PySide6": module,
        "PySide6.QtCore": qtcore,
        "PySide6.QtGui": qtgui,
        "PySide6.QtQuick": qtquick,
        "PySide6.QtWidgets": qtwidgets,
    }


@pytest.fixture(scope="module")
def fft_dialog_module():
    # names of all modules that will be temporarily replaced with mocks
    module_names = [
        "PySide6",
        "PySide6.QtCore",
        "PySide6.QtGui",
        "PySide6.QtQuick",
        "PySide6.QtWidgets",
        "kicad_xyce_plugin.fft_dialog",
    ]
    # save original entries so they can be restored on teardown
    saved_modules = {name: sys.modules.get(name) for name in module_names}
    # install mock modules so fft_dialog imports them instead of real PySide6
    mocked_modules = _create_mock_qt_modules()
    sys.modules.update(mocked_modules)
    # clear any previously cached import of fft_dialog
    sys.modules.pop("kicad_xyce_plugin.fft_dialog", None)
    try:
        # import fft_dialog with the mocked PySide6 modules in place
        module = importlib.import_module("kicad_xyce_plugin.fft_dialog")
        yield module
    finally:
        # remove fft_dialog so future imports start fresh
        sys.modules.pop("kicad_xyce_plugin.fft_dialog", None)
        for name, saved in saved_modules.items():
            # remove the entry if it was absent before this fixture ran
            if saved is None:
                sys.modules.pop(name, None)
            else:
                # restore the original module
                sys.modules[name] = saved


@pytest.fixture(autouse=True)
def _set_fft_dialog_module(fft_dialog_module):
    global _fft_dialog_module
    # update the module-level reference used by _make_dialog and test methods
    _fft_dialog_module = fft_dialog_module


def _make_dialog(abscissa_values=None, zoom_from=0, zoom_to=10, default_max_frequency=None):
    # build a FftDialog bypassing __init__ so no Qt objects are created
    if abscissa_values is None:
        abscissa_values = np.linspace(0.0, 1.0, 11)
    if default_max_frequency is None:
        default_max_frequency = _fft_dialog_module._DEFAULT_MAX_FREQUENCY
    abscissa = Expression("Time", abscissa_values, "s")
    e1 = Expression("V(R1)", np.ones(len(abscissa_values)), "V")
    e2 = Expression("I(L1)", np.ones(len(abscissa_values)) * 2.0, "A")
    dialog = object.__new__(_fft_dialog_module.FftDialog)
    dialog._expressions = [e1, e2]
    dialog._abscissa = abscissa
    dialog._zoom_from_index = zoom_from
    dialog._zoom_to_index = zoom_to
    dialog._selected_expressions = {e1, e2}
    dialog._result_expressions = []
    dialog._result_from_index = float(abscissa_values[0])
    dialog._result_to_index = float(abscissa_values[-1])
    dialog._result_window = WindowFunction.RECTANGULAR
    dialog._default_max_frequency = float(default_max_frequency)
    dialog._default_max_frequency_index = _fft_dialog_module._max_frequency_default_index(default_max_frequency)
    dialog._result_max_frequency = _fft_dialog_module._MAX_FREQUENCY_OPTIONS[dialog._default_max_frequency_index][1]
    dialog._result_normalize = True
    dialog._result_keep_dc = False
    dialog._result_output = FftOutput.MAGNITUDE
    dialog._accepted_calls = []
    dialog._rejected_calls = []
    dialog.accept = lambda: dialog._accepted_calls.append(True)
    dialog.reject = lambda: dialog._rejected_calls.append(True)
    return dialog, e1, e2


class TestFftDialogOnDialogAccepted:

    def test_no_selection_calls_reject(self):
        # arrange — clear all pre-selected expressions so the dialog has nothing to compute
        dialog, _e1, _e2 = _make_dialog()
        dialog._selected_expressions = set()
        # act
        dialog._on_dialog_accepted("Rectangular", 1e3, "Magnitude", False, "full", 0.0, 1.0, False)
        # assert
        assert len(dialog._rejected_calls) == 1
        assert dialog._result_expressions == []

    def test_on_expression_selection_changed_adds_expression(self):
        # arrange
        dialog, e1, _e2 = _make_dialog()
        dialog._selected_expressions = set()
        # act
        dialog._on_expression_selection_changed("V(R1)", True)
        # assert
        assert e1 in dialog._selected_expressions

    def test_on_expression_selection_changed_removes_expression(self):
        # arrange
        dialog, e1, _e2 = _make_dialog()
        # act
        dialog._on_expression_selection_changed("V(R1)", False)
        # assert
        assert e1 not in dialog._selected_expressions

    def test_on_expression_selection_changed_unknown_name_no_error(self):
        # arrange
        dialog, _e1, _e2 = _make_dialog()
        # act / assert — unknown name must not raise
        dialog._on_expression_selection_changed("NonExistentVar", True)

    def test_known_variable_stored_and_accepted(self):
        # arrange — both expressions are pre-selected by default
        dialog, e1, e2 = _make_dialog()
        # act
        dialog._on_dialog_accepted("Rectangular", 1e3, "Magnitude", False, "full", 0.0, 1.0, False)
        # assert
        assert e1 in dialog._result_expressions
        assert e2 in dialog._result_expressions
        assert len(dialog._accepted_calls) == 1

    def test_window_function_hamming_stored(self):
        # arrange
        dialog, _e1, _e2 = _make_dialog()
        # act
        dialog._on_dialog_accepted("Hamming", 1e3, "Magnitude", False, "full", 0.0, 1.0, False)
        # assert
        assert dialog._result_window == WindowFunction.HAMMING

    def test_window_function_unknown_falls_back_to_rectangular(self):
        # arrange
        dialog, _e1, _e2 = _make_dialog()
        # act
        dialog._on_dialog_accepted("???", 1e3, "Magnitude", False, "full", 0.0, 1.0, False)
        # assert
        assert dialog._result_window == WindowFunction.RECTANGULAR

    def test_max_frequency_stored(self):
        # arrange
        dialog, _e1, _e2 = _make_dialog()
        # act
        dialog._on_dialog_accepted("Rectangular", 1e6, "Magnitude", False, "full", 0.0, 1.0, False)
        # assert
        assert dialog._result_max_frequency == 1e6

    def test_max_frequency_non_numeric_falls_back_to_default(self):
        # arrange
        dialog, _e1, _e2 = _make_dialog()
        # act
        dialog._on_dialog_accepted("Rectangular", "bad", "Magnitude", False, "full", 0.0, 1.0, False)
        # assert
        assert dialog._result_max_frequency == _fft_dialog_module._MAX_FREQUENCY_OPTIONS[dialog._default_max_frequency_index][1]

    def test_max_frequency_non_positive_falls_back_to_default(self):
        # arrange
        dialog, _e1, _e2 = _make_dialog()
        # act
        dialog._on_dialog_accepted("Rectangular", 0.0, "Magnitude", False, "full", 0.0, 1.0, False)
        # assert
        assert dialog._result_max_frequency == _fft_dialog_module._MAX_FREQUENCY_OPTIONS[dialog._default_max_frequency_index][1]

    def test_output_type_magnitude_db_stored(self):
        # arrange
        dialog, _e1, _e2 = _make_dialog()
        # act
        dialog._on_dialog_accepted("Rectangular", 1e3, "Magnitude (dB)", False, "full", 0.0, 1.0, False)
        # assert
        assert dialog._result_output == FftOutput.MAGNITUDE_DB

    def test_output_type_phase_stored(self):
        # arrange
        dialog, _e1, _e2 = _make_dialog()
        # act
        dialog._on_dialog_accepted("Rectangular", 1e3, "Phase", False, "full", 0.0, 1.0, False)
        # assert
        assert dialog._result_output == FftOutput.PHASE

    def test_output_type_unknown_falls_back_to_magnitude(self):
        # arrange
        dialog, _e1, _e2 = _make_dialog()
        # act
        dialog._on_dialog_accepted("Rectangular", 1e3, "???", False, "full", 0.0, 1.0, False)
        # assert
        assert dialog._result_output == FftOutput.MAGNITUDE

    def test_normalize_flag_true(self):
        # arrange
        dialog, _e1, _e2 = _make_dialog()
        # act
        dialog._on_dialog_accepted("Rectangular", 1e3, "Magnitude", True, "full", 0.0, 1.0, False)
        # assert
        assert dialog._result_normalize is True

    def test_normalize_flag_false(self):
        # arrange
        dialog, _e1, _e2 = _make_dialog()
        # act
        dialog._on_dialog_accepted("Rectangular", 1e3, "Magnitude", False, "full", 0.0, 1.0, False)
        # assert
        assert dialog._result_normalize is False

    def test_range_mode_full_uses_all_samples(self):
        # arrange — 11-point abscissa
        dialog, _e1, _e2 = _make_dialog(np.linspace(0.0, 1.0, 11))
        # act
        dialog._on_dialog_accepted("Rectangular", 1e3, "Magnitude", False, "full", 0.0, 1.0, False)
        # assert
        assert dialog._result_from_index == 0.0
        assert dialog._result_to_index == 1.0

    def test_range_mode_unknown_falls_back_to_full(self):
        # arrange
        dialog, _e1, _e2 = _make_dialog(np.linspace(0.0, 1.0, 11))
        # act — unrecognised range_mode triggers full-range fallback
        dialog._on_dialog_accepted("Rectangular", 1e3, "Magnitude", False, "unknown_mode", 0.0, 1.0, False)
        # assert
        assert dialog._result_from_index == 0.0
        assert dialog._result_to_index == 1.0

    def test_keep_dc_flag_true(self):
        # arrange
        dialog, _e1, _e2 = _make_dialog()
        # act
        dialog._on_dialog_accepted("Rectangular", 1e3, "Magnitude", False, "full", 0.0, 1.0, True)
        # assert
        assert dialog._result_keep_dc is True

    def test_keep_dc_flag_false(self):
        # arrange
        dialog, _e1, _e2 = _make_dialog()
        # act
        dialog._on_dialog_accepted("Rectangular", 1e3, "Magnitude", False, "full", 0.0, 1.0, False)
        # assert
        assert dialog._result_keep_dc is False


class TestFftDialogInitAndQml:

    def test_constructor_sets_fields_and_ctx_properties(self):
        # arrange
        parent = MagicMock()
        expressions = [Expression("A", np.arange(5), "V"), Expression("B", np.arange(5), "A")]
        min_val = 0.0
        max_val = 10.0
        min_zoom = 2.0
        max_zoom = 8.0
        default_max_frequency = 1e6
        # act
        dialog = _fft_dialog_module.FftDialog(parent, expressions, min_val, max_val, min_zoom, max_zoom, default_max_frequency)
        # assert
        assert dialog._expressions == expressions
        assert dialog._selected_expressions == set(expressions)
        assert dialog._result_from_index == min_val
        assert dialog._result_to_index == max_val
        assert dialog._result_window == WindowFunction.HANNING
        assert dialog._result_max_frequency == 1e6
        assert dialog._result_normalize is True
        assert dialog._result_keep_dc is False
        assert dialog._result_output == FftOutput.MAGNITUDE
        assert "windowFunctions" in dialog._ctx_properties
        assert "outputTypes" in dialog._ctx_properties
        assert "maxFrequencyOptions" in dialog._ctx_properties
        selected_option = dialog._ctx_properties["maxFrequencyOptions"][dialog._ctx_properties["defaultMaxFrequencyIndex"]]
        assert selected_option["value"] == 1e6
        assert dialog._ctx_properties["abscissaMin"] == min_val
        assert dialog._ctx_properties["abscissaMax"] == max_val
        assert dialog._ctx_properties["zoomFromTime"] == min_zoom
        assert dialog._ctx_properties["zoomToTime"] == max_zoom

    def test_constructor_default_frequency_selects_closest_preset(self):
        # arrange
        parent = MagicMock()
        expressions = [Expression("A", np.arange(5), "V")]
        min_val = 0.0
        max_val = 1.0
        min_zoom = 0.0
        max_zoom = 1.0
        default_max_frequency = 8e5
        # act
        dialog = _fft_dialog_module.FftDialog(parent, expressions, min_val, max_val, min_zoom, max_zoom, default_max_frequency)
        # assert
        assert dialog._result_max_frequency == 1e6
        assert dialog._ctx_properties["maxFrequencyOptions"][dialog._ctx_properties["defaultMaxFrequencyIndex"]]["value"] == 1e6

    def test_on_qml_ready_injects_properties_and_connects_signals(self):
        # arrange
        dialog, _e1, _e2 = _make_dialog()
        mock_root = MagicMock()
        dialog._qml_view = MagicMock()
        dialog._qml_view.rootObject.return_value = mock_root
        dialog._ctx_properties = {"foo": 123, "bar": 456}
        # act
        dialog._on_qml_ready(_fft_dialog_module.QQuickView.Status.Ready)
        # assert
        mock_root.setProperty.assert_any_call("foo", 123)
        mock_root.setProperty.assert_any_call("bar", 456)
        mock_root.initializeExpressions.assert_called()
        mock_root.selectionChanged.connect.assert_called_with(dialog._on_expression_selection_changed)
        mock_root.dialogAccepted.connect.assert_called_with(dialog._on_dialog_accepted)
        mock_root.dialogRejected.connect.assert_called_with(dialog.reject)

    def test_result_expressions_property_default_is_empty_list(self):
        # arrange
        dialog, _e1, _e2 = _make_dialog()
        # act / assert
        assert dialog.result_expressions == []

    def test_result_expressions_property_after_accept(self):
        # arrange — both expressions are pre-selected; verify both appear in result
        dialog, e1, e2 = _make_dialog()
        dialog._on_dialog_accepted("Rectangular", 1e3, "Magnitude", False, "full", 0.0, 1.0, False)
        # act / assert
        assert dialog.result_expressions == [e1, e2]

    def test_result_from_index_property(self):
        # arrange
        dialog, _e1, _e2 = _make_dialog()
        dialog._result_from_index = 0.4
        # act / assert
        assert dialog.result_from_index == 0.4

    def test_result_to_index_property(self):
        # arrange
        dialog, _e1, _e2 = _make_dialog()
        dialog._result_to_index = 0.7
        # act / assert
        assert dialog.result_to_index == 0.7

    def test_result_window_property(self):
        # arrange
        dialog, _e1, _e2 = _make_dialog()
        dialog._result_window = WindowFunction.BLACKMAN
        # act / assert
        assert dialog.result_window == WindowFunction.BLACKMAN

    def test_result_max_frequency_property(self):
        # arrange
        dialog, _e1, _e2 = _make_dialog()
        dialog._result_max_frequency = 1e7
        # act / assert
        assert dialog.result_max_frequency == 1e7

    def test_result_normalize_property(self):
        # arrange
        dialog, _e1, _e2 = _make_dialog()
        dialog._result_normalize = True
        # act / assert
        assert dialog.result_normalize is True

    def test_result_keep_dc_property_default_is_false(self):
        # arrange
        dialog, _e1, _e2 = _make_dialog()
        # act / assert
        assert dialog.result_keep_dc is False

    def test_result_keep_dc_property_after_set(self):
        # arrange
        dialog, _e1, _e2 = _make_dialog()
        dialog._result_keep_dc = True
        # act / assert
        assert dialog.result_keep_dc is True

    def test_result_output_property(self):
        # arrange
        dialog, _e1, _e2 = _make_dialog()
        dialog._result_output = FftOutput.PHASE
        # act / assert
        assert dialog.result_output == FftOutput.PHASE
