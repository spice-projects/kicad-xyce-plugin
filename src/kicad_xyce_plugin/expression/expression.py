import numpy as np


class Expression:

    def __init__(self, name: str, steps: list[np.ndarray], unit: str, source: str | None = None, variable_type: str | None = None, data: np.ndarray | None = None):
        # name field
        self._name = name
        # step data
        self._steps = steps
        # unit field
        self._unit = unit
        # complex field: derived from the first step's dtype
        self._complex = self._steps[0].dtype == np.complex128
        # source field
        self._source = source
        # variable type field
        self._variable_type = variable_type
        # combined steps in a single array, cached on first access if not provided
        self._data = data

    @property
    def name(self) -> str:
        # return name
        return self._name

    @property
    def steps(self) -> list[np.ndarray]:
        # return the list of per-step arrays
        return self._steps

    @property
    def data(self) -> np.ndarray:
        # check data has been cached
        if self._data is None:
            # concatenate the per-step arrays into a single array
            self._data = self._steps[0] if len(self._steps) == 1 else np.concatenate(self._steps)
        # return the cached data
        return self._data

    @property
    def step_count(self) -> int:
        # return the number of steps
        return len(self._steps)

    def step_data(self, step_index: int) -> np.ndarray:
        # return the per-step array for the given step index; zero copy for raw parsed expressions whose steps are strided views into the mmap
        return self._steps[step_index]

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
