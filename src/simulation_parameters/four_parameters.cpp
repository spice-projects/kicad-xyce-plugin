#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

#include "four_parameters.h"

// normalize a string to uppercase
static std::string four_to_upper(std::string s) {
    // convert each character to upper case
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::toupper(c); });
    // return converted string
    return s;
}

FourParameters::FourParameters(std::string fundamental_frequency, std::vector<std::string> output_variables) :
    fundamental_frequency(std::move(fundamental_frequency)), output_variables(std::move(output_variables)) {}

std::optional<FourParameters> FourParameters::from_xyce_statement(const std::string& four_statement) {
    // tokenize the directive (simple whitespace split)
    std::vector<std::string> tokens;
    std::string current;
    // iterate characters
    for (const char ch : four_statement) {
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
    // reject non-four statements
    if (tokens.size() < 3 || four_to_upper(tokens[0]) != ".FOUR") {
        return std::nullopt;
    }
    // fundamental frequency is the second token
    const std::string fundamental_frequency = tokens[1];
    // output variables are all remaining tokens
    std::vector<std::string> output_variables(tokens.begin() + 2, tokens.end());
    // return model
    return FourParameters(fundamental_frequency, std::move(output_variables));
}

std::string FourParameters::to_xyce_statement() const {
    // init tokens
    std::vector<std::string> tokens = {".FOUR", fundamental_frequency};
    // add output variables
    for (const auto& var : output_variables) {
        tokens.push_back(var);
    }
    // build joined statement
    std::string result;
    for (size_t i = 0; i < tokens.size(); ++i) {
        // add separator
        if (i > 0) {
            result += ' ';
        }
        // add token
        result += tokens[i];
    }
    // return joined statement
    return result;
}

bool FourParameters::operator==(const FourParameters& other) const { return fundamental_frequency == other.fundamental_frequency && output_variables == other.output_variables; }
