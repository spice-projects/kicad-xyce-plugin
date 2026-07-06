from __future__ import annotations

from dataclasses import dataclass
from dataclasses import field

from ..netlist_parser import NetlistTopology
from .ac_simulation_parameters import AcSimulationParameters
from .data_block import DataBlock
from .dc_simulation_parameters import DCSimulationParameters
from .hb_simulation_parameters import HbSimulationParameters
from .lin_simulation_parameters import LinSimulationParameters
from .noise_simulation_parameters import NoiseSimulationParameters
from .op_simulation_parameters import OpSimulationParameters
from .option_parameters import OptionParameters
from .print_parameters import PrintParameters
from .step_parameters import StepParameters
from .transient_simulation_parameters import TransientSimulationParameters


@dataclass(frozen=True)
class SimulationConfig:

    analysis: AcSimulationParameters | DCSimulationParameters | HbSimulationParameters | LinSimulationParameters | NoiseSimulationParameters | OpSimulationParameters | TransientSimulationParameters | None
    steps: tuple[StepParameters, ...]
    data_blocks: tuple[DataBlock, ...] = field(default_factory=tuple)
    options: OptionParameters = field(default_factory=OptionParameters)
    unassociated_prints: tuple[PrintParameters, ...] = field(default_factory=tuple)

    @property
    def step(self) -> StepParameters:
        # return the first step for backward compatibility, or a disabled default
        if self.steps:
            # return first step
            return self.steps[0]
        # return disabled default
        return StepParameters()

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
        # parse all step directives preserving nested loop order
        steps = StepParameters.all_from_xyce_directives(directives)
        # parse all .DATA table blocks
        data_blocks = DataBlock.from_xyce_directives(directives)
        # parse the structured option directives
        options = OptionParameters.from_xyce_directives(directives)
        # init unassociated print list
        unassociated_prints: list[PrintParameters] = []
        # identify all handled print types for the current analysis to avoid duplicates
        handled_print_types: set[str] = set()
        # check if the analysis has print parameters already handled
        if analysis is not None and hasattr(analysis, "print_parameters") and analysis.print_parameters is not None:
            # mark the print type as handled
            handled_print_types.add(analysis.print_parameters.print_type.upper())
        # iterate all directives to find unassociated prints
        for directive in directives:
            # tokenize the directive
            tokens = directive.split()
            # skip non-print or empty directives
            if not tokens or tokens[0].upper() != ".PRINT":
                # continue loop
                continue
            # parse the print statement
            pp = PrintParameters.from_xyce_statement(directive)
            # check if print was successfully parsed and is not handled by the analysis
            if pp is not None and pp.print_type.upper() not in handled_print_types:
                # add to unassociated list
                unassociated_prints.append(pp)
        # return the combined configuration container
        return cls(analysis=analysis, steps=steps, data_blocks=data_blocks, options=options, unassociated_prints=tuple(unassociated_prints))

    def to_xyce_directives(self, topology: NetlistTopology | None = None) -> list[str]:
        # init output directive list
        directives: list[str] = []
        # extend with option directives
        directives.extend(self.options.to_xyce_directives(topology))
        # check if an analysis is configured
        if self.analysis is not None:
            # extend with analysis-specific directives
            directives.extend(self.analysis.to_xyce_directives(topology))
        # emit all step directives preserving the original nested loop order
        for step in self.steps:
            # extend with each step directive
            directives.extend(step.to_xyce_directives())
        # emit all .DATA table blocks
        for data_block in self.data_blocks:
            # extend with the data block directives
            directives.extend(data_block.to_xyce_directives())
        # extend with unassociated prints
        for pp in self.unassociated_prints:
            # append print directive string
            directives.append(pp.to_xyce_statement())
        # return the full consolidated directive list
        return directives
