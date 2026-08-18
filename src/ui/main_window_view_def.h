#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "../config/plugin_config.h"
#include "../expression/expression_manager.h"
#include "../file/xyce_output_file.h"
#include "../simulation_parameters/simulation_config.h"
#include "../step_information.h"
#include "main_window_state.h"

// abstract view backed by the main wx window, so the presenter can be tested without wx
class MainWindowView
{
public:
    virtual ~MainWindowView() = default;

    // window chrome
    virtual void set_title(const std::string& title) = 0;
    virtual void set_status_text(const std::string& text) = 0;
    virtual void apply_action_enablement(const ActionStateEnablement& enablement) = 0;

    // content views (netlist editor vs charts, mutually exclusive)
    virtual void show_netlist_view() = 0;
    virtual void show_charts_view() = 0;
    virtual void set_netlist_editor_content(const std::string& content) = 0;
    [[nodiscard]] virtual std::string netlist_editor_content() const = 0;
    virtual void set_netlist_editor_read_only(bool read_only) = 0;
    [[nodiscard]] virtual bool charts_shown() const = 0;

    // simulation output panel / log
    virtual void show_simulation_output_panel() = 0;
    virtual void hide_simulation_output_panel() = 0;
    virtual void clear_simulation_output() = 0;
    virtual void append_simulation_output_line(const std::string& line) = 0;
    [[nodiscard]] virtual bool simulation_output_panel_hidden() const = 0;
    [[nodiscard]] virtual bool simulation_output_has_content() const = 0;

    // charts
    virtual void update_charts(ExpressionManager& expression_manager, const StepInformation& step_information, AbscissaScale abscissa_scale, const std::vector<std::vector<std::string>>& suggested_plots) = 0;
    virtual void delete_all_charts() = 0;

    // parsed Xyce FFT calculation output files, forwarded to the charts context menu
    virtual void set_open_fft_calculation_files(const std::vector<std::shared_ptr<XyceOutputFile>>& files) = 0;

    // modal dialogs (still the view's job, they need a wx parent window)
    [[nodiscard]] virtual std::optional<SimulationConfig> show_simulation_parameters_dialog(const SimulationConfig& current) = 0;
    [[nodiscard]] virtual std::optional<PluginConfig> show_plugin_config_dialog(const PluginConfig& current) = 0;

    // simulation process lifecycle (presenter decides when, the view wires the wx process events)
    virtual void start_simulation_process(const std::string& program, const std::filesystem::path& netlist_path, const std::filesystem::path& working_directory) = 0;

    // window management
    virtual void spawn_raw_file_window(std::shared_ptr<XyceOutputFile> raw_file) = 0;
};

// event handler interface: the view calls back into these methods when the user
// interacts with the UI, without knowing who handles them (the presenter
// implements this interface). this is the other half of the MVP contract.
class MainWindowViewDefEvents
{
public:
    virtual ~MainWindowViewDefEvents() = default;

    // file operations
    virtual void on_open_xyce_file(const std::filesystem::path& path) = 0;
    virtual void on_save_netlist() = 0;

    // view switching
    virtual void on_show_netlist() = 0;
    virtual void on_show_charts() = 0;
    virtual void on_show_simulation_output() = 0;

    // simulation control
    virtual void on_run_simulation() = 0;
    virtual void on_configure_simulation() = 0;

    // plugin configuration
    virtual void on_configure_plugin() = 0;

    // accepted plugin configuration delivered by the view after the config
    // dialog closes; the slint dialog is non-modal, so the view reports the
    // result asynchronously instead of through show_plugin_config_dialog()
    virtual void on_plugin_config_dialog_result(const PluginConfig& config) = 0;

    // accepted simulation configuration delivered by the view after the
    // simulation parameters dialog closes; the slint dialog is non-modal, so
    // the view reports the result asynchronously instead of through
    // show_simulation_parameters_dialog()
    virtual void on_simulation_parameters_dialog_result(const SimulationConfig& config) = 0;

    // charts context menu
    virtual void on_chart_calculate_fft(size_t chart_index) = 0;
    virtual void on_chart_open_xyce_fft_calculation(size_t chart_index) = 0;
    virtual void on_chart_step_tool(size_t chart_index) = 0;
    virtual void on_chart_new_window(size_t chart_index) = 0;

    // simulation lifecycle events (forwarded by the view from the runner)
    virtual void on_simulation_finished(int exit_code, bool was_canceled) = 0;
    virtual void on_simulation_stdout(const std::string& line) = 0;
    virtual void on_simulation_stderr(const std::string& line) = 0;

    // editor events
    virtual void on_netlist_editor_modified() = 0;

    // kiCad schematic integration
    virtual void on_extract_schematic_netlist() = 0;
};

// refactored view interface that adds event handler wiring on top of
// MainWindowView. the view no longer owns or knows about the presenter;
// it simply forwards user interactions through the event handler interface.
// the parent creates both the view and the presenter and wires them together.
class MainWindowViewDef : public MainWindowView
{
public:
    // set the event handler that receives user-interaction callbacks
    virtual void set_event_handler(MainWindowViewDefEvents& handler) = 0;
};
