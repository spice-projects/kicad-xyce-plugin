#pragma once

#include <string>

class PluginConfig
{
public:
    PluginConfig() = default;

    explicit PluginConfig(std::string xyce_executable_path);

    // load plugin configuration from persistent storage (wxConfig)
    [[nodiscard]] static PluginConfig load();

    // create a default config by running discovery
    [[nodiscard]] static PluginConfig default_config();

    // persist the current configuration to wxConfig
    void save() const;

    // check whether the stored executable path is non-empty, exists, and is executable
    [[nodiscard]] bool is_xyce_executable_valid() const;

    // accessor for the stored executable path
    [[nodiscard]] const std::string& xyce_executable_path() const { return m_xyce_executable_path; }

    // set the executable path
    void set_xyce_executable_path(std::string path) { m_xyce_executable_path = std::move(path); }

    // search PATH and well-known install directories for a Xyce executable
    [[nodiscard]] static std::string discover_xyce_executable();

private:
    std::string m_xyce_executable_path;
};