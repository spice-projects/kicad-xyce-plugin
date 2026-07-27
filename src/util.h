#pragma once

#include <string>
#include <string_view>
#include <vector>

std::string to_upper(std::string_view);

std::string to_lower(std::string_view);

std::vector<std::string_view> tokenize(std::string_view text);

std::vector<std::string_view> split_by(std::string_view s, char delimiter);

std::string strip_chars(std::string_view view, std::string_view chars);
