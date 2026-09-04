#pragma once

#include <string>

// configuration for the KiCad Xyce plugin, managing the Xyce executable path
class PluginConfig
{
public:
    // construct default configuration
    PluginConfig() = default;

    // construct configuration with the given Xyce executable path
    explicit PluginConfig(std::string xyce_executable_path);

    // load configuration from disk, falling back to discovery
    [[nodiscard]] static PluginConfig load();

    // construct default configuration using discovered executable
    [[nodiscard]] static PluginConfig default_config();

    // persist configuration to disk
    void save() const;

    // check if the configured path points to a valid executable file
    [[nodiscard]] bool is_xyce_executable_valid() const;

    // get the configured Xyce executable path
    [[nodiscard]] const std::string& xyce_executable_path() const { return m_xyce_executable_path; }

    // set the Xyce executable path
    void set_xyce_executable_path(std::string path) { m_xyce_executable_path = std::move(path); }

    // get simulation history enabled flag
    [[nodiscard]] bool simulation_history_enabled() const { return m_simulation_history_enabled; }

    // set simulation history enabled flag
    void set_simulation_history_enabled(bool enabled) { m_simulation_history_enabled = enabled; }

    // get maximum number of simulation runs to keep
    [[nodiscard]] int simulation_history_max_runs() const { return m_simulation_history_max_runs; }

    // set maximum number of simulation runs to keep
    void set_simulation_history_max_runs(int max_runs) { m_simulation_history_max_runs = max_runs; }

    // search standard directories and PATH for a Xyce executable
    [[nodiscard]] static std::string discover_xyce_executable();

private:
    std::string m_xyce_executable_path;
    bool m_simulation_history_enabled{false};
    int m_simulation_history_max_runs{20};
};
