#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "../../config/plugin_config.h"
#include "../../file/xyce_output_file.h"
#include "../../netlist/netlist_source.h"
#include "../../simulation_parameters/simulation_config.h"
#include "../main_window_state.h"
#include "../main_window_view.h"

class XyceSimulationRunner;

// business/orchestration logic for the slint main window, decoupled from the
// ui through MainWindowView; mirrors the wxWidgets MainWindowPresenter so the
// two implementations can be kept side by side during the migration
class SlintMainWindowPresenter
{
public:
    SlintMainWindowPresenter(MainWindowView& view, std::unique_ptr<NetlistSource> netlist_source, PluginConfig plugin_config);

    ~SlintMainWindowPresenter();

    SlintMainWindowPresenter(const SlintMainWindowPresenter&) = delete;
    SlintMainWindowPresenter& operator=(const SlintMainWindowPresenter&) = delete;

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

    // charts context menu actions, decoupled from the chart implementation
    void chart_zoom_to_fit();
    void chart_autorange();
    void chart_zoom_abscissa_extent();
    void chart_add_remove_plots();
    void chart_delete_all_plots();
    void chart_calculate_fft();
    void chart_open_xyce_fft_calculation();
    void chart_step_tool();
    void chart_add_chart();
    void chart_delete_chart();
    void chart_new_window();

    // kicad plugin mode
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
    bool m_netlist_has_content = false;

    std::optional<std::shared_ptr<XyceOutputFile>> m_xyce_raw_file;

    std::vector<std::shared_ptr<XyceOutputFile>> m_fft_files;

    SimulationConfig m_simulation_config;
    PluginConfig m_plugin_config;

    std::string m_base_title;
};
