from .ac_simulation_parameters import AcSimulationParameters
from .dc_simulation_parameters import DCSimulationParameters
from .hb_simulation_parameters import HbSimulationParameters
from .lin_simulation_parameters import LinSimulationParameters
from .noise_simulation_parameters import NoiseSimulationParameters
from .op_simulation_parameters import OpSimulationParameters
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


def from_xyce_directives(directives: list[str]) -> AcSimulationParameters | DCSimulationParameters | HbSimulationParameters | LinSimulationParameters | NoiseSimulationParameters | OpSimulationParameters | TransientSimulationParameters | None:
    # loop simulation parameters types
    for simulation_parameters_type in ALL_SIMULATION_PARAMETERS_TYPES:
        # try to parse directives
        simulation_parameters = simulation_parameters_type.from_xyce_directives(directives)
        if simulation_parameters is not None:
            return simulation_parameters
    # directives did not match any simulation parameters type
    return None
