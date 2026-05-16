import numpy as np


class Expression:
    """A named, evaluated expression with a data array and a propagated unit.

    Instances are produced by :class:`~viewer.expression_manager.ExpressionManager`
    and represent any computed trace - whether derived from a .alias directive,
    typed interactively by the user, or loaded directly from the QRAW file.
    """

    def __init__(self, name: str, data: np.ndarray, unit: str, source: str | None = None, variable_type: str | None = None):
        # fields
        self._name = name
        self._data = data
        self._unit = unit
        self._complex = data.dtype == np.complex128
        self._source = source
        self._variable_type = variable_type

    @property
    def name(self) -> str:
        """Display name of the expression (e.g. ``"V(R1)"`` or ``"10 * V(R1)"``."""
        return self._name

    @property
    def data(self) -> np.ndarray:
        """Evaluated data array, one value per simulation point."""
        return self._data

    @property
    def unit(self) -> str:
        """Physical unit propagated through the expression tree (e.g. ``"V"``, ``"A"``, ``"W"``)."""
        return self._unit

    @property
    def complex(self) -> bool:
        """Indicates whether the expression evaluates to a complex number."""
        return self._complex

    @property
    def source(self) -> str | None:
        """Original source expression string, if available."""
        return self._source

    @property
    def variable_type(self) -> str | None:
        """Original QRAW variable type, when available (e.g. ``"parameter"``)."""
        return self._variable_type
