#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <main_window.h>

#include <slint.h>

#include "../../config/plugin_config.h"
#include "../../netlist/netlist_source.h"
#include "../main_window_view.h"
#include "main_window_presenter.h"

#ifdef __APPLE__
#include "charts_renderer.h"
#endif

// slint view adapter implementing the MainWindowView interface over the slint
// MainWindow component; owns the window and the presenter and forwards the
// global toolbar actions into the presenter methods
class SlintMainWindowView : public MainWindowView
{
public:
    SlintMainWindowView(std::unique_ptr<NetlistSource> netlist_source, PluginConfig plugin_config);

    ~SlintMainWindowView() override = default;

    SlintMainWindowView(const SlintMainWindowView&) = delete;
    SlintMainWindowView& operator=(const SlintMainWindowView&) = delete;

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
    // open file action (file dialog wiring pending)
    void handle_open();

    // create the charts renderer the first time the charts panel is shown
    void ensure_charts_renderer();

    slint::ComponentHandle<MainWindow> m_window;
    std::unique_ptr<SlintMainWindowPresenter> m_presenter;
    std::string m_netlist_content;
    bool m_netlist_read_only = false;

#ifdef __APPLE__
    std::unique_ptr<ChartsRenderer> m_charts_renderer;
#endif
};
