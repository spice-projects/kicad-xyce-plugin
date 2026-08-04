#include <cctype>
#include <optional>
#include <string>
#include <vector>

#include "../util.h"
#include "noise_simulation_parameters.h"

DeviceNoiseOperator::DeviceNoiseOperator(std::string type, std::string node, std::string source) :
    type(std::move(type)), node(std::move(node)), source(std::move(source)) {}

bool DeviceNoiseOperator::operator==(const DeviceNoiseOperator& other) const {
    // compare all fields for equality
    return type == other.type && node == other.node && source == other.source;
}

NoiseSimulationParameters::NoiseSimulationParameters(std::string output_node, std::string ref_node, std::string source_name, std::string start_freq_value, std::string end_freq_value, std::string num_points_value, std::string sweep_type, std::vector<DeviceNoiseOperator> device_noise_operators, std::string data_table_name, bool replace_ground, std::optional<PrintParameters> print_parameters) :
    output_node(std::move(output_node)), ref_node(std::move(ref_node)), source_name(std::move(source_name)), start_freq_value(std::move(start_freq_value)), end_freq_value(std::move(end_freq_value)), num_points_value(std::move(num_points_value)), sweep_type(std::move(sweep_type)), device_noise_operators(std::move(device_noise_operators)), data_table_name(std::move(data_table_name)), replace_ground(replace_ground), print_parameters(std::move(print_parameters)) {}

std::optional<NoiseSimulationParameters> NoiseSimulationParameters::from_xyce_directives(const std::vector<std::string>& directives) {
    // init defaults
    std::string output_node;
    std::string ref_node;
    std::string source_name;
    std::string start_freq_value;
    std::string end_freq_value;
    std::string num_points_value;
    std::string sweep_type;
    std::vector<DeviceNoiseOperator> device_noise_operators;
    std::string data_table_name;
    bool replace_ground = true;
    std::optional<PrintParameters> print_parameters;

    // flag indicating whether a valid directive was found
    bool found = false;

    // parse directives
    for (const auto& directive : directives) {
        // tokenize the directive
        const auto tokens = tokenize(directive);

        // skip empty directives
        if (tokens.empty()) {
            continue;
        }

        const std::string cmd = to_upper(tokens[0]);

        // parse print directives and retain noise-specific output config
        if (cmd == ".PRINT") {
            // parse the print statement from the directive
            const auto print_statement = PrintParameters::from_xyce_statement(directive);
            // retain noise print parameters when found
            if (print_statement) {
                const std::string print_type_upper = to_upper(print_statement->print_type);
                if (print_type_upper == "NOISE") {
                    // store the parsed print parameters
                    print_parameters = *print_statement;
                    continue;
                }
            }
        }

        // handle preprocess replaceground
        if (cmd == ".PREPROCESS" && tokens.size() > 2 && to_upper(tokens[1]) == "REPLACEGROUND") {
            // set flag based on value
            replace_ground = (to_upper(tokens[2]) == "TRUE");
            continue;
        }

        // skip non-NOISE directives
        if (cmd != ".NOISE") {
            continue;
        }

        // flag indicating a valid NOISE directive was found
        found = true;

        // parse output node (position 1)
        if (tokens.size() >= 2) {
            output_node = tokens[1];
        }

        // parse ref node (position 2)
        if (tokens.size() >= 3) {
            ref_node = tokens[2];
        }

        // parse source name (position 3)
        if (tokens.size() >= 4) {
            source_name = tokens[3];
        }

        // parse start frequency (position 4)
        if (tokens.size() >= 5) {
            start_freq_value = tokens[4];
        }

        // parse end frequency (position 5)
        if (tokens.size() >= 6) {
            end_freq_value = tokens[5];
        }

        // parse number of points (position 6)
        if (tokens.size() >= 7) {
            num_points_value = tokens[6];
        }

        // parse sweep type (position 7)
        if (tokens.size() >= 8) {
            sweep_type = tokens[7];
        }

        // parse data table name (position 8, DATA sweep only)
        if (tokens.size() >= 9 && to_upper(tokens[7]) == "DATA") {
            data_table_name = tokens[8];
        }

        // parse device noise operators (DNI/DNO)
        for (size_t i = 9; i + 2 < tokens.size(); i += 3) {
            const auto type = tokens[i];
            const auto node = tokens[i + 1];
            const auto source = tokens[i + 2];
            // append device noise operator
            device_noise_operators.emplace_back(std::string(type), std::string(node), std::string(source));
        }
    }

    // return instance if a valid directive was found
    if (!found) {
        return std::nullopt;
    }

    return NoiseSimulationParameters(output_node, ref_node, source_name, start_freq_value, end_freq_value, num_points_value, sweep_type, device_noise_operators, data_table_name, replace_ground, print_parameters);
}

std::vector<std::string> NoiseSimulationParameters::to_xyce_directives(const NetlistTopology& topology) const {
    // init output directive list
    std::vector<std::string> directives;

    // prepend replaceground preprocessor directive when enabled
    if (replace_ground) {
        directives.push_back(".PREPROCESS REPLACEGROUND TRUE");
    }

    // start with the noise analysis directive
    std::string noise_directive = ".NOISE " + output_node + " " + ref_node + " " + source_name + " " + start_freq_value + " " + end_freq_value + " " + num_points_value + " " + sweep_type;

    // append data table name for DATA sweep
    if (!data_table_name.empty()) {
        noise_directive += " " + data_table_name;
    }

    // append device noise operators
    for (const auto& dno : device_noise_operators) {
        noise_directive += " " + dno.type + " " + dno.node + " " + dno.source;
    }

    directives.push_back(noise_directive);

    // append noise print directive with topology-aware wildcard expansion
    if (print_parameters) {
        directives.push_back(print_parameters->to_xyce_statement());
    }

    // return the full directive list
    return directives;
}

bool NoiseSimulationParameters::operator==(const NoiseSimulationParameters& other) const {
    // compare all fields for equality
    return output_node == other.output_node && ref_node == other.ref_node && source_name == other.source_name && start_freq_value == other.start_freq_value && end_freq_value == other.end_freq_value && num_points_value == other.num_points_value && sweep_type == other.sweep_type && device_noise_operators == other.device_noise_operators && data_table_name == other.data_table_name && replace_ground == other.replace_ground && print_parameters == other.print_parameters;
}
