#include <cctype>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

#if !defined(_WIN32)
#include <unistd.h>
#endif

#include <spdlog/spdlog.h>

#include "../core/util.h"
#include "plugin_config.h"

namespace
{
    // escape a string value for JSON serialization
    std::string json_escape(std::string_view str) {
        // allocate output buffer
        std::string result;
        // reserve space to avoid frequent reallocations
        result.reserve(str.size() + 16);
        // escape special JSON characters
        for (char c : str) {
            if (c == '\\') {
                result += "\\\\";
            }
            else if (c == '"') {
                result += "\\\"";
            }
            else if (c == '\n') {
                result += "\\n";
            }
            else if (c == '\r') {
                result += "\\r";
            }
            else if (c == '\t') {
                result += "\\t";
            }
            else {
                result += c;
            }
        }
        return result;
    }

    // parse a JSON string value starting at an opening quote position
    std::optional<std::string> parse_json_string(std::string_view content, size_t quote_pos) {
        // verify the start character is an opening quote
        if (quote_pos >= content.size() || content[quote_pos] != '"')
            return std::nullopt;
        // output parsed string
        std::string result;
        // escape state tracker
        bool escaping = false;
        // iterate characters following the opening quote
        for (size_t i = quote_pos + 1; i < content.size(); ++i) {
            char c = content[i];
            if (escaping) {
                if (c == '"') {
                    result += '"';
                }
                else if (c == '\\') {
                    result += '\\';
                }
                else if (c == 'n') {
                    result += '\n';
                }
                else if (c == 'r') {
                    result += '\r';
                }
                else if (c == 't') {
                    result += '\t';
                }
                else {
                    result += c;
                }
                escaping = false;
            }
            else if (c == '\\') {
                escaping = true;
            }
            else if (c == '"') {
                // reached matching unescaped closing quote
                return result;
            }
            else {
                result += c;
            }
        }
        return std::nullopt;
    }

    // extract the Xyce executable path from serialized JSON content
    std::optional<std::string> extract_xyce_path(std::string_view content) {
        // search for "xyce_executable_path" key
        constexpr std::string_view key = "\"xyce_executable_path\"";
        size_t key_pos = content.find(key);
        if (key_pos == std::string_view::npos) {
            // also check legacy key name "XyceExecutablePath"
            constexpr std::string_view legacy_key = "\"XyceExecutablePath\"";
            key_pos = content.find(legacy_key);
            if (key_pos == std::string_view::npos)
                return std::nullopt;
            key_pos += legacy_key.size();
        }
        else {
            key_pos += key.size();
        }
        // find the colon after the key
        const size_t colon_pos = content.find(':', key_pos);
        if (colon_pos == std::string_view::npos)
            return std::nullopt;
        // find the opening quote of the value
        const size_t value_start = content.find('"', colon_pos + 1);
        if (value_start == std::string_view::npos)
            return std::nullopt;
        // parse the string value from JSON
        return parse_json_string(content, value_start);
    }

    // extract a boolean value from serialized JSON content
    std::optional<bool> extract_json_bool(std::string_view content, std::string_view key) {
        // search for the key in the content
        size_t key_pos = content.find(key);
        if (key_pos == std::string_view::npos)
            return std::nullopt;
        // skip past the key to find the value
        key_pos += key.size();
        // find the colon after the key
        const size_t colon_pos = content.find(':', key_pos);
        if (colon_pos == std::string_view::npos)
            return std::nullopt;
        // skip whitespace after colon
        size_t value_start = colon_pos + 1;
        while (value_start < content.size() && (content[value_start] == ' ' || content[value_start] == '\t'))
            ++value_start;
        // check if we reached the end of the content
        if (value_start >= content.size())
            return std::nullopt;
        // check for true or false
        if (content.substr(value_start, 4) == "true")
            return true;
        // check for false
        if (content.substr(value_start, 5) == "false")
            return false;
        // value is not a valid boolean
        return std::nullopt;
    }

    // extract an integer value from serialized JSON content
    std::optional<int> extract_json_int(std::string_view content, std::string_view key) {
        // search for the key in the content
        size_t key_pos = content.find(key);
        if (key_pos == std::string_view::npos)
            return std::nullopt;
        // skip past the key to find the value
        key_pos += key.size();
        // find the colon after the key
        const size_t colon_pos = content.find(':', key_pos);
        if (colon_pos == std::string_view::npos)
            return std::nullopt;
        // skip whitespace after colon
        size_t value_start = colon_pos + 1;
        while (value_start < content.size() && (content[value_start] == ' ' || content[value_start] == '\t'))
            ++value_start;
        // check if we reached the end of the content
        if (value_start >= content.size())
            return std::nullopt;
        // parse the integer
        int result = 0;
        bool negative = false;
        if (content[value_start] == '-') {
            negative = true;
            ++value_start;
        }
        // parse digits
        while (value_start < content.size() && std::isdigit(static_cast<unsigned char>(content[value_start]))) {
            result = result * 10 + (content[value_start] - '0');
            ++value_start;
        }
        // return the parsed integer
        return negative ? -result : result;
    }

    // resolve the cross-platform path to the plugin configuration file
    std::filesystem::path get_config_file_path() {
#if defined(_WIN32)
        // check APPDATA environment variable on Windows
        if (const auto appdata = get_environment_variable("APPDATA"); appdata.has_value() && !appdata->empty())
            return std::filesystem::path(*appdata) / "kicad-xyce-plugin" / "config.json";
        // check USERPROFILE environment variable as fallback on Windows
        if (const auto userprofile = get_environment_variable("USERPROFILE"); userprofile.has_value() && !userprofile->empty())
            return std::filesystem::path(*userprofile) / "AppData" / "Roaming" / "kicad-xyce-plugin" / "config.json";
#elif defined(__APPLE__)
        // check XDG_CONFIG_HOME first if explicitly set
        if (const auto xdg = get_environment_variable("XDG_CONFIG_HOME"); xdg.has_value() && !xdg->empty())
            return std::filesystem::path(*xdg) / "kicad-xyce-plugin" / "config.json";
        // use standard macOS Preferences directory
        if (const auto home = get_environment_variable("HOME"); home.has_value() && !home->empty())
            return std::filesystem::path(*home) / "Library" / "Preferences" / "kicad-xyce-plugin" / "config.json";
#else
        // check XDG_CONFIG_HOME on POSIX/Linux
        if (const auto xdg = get_environment_variable("XDG_CONFIG_HOME"); xdg.has_value() && !xdg->empty())
            return std::filesystem::path(*xdg) / "kicad-xyce-plugin" / "config.json";
        // fallback to ~/.config on POSIX/Linux
        if (const auto home = get_environment_variable("HOME"); home.has_value() && !home->empty())
            return std::filesystem::path(*home) / ".config" / "kicad-xyce-plugin" / "config.json";
#endif
        // fallback to current working directory
        return std::filesystem::path{"config.json"};
    }

    // iterate PATH entries looking for a file named "Xyce" (or "Xyce.exe" on Windows)
    std::string search_path_for_xyce() {
        // query the PATH environment variable
        const auto path_env = get_environment_variable("PATH");
        if (!path_env.has_value() || path_env->empty())
            return {};
#if defined(_WIN32)
        // Windows uses semicolon as path separator
        constexpr char separator = ';';
#else
        // POSIX uses colon as path separator
        constexpr char separator = ':';
#endif
        // split PATH entries by separator
        const auto directories = split_by(*path_env, separator);
        // iterate over each directory in PATH
        for (const auto dir_view : directories) {
            if (dir_view.empty())
                continue;
            const std::filesystem::path dir_path(dir_view);
            std::error_code ec;
#if defined(_WIN32)
            // check for Xyce.exe on Windows
            const auto candidate_exe = dir_path / "Xyce.exe";
            if (std::filesystem::is_regular_file(candidate_exe, ec))
                return candidate_exe.string();
            ec.clear();
            // check for Xyce on Windows
            const auto candidate = dir_path / "Xyce";
            if (std::filesystem::is_regular_file(candidate, ec))
                return candidate.string();
#else
            // check for Xyce on POSIX
            const auto candidate = dir_path / "Xyce";
            if (std::filesystem::is_regular_file(candidate, ec)) {
                // check if the file is executable
                if (::access(candidate.c_str(), X_OK) == 0)
                    return candidate.string();
            }
#endif
        }
        return {};
    }
} // namespace

PluginConfig::PluginConfig(std::string xyce_executable_path) :
    m_xyce_executable_path(std::move(xyce_executable_path)) {}

PluginConfig PluginConfig::load() {
    // resolve configuration file path
    const auto config_path = get_config_file_path();
    std::error_code ec;
    // read configuration from disk if the file exists
    if (std::filesystem::is_regular_file(config_path, ec)) {
        // open the config file for reading
        std::ifstream file(config_path);
        if (file.is_open()) {
            // read the entire file into a string buffer
            std::stringstream buffer;
            buffer << file.rdbuf();
            // get the string content of the buffer
            const auto content = buffer.str();
            // attempt to extract the Xyce executable path from the JSON content
            const auto stored_path = extract_xyce_path(content);
            // return the stored path if it is valid and non-empty
            if (stored_path.has_value() && !stored_path->empty()) {
                // log information
                spdlog::debug("Loaded Xyce executable path from config: {}", *stored_path);
                // create config with the stored path
                PluginConfig config(*stored_path);
                // extract simulation history settings
                if (const auto hist_enabled = extract_json_bool(content, "\"simulation_history_enabled\"")) {
                    // log information
                    spdlog::debug("Loaded simulation_history_enabled from config: {}", *hist_enabled);
                    // set the value in the config object
                    config.m_simulation_history_enabled = *hist_enabled;
                }
                if (const auto hist_max = extract_json_int(content, "\"simulation_history_max_runs\"")) {
                    // log information
                    spdlog::debug("Loaded simulation_history_max_runs from config: {}", *hist_max);
                    // set the value in the config object
                    config.m_simulation_history_max_runs = *hist_max;
                }
                // return the configured object
                return config;
            }
        }
    }
    // discover the Xyce executable path when not found in config
    auto discovered = discover_xyce_executable();
    if (!discovered.empty()) {
        // log information
        spdlog::debug("Discovered Xyce executable path: {}", discovered);
        // return the discovered path
        return PluginConfig(std::move(discovered));
    }
    // log information
    spdlog::info("No Xyce executable discovered");
    // exit
    return PluginConfig();
}

PluginConfig PluginConfig::default_config() { return PluginConfig(discover_xyce_executable()); }

void PluginConfig::save() const {
    // resolve configuration file path
    const auto config_path = get_config_file_path();
    std::error_code ec;
    // ensure the parent directory exists
    std::filesystem::create_directories(config_path.parent_path(), ec);
    if (ec) {
        // log warning
        spdlog::warn("Failed to create config directory: {}", ec.message());
        return;
    }
    // open the config file for writing
    std::ofstream file(config_path);
    if (!file.is_open()) {
        // log warning
        spdlog::warn("Failed to save Xyce executable path to config");
        return;
    }
    // serialize configuration as JSON
    file << "{\n";
    file << "    \"xyce_executable_path\": \"" << json_escape(m_xyce_executable_path) << "\",\n";
    file << "    \"simulation_history_enabled\": " << (m_simulation_history_enabled ? "true" : "false") << ",\n";
    file << "    \"simulation_history_max_runs\": " << m_simulation_history_max_runs << "\n";
    file << "}\n";
    // log information
    spdlog::debug("Saved plugin configuration to disk");
}

bool PluginConfig::is_xyce_executable_valid() const {
    // reject empty paths before filesystem checks
    if (m_xyce_executable_path.empty())
        return false;
    // check if the path exists and is a regular file
    std::error_code ec;
    const auto fs_status = std::filesystem::status(m_xyce_executable_path, ec);
    // reject paths that do not reference an existing regular file
    if (ec || !std::filesystem::is_regular_file(fs_status))
        return false;
#if defined(_WIN32)
    // on Windows, a regular file that exists is valid
    return true;
#else
    // require executable permission on POSIX
    return ::access(m_xyce_executable_path.c_str(), X_OK) == 0;
#endif
}

std::string PluginConfig::discover_xyce_executable() {
    // check PATH first (covers source builds, Spack, and custom installs)
    std::string found = search_path_for_xyce();
    if (!found.empty()) {
        // log information
        spdlog::debug("Found Xyce on PATH: {}", found);
        // exit
        return found;
    }
    // fall back to well-known binary-installer directories
#if defined(_WIN32)
    // check "C:\Program Files" for XyceNF_* directories (binary installer)
    const std::filesystem::path base = "C:\\Program Files";
    const std::string prefix = "XyceNF_";
    const std::string exe_rel = "bin\\Xyce.exe";
#else
    // check /usr/local for XyceNF_* directories (binary installer)
    const std::filesystem::path base = "/usr/local";
    const std::string prefix = "XyceNF_";
    const std::string exe_rel = "bin/Xyce";
#endif
    std::error_code ec;
    if (std::filesystem::exists(base, ec) && std::filesystem::is_directory(base, ec)) {
        std::string best;
        // iterate over subdirectories matching the prefix
        for (const auto& entry : std::filesystem::directory_iterator(base, ec)) {
            if (ec)
                break;
            if (entry.is_directory(ec)) {
                const std::string name = entry.path().filename().string();
                if (name.starts_with(prefix)) {
                    // keep the highest-versioned directory name
                    if (best.empty() || name > best)
                        best = name;
                }
            }
        }
        // check if a matching directory was found
        if (!best.empty()) {
            const auto candidate = base / best / exe_rel;
            ec.clear();
            if (std::filesystem::is_regular_file(candidate, ec)) {
#if !defined(_WIN32)
                if (::access(candidate.c_str(), X_OK) == 0) {
                    spdlog::debug("Found Xyce in install directory: {}", candidate.string());
                    return candidate.string();
                }
#else
                spdlog::debug("Found Xyce in install directory: {}", candidate.string());
                return candidate.string();
#endif
            }
        }
    }
    // log information
    spdlog::warn("Xyce executable not found via discovery");
    // exit with empty string
    return {};
}
