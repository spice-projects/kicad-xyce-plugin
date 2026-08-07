#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "../config/plugin_config.h"
#include "../file/xyce_output_file.h"
#include "../netlist/netlist_source.h"
#include "../simulation_parameters/simulation_config.h"
#include "main_window_state.h"
#include "main_window_view.h"

class XyceSimulationRunner;

// business/orchestration logic for the main window, decoupled from wx through MainWindowView
class MainWindowPresenter
{
public:
    MainWindowPresenter(MainWindowView& view, std::unique_ptr<NetlistSource> netlist_source, PluginConfig plugin_config);

    ~MainWindowPresenter();

    // file operations
    void open_netlist_file(const std::filesystem::path& path);
    void open_raw_file(const std::filesystem::path& path);
    void load_raw_file(std::shared_ptr<XyceOutputFile> raw_file);
    void save_netlist();

    // configuration
    void configure_simulation();
    void configure_plugin();

    // simulation
    void run_simulation();
    void handle_simulation_finished(int exit_code, bool was_canceled);
    void handle_simulation_stdout(const std::string& line);
    void handle_simulation_stderr(const std::string& line);

    // editor
    void handle_netlist_editor_modified();

    // kiCad plugin mode
    void extract_schematic_netlist();

    // accessors
    [[nodiscard]] std::shared_ptr<XyceSimulationRunner> simulation_runner() const;
    [[nodiscard]] const std::optional<std::shared_ptr<XyceOutputFile>>& raw_file() const;
    [[nodiscard]] const std::vector<std::shared_ptr<XyceOutputFile>>& fft_files() const;

    // recompute and forward the action enablement to the view
    void refresh_action_states();

private:
    bool update_xyce_raw_file(std::optional<std::shared_ptr<XyceOutputFile>> raw_file, bool delete_charts);

    void show_raw_file_view();

    void set_base_title(const std::string& title);

    bool set_netlist_editor_dirty(bool flag);

    bool update_netlist_editor_content(const std::string& content, bool dirty_flag);

    MainWindowView& m_view;
    std::unique_ptr<NetlistSource> m_netlist_source;
    bool m_netlist_editor_dirty = false;
    AppState m_app_state = AppState::Empty;
    bool m_simulation_running = false;
    bool m_netlist_has_content = false;

    std::optional<std::shared_ptr<XyceOutputFile>> m_xyce_raw_file;

    std::vector<std::shared_ptr<XyceOutputFile>> m_fft_files;

    SimulationConfig m_simulation_config;
    PluginConfig m_plugin_config;

    std::shared_ptr<XyceSimulationRunner> m_simulation_runner;

    std::string m_base_title;
};