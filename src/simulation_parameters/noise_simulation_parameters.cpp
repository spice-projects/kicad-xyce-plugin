#include <algorithm>
#include <cctype>
#include <optional>
#include <string>
#include <vector>

#include "noise_simulation_parameters.h"

// normalize a string to uppercase
static std::string noise_to_upper(std::string s) {
    // convert each character to upper case
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::toupper(c); });
    // return converted string
    return s;
}

// tokenize a directive by whitespace
static std::vector<std::string> noise_tokenize(const std::string& directive) {
    // init token list
    std::vector<std::string> tokens;
    // init current token buffer
    std::string current;
    // iterate characters
    for (const char ch : directive) {
        // check whitespace splitter
        if (std::isspace(static_cast<unsigned char>(ch))) {
            // flush current token when non-empty
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
            // next
            continue;
        }
        // append char
        current += ch;
    }
    // flush trailing token
    if (!current.empty()) {
        tokens.push_back(current);
    }
    // return tokens
    return tokens;
}

DeviceNoiseOperator::DeviceNoiseOperator(std::string type, std::string node, std::string source) :
    type(std::move(type)), node(std::move(node)), source(std::move(source)) {}

bool DeviceNoiseOperator::operator==(const DeviceNoiseOperator& other) const { return type == other.type && node == other.node && source == other.source; }

NoiseSimulationParameters::NoiseSimulationParameters(std::string output_node, std::string ref_node, std::string source_name, std::string start_freq_value, std::string end_freq_value, std::string num_points_value, std::string sweep_type, std::vector<DeviceNoiseOperator> device_noise_operators, bool replace_ground, std::optional<PrintParameters> print_parameters) :
    output_node(std::move(output_node)), ref_node(std::move(ref_node)), source_name(std::move(source_name)), start_freq_value(std::move(start_freq_value)), end_freq_value(std::move(end_freq_value)), num_points_value(std::move(num_points_value)), sweep_type(std::move(sweep_type)), device_noise_operators(std::move(device_noise_operators)), replace_ground(replace_ground), print_parameters(std::move(print_parameters)) {}

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
    bool replace_ground = true;
    std::optional<PrintParameters> print_parameters;

    // flag indicating whether a valid directive was found
    bool found = false;

    // parse directives
    for (const auto& directive : directives) {
        // tokenize the directive
        const auto tokens = noise_tokenize(directive);

        // skip empty directives
        if (tokens.empty()) {
            continue;
        }

        const std::string cmd = noise_to_upper(tokens[0]);

        // parse print directives and retain noise-specific output config
        if (cmd == ".PRINT") {
            // parse the print statement from the directive
            const auto print_statement = PrintParameters::from_xyce_statement(directive);
            // retain noise print parameters when found
            if (print_statement) {
                const std::string print_type_upper = noise_to_upper(print_statement->print_type);
                if (print_type_upper == "NOISE") {
                    // store the parsed print parameters
                    print_parameters = *print_statement;
                    continue;
                }
            }
        }

        // handle preprocess replaceground
        if (cmd == ".PREPROCESS" && tokens.size() > 2 && noise_to_upper(tokens[1]) == "REPLACEGROUND") {
            // set flag based on value
            replace_ground = (noise_to_upper(tokens[2]) == "TRUE");
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

        // parse device noise operators (DNI/DNO)
        for (size_t i = 8; i + 2 < tokens.size(); i += 3) {
            const std::string type = tokens[i];
            const std::string node = tokens[i + 1];
            const std::string source = tokens[i + 2];
            // append device noise operator
            device_noise_operators.emplace_back(type, node, source);
        }
    }

    // return instance if a valid directive was found
    if (!found) {
        return std::nullopt;
    }

    return NoiseSimulationParameters(output_node, ref_node, source_name, start_freq_value, end_freq_value, num_points_value, sweep_type, device_noise_operators, replace_ground, print_parameters);
}

std::vector<std::string> NoiseSimulationParameters::to_xyce_directives() const {
    // init output directive list
    std::vector<std::string> directives;

    // prepend replaceground preprocessor directive when enabled
    if (replace_ground) {
        directives.push_back(".PREPROCESS REPLACEGROUND TRUE");
    }

    // start with the noise analysis directive
    std::string noise_directive = ".NOISE " + output_node + " " + ref_node + " " + source_name + " " + start_freq_value + " " + end_freq_value + " " + num_points_value + " " + sweep_type;

    // append device noise operators
    for (const auto& dno : device_noise_operators) {
        noise_directive += " " + dno.type + " " + dno.node + " " + dno.source;
    }

    directives.push_back(noise_directive);

    // append noise print directive when configured
    if (print_parameters) {
        directives.push_back(print_parameters->to_xyce_statement());
    }

    // return the full directive list
    return directives;
}

bool NoiseSimulationParameters::operator==(const NoiseSimulationParameters& other) const { return output_node == other.output_node && ref_node == other.ref_node && source_name == other.source_name && start_freq_value == other.start_freq_value && end_freq_value == other.end_freq_value && num_points_value == other.num_points_value && sweep_type == other.sweep_type && device_noise_operators == other.device_noise_operators && replace_ground == other.replace_ground && print_parameters == other.print_parameters; }
