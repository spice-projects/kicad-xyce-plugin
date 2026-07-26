#include <algorithm>
#include <cctype>
#include <optional>
#include <string>
#include <vector>

#include "lin_simulation_parameters.h"

// normalize a string to uppercase
static std::string lin_to_upper(std::string s) {
    // convert each character to upper case
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::toupper(c); });
    // return converted string
    return s;
}

// tokenize a directive by whitespace
static std::vector<std::string> lin_tokenize(const std::string& directive) {
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

LinSimulationParameters::LinSimulationParameters(bool sparcalc, std::string format, std::string lintype, std::string dataformat, std::string file, std::string width, std::string precision, std::string sweep_mode, std::string points, std::string start, std::string end, std::string data_table_name, bool replace_ground, std::optional<PrintParameters> print_parameters) :
    sparcalc(sparcalc), format(std::move(format)), lintype(std::move(lintype)), dataformat(std::move(dataformat)), file(std::move(file)), width(std::move(width)), precision(std::move(precision)), sweep_mode(std::move(sweep_mode)), points(std::move(points)), start(std::move(start)), end(std::move(end)), data_table_name(std::move(data_table_name)), replace_ground(replace_ground), print_parameters(std::move(print_parameters)) {}

std::optional<LinSimulationParameters> LinSimulationParameters::from_xyce_directives(const std::vector<std::string>& directives) {
    // init defaults
    bool sparcalc = true;
    std::string format = "TOUCHSTONE2";
    std::string lintype = "S";
    std::string dataformat = "RI";
    std::string file;
    std::string width;
    std::string precision;
    std::string sweep_mode = "LIN";
    std::string points;
    std::string start;
    std::string end;
    std::string data_table_name;
    bool replace_ground = false;
    std::optional<PrintParameters> print_parameters;

    // flag indicating whether a valid directive was found
    bool found = false;

    // parse directives
    for (const auto& directive : directives) {
        // tokenize the directive
        const auto tokens = lin_tokenize(directive);

        // skip empty directives
        if (tokens.empty()) {
            continue;
        }

        const std::string cmd = lin_to_upper(tokens[0]);

        // parse print directives and retain lin-specific output config
        if (cmd == ".PRINT") {
            // parse the print statement from the directive
            const auto print_statement = PrintParameters::from_xyce_statement(directive);
            // retain ac print parameters when found
            if (print_statement) {
                const std::string print_type_upper = lin_to_upper(print_statement->print_type);
                if (print_type_upper == "AC") {
                    // store the parsed print parameters
                    print_parameters = *print_statement;
                    continue;
                }
            }
        }

        // handle preprocess replaceground
        if (cmd == ".PREPROCESS" && tokens.size() > 2 && lin_to_upper(tokens[1]) == "REPLACEGROUND") {
            // set replace_ground based on the third token
            replace_ground = (lin_to_upper(tokens[2]) == "TRUE");
            continue;
        }

        // parse the embedded ac sweep directive
        if (cmd == ".AC") {
            if (tokens.size() < 2) {
                continue;
            }

            const std::string second = lin_to_upper(tokens[1]);

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
            continue;
        }

        // skip non-LIN directives
        if (cmd != ".LIN") {
            continue;
        }

        // flag indicating a valid LIN directive was found
        found = true;

        // parse keyword=value pairs from the .LIN directive
        for (size_t i = 1; i < tokens.size(); ++i) {
            const auto& token = tokens[i];
            // normalize the token for case-insensitive key detection
            const std::string upper = lin_to_upper(token);

            // skip tokens without an equals sign
            if (upper.find('=') == std::string::npos) {
                continue;
            }

            // split key and value at the first equals sign
            const auto eq_pos = token.find('=');
            const std::string key = token.substr(0, eq_pos);
            const std::string val = token.substr(eq_pos + 1);

            const std::string key_upper = lin_to_upper(key);

            if (key_upper == "SPARCALC") {
                // set sparcalc from the value
                sparcalc = (lin_to_upper(val) == "1" || lin_to_upper(val) == "TRUE" || lin_to_upper(val) == "YES");
            }
            else if (key_upper == "FORMAT") {
                // set the output format
                format = lin_to_upper(val);
            }
            else if (key_upper == "TYPE") {
                // set the s-parameter type
                lintype = lin_to_upper(val);
            }
            else if (key_upper == "DATAFORMAT") {
                // set the data format
                dataformat = lin_to_upper(val);
            }
            else if (key_upper == "FILE") {
                // set the output file name
                file = val;
            }
            else if (key_upper == "WIDTH") {
                // set the column width
                width = val;
            }
            else if (key_upper == "PRECISION") {
                // set the output precision
                precision = val;
            }
        }
    }

    // return instance if a valid directive was found
    if (!found) {
        return std::nullopt;
    }

    return LinSimulationParameters(sparcalc, format, lintype, dataformat, file, width, precision, sweep_mode, points, start, end, data_table_name, replace_ground, print_parameters);
}

std::vector<std::string> LinSimulationParameters::to_xyce_directives() const {
    // init output directive list
    std::vector<std::string> directives;

    // prepend replaceground preprocessing when enabled
    if (replace_ground) {
        directives.push_back(".PREPROCESS REPLACEGROUND TRUE");
    }

    // build the embedded ac sweep directive
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

    // build and append the .LIN directive
    std::string lin_directive = ".LIN";
    if (!sparcalc) {
        lin_directive += " SPARCALC=0";
    }
    if (format != "TOUCHSTONE2") {
        lin_directive += " FORMAT=" + format;
    }
    if (lintype != "S") {
        lin_directive += " TYPE=" + lintype;
    }
    if (dataformat != "RI") {
        lin_directive += " DATAFORMAT=" + dataformat;
    }
    if (!file.empty()) {
        lin_directive += " FILE=" + file;
    }
    if (!width.empty()) {
        lin_directive += " WIDTH=" + width;
    }
    if (!precision.empty()) {
        lin_directive += " PRECISION=" + precision;
    }
    directives.push_back(lin_directive);

    // append ac print directive when configured
    if (print_parameters) {
        const std::string print_type_upper = lin_to_upper(print_parameters->print_type);
        if (print_type_upper == "AC") {
            directives.push_back(print_parameters->to_xyce_statement());
        }
    }

    // return the full directive list
    return directives;
}

bool LinSimulationParameters::operator==(const LinSimulationParameters& other) const { return sparcalc == other.sparcalc && format == other.format && lintype == other.lintype && dataformat == other.dataformat && file == other.file && width == other.width && precision == other.precision && sweep_mode == other.sweep_mode && points == other.points && start == other.start && end == other.end && data_table_name == other.data_table_name && replace_ground == other.replace_ground && print_parameters == other.print_parameters; }
