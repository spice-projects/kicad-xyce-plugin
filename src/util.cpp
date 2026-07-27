#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
#include <vector>

#include "util.h"

std::string to_upper(std::string_view view) {
    // result
    std::string result{view};
    // convert each character to upper case
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) { return std::toupper(c); });
    // return converted string
    return result;
}

std::string to_lower(std::string_view view) {
    // result
    std::string result{view};
    // convert each character to lower case
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) { return std::tolower(c); });
    // return converted string
    return result;
}

std::vector<std::string_view> tokenize(std::string_view view) {
    // output
    std::vector<std::string_view> tokens;
    // reserve space for a few tokens to avoid reallocations
    tokens.reserve(8);
    // pointers to the current position and end of the string
    const char* p = view.data();
    const char* end = p + view.size();
    // iterate through the string
    while (p != end) {
        // skip leading whitespace
        while (p != end && std::isspace(static_cast<unsigned char>(*p)))
            ++p;
        // mark the start of the token
        const char* first = p;
        // find the end of the token
        while (p != end && !std::isspace(static_cast<unsigned char>(*p)))
            ++p;
        // add the token to the list if it's not empty
        if (first != p)
            tokens.emplace_back(first, static_cast<size_t>(p - first));
    }
    return tokens;
}

std::vector<std::string_view> split_by(std::string_view view, char delimiter) {
    // init result list
    std::vector<std::string_view> parts;
    // begin and end pointers for the string view
    const char* begin = view.data();
    const char* end = begin + view.size();
    // iterate through the string view and split by the delimiter
    while (begin < end) {
        // skip delimeter
        while (begin < end && *begin == delimiter)
            ++begin;
        // exit if we reached the end
        if (begin == end)
            break;
        // current position pointer
        const char* pos = begin;
        // find the next occurrence of the delimiter
        while (pos != end && *pos != delimiter)
            ++pos;
        // add non-empty token
        parts.emplace_back(begin, static_cast<std::size_t>(pos - begin));
        // move the begin pointer
        begin = pos;
    }
    return parts;
}

std::string strip_chars(std::string_view view, std::string_view chars) {
    // result
    std::string result;
    // reserve space for the result to avoid reallocations
    result.reserve(view.size());
    // loop characters in view
    for (char ch : view) {
        // append char if not in chars
        if (chars.find(ch) == std::string_view::npos)
            result += ch;
    }
    return result;
}
