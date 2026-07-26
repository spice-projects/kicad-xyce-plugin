#include <algorithm>
#include <cctype>
#include <optional>
#include <string>
#include <vector>

#include "op_simulation_parameters.h"

// normalize a string to uppercase
static std::string op_to_upper(std::string s) {
    // convert each character to upper case
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::toupper(c); });
    // return converted string
    return s;
}

// tokenize a directive by whitespace
static std::vector<std::string> op_tokenize(const std::string& directive) {
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

NodesetEntry::NodesetEntry(std::string node, std::string voltage) :
    node(std::move(node)), voltage(std::move(voltage)) {}

bool NodesetEntry::operator==(const NodesetEntry& other) const { return node == other.node && voltage == other.voltage; }

IcEntry::IcEntry(std::string node, std::string voltage) :
    node(std::move(node)), voltage(std::move(voltage)) {}

bool IcEntry::operator==(const IcEntry& other) const { return node == other.node && voltage == other.voltage; }

OpSimulationParameters::OpSimulationParameters(bool print_dc_enabled, bool print_dc_all_nodes, bool print_dc_all_currents, std::vector<std::string> print_dc_specific_variables, std::string print_dc_format, std::string print_dc_file, bool save_enabled, std::string save_type, std::string save_file, std::vector<NodesetEntry> nodeset_entries, std::vector<IcEntry> ic_entries, bool replace_ground, std::optional<PrintParameters> print_parameters) :
    print_dc_enabled(print_dc_enabled), print_dc_all_nodes(print_dc_all_nodes), print_dc_all_currents(print_dc_all_currents), print_dc_specific_variables(std::move(print_dc_specific_variables)), print_dc_format(std::move(print_dc_format)), print_dc_file(std::move(print_dc_file)), save_enabled(save_enabled), save_type(std::move(save_type)), save_file(std::move(save_file)), nodeset_entries(std::move(nodeset_entries)), ic_entries(std::move(ic_entries)), replace_ground(replace_ground), print_parameters(std::move(print_parameters)) {}

std::optional<OpSimulationParameters> OpSimulationParameters::from_xyce_directives(const std::vector<std::string>& directives) {
    // preprocess replaceground flag
    bool replace_ground = true;
    // init flag
    bool print_dc_enabled = false;
    // init list
    std::vector<std::string> print_dc_vars;
    // init parsed print parameters object
    std::optional<PrintParameters> print_parameters_parsed;
    // init flag
    bool save_enabled = false;
    // init list
    std::vector<NodesetEntry> nodeset_entries;
    // init list
    std::vector<IcEntry> ic_entries;

    // flag indicating whether a valid directive was found
    bool found = false;

    // parse directives
    for (const auto& directive : directives) {
        // tokenize the directive
        const auto tokens = op_tokenize(directive);

        // skip empty directives
        if (tokens.empty()) {
            continue;
        }

        const std::string cmd = op_to_upper(tokens[0]);

        // check .OP directive
        if (cmd == ".OP") {
            // set flag
            found = true;
            continue;
        }

        // handle print dc
        if (cmd == ".PRINT" && tokens.size() > 1 && op_to_upper(tokens[1]) == "DC") {
            // set enabled
            print_dc_enabled = true;
            // parse via PrintParameters for structured wildcard/format access
            const auto print_parameters_parsed_opt = PrintParameters::from_xyce_statement(directive);
            if (print_parameters_parsed_opt) {
                print_parameters_parsed = *print_parameters_parsed_opt;
            }
            // also collect raw vars for backward-compatible list
            for (size_t i = 2; i < tokens.size(); ++i) {
                print_dc_vars.push_back(tokens[i]);
            }
            continue;
        }

        // handle save
        if (cmd == ".SAVE") {
            // set enabled
            save_enabled = true;
            continue;
        }

        // handle nodeset
        if (cmd == ".NODESET") {
            // process pairs
            for (size_t i = 1; i < tokens.size(); ++i) {
                const auto& pair = tokens[i];
                // check if pair valid
                if (pair.find('=') != std::string::npos) {
                    // split node and voltage
                    const auto eq_pos = pair.find('=');
                    const std::string node_part = pair.substr(0, eq_pos);
                    const std::string voltage = pair.substr(eq_pos + 1);
                    // validate
                    if (node_part.substr(0, 2) == "V(" && node_part.back() == ')') {
                        // append entry
                        nodeset_entries.emplace_back(node_part.substr(2, node_part.length() - 3), voltage);
                    }
                }
            }
            continue;
        }

        // handle initial conditions (.IC and .DCVOLT use the same format)
        if (cmd == ".IC" || cmd == ".DCVOLT") {
            // iterate tokens looking for V(node)=val or node val pairs
            for (size_t i = 1; i < tokens.size(); ++i) {
                const auto& token = tokens[i];
                if (token.find('=') != std::string::npos) {
                    // V(node)=val form
                    const auto eq_pos = token.find('=');
                    const std::string lhs = token.substr(0, eq_pos);
                    const std::string voltage = token.substr(eq_pos + 1);
                    std::string node;
                    if (lhs.substr(0, 2) == "V(" && lhs.back() == ')') {
                        node = lhs.substr(2, lhs.length() - 3);
                    }
                    else {
                        node = lhs;
                    }
                    ic_entries.emplace_back(node, voltage);
                }
                else if (i + 1 < tokens.size()) {
                    // node val pair form
                    ic_entries.emplace_back(token, tokens[i + 1]);
                    ++i;
                }
            }
            continue;
        }

        // preprocess replaceground
        if (cmd == ".PREPROCESS" && tokens.size() > 2 && op_to_upper(tokens[1]) == "REPLACEGROUND") {
            // set flag based on value
            replace_ground = (op_to_upper(tokens[2]) == "TRUE");
        }
    }

    // return instance if a valid directive was found
    if (!found) {
        return std::nullopt;
    }

    return OpSimulationParameters(print_dc_enabled, false, false, print_dc_vars, "", "", save_enabled, "NODESET", "", nodeset_entries, ic_entries, replace_ground, print_parameters_parsed);
}

std::vector<std::string> OpSimulationParameters::to_xyce_directives() const {
    // init output directive list
    std::vector<std::string> directives;

    // start with the .OP directive
    directives.push_back(".OP");

    // use print_parameters when set (newer wildcard approach)
    if (print_parameters) {
        // emit the statement directly using the structured print parameters
        directives.push_back(print_parameters->to_xyce_statement());
    }
    // check enabled
    else if (print_dc_enabled) {
        // start tokens
        std::string tokens = ".PRINT DC";
        // check format
        if (!print_dc_format.empty()) {
            // append format
            tokens += " FORMAT=" + print_dc_format;
        }
        // check file
        if (!print_dc_file.empty()) {
            // append file
            tokens += " FILE=" + print_dc_file;
        }
        // add custom vars
        for (const auto& var : print_dc_specific_variables) {
            tokens += " " + var;
        }
        directives.push_back(tokens);
    }

    // check save enabled
    if (save_enabled) {
        // build tokens
        std::string tokens = ".SAVE";
        // append type
        tokens += " TYPE=" + save_type;
        // check file
        if (!save_file.empty()) {
            // append file
            tokens += " FILE=" + save_file;
        }
        directives.push_back(tokens);
    }

    // check nodeset entries
    if (!nodeset_entries.empty()) {
        // format pairs
        std::string pairs;
        for (const auto& entry : nodeset_entries) {
            if (!pairs.empty()) {
                pairs += " ";
            }
            pairs += "V(" + entry.node + ")=" + entry.voltage;
        }
        // append directive
        directives.push_back(".NODESET " + pairs);
    }

    // check initial condition entries
    if (!ic_entries.empty()) {
        // format pairs
        std::string pairs;
        for (const auto& entry : ic_entries) {
            if (!pairs.empty()) {
                pairs += " ";
            }
            pairs += "V(" + entry.node + ")=" + entry.voltage;
        }
        // append directive
        directives.push_back(".IC " + pairs);
    }

    // prepend replaceground preprocessing when enabled
    if (replace_ground) {
        directives.insert(directives.begin(), ".PREPROCESS REPLACEGROUND TRUE");
    }

    // return directives
    return directives;
}

bool OpSimulationParameters::operator==(const OpSimulationParameters& other) const { return print_dc_enabled == other.print_dc_enabled && print_dc_all_nodes == other.print_dc_all_nodes && print_dc_all_currents == other.print_dc_all_currents && print_dc_specific_variables == other.print_dc_specific_variables && print_dc_format == other.print_dc_format && print_dc_file == other.print_dc_file && save_enabled == other.save_enabled && save_type == other.save_type && save_file == other.save_file && nodeset_entries == other.nodeset_entries && ic_entries == other.ic_entries && replace_ground == other.replace_ground && print_parameters == other.print_parameters; }
