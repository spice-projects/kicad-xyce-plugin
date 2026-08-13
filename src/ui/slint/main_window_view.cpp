#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <slint.h>
#include <spdlog/spdlog.h>

#include "../file_dialog.h"
#include "main_window_view.h"

SlintMainWindowView2::SlintMainWindowView2(std::unique_ptr<NetlistSource> /*netlist_source*/, PluginConfig /*plugin_config*/) :
    m_window(MainWindow::create()) {}

void SlintMainWindowView2::set_event_handler(MainWindowViewDefEvents& handler) {
    // store the event handler reference for later use
    m_event_handler = &handler;

    // bind the toolbar actions to the event handler methods
    const auto& actions = m_window->global<MainWindowActions>();
    // file group
    actions.on_open_xyce_file([this] { handle_open(); });
    actions.on_save_netlist([this] { m_event_handler->on_save_netlist(); });
    // content view group
    actions.on_show_netlist([this] { m_event_handler->on_show_netlist(); });
    actions.on_show_charts([this] { m_event_handler->on_show_charts(); });
    actions.on_show_simulation_output([this] { m_event_handler->on_show_simulation_output(); });
    // simulation group
    actions.on_run_simulation([this] { m_event_handler->on_run_simulation(); });
    actions.on_configure_simulation([this] { m_event_handler->on_configure_simulation(); });
    // plugin configuration
    actions.on_configure_plugin([this] { m_event_handler->on_configure_plugin(); });
    // exit
    actions.on_exit([this] { m_window->hide(); });

    // charts context menu actions; chart_position is a float [0..1] from the
    // slint panel, the renderer translates it to an index using its own count
    const auto& chart_actions = m_window->global<ChartsPanelActions>();
    chart_actions.on_zoom_to_fit([this](float chart_position) { m_charts_renderer->zoom_to_fit(chart_position); });
    chart_actions.on_autorange([this](float chart_position) { m_charts_renderer->autorange(chart_position); });
    chart_actions.on_zoom_abscissa_extent([this](float chart_position) { m_charts_renderer->zoom_abscissa_extent(chart_position); });
    chart_actions.on_delete_all_plots([this](float chart_position) { m_charts_renderer->delete_all_plots(chart_position); });
    chart_actions.on_add_chart([this](float) {
        // add chart
        m_charts_renderer->add_chart();
        // refresh charts to show the new chart
        m_charts_renderer->refresh_charts();
    });
    chart_actions.on_delete_chart([this](float chart_position) { m_charts_renderer->delete_chart(chart_position); });
    // events that need presenter involvement: convert float to int via renderer
    chart_actions.on_add_remove_plots([this](float chart_position) { m_event_handler->on_chart_add_remove_plots(m_charts_renderer->chart_count() > 0 ? static_cast<size_t>(chart_position * static_cast<float>(m_charts_renderer->chart_count())) : 0); });
    chart_actions.on_calculate_fft([this](float chart_position) { m_event_handler->on_chart_calculate_fft(m_charts_renderer->chart_count() > 0 ? static_cast<size_t>(chart_position * static_cast<float>(m_charts_renderer->chart_count())) : 0); });
    chart_actions.on_open_xyce_fft_calculation([this](float chart_position) { m_event_handler->on_chart_open_xyce_fft_calculation(m_charts_renderer->chart_count() > 0 ? static_cast<size_t>(chart_position * static_cast<float>(m_charts_renderer->chart_count())) : 0); });
    chart_actions.on_step_tool([this](float chart_position) { m_event_handler->on_chart_step_tool(m_charts_renderer->chart_count() > 0 ? static_cast<size_t>(chart_position * static_cast<float>(m_charts_renderer->chart_count())) : 0); });
    chart_actions.on_new_window([this](float chart_position) { m_event_handler->on_chart_new_window(m_charts_renderer->chart_count() > 0 ? static_cast<size_t>(chart_position * static_cast<float>(m_charts_renderer->chart_count())) : 0); });
}

void SlintMainWindowView2::show() {
    // show the slint main window
    m_window->show();
}

slint::Window& SlintMainWindowView2::window() {
    // expose the underlying slint window for event handling
    return m_window->window();
}

void SlintMainWindowView2::set_title(const std::string& title) {
    // the window title is set in the .slint file; runtime updates can be wired
    // once the slint api supports it
    spdlog::info("set title: {}", title);
}

void SlintMainWindowView2::set_status_text(const std::string& text) {
    // status bar wiring pending; the slint window does not have a native status bar
    spdlog::info("status: {}", text);
}

void SlintMainWindowView2::apply_action_enablement(const ActionStateEnablement& enablement) {
    // toolbar enablement bindings land with the toolbar state wiring
    spdlog::info("enablement: run={} configure={} save={}", enablement.run_simulation, enablement.configure_simulation, enablement.save);
}

void SlintMainWindowView2::show_netlist_view() {
    // netlist editor view wiring pending
    spdlog::info("show netlist view");
}

void SlintMainWindowView2::show_charts_view() {
    // reveal the charts panel in the content area
    m_window->set_charts_visible(true);
    // create the charts renderer the first time the charts panel is shown
    ensure_charts_renderer();
}

void SlintMainWindowView2::set_netlist_editor_content(const std::string& content) {
    // store the netlist content for the presenter to retrieve
    m_netlist_content = content;
    spdlog::info("netlist content set ({} bytes)", content.size());
}

std::string SlintMainWindowView2::netlist_editor_content() const {
    // return the cached netlist content
    return m_netlist_content;
}

void SlintMainWindowView2::set_netlist_editor_read_only(bool read_only) {
    // store the read-only state for the editor
    m_netlist_read_only = read_only;
    spdlog::info("netlist read only = {}", read_only);
}

bool SlintMainWindowView2::charts_shown() const {
    // report whether the charts panel is currently visible
    return m_window->get_charts_visible();
}

void SlintMainWindowView2::show_simulation_output_panel() {
    // simulation output panel wiring pending
    spdlog::info("show simulation output");
}

void SlintMainWindowView2::hide_simulation_output_panel() {
    // simulation output panel wiring pending
    spdlog::info("hide simulation output");
}

void SlintMainWindowView2::clear_simulation_output() { spdlog::info("clear simulation output"); }

void SlintMainWindowView2::append_simulation_output_line(const std::string& line) { spdlog::info("sim output: {}", line); }

bool SlintMainWindowView2::simulation_output_panel_hidden() const { return true; }

bool SlintMainWindowView2::simulation_output_has_content() const { return false; }

void SlintMainWindowView2::update_charts(ExpressionManager& expression_manager, const StepInformation& step_information, AbscissaScale abscissa_scale, const std::vector<std::vector<std::string>>& suggested_plots) {
    // the presenter updates the charts before showing the charts view, so create
    // the renderer here if it does not exist yet
    ensure_charts_renderer();
    // forward the data to the renderer
    if (m_charts_renderer) {
        m_charts_renderer->update(expression_manager, step_information, abscissa_scale, suggested_plots);
    }
}

void SlintMainWindowView2::delete_all_charts() {
    // the renderer may not exist yet on the first file open
    if (m_charts_renderer) {
        m_charts_renderer->delete_all_charts();
    }
}

void SlintMainWindowView2::set_open_fft_calculation_files(const std::vector<std::shared_ptr<XyceOutputFile>>&) { spdlog::info("set fft files"); }

std::optional<SimulationConfig> SlintMainWindowView2::show_simulation_parameters_dialog(const SimulationConfig& /*current*/) {
    // dialog wired once the simulation dialog component is integrated
    spdlog::info("show simulation parameters dialog");
    return std::nullopt;
}

std::optional<PluginConfig> SlintMainWindowView2::show_plugin_config_dialog(const PluginConfig& /*current*/) {
    // dialog wired once the plugin config dialog component is integrated
    spdlog::info("show plugin config dialog");
    return std::nullopt;
}

void SlintMainWindowView2::start_simulation_process(const std::string& program, const std::filesystem::path& netlist_path, const std::filesystem::path& working_directory) { spdlog::info("start simulation: {} {} cwd={}", program, netlist_path.string(), working_directory.string()); }

void SlintMainWindowView2::spawn_raw_file_window(std::shared_ptr<XyceOutputFile> /*raw_file*/) { spdlog::info("spawn raw file window"); }

void SlintMainWindowView2::handle_open() {
    // run the native file dialog
    const auto filepath = FileDialog::open_xyce_file();
    // user canceled the dialog
    if (!filepath.has_value())
        return;
    // forward the selected file to the event handler (presenter)
    m_event_handler->on_open_xyce_file(filepath.value());
}

void SlintMainWindowView2::ensure_charts_renderer() {
    // the renderer already exists
    if (m_charts_renderer)
        return;
    // create the renderer on the first charts panel show
    m_charts_renderer = std::make_unique<ChartsRenderer>();
    // attach the renderer to the slint window
    m_charts_renderer->attach(m_window->window());
    // position the renderer over the charts panel region and render a frame
    const auto size = m_window->window().size();
    const auto scale = m_window->window().scale_factor();
    // the charts panel fills the window below the 59px toolbar
    const uint32_t toolbar_px = static_cast<uint32_t>(59 * scale);
    m_charts_renderer->set_frame(0, toolbar_px, size.width, size.height - toolbar_px, scale);
    // initialize the ImGui/ImPlot backend
    m_charts_renderer->initialize();
    // render the first frame
    m_charts_renderer->render();
}
