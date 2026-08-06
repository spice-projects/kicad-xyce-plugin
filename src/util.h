#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

std::optional<std::string> get_environment_variable(std::string_view name);

std::string to_upper(std::string_view);

std::string to_lower(std::string_view);

std::vector<std::string_view> tokenize(std::string_view text);

std::vector<std::string_view> split_by(std::string_view s, char delimiter);

std::string strip_chars(std::string_view view, std::string_view chars);
