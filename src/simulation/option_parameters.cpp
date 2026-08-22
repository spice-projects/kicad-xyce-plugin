#include <cctype>
#include <string>
#include <vector>

#include "../core/util.h"
#include "option_parameters.h"

struct NetlistTopology;

// parse a series of option tokens into a normalized map
static std::map<std::string, std::string> parse_option_tokens(const std::vector<std::string>& tokens) {
    // parse a series of option tokens into a normalized dictionary
    std::map<std::string, std::string> options;
    for (const auto& token : tokens) {
        // skip empty tokens produced by extra whitespace
        if (token.empty()) {
            continue;
        }
        // split key/value pairs and normalize keys to uppercase
        const auto eq_pos = token.find('=');
        if (eq_pos != std::string::npos) {
            const std::string key = to_upper(token.substr(0, eq_pos));
            const std::string val = token.substr(eq_pos + 1);
            options[key] = val;
            continue;
        }
        // support flag-style options without an explicit value
        options[to_upper(token)] = "";
    }
    return options;
}

OptionParameters::OptionParameters(std::map<std::string, std::string> device, std::map<std::string, std::string> timeint, std::map<std::string, std::string> nonlin, std::map<std::string, std::string> linsol, std::map<std::string, std::string> fft, std::map<std::string, std::string> diagnostic) :
    device(std::move(device)), timeint(std::move(timeint)), nonlin(std::move(nonlin)), linsol(std::move(linsol)), fft(std::move(fft)), diagnostic(std::move(diagnostic)) {}

OptionParameters OptionParameters::from_xyce_directives(const std::vector<std::string>& directives) {
    // init option groups
    std::map<std::string, std::string> device;
    std::map<std::string, std::string> timeint;
    std::map<std::string, std::string> nonlin;
    std::map<std::string, std::string> linsol;
    std::map<std::string, std::string> fft;
    std::map<std::string, std::string> diagnostic;

    // parse each directive looking for supported option packages
    for (const auto& directive : directives) {
        // break directive into tokens
        std::vector<std::string> tokens;
        std::string current;
        for (const char ch : directive) {
            if (std::isspace(static_cast<unsigned char>(ch))) {
                if (!current.empty()) {
                    tokens.push_back(current);
                    current.clear();
                }
                continue;
            }
            current += ch;
        }
        if (!current.empty()) {
            tokens.push_back(current);
        }

        // skip empty directives
        if (tokens.empty()) {
            continue;
        }

        // handle only .OPTIONS directives
        if (to_upper(tokens[0]) != ".OPTIONS" || tokens.size() <= 1) {
            continue;
        }

        // normalize the package name
        const std::string package = to_upper(tokens[1]);

        if (package == "DEVICE") {
            device = parse_option_tokens(std::vector<std::string>(tokens.begin() + 2, tokens.end()));
            continue;
        }
        if (package == "TIMEINT") {
            timeint = parse_option_tokens(std::vector<std::string>(tokens.begin() + 2, tokens.end()));
            continue;
        }
        if (package == "NONLIN") {
            nonlin = parse_option_tokens(std::vector<std::string>(tokens.begin() + 2, tokens.end()));
            continue;
        }
        if (package == "LINSOL") {
            linsol = parse_option_tokens(std::vector<std::string>(tokens.begin() + 2, tokens.end()));
            continue;
        }
        if (package == "FFT") {
            fft = parse_option_tokens(std::vector<std::string>(tokens.begin() + 2, tokens.end()));
            continue;
        }
        if (package == "DIAGNOSTIC") {
            diagnostic = parse_option_tokens(std::vector<std::string>(tokens.begin() + 2, tokens.end()));
            continue;
        }
    }

    return OptionParameters(device, timeint, nonlin, linsol, fft, diagnostic);
}

std::vector<std::string> OptionParameters::to_xyce_directives(const NetlistTopology& topology) const {
    (void)topology;
    // serialize configured option blocks in a deterministic order
    std::vector<std::string> directives;

    // build helper lambda to format options
    auto format_options = [](const std::map<std::string, std::string>& options) -> std::string {
        std::string result;
        for (const auto& [key, value] : options) {
            if (!result.empty()) {
                result += " ";
            }
            if (!value.empty()) {
                result += key + "=" + value;
            }
            else {
                result += key;
            }
        }
        return result;
    };

    if (!device.empty()) {
        directives.push_back(".OPTIONS DEVICE " + format_options(device));
    }
    if (!timeint.empty()) {
        directives.push_back(".OPTIONS TIMEINT " + format_options(timeint));
    }
    if (!nonlin.empty()) {
        directives.push_back(".OPTIONS NONLIN " + format_options(nonlin));
    }
    if (!linsol.empty()) {
        directives.push_back(".OPTIONS LINSOL " + format_options(linsol));
    }
    if (!fft.empty()) {
        directives.push_back(".OPTIONS FFT " + format_options(fft));
    }
    if (!diagnostic.empty()) {
        directives.push_back(".OPTIONS DIAGNOSTIC " + format_options(diagnostic));
    }

    return directives;
}

bool OptionParameters::operator==(const OptionParameters& other) const {
    // compare all fields for equality
    return device == other.device && timeint == other.timeint && nonlin == other.nonlin && linsol == other.linsol && fft == other.fft && diagnostic == other.diagnostic;
}
