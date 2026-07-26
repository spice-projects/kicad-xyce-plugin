// #pragma once

// #include <string>
// #include <vector>

// #include "data_block.h"
// #include "option_parameters.h"
// #include "print_parameters.h"
// #include "step_parameters.h"

// // forward declarations
// class AcSimulationParameters;
// class DCSimulationParameters;
// class HbSimulationParameters;
// class LinSimulationParameters;
// class NoiseSimulationParameters;
// class OpSimulationParameters;
// class TransientSimulationParameters;

// // simulation config class — aggregates all simulation parameters
// class SimulationConfig
// {
// public:
//     // construct a simulation config from individual components
//     SimulationConfig(std::string analysis_type, std::unique_ptr<void> analysis, std::vector<StepParameters> steps, std::vector<DataBlock> data_blocks, OptionParameters options, std::vector<PrintParameters> unassociated_prints);

//     // parse all directives into a SimulationConfig instance
//     [[nodiscard]] static SimulationConfig from_xyce_directives(const std::vector<std::string>& directives);

//     // serialize this instance to a list of Xyce directive strings
//     [[nodiscard]] std::vector<std::string> to_xyce_directives() const;

//     // get the first step for backward compatibility
//     [[nodiscard]] StepParameters step() const;

//     // analysis type identifier (e.g. "AC", "DC", "TRAN", etc.)
//     std::string analysis_type;
//     // pointer to the analysis parameters (type-erased)
//     std::unique_ptr<void> analysis;
//     // step directives in order
//     std::vector<StepParameters> steps;
//     // data table blocks
//     std::vector<DataBlock> data_blocks;
//     // option directives
//     OptionParameters options;
//     // print directives not associated with any analysis
//     std::vector<PrintParameters> unassociated_prints;
// };
