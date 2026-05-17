import numpy as np


class Expression:
    # expression class
    def __init__(self, name: str, data: np.ndarray, unit: str, source: str | None = None, variable_type: str | None = None):
        # name field
        self._name = name
        # data field
        self._data = data
        # unit field
        self._unit = unit
        # complex field
        self._complex = data.dtype == np.complex128
        # source field
        self._source = source
        # variable type field
        self._variable_type = variable_type

    @property
    def name(self) -> str:
        # return name
        return self._name

    @property
    def data(self) -> np.ndarray:
        # return data
        return self._data

    @property
    def unit(self) -> str:
        # return unit
        return self._unit

    @property
    def complex(self) -> bool:
        # return complex status
        return self._complex

    @property
    def source(self) -> str | None:
        # return source
        return self._source

    @property
    def variable_type(self) -> str | None:
        # return variable type
        return self._variable_type
