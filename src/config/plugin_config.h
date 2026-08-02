#pragma once

#include <string>

class PluginConfig
{
public:
    PluginConfig() = default;

    explicit PluginConfig(std::string xyce_executable_path);

    [[nodiscard]] static PluginConfig load();

    [[nodiscard]] static PluginConfig default_config();

    void save() const;

    [[nodiscard]] bool is_xyce_executable_valid() const;

    [[nodiscard]] const std::string& xyce_executable_path() const { return m_xyce_executable_path; }

    void set_xyce_executable_path(std::string path) { m_xyce_executable_path = std::move(path); }

    [[nodiscard]] static std::string discover_xyce_executable();

private:
    std::string m_xyce_executable_path;
};
