#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <slint.h>
#include <spdlog/spdlog.h>

#include "../file_dialog.h"
#include "main_window_presenter.h"
#include "main_window_view.h"

SlintMainWindowView::SlintMainWindowView(std::unique_ptr<NetlistSource> netlist_source, PluginConfig plugin_config) :
    m_window(MainWindow::create()), m_presenter(std::make_unique<SlintMainWindowPresenter>(*this, std::move(netlist_source), std::move(plugin_config))) {
    // bind the toolbar actions to the presenter methods
    const auto& actions = m_window->global<MainWindowActions>();
    // file group
    actions.on_open_xyce_file([this] { handle_open(); });
    actions.on_save_netlist([this] { m_presenter->save_netlist(); });
    // content view group
    actions.on_show_netlist([this] {
        // switch to the netlist view and refresh the toolbar state
        show_netlist_view();
        // refresh state
        m_presenter->refresh_action_states();
    });
    actions.on_show_charts([this] {
        // switch to the charts view and refresh the toolbar state
        show_charts_view();
        // refresh state
        m_presenter->refresh_action_states();
    });
    actions.on_show_simulation_output([this] {
        // show the simulation output panel and refresh the toolbar state
        show_simulation_output_panel();
        // refresh state
        m_presenter->refresh_action_states();
    });
    // simulation group
    actions.on_run_simulation([this] { m_presenter->run_simulation(); });
    actions.on_configure_simulation([this] { m_presenter->configure_simulation(); });
    // plugin configuration
    actions.on_configure_plugin([this] { m_presenter->configure_plugin(); });
    // charts context menu actions
    const auto& chart_actions = m_window->global<ChartsPanelActions>();
    chart_actions.on_zoom_to_fit([this] { m_presenter->chart_zoom_to_fit(); });
    chart_actions.on_autorange([this] { m_presenter->chart_autorange(); });
    chart_actions.on_zoom_abscissa_extent([this] { m_presenter->chart_zoom_abscissa_extent(); });
    chart_actions.on_add_remove_plots([this] { m_presenter->chart_add_remove_plots(); });
    chart_actions.on_delete_all_plots([this] { m_presenter->chart_delete_all_plots(); });
    chart_actions.on_calculate_fft([this] { m_presenter->chart_calculate_fft(); });
    chart_actions.on_open_xyce_fft_calculation([this] { m_presenter->chart_open_xyce_fft_calculation(); });
    chart_actions.on_step_tool([this] { m_presenter->chart_step_tool(); });
    chart_actions.on_add_chart([this] { m_presenter->chart_add_chart(); });
    chart_actions.on_delete_chart([this] { m_presenter->chart_delete_chart(); });
    chart_actions.on_new_window([this] { m_presenter->chart_new_window(); });
    // exit
    actions.on_exit([this] { m_window->hide(); });
}

void SlintMainWindowView::show() {
    // show the slint main window
    m_window->show();
}

slint::Window& SlintMainWindowView::window() {
    // expose the underlying slint window for event handling (e.g. close-requested)
    return m_window->window();
}

void SlintMainWindowView::set_title(const std::string& title) {
    // the window title is set in the .slint file; runtime updates can be wired
    // once the slint api supports it
    spdlog::info("set title: {}", title);
}

void SlintMainWindowView::set_status_text(const std::string& text) {
    // status bar wiring pending; the slint window does not have a native status bar
    spdlog::info("status: {}", text);
}

void SlintMainWindowView::apply_action_enablement(const ActionStateEnablement& enablement) {
    // toolbar enablement bindings land with the toolbar state wiring
    spdlog::info("enablement: run={} configure={} save={}", enablement.run_simulation, enablement.configure_simulation, enablement.save);
}

void SlintMainWindowView::show_netlist_view() {
    // netlist editor view wiring pending
    spdlog::info("show netlist view");
}

void SlintMainWindowView::show_charts_view() {
    // reveal the charts panel in the content area
    m_window->set_charts_visible(true);
    // create the charts renderer the first time the charts panel is shown
    ensure_charts_renderer();
}

void SlintMainWindowView::set_netlist_editor_content(const std::string& content) {
    // store the netlist content for the presenter to retrieve
    m_netlist_content = content;
    spdlog::info("netlist content set ({} bytes)", content.size());
}

std::string SlintMainWindowView::netlist_editor_content() const {
    // return the cached netlist content
    return m_netlist_content;
}

void SlintMainWindowView::set_netlist_editor_read_only(bool read_only) {
    // store the read-only state for the editor
    m_netlist_read_only = read_only;
    spdlog::info("netlist read only = {}", read_only);
}

bool SlintMainWindowView::charts_shown() const {
    // report whether the charts panel is currently visible
    return m_window->get_charts_visible();
}

void SlintMainWindowView::show_simulation_output_panel() {
    // simulation output panel wiring pending
    spdlog::info("show simulation output");
}

void SlintMainWindowView::hide_simulation_output_panel() {
    // simulation output panel wiring pending
    spdlog::info("hide simulation output");
}

void SlintMainWindowView::clear_simulation_output() { spdlog::info("clear simulation output"); }

void SlintMainWindowView::append_simulation_output_line(const std::string& line) { spdlog::info("sim output: {}", line); }

bool SlintMainWindowView::simulation_output_panel_hidden() const { return true; }

bool SlintMainWindowView::simulation_output_has_content() const { return false; }

void SlintMainWindowView::update_charts(ExpressionManager& expression_manager, const StepInformation& step_information, AbscissaScale abscissa_scale, const std::vector<std::vector<std::string>>& suggested_plots) {
    // the presenter updates the charts before showing the charts view, so create
    // the renderer here if it does not exist yet (the attach/set_frame work is
    // shared with ensure_charts_renderer and is a no-op when already created)
    ensure_charts_renderer();
    // forward the data to the renderer
    if (m_charts_renderer) {
        m_charts_renderer->update(expression_manager, step_information, abscissa_scale, suggested_plots);
    }
}

void SlintMainWindowView::delete_all_charts() {
    // the renderer may not exist yet on the first file open
    if (m_charts_renderer) {
        m_charts_renderer->delete_all_charts();
    }
}

void SlintMainWindowView::set_open_fft_calculation_files(const std::vector<std::shared_ptr<XyceOutputFile>>&) { spdlog::info("set fft files"); }

std::optional<SimulationConfig> SlintMainWindowView::show_simulation_parameters_dialog(const SimulationConfig& /*current*/) {
    // dialog wired once the simulation dialog component is integrated
    spdlog::info("show simulation parameters dialog");
    return std::nullopt;
}

std::optional<PluginConfig> SlintMainWindowView::show_plugin_config_dialog(const PluginConfig& /*current*/) {
    // dialog wired once the plugin config dialog component is integrated
    spdlog::info("show plugin config dialog");
    return std::nullopt;
}

void SlintMainWindowView::start_simulation_process(const std::string& program, const std::filesystem::path& netlist_path, const std::filesystem::path& working_directory) { spdlog::info("start simulation: {} {} cwd={}", program, netlist_path.string(), working_directory.string()); }

void SlintMainWindowView::spawn_raw_file_window(std::shared_ptr<XyceOutputFile> /*raw_file*/) { spdlog::info("spawn raw file window"); }

void SlintMainWindowView::handle_open() {
    // run the native file dialog
    const auto filepath = FileDialog::open_xyce_file();
    // user canceled the dialog
    if (!filepath.has_value())
        return;
    // analyze the file extension
    const auto extension = filepath->extension().string();
    // netlist file extension
    if (extension == ".cir") {
        // forward the netlist file to the presenter
        m_presenter->open_netlist_file(filepath.value());
        // exit
        return;
    }
    // raw file extension
    if (extension == ".raw") {
        // forward the raw file to the presenter
        m_presenter->open_raw_file(filepath.value());
    }
}

void SlintMainWindowView::ensure_charts_renderer() {
    // the renderer already exists
    if (m_charts_renderer)
        return;
    // create the renderer on the first charts panel show
    m_charts_renderer = std::make_unique<ChartsRenderer>();
    // attach the renderer to the slint window; the platform backend extracts the native content view (NSView on macOS)
    m_charts_renderer->attach(m_window->window());
    // position the renderer over the charts panel region and render a frame
    const auto size = m_window->window().size();
    const auto scale = m_window->window().scale_factor();
    // the charts panel fills the window below the 59px toolbar; the content
    // view is flipped (upper-left origin), so offset from the top
    const uint32_t toolbar_px = static_cast<uint32_t>(59 * scale);
    m_charts_renderer->set_frame(0, toolbar_px, size.width, size.height - toolbar_px, scale);
    // initialize the ImGui/ImPlot backend
    m_charts_renderer->initialize();
    // render the first frame
    m_charts_renderer->render();
}
