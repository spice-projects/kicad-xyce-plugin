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
#include "../main_window_view_def.h"

class XyceSimulationRunner;

// business/orchestration logic for the slint main window, decoupled from the
// ui through MainWindowViewDef and MainWindowViewDefEvents; the presenter
// implements the event handler interface and receives user-interaction
// callbacks from the view without the view knowing the presenter exists
class SlintMainWindowPresenter2 : public MainWindowViewDefEvents
{
public:
    SlintMainWindowPresenter2(MainWindowViewDef& view, std::unique_ptr<NetlistSource> netlist_source, PluginConfig plugin_config);

    ~SlintMainWindowPresenter2() override;

    SlintMainWindowPresenter2(const SlintMainWindowPresenter2&) = delete;
    SlintMainWindowPresenter2& operator=(const SlintMainWindowPresenter2&) = delete;

    // file operations
    void on_open_xyce_file(const std::filesystem::path& path) override;
    void on_save_netlist() override;

    // view switching
    void on_show_netlist() override;
    void on_show_charts() override;
    void on_show_simulation_output() override;

    // simulation control
    void on_run_simulation() override;
    void on_configure_simulation() override;

    // plugin configuration
    void on_configure_plugin() override;

    // charts context menu
    void on_chart_zoom_to_fit() override;
    void on_chart_autorange() override;
    void on_chart_zoom_abscissa_extent() override;
    void on_chart_add_remove_plots() override;
    void on_chart_delete_all_plots() override;
    void on_chart_calculate_fft() override;
    void on_chart_open_xyce_fft_calculation() override;
    void on_chart_step_tool() override;
    void on_chart_add_chart() override;
    void on_chart_delete_chart() override;
    void on_chart_new_window() override;

    // simulation lifecycle events (forwarded by the view from the runner)
    void on_simulation_finished(int exit_code, bool was_canceled) override;
    void on_simulation_stdout(const std::string& line) override;
    void on_simulation_stderr(const std::string& line) override;

    // editor events
    void on_netlist_editor_modified() override;

    // kiCad schematic integration
    void on_extract_schematic_netlist() override;

    // accessors
    [[nodiscard]] std::shared_ptr<XyceSimulationRunner> simulation_runner() const;
    [[nodiscard]] const std::optional<std::shared_ptr<XyceOutputFile>>& raw_file() const;
    [[nodiscard]] const std::vector<std::shared_ptr<XyceOutputFile>>& fft_files() const;

private:
    bool update_xyce_raw_file(std::optional<std::shared_ptr<XyceOutputFile>> raw_file, bool delete_charts);

    void show_raw_file_view();

    void set_base_title(const std::string& title);

    bool set_netlist_editor_dirty(bool flag);

    bool update_netlist_editor_content(const std::string& content, bool dirty_flag);

    // recompute and forward the action enablement to the view
    void refresh_action_states();

    MainWindowViewDef& m_view;
    std::unique_ptr<NetlistSource> m_netlist_source;
    bool m_netlist_editor_dirty = false;
    bool m_netlist_has_content = false;

    std::optional<std::shared_ptr<XyceOutputFile>> m_xyce_raw_file;

    std::vector<std::shared_ptr<XyceOutputFile>> m_fft_files;

    SimulationConfig m_simulation_config;
    PluginConfig m_plugin_config;

    std::string m_base_title;
};
