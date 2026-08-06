from .ac_simulation_parameters import AcSimulationParameters
from .dc_simulation_parameters import DCSimulationParameters
from .hb_simulation_parameters import HbSimulationParameters
from .lin_simulation_parameters import LinSimulationParameters
from .noise_simulation_parameters import NoiseSimulationParameters
from .op_simulation_parameters import OpSimulationParameters
from .simulation_config import SimulationConfig
from .transient_simulation_parameters import TransientSimulationParameters

# LinSimulationParameters MUST appear before AcSimulationParameters because
# .LIN netlists also contain a .AC directive; the Lin class embeds the AC
# sweep so it must claim the match first.
ALL_SIMULATION_PARAMETERS_TYPES = [
    LinSimulationParameters,
    AcSimulationParameters,
    HbSimulationParameters,
    NoiseSimulationParameters,
    DCSimulationParameters,
    OpSimulationParameters,
    TransientSimulationParameters,
]


def from_xyce_directives(directives: list[str]) -> SimulationConfig:
    # use the unified config factory to parse all directives
    return SimulationConfig.from_xyce_directives(directives)
