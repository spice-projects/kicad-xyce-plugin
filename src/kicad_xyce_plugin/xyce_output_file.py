import mmap
from enum import Enum
from pathlib import Path
from typing import Any

from .expression import Expression, ExpressionManager


class AbscissaScale(Enum):
    LINEAR = "lin"
    DECADE = "dec"
    OCTAVE = "oct"


class StepInformation:

    def __init__(self, keys: list[str], values: list[tuple], abscissa_indices: list[slice], abscissa_value_ranges: list[tuple[float, float]]):
        # fields
        self._keys = keys
        self._values = values
        self._abscissa_indices = abscissa_indices
        self._abscissa_value_ranges = abscissa_value_ranges
        # number of steps
        self._step_count = len(abscissa_indices)
        # determine if abscissa is ascending or descending based on the first step's abscissa value range
        self._abscissa_ascending = self._abscissa_value_ranges[0][0] <= self._abscissa_value_ranges[0][1] if len(self._abscissa_value_ranges) > 0 else True
        # check abscissa direction
        if self._abscissa_ascending:
            # left and right values
            self._abscissa_left_value = float(min((value_range[0] for value_range in self._abscissa_value_ranges), default=0.0))
            self._abscissa_right_value = float(max((value_range[1] for value_range in self._abscissa_value_ranges), default=0.0))
        else:
            # left and right values
            self._abscissa_left_value = float(max((value_range[0] for value_range in self._abscissa_value_ranges), default=0.0))
            self._abscissa_right_value = float(min((value_range[1] for value_range in self._abscissa_value_ranges), default=0.0))

    @property
    def keys(self) -> list[str]:
        return self._keys

    @property
    def values(self) -> list[tuple]:
        return self._values

    @property
    def abscissa_indices(self) -> list[slice]:
        return self._abscissa_indices

    @property
    def length(self) -> int:
        return self._step_count

    @property
    def abscissa_left_value(self) -> float:
        return self._abscissa_left_value

    @property
    def abscissa_right_value(self) -> float:
        return self._abscissa_right_value

    @property
    def abscissa_ascending(self) -> bool:
        return self._abscissa_ascending

    def step_abscissa_left_value(self, step_index: int) -> float:
        return self._abscissa_value_ranges[step_index][0]

    def step_abscissa_right_value(self, step_index: int) -> float:
        return self._abscissa_value_ranges[step_index][1]


class VariableTypeInformation:

    def __init__(self, name: str, unit: str):
        self._name = name
        self._unit = unit

    @property
    def name(self) -> str:
        return self._name

    @property
    def unit(self) -> str:
        return self._unit


class VariableType(Enum):
    FREQUENCY = VariableTypeInformation("frequency", "Hz")
    VOLTAGE = VariableTypeInformation("voltage", "V")
    CURRENT = VariableTypeInformation("current", "A")
    TIME = VariableTypeInformation("time", "s")
    POWER = VariableTypeInformation("power", "W")
    PARAMETER = VariableTypeInformation("parameter", "")
    PHASE = VariableTypeInformation("phase", "°")
    UNKNOWN = VariableTypeInformation("unknown", "")


class PlotSuggestion:

    def __init__(self, chart_type: str, expressions: list[Expression]):
        # fields
        self._chart_type = chart_type
        self._expressions = expressions

    @property
    def chart_type(self) -> str:
        return self._chart_type

    @property
    def expressions(self) -> list[Expression]:
        return self._expressions


class XyceOutputFile:

    def __init__(self, filename: Path, title: str, complex: bool, step_information: StepInformation, abscissa: Expression, abscissa_scale: AbscissaScale, expression_manager: ExpressionManager, _mmap: mmap.mmap | None = None, metadata: dict[str, Any] = {}):
        # fields
        self._filename = filename
        self._title = title
        self._complex = complex
        self._step_information = step_information
        self._abscissa = abscissa
        self._abscissa_scale = abscissa_scale
        self._expression_manager = expression_manager
        # keep the mmap alive for as long as this object exists — Variable._values arrays are zero-copy views into the mmap buffer; closing the mmap would invalidate all of them
        self._mmap = _mmap
        # store metadata
        self._metadata = metadata

    @property
    def filename(self) -> Path:
        return self._filename

    @property
    def title(self) -> str:
        return self._title

    @property
    def complex(self) -> bool:
        return self._complex

    @property
    def step_information(self) -> StepInformation:
        return self._step_information

    @property
    def steps(self) -> int:
        return self._step_information.length

    @property
    def abscissa(self) -> Expression:
        return self._abscissa

    @property
    def abscissa_scale(self) -> AbscissaScale:
        return self._abscissa_scale

    @property
    def chart_type(self) -> str:
        # abscissa unit unambiguously determines the chart layout
        if self._abscissa.unit == "Hz":
            return "AC"
        if self._abscissa.unit == "s":
            return "TRANSIENT"
        # VOLTAGE (DC transfer sweep) and PARAMETER (operating point sweep) both use the DC layout
        return "DC"

    @property
    def expression_manager(self) -> ExpressionManager:
        return self._expression_manager

    @property
    def metadata(self) -> dict[str, Any]:
        # return metadata
        return self._metadata
