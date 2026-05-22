from __future__ import annotations

from dataclasses import dataclass

from netlist_parser import NetlistTopology
from .ac_simulation_parameters import AcSimulationParameters
from .dc_simulation_parameters import DCSimulationParameters
from .hb_simulation_parameters import HbSimulationParameters
from .lin_simulation_parameters import LinSimulationParameters
from .noise_simulation_parameters import NoiseSimulationParameters
from .op_simulation_parameters import OpSimulationParameters
from .sens_simulation_parameters import SensSimulationParameters
from .step_parameters import StepParameters
from .transient_simulation_parameters import TransientSimulationParameters


@dataclass(frozen=True)
class SimulationConfig:

    analysis: AcSimulationParameters | DCSimulationParameters | HbSimulationParameters | LinSimulationParameters | NoiseSimulationParameters | OpSimulationParameters | TransientSimulationParameters | None
    step: StepParameters
    sensitivity: SensSimulationParameters | None = None

    @classmethod
    def from_xyce_directives(cls, directives: list[str]) -> "SimulationConfig":
        # init analysis result to none
        analysis = None
        # import simulation types list to avoid circular dependencies
        from .simulation_parameters import ALL_SIMULATION_PARAMETERS_TYPES
        # iterate all registered simulation types to find a match
        for simulation_parameters_type in ALL_SIMULATION_PARAMETERS_TYPES:
            # try to parse the directive list into a specific simulation type
            simulation_parameters = simulation_parameters_type.from_xyce_directives(directives)
            # check if a match was found
            if simulation_parameters is not None:
                # store the analysis parameters
                analysis = simulation_parameters
                # stop searching once the first valid analysis is found
                break
        # parse the step parameters from the same directive list
        step = StepParameters.from_xyce_directives(directives)
        # parse the sensitivity parameters independently as they are additive
        sensitivity = SensSimulationParameters.from_xyce_directives(directives)
        # return the combined configuration container
        return cls(analysis=analysis, step=step, sensitivity=sensitivity)

    def to_xyce_directives(self, topology: NetlistTopology | None = None) -> list[str]:
        # init output directive list
        directives: list[str] = []
        # check if an analysis is configured
        if self.analysis is not None:
            # extend with analysis-specific directives
            directives.extend(self.analysis.to_xyce_directives(topology))
        # extend with step-specific directives
        directives.extend(self.step.to_xyce_directives())
        # check if sensitivity is configured
        if self.sensitivity is not None:
            # extend with sensitivity-specific directives
            directives.extend(self.sensitivity.to_xyce_directives(topology))
        # return the full consolidated directive list
        return directives
