#include <algorithm>
#include <cctype>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "core/util.h"
#include "netlist_lexer.h"

namespace
{

    // Y-prefix device types supported by Xyce with multi-letter codes
    const std::vector<std::string_view> Y_PREFIXES = {"YMEMRISTOR", "YPDE", "YACC", "YLIN"};

    // standard node counts for single-letter device types
    const std::map<char, int, std::less<>> NODE_COUNTS = {
        {'B', 2}, {'C', 2}, {'D', 2}, {'E', 4}, {'F', 2}, {'G', 4}, {'H', 2}, {'I', 2}, {'J', 3}, {'K', 0}, {'L', 2}, {'M', 4}, {'O', 4}, {'P', 2}, {'Q', 3}, {'R', 2}, {'S', 4}, {'T', 4}, {'U', 2}, {'V', 2}, {'W', 2}, {'Z', 3},
    };

    // recognized simulation and modeling keywords in Xyce and SPICE
    const std::set<std::string, std::less<>> KEYWORDS = {
        "AC", "AM", "BLACKMAN", "CSV", "DATA", "DC", "DEC", "DEVICE", "DISTOF1", "DISTOF2",
        "END", "ENDDATA", "ENDS", "EXP", "FALSE", "FFT", "FILE", "FORMAT", "FOUR",
        "GLOBAL", "HAMMING", "HANN", "HB", "HBINT", "IC", "IDB", "II", "IM", "INC",
        "INCLUDE", "IP", "IR", "LIB", "LIN", "LINSOL", "LINSOL-HB", "MEAS", "MEASURE",
        "MODEL", "NOISE", "NODESET", "NONLIN", "NONLIN-HB", "NP", "OCT", "OFF", "ON",
        "OP", "OPTIONS", "P", "PARAM", "PARAMS", "PARAMS:", "PDB", "PLOT", "PREPROCESS",
        "PRINT", "PROBE", "PULSE", "PWL", "RAW", "RECTANGULAR", "REPLACEGROUND", "SAVE",
        "SENS", "SFFM", "SIN", "STD", "STEP", "SUBCKT", "TEMP", "TIMEINT", "TITLE",
        "TRAN", "TRUE", "UNORM", "V", "VDB", "VI", "VM", "VP", "VR", "WINDOW",
    };

    // split raw text into line string views preserving newlines
    std::vector<std::string_view> split_lines(std::string_view text) {
        // empty input yields a single empty line
        if (text.empty())
            return {""};
        // container for extracted lines
        std::vector<std::string_view> lines;
        // start index for line search
        size_t start = 0;
        // iterate through characters searching for newline delimiters
        while (start < text.size()) {
            // find next carriage return or line feed
            auto pos = text.find_first_of("\r\n", start);
            // handle the final line when no more newlines exist
            if (pos == std::string_view::npos) {
                // append the remaining text slice
                lines.emplace_back(text.substr(start));
                // exit the loop
                break;
            }
            // append line content up to the newline delimiter
            lines.emplace_back(text.substr(start, pos - start));
            // advance past CRLF sequence or single newline character
            if (text[pos] == '\r' && pos + 1 < text.size() && text[pos + 1] == '\n')
                start = pos + 2;
            else
                start = pos + 1;
        }
        // trailing newline produces a final empty line
        if (!text.empty() && (text.back() == '\n' || text.back() == '\r'))
            lines.emplace_back("");
        // return the complete set of line views
        return lines;
    }

    // test whether a string view represents a SPICE numeric literal with optional unit
    bool is_spice_number(std::string_view s) {
        // empty string cannot be a number
        if (s.empty())
            return false;
        // current scanning index
        size_t idx = 0;
        // consume optional leading sign
        if (s[idx] == '+' || s[idx] == '-')
            idx++;
        // sign alone is not a valid number
        if (idx >= s.size())
            return false;
        // track whether at least one digit was encountered
        bool has_digits = false;
        // consume integer digits
        while (idx < s.size() && std::isdigit(static_cast<unsigned char>(s[idx]))) {
            // mark that a digit was found
            has_digits = true;
            // advance to next character
            idx++;
        }
        // consume optional fractional part
        if (idx < s.size() && s[idx] == '.') {
            // advance past the decimal point
            idx++;
            // consume fractional digits
            while (idx < s.size() && std::isdigit(static_cast<unsigned char>(s[idx]))) {
                // mark that a digit was found
                has_digits = true;
                // advance to next character
                idx++;
            }
        }
        // number must have at least one digit
        if (!has_digits)
            return false;
        // consume optional scientific exponent
        if (idx < s.size() && (s[idx] == 'e' || s[idx] == 'E')) {
            // tentative exponent index
            size_t e_idx = idx + 1;
            // consume optional exponent sign
            if (e_idx < s.size() && (s[e_idx] == '+' || s[e_idx] == '-'))
                e_idx++;
            // verify that digits follow the exponent marker
            if (e_idx < s.size() && std::isdigit(static_cast<unsigned char>(s[e_idx]))) {
                // commit exponent start
                idx = e_idx;
                // consume exponent digits
                while (idx < s.size() && std::isdigit(static_cast<unsigned char>(s[idx])))
                    idx++;
            }
        }
        // any remaining characters represent scale factor and units and must not contain digits
        for (size_t k = idx; k < s.size(); ++k) {
            // reject if alphanumeric part contains embedded digits
            if (std::isdigit(static_cast<unsigned char>(s[k])))
                return false;
        }
        // string is a valid SPICE number
        return true;
    }

    // determine the expected node count for a device instance name
    int get_device_node_count(std::string_view name) {
        // empty name has no nodes
        if (name.empty())
            return 0;
        // convert name to uppercase for matching
        const std::string upper = to_upper(name);
        // check Y-prefix devices first
        for (const auto& prefix : Y_PREFIXES) {
            // compare against prefix length
            if (upper.size() >= prefix.size() && std::equal(prefix.begin(), prefix.end(), upper.begin())) {
                // YACC has 3 nodes, YPDE has 0, YMEMRISTOR and YLIN have 2
                if (prefix == "YACC")
                    return 3;
                if (prefix == "YPDE")
                    return 0;
                return 2;
            }
        }
        // check standard single letter device types
        const char first_char = upper[0];
        // subcircuit calls have variable node count
        if (first_char == 'X')
            return -1;
        // find device type in table
        const auto it = NODE_COUNTS.find(first_char);
        // return table count or default 2
        if (it != NODE_COUNTS.end())
            return it->second;
        return 2;
    }

    // test whether an identifier matches a valid SPICE device prefix
    bool is_device_name(std::string_view s) {
        // empty string cannot be a device
        if (s.empty())
            return false;
        // reject if starts with a digit or dot
        if (std::isdigit(static_cast<unsigned char>(s[0])) || s[0] == '.')
            return false;
        // convert to uppercase
        const std::string upper = to_upper(s);
        // check multi-letter Y prefixes
        for (const auto& prefix : Y_PREFIXES) {
            // match prefix
            if (upper.size() >= prefix.size() && std::equal(prefix.begin(), prefix.end(), upper.begin()))
                return true;
        }
        // check single letter prefix
        const char first = upper[0];
        // check subcircuit instance prefix
        if (first == 'X')
            return true;
        // check known single-letter device types
        return NODE_COUNTS.contains(first);
    }

    // test whether a string is a recognized keyword
    bool is_keyword(std::string_view s) {
        // convert to uppercase for lookup
        const std::string upper = to_upper(s);
        // query keyword set
        return KEYWORDS.contains(upper);
    }

    // tokenize a single line preserving all characters
    NetlistTokenLine tokenize_single_line(std::string_view line) {
        // container for line tokens
        NetlistTokenLine result;
        // empty line produces empty token list
        if (line.empty())
            return result;
        // total line length
        const size_t n = line.size();
        // find first non-whitespace character position
        const auto first_non_ws = line.find_first_not_of(" \t");
        // line containing only whitespace produces single whitespace token
        if (first_non_ws == std::string_view::npos) {
            // emit entire whitespace line
            result.m_tokens.push_back({std::string(line), NetlistTokenType::WHITESPACE});
            return result;
        }
        // full comment line begins with '*', ';', or '$'
        const char first_char = line[first_non_ws];
        if (first_char == '*' || first_char == ';' || first_char == '$') {
            // emit leading whitespace if present
            if (first_non_ws > 0)
                result.m_tokens.push_back({std::string(line.substr(0, first_non_ws)), NetlistTokenType::WHITESPACE});
            // emit the remainder of the line as a comment
            result.m_tokens.push_back({std::string(line.substr(first_non_ws)), NetlistTokenType::COMMENT});
            return result;
        }
        // determine line classification flags
        const bool is_continuation = (first_char == '+');
        const bool is_directive_line = (first_char == '.');
        // tracking state for device instance lines
        bool has_emitted_device = is_continuation || is_directive_line;
        int expected_nodes = 0;
        int nodes_consumed = 0;
        // current scan index
        size_t i = 0;
        // iterate across line characters
        while (i < n) {
            // scan consecutive whitespace characters
            if (line[i] == ' ' || line[i] == '\t') {
                // start of whitespace segment
                const size_t start = i;
                // consume whitespace characters
                while (i < n && (line[i] == ' ' || line[i] == '\t'))
                    i++;
                // emit whitespace token
                result.m_tokens.push_back({std::string(line.substr(start, i - start)), NetlistTokenType::WHITESPACE});
                continue;
            }
            // inline comment begins with ';' or '$'
            if (line[i] == ';' || line[i] == '$') {
                // emit the rest of the line as an inline comment
                result.m_tokens.push_back({std::string(line.substr(i)), NetlistTokenType::COMMENT});
                break;
            }
            // mathematical expression enclosed in curly braces
            if (line[i] == '{') {
                // start of expression
                const size_t start = i;
                // find matching closing brace
                const auto closing = line.find('}', start + 1);
                // compute end position
                const size_t end = (closing != std::string_view::npos) ? closing + 1 : n;
                // emit expression token
                result.m_tokens.push_back({std::string(line.substr(start, end - start)), NetlistTokenType::EXPRESSION});
                // advance index past expression
                i = end;
                continue;
            }
            // quoted string literal
            if (line[i] == '"' || line[i] == '\'') {
                // quote delimiter
                const char quote = line[i];
                // start of string
                const size_t start = i;
                // find matching closing quote
                const auto closing = line.find(quote, start + 1);
                // compute end position
                const size_t end = (closing != std::string_view::npos) ? closing + 1 : n;
                // emit string literal token
                result.m_tokens.push_back({std::string(line.substr(start, end - start)), NetlistTokenType::STRING_LITERAL});
                // advance index past string
                i = end;
                continue;
            }
            // continuation character at start of continuation line
            if (is_continuation && i == first_non_ws && line[i] == '+') {
                // emit continuation token
                result.m_tokens.push_back({std::string(line.substr(i, 1)), NetlistTokenType::CONTINUATION});
                // advance past '+'
                i++;
                continue;
            }
            // simulation directive starting with '.'
            if (line[i] == '.' && (i + 1 < n && !std::isdigit(static_cast<unsigned char>(line[i + 1])))) {
                // start of directive
                const size_t start = i;
                // advance past leading dot
                i++;
                // consume directive name characters
                while (i < n && !std::isspace(static_cast<unsigned char>(line[i])) && line[i] != '=' && line[i] != '(' && line[i] != ')' && line[i] != ';' && line[i] != '$')
                    i++;
                // emit directive token
                result.m_tokens.push_back({std::string(line.substr(start, i - start)), NetlistTokenType::DIRECTIVE});
                continue;
            }
            // single character operators and delimiters
            if (line[i] == '=' || line[i] == '(' || line[i] == ')' || line[i] == ',' || line[i] == '^') {
                // emit operator token
                result.m_tokens.push_back({std::string(line.substr(i, 1)), NetlistTokenType::OPERATOR});
                // advance index
                i++;
                continue;
            }
            // colon operator (unless part of PARAMS: keyword)
            if (line[i] == ':') {
                // emit operator token
                result.m_tokens.push_back({std::string(line.substr(i, 1)), NetlistTokenType::OPERATOR});
                // advance index
                i++;
                continue;
            }
            // arithmetic operators '+', '-', '*', '/' when not leading numbers
            if ((line[i] == '+' || line[i] == '-') && (i + 1 >= n || (!std::isdigit(static_cast<unsigned char>(line[i + 1])) && line[i + 1] != '.'))) {
                // emit operator token
                result.m_tokens.push_back({std::string(line.substr(i, 1)), NetlistTokenType::OPERATOR});
                // advance index
                i++;
                continue;
            }
            if (line[i] == '*' || line[i] == '/') {
                // emit operator token
                result.m_tokens.push_back({std::string(line.substr(i, 1)), NetlistTokenType::OPERATOR});
                // advance index
                i++;
                continue;
            }
            // word token scanning
            const size_t start = i;
            // consume characters forming a word or token
            while (i < n && !std::isspace(static_cast<unsigned char>(line[i])) && line[i] != '=' && line[i] != '(' && line[i] != ')' && line[i] != ',' && line[i] != '^' && line[i] != ';' && line[i] != '$' && line[i] != '{' && line[i] != '"' && line[i] != '\'') {
                // stop at standalone operators if not start of number or scientific exponent
                if ((line[i] == '+' || line[i] == '-') && i > start) {
                    const char prev = line[i - 1];
                    if (prev != 'e' && prev != 'E')
                        break;
                }
                i++;
            }
            // extracted word view
            const std::string_view word = line.substr(start, i - start);
            // classify word token
            if (!has_emitted_device && is_device_name(word)) {
                // mark that device was emitted
                has_emitted_device = true;
                // record expected node count
                expected_nodes = get_device_node_count(word);
                // emit device token
                result.m_tokens.push_back({std::string(word), NetlistTokenType::DEVICE});
            }
            else if (is_keyword(word)) {
                // emit keyword token
                result.m_tokens.push_back({std::string(word), NetlistTokenType::KEYWORD});
            }
            else if (has_emitted_device && !is_directive_line && (expected_nodes < 0 || nodes_consumed < expected_nodes)) {
                // increment consumed nodes counter
                nodes_consumed++;
                // emit node token
                result.m_tokens.push_back({std::string(word), NetlistTokenType::NODE});
            }
            else if (to_upper(word) == "GND" || to_upper(word) == "GROUND") {
                // emit ground node token
                result.m_tokens.push_back({std::string(word), NetlistTokenType::NODE});
            }
            else if (is_spice_number(word)) {
                // emit numeric literal token
                result.m_tokens.push_back({std::string(word), NetlistTokenType::NUMBER});
            }
            else {
                // emit default plain text token
                result.m_tokens.push_back({std::string(word), NetlistTokenType::PLAIN_TEXT});
            }
        }
        // return completed tokenized line
        return result;
    }

} // namespace

std::vector<NetlistTokenLine> tokenize_netlist(std::string_view netlist) {
    // split input text into individual lines
    const auto raw_lines = split_lines(netlist);
    // allocate result line vector
    std::vector<NetlistTokenLine> tokenized_lines;
    tokenized_lines.reserve(raw_lines.size());
    // tokenize each line independently
    for (const auto& line : raw_lines) {
        // tokenize single line and add to collection
        tokenized_lines.emplace_back(tokenize_single_line(line));
    }
    // return all tokenized lines
    return tokenized_lines;
}
