#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <string>

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
    virtual void update_charts(ExpressionManager& expression_manager, const StepInformation& step_information, const std::string& abscissa_label, AbscissaScale abscissa_scale) = 0;
    virtual void delete_all_charts() = 0;

    // modal dialogs (still the view's job, they need a wx parent window)
    [[nodiscard]] virtual std::optional<SimulationConfig> show_simulation_parameters_dialog(const SimulationConfig& current) = 0;
    [[nodiscard]] virtual std::optional<PluginConfig> show_plugin_config_dialog(const PluginConfig& current) = 0;

    // simulation process lifecycle (presenter decides when, the view wires the wx process events)
    virtual void start_simulation_process(const std::string& program, const std::filesystem::path& netlist_path, const std::filesystem::path& working_directory) = 0;

    // window management
    virtual void spawn_raw_file_window(std::shared_ptr<XyceOutputFile> raw_file) = 0;
};
