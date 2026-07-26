#include <algorithm>
#include <cctype>
#include <optional>
#include <regex>
#include <string>
#include <vector>

#include "transient_simulation_parameters.h"

// normalize a string to uppercase
static std::string tran_to_upper(std::string s) {
    // convert each character to upper case
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::toupper(c); });
    // return converted string
    return s;
}

// tokenize a directive by whitespace
static std::vector<std::string> tran_tokenize(const std::string& directive) {
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

TransientSchedulePoint::TransientSchedulePoint(std::string time_value, std::string max_time_step_value) :
    time_value(std::move(time_value)), max_time_step_value(std::move(max_time_step_value)) {}

bool TransientSchedulePoint::operator==(const TransientSchedulePoint& other) const { return time_value == other.time_value && max_time_step_value == other.max_time_step_value; }

TransientSimulationParameters::TransientSimulationParameters(std::string initial_step_value, std::string final_time_value, std::string start_time_value, std::string step_ceiling_value, std::string op_keyword, std::vector<TransientSchedulePoint> schedule_points, bool replace_ground, std::optional<PrintParameters> print_parameters, std::vector<FftParameters> fft_parameters, std::vector<FourParameters> four_parameters, std::vector<MeasureEntry> measure_parameters, std::optional<SensParameter> sensitivity) :
    initial_step_value(std::move(initial_step_value)), final_time_value(std::move(final_time_value)), start_time_value(std::move(start_time_value)), step_ceiling_value(std::move(step_ceiling_value)), op_keyword(std::move(op_keyword)), schedule_points(std::move(schedule_points)), replace_ground(replace_ground), print_parameters(std::move(print_parameters)), fft_parameters(std::move(fft_parameters)), four_parameters(std::move(four_parameters)), measure_parameters(std::move(measure_parameters)), sensitivity(std::move(sensitivity)) {}

std::optional<TransientSimulationParameters> TransientSimulationParameters::from_xyce_directives(const std::vector<std::string>& directives) {
    // init defaults
    std::string initial_step_value;
    std::string final_time_value;
    std::string start_time_value;
    std::string step_ceiling_value;
    std::string op_keyword;
    std::vector<TransientSchedulePoint> schedule_points;
    bool replace_ground = true;
    std::optional<PrintParameters> print_parameters;
    std::vector<FftParameters> fft_parameters;
    std::vector<FourParameters> four_parameters;
    std::vector<MeasureEntry> measure_parameters;
    std::optional<SensParameter> sensitivity;

    // flag indicating whether a valid directive was found
    bool found = false;

    // parse directives
    for (const auto& directive : directives) {
        // tokenize the directive
        const auto tokens = tran_tokenize(directive);

        // skip empty directives
        if (tokens.empty()) {
            continue;
        }

        const std::string cmd = tran_to_upper(tokens[0]);

        // parse print directives and retain transient-specific output config
        if (cmd == ".PRINT") {
            // parse the print statement from the directive
            const auto print_statement = PrintParameters::from_xyce_statement(directive);
            // retain transient print parameters when found
            if (print_statement) {
                const std::string print_type_upper = tran_to_upper(print_statement->print_type);
                if (print_type_upper == "TRAN" || print_type_upper == "TRANADJOINT") {
                    // store the parsed print parameters
                    print_parameters = *print_statement;
                    continue;
                }
            }
        }

        // parse fft directives
        if (cmd == ".FFT") {
            // parse the fft statement from the directive
            const auto fft_statement = FftParameters::from_xyce_statement(directive);
            // retain fft parameters when found
            if (fft_statement) {
                // append the parsed fft parameters
                fft_parameters.push_back(*fft_statement);
            }
            continue;
        }

        // parse four directives
        if (cmd == ".FOUR") {
            // parse the four statement from the directive
            const auto four_statement = FourParameters::from_xyce_statement(directive);
            // retain four parameters when found
            if (four_statement) {
                // append the parsed four parameters
                four_parameters.push_back(*four_statement);
            }
            continue;
        }

        // parse measure directives
        if (cmd == ".MEASURE" || cmd == ".MEAS") {
            // parse the measure statement from the directive
            const auto measure_statement = MeasureEntry::from_xyce_statement(directive);
            // retain measure parameters when found and analysis type matches
            if (measure_statement) {
                const std::string analysis_type_upper = tran_to_upper(measure_statement->analysis_type);
                if (analysis_type_upper == "TRAN" || analysis_type_upper == "TRAN_CONT") {
                    // append the parsed measure parameters
                    measure_parameters.push_back(*measure_statement);
                }
            }
            continue;
        }

        // handle preprocess replaceground
        if (cmd == ".PREPROCESS" && tokens.size() > 2 && tran_to_upper(tokens[1]) == "REPLACEGROUND") {
            // set flag based on value
            replace_ground = (tran_to_upper(tokens[2]) == "TRUE");
            continue;
        }

        // skip non-TRAN directives
        if (cmd != ".TRAN") {
            continue;
        }

        // flag indicating a valid TRAN directive was found
        found = true;

        // extract schedule clause from raw directive if present
        std::string directive_without_schedule = directive;
        std::smatch schedule_match;
        static const std::regex schedule_re(R"(\{schedule\s*\(([^}]*)\)\s*\})", std::regex::icase);
        if (std::regex_search(directive, schedule_match, schedule_re)) {
            // split comma-separated time/step pairs from schedule content
            const std::string schedule_content = schedule_match[1].str();
            // strip each value and filter empties
            std::vector<std::string> schedule_values;
            std::string current_val;
            for (const char c : schedule_content) {
                if (c == ',') {
                    if (!current_val.empty()) {
                        schedule_values.push_back(current_val);
                        current_val.clear();
                    }
                }
                else if (!std::isspace(static_cast<unsigned char>(c))) {
                    current_val += c;
                }
            }
            if (!current_val.empty()) {
                schedule_values.push_back(current_val);
            }

            // pair consecutive entries as (time, max_step)
            for (size_t i = 0; i + 1 < schedule_values.size(); i += 2) {
                schedule_points.emplace_back(schedule_values[i], schedule_values[i + 1]);
            }

            // strip schedule clause so remaining tokens parse cleanly
            directive_without_schedule = directive.substr(0, schedule_match.position());
        }

        // re-tokenize after schedule removal
        const auto tokens_after_schedule = tran_tokenize(directive_without_schedule);

        // parse initial step (required, position 1)
        if (tokens_after_schedule.size() >= 2) {
            initial_step_value = tokens_after_schedule[1];
        }

        // parse final time (required, position 2)
        if (tokens_after_schedule.size() >= 3) {
            final_time_value = tokens_after_schedule[2];
        }

        // separate NOOP/UIC keywords from positional arguments
        std::vector<std::string> positional;
        for (size_t i = 3; i < tokens_after_schedule.size(); ++i) {
            const std::string upper = tran_to_upper(tokens_after_schedule[i]);
            if (upper == "NOOP" || upper == "UIC") {
                // capture op keyword
                op_keyword = upper;
            }
            else {
                // accumulate remaining positional args
                positional.push_back(tokens_after_schedule[i]);
            }
        }

        // assign optional start time (position 3)
        if (positional.size() >= 1) {
            start_time_value = positional[0];
        }

        // assign optional step ceiling (position 4)
        if (positional.size() >= 2) {
            step_ceiling_value = positional[1];
        }
    }

    // parse sensitivity as a companion directive before analysis detection
    // For now, we'll return nullopt for sensitivity
    // In the full implementation, this would parse .SENS directives

    // return instance if a valid directive was found
    if (!found) {
        return std::nullopt;
    }

    return TransientSimulationParameters(initial_step_value, final_time_value, start_time_value, step_ceiling_value, op_keyword, schedule_points, replace_ground, print_parameters, fft_parameters, four_parameters, measure_parameters, sensitivity);
}

std::vector<std::string> TransientSimulationParameters::to_xyce_directives() const {
    // init output directive list
    std::vector<std::string> directives;

    // prepend replaceground preprocessor directive when enabled
    if (replace_ground) {
        directives.push_back(".PREPROCESS REPLACEGROUND TRUE");
    }

    // start with the transient analysis directive
    std::string tran_directive = ".TRAN " + initial_step_value + " " + final_time_value;

    // add start time if specified
    if (!start_time_value.empty()) {
        tran_directive += " " + start_time_value;
    }

    // add step ceiling if specified
    if (!step_ceiling_value.empty()) {
        tran_directive += " " + step_ceiling_value;
    }

    // add NOOP/UIC keyword if specified
    if (!op_keyword.empty()) {
        tran_directive += " " + op_keyword;
    }

    // add schedule points if present
    if (!schedule_points.empty()) {
        std::string schedule_str = " {schedule(";
        for (size_t i = 0; i < schedule_points.size(); ++i) {
            if (i > 0) {
                schedule_str += ", ";
            }
            schedule_str += schedule_points[i].time_value + ", " + schedule_points[i].max_time_step_value;
        }
        schedule_str += ")}";
        tran_directive += schedule_str;
    }

    directives.push_back(tran_directive);

    // append transient print directive when configured
    if (print_parameters) {
        directives.push_back(print_parameters->to_xyce_statement());
    }

    // append sensitivity directives when configured
    // For now, skip sensitivity directives
    // In the full implementation, this would call sensitivity->to_xyce_directives()

    // append fft directives
    for (const auto& fft : fft_parameters) {
        directives.push_back(fft.to_xyce_statement());
    }

    // append four directives
    for (const auto& four : four_parameters) {
        directives.push_back(four.to_xyce_statement());
    }

    // append measure directives
    for (const auto& measure : measure_parameters) {
        directives.push_back(measure.to_xyce_statement());
    }

    // return the full directive list
    return directives;
}

bool TransientSimulationParameters::operator==(const TransientSimulationParameters& other) const { return initial_step_value == other.initial_step_value && final_time_value == other.final_time_value && start_time_value == other.start_time_value && step_ceiling_value == other.step_ceiling_value && op_keyword == other.op_keyword && schedule_points == other.schedule_points && replace_ground == other.replace_ground && print_parameters == other.print_parameters && fft_parameters == other.fft_parameters && four_parameters == other.four_parameters && measure_parameters == other.measure_parameters && sensitivity == other.sensitivity; }
