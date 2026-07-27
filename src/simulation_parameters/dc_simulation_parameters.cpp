#include <cctype>
#include <optional>
#include <string>
#include <vector>

#include "../util.h"
#include "dc_simulation_parameters.h"

DCSimulationParameters::DCSimulationParameters(std::string sweep_mode, std::string primary_variable, std::string start, std::string stop, std::string step, std::string points, std::vector<std::string> list_values, std::string data_table_name, std::string secondary_variable, std::string secondary_start, std::string secondary_stop, std::string secondary_step, std::string secondary_points, bool replace_ground, std::optional<PrintParameters> print_parameters, std::vector<MeasureEntry> measure_parameters, std::optional<SensParameter> sensitivity) :
    sweep_mode(std::move(sweep_mode)), primary_variable(std::move(primary_variable)), start(std::move(start)), stop(std::move(stop)), step(std::move(step)), points(std::move(points)), list_values(std::move(list_values)), data_table_name(std::move(data_table_name)), secondary_variable(std::move(secondary_variable)), secondary_start(std::move(secondary_start)), secondary_stop(std::move(secondary_stop)), secondary_step(std::move(secondary_step)), secondary_points(std::move(secondary_points)), replace_ground(replace_ground), print_parameters(std::move(print_parameters)), measure_parameters(std::move(measure_parameters)), sensitivity(std::move(sensitivity)) {}

std::optional<DCSimulationParameters> DCSimulationParameters::from_xyce_directives(const std::vector<std::string>& directives) {
    // init defaults
    std::string sweep_mode = "LIN";
    std::string primary_variable;
    std::string start;
    std::string stop;
    std::string step;
    std::string points;
    std::vector<std::string> list_values;
    std::string data_table_name;
    std::string secondary_variable;
    std::string secondary_start;
    std::string secondary_stop;
    std::string secondary_step;
    std::string secondary_points;
    bool replace_ground = true;
    std::optional<PrintParameters> print_parameters;
    std::vector<MeasureEntry> measure_parameters;
    std::optional<SensParameter> sensitivity;

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

        // parse print directives and retain dc-specific output config
        if (cmd == ".PRINT") {
            // parse the print statement from the directive
            const auto print_statement = PrintParameters::from_xyce_statement(directive);
            // retain dc print parameters when found
            if (print_statement) {
                const std::string print_type_upper = to_upper(print_statement->print_type);
                if (print_type_upper == "DC" || print_type_upper == "HOMOTOPY") {
                    // store the parsed print parameters
                    print_parameters = *print_statement;
                    continue;
                }
            }
        }

        // handle preprocess replaceground
        if (cmd == ".PREPROCESS" && tokens.size() > 2 && to_upper(tokens[1]) == "REPLACEGROUND") {
            // set replace_ground based on the third token
            replace_ground = (to_upper(tokens[2]) == "TRUE");
            continue;
        }

        // parse measure directives
        if (cmd == ".MEASURE" || cmd == ".MEAS") {
            // parse the measure statement from the directive
            const auto measure_statement = MeasureEntry::from_xyce_statement(directive);
            // retain measure parameters when found and analysis type matches
            if (measure_statement) {
                const std::string analysis_type_upper = to_upper(measure_statement->analysis_type);
                if (analysis_type_upper == "DC" || analysis_type_upper == "DC_CONT") {
                    // append the parsed measure parameters
                    measure_parameters.push_back(*measure_statement);
                }
            }
            continue;
        }

        // skip non-DC directives
        if (cmd != ".DC") {
            continue;
        }

        // flag indicating a valid DC directive was found
        found = true;

        // handle DATA sweep: .DC DATA=<tablename>
        if (tokens.size() == 2 && tokens[1].find('=') != std::string::npos && to_upper(tokens[1].substr(0, 5)) == "DATA=") {
            // set sweep mode and data table name
            sweep_mode = "DATA";
            data_table_name = tokens[1].substr(5);
            continue;
        }

        if (tokens.size() < 2) {
            continue;
        }

        const std::string second = to_upper(tokens[1]);

        // detect decade or octave log sweep: .DC DEC|OCT var start stop points
        if (second == "DEC" || second == "OCT") {
            sweep_mode = second;
            // primary sweep tokens: MODE var start stop points
            if (tokens.size() >= 6) {
                primary_variable = tokens[2];
                start = tokens[3];
                stop = tokens[4];
                points = tokens[5];
            }
            // optional secondary sweep: MODE var2 start2 stop2 points2
            if (tokens.size() >= 11 && (to_upper(tokens[6]) == "DEC" || to_upper(tokens[6]) == "OCT")) {
                secondary_variable = tokens[7];
                secondary_start = tokens[8];
                secondary_stop = tokens[9];
                secondary_points = tokens[10];
            }
            continue;
        }

        // detect LIST sweep: .DC var LIST val [val ...]
        if (tokens.size() >= 3 && to_upper(tokens[2]) == "LIST") {
            // set sweep mode, primary variable, and list values
            sweep_mode = "LIST";
            primary_variable = tokens[1];
            for (size_t i = 3; i < tokens.size(); ++i) {
                list_values.push_back(std::string(tokens[i]));
            }
            continue;
        }

        // linear sweep: .DC [LIN] var start stop step [var2 start2 stop2 step2]
        sweep_mode = "LIN";
        if (second == "LIN") {
            // explicit LIN keyword
            if (tokens.size() >= 6) {
                primary_variable = tokens[2];
                start = tokens[3];
                stop = tokens[4];
                step = tokens[5];
            }
            // optional secondary sweep tokens: var2 start2 stop2 step2
            if (tokens.size() >= 10) {
                secondary_variable = tokens[6];
                secondary_start = tokens[7];
                secondary_stop = tokens[8];
                secondary_step = tokens[9];
            }
        }
        else {
            // implicit LIN
            if (tokens.size() >= 5) {
                primary_variable = tokens[1];
                start = tokens[2];
                stop = tokens[3];
                step = tokens[4];
            }
            // optional secondary sweep tokens: var2 start2 stop2 step2
            if (tokens.size() >= 9) {
                secondary_variable = tokens[5];
                secondary_start = tokens[6];
                secondary_stop = tokens[7];
                secondary_step = tokens[8];
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

    return DCSimulationParameters(sweep_mode, primary_variable, start, stop, step, points, list_values, data_table_name, secondary_variable, secondary_start, secondary_stop, secondary_step, secondary_points, replace_ground, print_parameters, measure_parameters, sensitivity);
}

std::vector<std::string> DCSimulationParameters::to_xyce_directives() const {
    // init output directive list
    std::vector<std::string> directives;

    // prepend replaceground preprocessing when enabled
    if (replace_ground) {
        directives.push_back(".PREPROCESS REPLACEGROUND TRUE");
    }

    // build the core dc directive based on the selected sweep mode
    std::string dc_directive = ".DC";

    if (sweep_mode == "DATA") {
        dc_directive += " DATA=" + data_table_name;
    }
    else if (sweep_mode == "LIST") {
        dc_directive += " " + primary_variable + " LIST";
        for (const auto& val : list_values) {
            dc_directive += " " + val;
        }
    }
    else if (sweep_mode == "LIN") {
        dc_directive += " " + primary_variable + " " + start + " " + stop + " " + step;
        if (!secondary_variable.empty()) {
            dc_directive += " " + secondary_variable + " " + secondary_start + " " + secondary_stop + " " + secondary_step;
        }
    }
    else {
        // log sweep (DEC or OCT)
        dc_directive += " " + sweep_mode + " " + primary_variable + " " + start + " " + stop + " " + points;
        if (!secondary_variable.empty()) {
            dc_directive += " " + secondary_variable + " " + secondary_start + " " + secondary_stop + " " + secondary_points;
        }
    }

    directives.push_back(dc_directive);

    // append dc print directive when configured
    if (print_parameters) {
        directives.push_back(print_parameters->to_xyce_statement());
    }

    // append sensitivity directives when configured
    if (sensitivity) {
        const auto sens_directives = sensitivity->to_xyce_directives();
        directives.insert(directives.end(), sens_directives.begin(), sens_directives.end());
    }

    // append measure directives
    for (const auto& measure : measure_parameters) {
        directives.push_back(measure.to_xyce_statement());
    }

    // return the full directive list
    return directives;
}

bool DCSimulationParameters::operator==(const DCSimulationParameters& other) const {
    // compare all fields for equality
    return sweep_mode == other.sweep_mode && primary_variable == other.primary_variable && start == other.start && stop == other.stop && step == other.step && points == other.points && list_values == other.list_values && data_table_name == other.data_table_name && secondary_variable == other.secondary_variable && secondary_start == other.secondary_start && secondary_stop == other.secondary_stop && secondary_step == other.secondary_step && secondary_points == other.secondary_points && replace_ground == other.replace_ground && print_parameters == other.print_parameters && measure_parameters == other.measure_parameters && sensitivity == other.sensitivity;
}
