#pragma once

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <main_window.h>

#include <slint.h>

#include "../../config/plugin_config.h"
#include "../../netlist/netlist_source.h"
#include "../main_window_view_def.h"
#include "add_plot_dialog_view.h"
#include "charts_renderer.h"
#include "modal_manager.h"
#include "plugin_config_dialog_view.h"
#include "simulation_parameters_dialog_view.h"

class SlintMainWindowView2 : public MainWindowViewDef
{
public:
    SlintMainWindowView2(std::unique_ptr<NetlistSource> netlist_source, PluginConfig plugin_config);

    ~SlintMainWindowView2() override = default;

    SlintMainWindowView2(const SlintMainWindowView2&) = delete;
    SlintMainWindowView2& operator=(const SlintMainWindowView2&) = delete;

    void set_event_handler(MainWindowViewDefEvents& handler) override;

    // access the underlying slint window for event handling
    slint::Window& window();

    // show the main window
    void show();

    // main window view interface
    void set_title(const std::string& title) override;
    void set_status_text(const std::string& text) override;
    void apply_action_enablement(const ActionStateEnablement& enablement) override;
    void show_netlist_view() override;
    void show_charts_view() override;
    void set_netlist_editor_content(const std::string& content) override;
    std::string netlist_editor_content() const override;
    void set_netlist_editor_read_only(bool read_only) override;
    bool charts_shown() const override;
    void show_simulation_output_panel() override;
    void hide_simulation_output_panel() override;
    void clear_simulation_output() override;
    void append_simulation_output_line(const std::string& line) override;
    bool simulation_output_panel_hidden() const override;
    bool simulation_output_has_content() const override;
    void update_charts(ExpressionManager& expression_manager, const StepInformation& step_information, AbscissaScale abscissa_scale, const std::vector<std::vector<std::string>>& suggested_plots) override;
    void delete_all_charts() override;
    void set_open_fft_calculation_files(const std::vector<std::shared_ptr<XyceOutputFile>>& files) override;
    std::optional<SimulationConfig> show_simulation_parameters_dialog(const SimulationConfig& current) override;
    std::optional<PluginConfig> show_plugin_config_dialog(const PluginConfig& current) override;
    void start_simulation_process(const std::string& program, const std::filesystem::path& netlist_path, const std::filesystem::path& working_directory) override;
    void spawn_raw_file_window(std::shared_ptr<XyceOutputFile> raw_file) override;

private:
    // open file action (file dialog)
    void handle_open();

    // create the charts renderer the first time the charts panel is shown
    void ensure_charts_renderer();

    void show_add_remove_plots_dialog(float chart_position);

    // modal dialog lifecycle: dialogs are mutually exclusive (only the main
    // window opens them), so a single flag gates all of them
    bool begin_modal_dialog();

    void end_modal_dialog();

    // invoke the given callback only when no modal dialog is open; rejects the
    // interaction otherwise, as the caller cannot act behind a modal dialog
    template <typename Fn>
    void guard_modal(Fn&& fn) {
        // check another modal dialog is not open; if it is, reject the interaction
        if (m_modal_dialog_open)
            return;
        // invoke the callback, which may open a modal dialog and set the modal state
        std::forward<Fn>(fn)();
    }

    // track whether a modal dialog is currently open
    bool m_modal_dialog_open = false;

    // the window of the dialog currently open, used to restore the native
    // parent/dialog relationship when the modal state is released
    slint::Window* m_modal_dialog_window = nullptr;

    slint::ComponentHandle<main_window::MainWindow> m_window;
    MainWindowViewDefEvents* m_event_handler = nullptr;
    std::string m_netlist_content;
    bool m_netlist_read_only = false;

    // platform-neutral charts renderer
    std::unique_ptr<ChartsRenderer> m_charts_renderer;

    // add plot dialog, kept alive for the lifetime of the view
    std::unique_ptr<add_plot_dialog_view::AddPlotDialogView> m_add_plot_dialog;

    // plugin config dialog, kept alive for the lifetime of the view
    std::unique_ptr<plugin_config_dialog_view::PluginConfigDialogView> m_plugin_config_dialog;

    // simulation parameters dialog, kept alive for the lifetime of the view
    std::unique_ptr<simulation_parameters_dialog_view::SimulationParametersDialogView> m_simulation_parameters_dialog;
};
