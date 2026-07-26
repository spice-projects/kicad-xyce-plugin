#include <algorithm>
#include <cctype>
#include <optional>
#include <string>
#include <vector>

#include "ac_simulation_parameters.h"

// normalize a string to uppercase
static std::string ac_to_upper(std::string s) {
    // convert each character to upper case
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::toupper(c); });
    // return converted string
    return s;
}

// tokenize a directive by whitespace
static std::vector<std::string> ac_tokenize(const std::string& directive) {
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

AcSimulationParameters::AcSimulationParameters(std::string sweep_mode, std::string points, std::string start, std::string end, std::string data_table_name, bool replace_ground, std::optional<PrintParameters> print_parameters, std::vector<MeasureEntry> measure_parameters, std::optional<SensParameter> sensitivity) :
    sweep_mode(std::move(sweep_mode)), points(std::move(points)), start(std::move(start)), end(std::move(end)), data_table_name(std::move(data_table_name)), replace_ground(replace_ground), print_parameters(std::move(print_parameters)), measure_parameters(std::move(measure_parameters)), sensitivity(std::move(sensitivity)) {}

std::optional<AcSimulationParameters> AcSimulationParameters::from_xyce_directives(const std::vector<std::string>& directives) {
    // init defaults
    std::string sweep_mode = "LIN";
    std::string points;
    std::string start;
    std::string end;
    std::string data_table_name;
    bool replace_ground = true;
    std::optional<PrintParameters> print_parameters;
    std::vector<MeasureEntry> measure_parameters;
    std::optional<SensParameter> sensitivity;

    // flag indicating whether a valid directive was found
    bool found = false;

    // parse directives
    for (const auto& directive : directives) {
        // tokenize the directive
        const auto tokens = ac_tokenize(directive);

        // skip empty directives
        if (tokens.empty()) {
            continue;
        }

        const std::string cmd = ac_to_upper(tokens[0]);

        // parse print directives and retain ac-specific output config
        if (cmd == ".PRINT") {
            // parse the print statement from the directive
            const auto print_statement = PrintParameters::from_xyce_statement(directive);
            // retain ac print parameters when found
            if (print_statement) {
                const std::string print_type_upper = ac_to_upper(print_statement->print_type);
                if (print_type_upper == "AC" || print_type_upper == "AC_IC") {
                    // store the parsed print parameters
                    print_parameters = *print_statement;
                    continue;
                }
            }
        }

        // handle preprocess replaceground
        if (cmd == ".PREPROCESS" && tokens.size() > 2 && ac_to_upper(tokens[1]) == "REPLACEGROUND") {
            // set replace_ground based on the third token
            replace_ground = (ac_to_upper(tokens[2]) == "TRUE");
            continue;
        }

        // parse measure directives
        if (cmd == ".MEASURE" || cmd == ".MEAS") {
            // parse the measure statement from the directive
            const auto measure_statement = MeasureEntry::from_xyce_statement(directive);
            // retain measure parameters when found and analysis type matches
            if (measure_statement) {
                const std::string analysis_type_upper = ac_to_upper(measure_statement->analysis_type);
                if (analysis_type_upper == "AC" || analysis_type_upper == "AC_CONT") {
                    // append the parsed measure parameters
                    measure_parameters.push_back(*measure_statement);
                }
            }
            continue;
        }

        // skip non-AC directives
        if (cmd != ".AC") {
            continue;
        }

        // flag indicating a valid AC directive was found
        found = true;

        if (tokens.size() < 2) {
            continue;
        }

        const std::string second = ac_to_upper(tokens[1]);

        // handle DATA sweep: .AC DATA=<tablename>
        if (second.substr(0, 5) == "DATA=" && second.find('=') != std::string::npos) {
            // set sweep mode and data table name
            sweep_mode = "DATA";
            data_table_name = second.substr(5);
            continue;
        }

        // detect decade or octave log sweep: .AC DEC|OCT <points> <start> <end>
        if (second == "DEC" || second == "OCT") {
            sweep_mode = second;
            if (tokens.size() >= 5) {
                points = tokens[2];
                start = tokens[3];
                end = tokens[4];
            }
            continue;
        }

        // linear sweep: .AC [LIN] <points> <start> <end>
        sweep_mode = "LIN";
        if (second == "LIN") {
            // explicit LIN keyword
            if (tokens.size() >= 5) {
                points = tokens[2];
                start = tokens[3];
                end = tokens[4];
            }
        }
        else {
            // implicit LIN
            if (tokens.size() >= 4) {
                points = tokens[1];
                start = tokens[2];
                end = tokens[3];
            }
        }
    }

    // parse sensitivity as a companion directive before analysis detection
    // For now, we'll return nullopt for sensitivity
    // In the full implementation, this would parse .SENS directives

    // return instance if a valid directive was found
    if (!found) {
        return std::nullopt;
    }

    return AcSimulationParameters(sweep_mode, points, start, end, data_table_name, replace_ground, print_parameters, measure_parameters, sensitivity);
}

std::vector<std::string> AcSimulationParameters::to_xyce_directives() const {
    // init output directive list
    std::vector<std::string> directives;

    // prepend replaceground preprocessing when enabled
    if (replace_ground) {
        directives.push_back(".PREPROCESS REPLACEGROUND TRUE");
    }

    // build the core ac directive
    std::string ac_directive = ".AC";
    if (sweep_mode == "DATA") {
        ac_directive += " DATA=" + data_table_name;
    }
    else if (sweep_mode == "DEC" || sweep_mode == "OCT") {
        ac_directive += " " + sweep_mode + " " + points + " " + start + " " + end;
    }
    else {
        // lin sweep (explicit)
        ac_directive += " LIN " + points + " " + start + " " + end;
    }

    directives.push_back(ac_directive);

    // append ac print directive when configured
    if (print_parameters) {
        directives.push_back(print_parameters->to_xyce_statement());
    }

    // append sensitivity directives when configured
    // For now, skip sensitivity directives
    // In the full implementation, this would call sensitivity->to_xyce_directives()

    // append measure directives
    for (const auto& measure : measure_parameters) {
        directives.push_back(measure.to_xyce_statement());
    }

    // return the full directive list
    return directives;
}

bool AcSimulationParameters::operator==(const AcSimulationParameters& other) const {
    // compare all fields for equality
    return sweep_mode == other.sweep_mode && points == other.points && start == other.start && end == other.end && data_table_name == other.data_table_name && replace_ground == other.replace_ground && print_parameters == other.print_parameters && measure_parameters == other.measure_parameters && sensitivity == other.sensitivity;
}
