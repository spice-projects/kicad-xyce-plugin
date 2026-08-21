#include <algorithm>
#include <chrono>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <slint.h>
#include <spdlog/spdlog.h>

#include "../app/app.h"
#include "clipboard.h"
#include "file_dialog.h"
#include "main_window_view.h"
#include "modal_manager.h"

SlintMainWindowView2::SlintMainWindowView2(std::unique_ptr<NetlistSource> /*netlist_source*/, PluginConfig /*plugin_config*/) :
    m_window(main_window::MainWindow::create()), m_simulation_log(std::make_shared<slint::VectorModel<slint::SharedString>>()) {
    // expose the log model to the output panel
    m_window->set_simulation_output_log(m_simulation_log);
}

void SlintMainWindowView2::set_event_handler(MainWindowViewDefEvents& handler) {
    // store the event handler reference for later use
    m_event_handler = &handler;
    // bind the toolbar actions to the event handler methods
    const auto& actions = m_window->global<main_window::MainWindowActions>();
    // file group
    actions.on_open_xyce_file([this] { handle_open(); });
    actions.on_save_netlist([this] { guard_modal([this] { m_event_handler->on_save_netlist(); }); });
    // content view group
    actions.on_show_netlist([this] { guard_modal([this] { m_event_handler->on_show_netlist(); }); });
    actions.on_show_charts([this] { guard_modal([this] { m_event_handler->on_show_charts(); }); });
    actions.on_show_simulation_output([this] { guard_modal([this] { m_event_handler->on_show_simulation_output(); }); });
    actions.on_close_simulation_output([this] { guard_modal([this] { m_event_handler->on_close_simulation_output(); }); });
    // the copy action is a pure view operation on the buffered log
    actions.on_copy_simulation_output([this](int start, int end) { copy_simulation_selection(start, end); });
    // simulation group
    actions.on_run_simulation([this] { guard_modal([this] { m_event_handler->on_run_simulation(); }); });
    actions.on_stop_simulation([this] { guard_modal([this] { m_event_handler->on_cancel_simulation(); }); });
    actions.on_configure_simulation([this] { guard_modal([this] { m_event_handler->on_configure_simulation(); }); });
    // plugin configuration
    actions.on_configure_plugin([this] { guard_modal([this] { m_event_handler->on_configure_plugin(); }); });
    // exit
    actions.on_exit([this] {
        // do not close the window while a modal dialog is open
        if (m_modal_dialog_open)
            return;
        // close the window, which will terminate the application
        m_window->hide();
    });

    // netlist editor edits are reported live so the presenter can track the
    // dirty state and content changes
    actions.on_netlist_edited([this] { m_event_handler->on_netlist_editor_modified(); });

    // charts context menu actions; chart_position is a float [0..1] from the
    // slint panel, the renderer translates it to an index using its own count
    const auto& chart_actions = m_window->global<main_window::ChartsPanelActions>();
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
    chart_actions.on_add_remove_plots([this](float chart_position) { show_add_remove_plots_dialog(chart_position); });
    chart_actions.on_calculate_fft([this](float chart_position) { m_event_handler->on_chart_calculate_fft(m_charts_renderer->chart_count() > 0 ? static_cast<size_t>(chart_position * static_cast<float>(m_charts_renderer->chart_count())) : 0); });
    chart_actions.on_open_xyce_fft_calculation([this](float chart_position) { m_event_handler->on_chart_open_xyce_fft_calculation(m_charts_renderer->chart_count() > 0 ? static_cast<size_t>(chart_position * static_cast<float>(m_charts_renderer->chart_count())) : 0); });
    chart_actions.on_step_tool([this](float chart_position) { m_event_handler->on_chart_step_tool(m_charts_renderer->chart_count() > 0 ? static_cast<size_t>(chart_position * static_cast<float>(m_charts_renderer->chart_count())) : 0); });
    chart_actions.on_new_window([this](float chart_position) { m_event_handler->on_chart_new_window(m_charts_renderer->chart_count() > 0 ? static_cast<size_t>(chart_position * static_cast<float>(m_charts_renderer->chart_count())) : 0); });
    // chart drag-zoom interactions
    chart_actions.on_zoom_drag_started([this](float x, float y) {
        if (m_charts_renderer)
            m_charts_renderer->zoom_drag_started(x, y);
    });
    chart_actions.on_zoom_drag_moved([this](float x, float y) {
        if (m_charts_renderer)
            m_charts_renderer->zoom_drag_moved(x, y);
    });
    chart_actions.on_zoom_drag_ended([this] {
        if (m_charts_renderer)
            m_charts_renderer->zoom_drag_ended();
    });
    chart_actions.on_zoom_drag_canceled([this] {
        if (m_charts_renderer)
            m_charts_renderer->zoom_drag_canceled();
    });
    // chart hover readout interactions
    chart_actions.on_hover_moved([this](float x, float y) {
        if (m_charts_renderer)
            m_charts_renderer->hover_moved(x, y);
    });
    chart_actions.on_hover_ended([this] {
        if (m_charts_renderer)
            m_charts_renderer->hover_ended();
    });
}

void SlintMainWindowView2::show() {
    // show the slint main window
    m_window->show();
    // the native content view may not exist yet on the very first show, so a
    // charts renderer created right after show() fails to attach; retry once the
    // native view exists so a window spawned with a raw file renders its charts
    slint::Timer::single_shot(std::chrono::milliseconds(50), [this] { update_charts_frame(); });
}

slint::Window& SlintMainWindowView2::window() {
    // expose the underlying slint window for event handling
    return m_window->window();
}

void SlintMainWindowView2::set_title(const std::string& title) {
    // update the window title property, which is bound to the native title
    m_window->set_window_title(slint::SharedString(title));
}

void SlintMainWindowView2::set_status_text(const std::string& text) {
    // retain the latest permanent status text
    m_last_status_text = text;
    // update the status bar text in the slint window
    m_window->set_status_text(slint::SharedString(text));
}

void SlintMainWindowView2::set_simulation_running(bool running) {
    // toggle the Run/Stop toolbar action
    m_window->set_simulation_running(running);
}

void SlintMainWindowView2::apply_action_enablement(const ActionStateEnablement& enablement) {
    // do not re-enable toolbar actions while a modal dialog blocks input
    if (m_modal_dialog_open)
        return;
    // update the slint window with the action enablement state
    m_window->set_enable_open(enablement.open);
    m_window->set_enable_save(enablement.save);
    m_window->set_enable_run_simulation(enablement.run_simulation);
    m_window->set_enable_configure_simulation(enablement.configure_simulation);
    m_window->set_enable_show_netlist(enablement.show_netlist);
    m_window->set_enable_show_charts(enablement.show_charts);
    m_window->set_enable_show_sim_output(enablement.show_simulation_output);
    // chart context tools
    m_window->set_enable_fft(enablement.fft);
    m_window->set_enable_step_tool(enablement.step_tool);
}

void SlintMainWindowView2::show_netlist_view() {
    // reveal the netlist panel and hide the charts panel
    m_window->set_charts_visible(false);
    // reposition the renderer so it no longer covers the netlist editor
    update_charts_frame();
}

void SlintMainWindowView2::show_charts_view() {
    // reveal the charts panel in the content area
    m_window->set_charts_visible(true);
    // create the charts renderer the first time the charts panel is shown
    ensure_charts_renderer();
    // reposition the overlay over the now-visible charts panel and render a
    // frame; without this the overlay stays at the last frame (e.g. zero-sized
    // when the renderer was created while the charts view was hidden)
    update_charts_frame();
}

void SlintMainWindowView2::set_netlist_editor_content(const std::string& content) {
    // store the netlist content for the presenter to retrieve
    m_netlist_content = content;
    // push the content into the slint editor widget
    m_window->set_netlist_text(slint::SharedString(content));
}

std::string SlintMainWindowView2::netlist_editor_content() const {
    // return the cached netlist content
    return m_netlist_content;
}

void SlintMainWindowView2::set_netlist_editor_read_only(bool read_only) {
    // store the read-only state for the editor
    m_netlist_read_only = read_only;
    // push the read-only state into the slint editor widget
    m_window->set_netlist_read_only(read_only);
}

bool SlintMainWindowView2::charts_shown() const {
    // report whether the charts panel is currently visible
    return m_window->get_charts_visible();
}

void SlintMainWindowView2::show_simulation_output_panel() {
    // reveal the simulation output panel in the body
    m_window->set_simulation_output_visible(true);
    // reposition the charts renderer above the output panel
    update_charts_frame();
}

void SlintMainWindowView2::hide_simulation_output_panel() {
    // hide the simulation output panel from the body
    m_window->set_simulation_output_visible(false);
    // reposition the charts renderer over the released space
    update_charts_frame();
}

// replace tab characters with spaces at fixed 8-column stops so the log columns
// line up; the slint text renderer draws an unsupported tab as a box character
namespace
{
    std::string expand_tabs(std::string_view line) {
        std::string expanded;
        expanded.reserve(line.size());
        std::size_t column = 0;
        for (const char c : line) {
            if (c == '\t') {
                const std::size_t to_next_stop = 8 - (column % 8);
                expanded.append(to_next_stop, ' ');
                column += to_next_stop;
            }
            else {
                expanded.push_back(c);
                ++column;
            }
        }
        return expanded;
    }
} // namespace

void SlintMainWindowView2::clear_simulation_output() {
    // drop all buffered log lines from the panel model
    m_simulation_log->clear();
}

void SlintMainWindowView2::append_simulation_output_line(const std::string& line) {
    // append the line to the panel log model; tabs are expanded to spaces
    // because the slint text renderer has no tab support
    m_simulation_log->push_back(slint::SharedString(expand_tabs(line)));
}

void SlintMainWindowView2::copy_simulation_selection(int start, int end) {
    // the panel reports no selection with start < 0 or start > end
    if (start < 0 || end < start)
        return;
    // clamp the selection to the buffered log
    const std::size_t row_count = m_simulation_log->row_count();
    if (row_count == 0)
        return;
    const std::size_t first = std::min(static_cast<std::size_t>(start), row_count - 1);
    const std::size_t last = std::min(static_cast<std::size_t>(end), row_count - 1);
    // join the selected lines
    std::string text;
    for (std::size_t i = first; i <= last; ++i) {
        if (!text.empty())
            text.push_back('\n');
        text.append(m_simulation_log->row_data(i).value_or(slint::SharedString()));
    }
    copy_to_clipboard(text);
}

bool SlintMainWindowView2::simulation_output_panel_hidden() const {
    // report whether the simulation output panel is currently hidden
    return !m_window->get_simulation_output_visible();
}

bool SlintMainWindowView2::simulation_output_has_content() const {
    // report whether the buffered log holds any lines
    return m_simulation_log->row_count() > 0;
}

void SlintMainWindowView2::update_charts(ExpressionManager& expression_manager, const StepInformation& step_information, AbscissaScale abscissa_scale, const std::vector<std::vector<std::string>>& suggested_plots) {
    // the presenter updates the charts before showing the charts view, so create
    // the renderer here if it does not exist yet
    ensure_charts_renderer();
    // forward the data to the renderer
    m_charts_renderer->update(expression_manager, step_information, abscissa_scale, suggested_plots);
}

void SlintMainWindowView2::delete_all_charts() {
    // the renderer may not exist yet on the first file open
    if (m_charts_renderer) {
        // delete all charts from the renderer and refresh the slint panel
        m_charts_renderer->delete_all_charts();
    }
}

void SlintMainWindowView2::set_open_fft_calculation_files(const std::vector<std::shared_ptr<XyceOutputFile>>& files) {
    // drive the charts context menu "Open Xyce FFT Calculation" action from the
    // parsed FFT calculation output files
    m_window->set_has_fft_files(!files.empty());
}

std::optional<SimulationConfig> SlintMainWindowView2::show_simulation_parameters_dialog(const SimulationConfig& current) {
    // the presenter must be wired before the dialog can deliver its result
    if (m_event_handler == nullptr)
        return std::nullopt;
    // check a modal dialog is already open, do not stack another one
    if (!begin_modal_dialog())
        return std::nullopt;
    // create the dialog view wrapper on first use
    if (!m_simulation_parameters_dialog)
        m_simulation_parameters_dialog = std::make_unique<simulation_parameters_dialog_view::SimulationParametersDialogView>();
    // show the dialog seeded with the current configuration; the slint dialog is
    // non-modal, so the accepted configuration is delivered asynchronously through
    // MainWindowViewDefEvents::on_simulation_parameters_dialog_result, and the modal
    // state (kept in this view) is released through the on_closed callback on both
    // accept and cancel
    m_simulation_parameters_dialog->show(current, *m_event_handler, [this] { end_modal_dialog(); });
    // remember the dialog window for unblocking when it closes
    m_modal_dialog_window = &m_simulation_parameters_dialog->window();
    // block input to the native main window while the dialog is shown
    modal_manager::set_input_blocked(window(), *m_modal_dialog_window, true);
    // nothing to return, TODO: remove return value once wxWidgets dialog is replaced with slint
    return std::nullopt;
}

std::optional<PluginConfig> SlintMainWindowView2::show_plugin_config_dialog(const PluginConfig& current) {
    // the presenter must be wired before the dialog can deliver its result
    if (m_event_handler == nullptr)
        return std::nullopt;
    // check a modal dialog is already open, do not stack another one
    if (!begin_modal_dialog())
        return std::nullopt;
    // create the dialog view wrapper on first use
    if (!m_plugin_config_dialog)
        m_plugin_config_dialog = std::make_unique<plugin_config_dialog_view::PluginConfigDialogView>();
    // show the dialog seeded with the current configuration; the slint dialog is
    // non-modal, so the accepted configuration is delivered asynchronously through
    // MainWindowViewDefEvents::on_plugin_config_dialog_result, and the modal state
    // (kept in this view) is released through the on_closed callback on both accept
    // and cancel
    m_plugin_config_dialog->show(current, *m_event_handler, [this] { end_modal_dialog(); });
    // remember the dialog window for unblocking when it closes
    m_modal_dialog_window = &m_plugin_config_dialog->window();
    // block input to the native main window while the dialog is shown
    modal_manager::set_input_blocked(window(), *m_modal_dialog_window, true);
    // nothing to return, TODO: remove return value once wxWidgets dialog is replaced with slint
    return std::nullopt;
}

void SlintMainWindowView2::start_simulation_process(const std::string& program, const std::filesystem::path& netlist_path, const std::filesystem::path& working_directory) {
    // create a fresh runner for this run
    m_simulation_runner = std::make_unique<SimulationRunner>();
    // forward the process output and termination to the event handler; the
    // callbacks are already marshalled to the slint event loop thread by the runner
    m_simulation_runner->set_stdout_callback([this](const std::string& line) { m_event_handler->on_simulation_stdout(line); });
    m_simulation_runner->set_stderr_callback([this](const std::string& line) { m_event_handler->on_simulation_stderr(line); });
    m_simulation_runner->set_finished_callback([this](int exit_code, bool was_canceled) { m_event_handler->on_simulation_finished(exit_code, was_canceled); });
    // launch the child process
    m_simulation_runner->start(program, netlist_path, working_directory);
}

void SlintMainWindowView2::cancel_simulation_process() {
    // request a graceful shutdown when a runner is active
    if (m_simulation_runner)
        m_simulation_runner->cancel();
}

void SlintMainWindowView2::spawn_raw_file_window(std::shared_ptr<XyceOutputFile> raw_file) {
    // delegate the new-window creation to the app, which owns the window wiring
    // (create the view/presenter pair, bind, show) in a single place
    App::instance().new_window(std::move(raw_file));
}

void SlintMainWindowView2::show_add_remove_plots_dialog(float chart_position) {
    // a modal dialog is already open, do not stack another one
    if (!begin_modal_dialog())
        return;
    // the dialog needs the charts renderer, which is created on first charts show
    ensure_charts_renderer();
    // create the dialog view wrapper on first use
    if (!m_add_plot_dialog) {
        m_add_plot_dialog = std::make_unique<add_plot_dialog_view::AddPlotDialogView>(*m_charts_renderer);
    }
    // show the dialog for the chart at the given position
    m_add_plot_dialog->show_for_chart(chart_position, [this] { end_modal_dialog(); });
    // remember the dialog window for unblocking when it closes
    m_modal_dialog_window = &m_add_plot_dialog->window();
    // block input to the native main window while the dialog is shown
    modal_manager::set_input_blocked(window(), *m_modal_dialog_window, true);
}

void SlintMainWindowView2::show_fft_dialog(size_t chart_index) {
    // a modal dialog is already open, do not stack another one
    if (!begin_modal_dialog())
        return;
    // the dialog needs the charts renderer, which is created on first charts show
    ensure_charts_renderer();
    // create the dialog view wrapper on first use
    if (!m_fft_dialog)
        m_fft_dialog = std::make_unique<fft_dialog_view::FftDialogView>(*m_charts_renderer);
    // show the dialog for the chart at the given index; the accepted expressions
    // and FFT parameters are delivered asynchronously through
    // on_fft_dialog_result, and the modal state is released through the on_closed
    // callback on both accept and cancel
    m_fft_dialog->show_for_chart(chart_index, *m_event_handler, [this] { end_modal_dialog(); });
    // remember the dialog window for unblocking when it closes
    m_modal_dialog_window = &m_fft_dialog->window();
    // block input to the native main window while the dialog is shown
    modal_manager::set_input_blocked(window(), *m_modal_dialog_window, true);
}

void SlintMainWindowView2::show_step_tool_dialog(size_t chart_index) {
    // a modal dialog is already open, do not stack another one
    if (!begin_modal_dialog())
        return;
    // the dialog needs the charts renderer, which is created on first charts show
    ensure_charts_renderer();
    // create the dialog view wrapper on first use
    if (!m_step_tool_dialog)
        m_step_tool_dialog = std::make_unique<step_tool_dialog_view::StepToolDialogView>(*m_charts_renderer);
    // show the dialog for the chart at the given index; the accepted step
    // selection is applied back to the chart through the renderer, and the
    // modal state is released through the on_closed callback on both accept
    // and cancel
    m_step_tool_dialog->show_for_chart(chart_index, [this] { end_modal_dialog(); });
    // remember the dialog window for unblocking when it closes
    m_modal_dialog_window = &m_step_tool_dialog->window();
    // block input to the native main window while the dialog is shown
    modal_manager::set_input_blocked(window(), *m_modal_dialog_window, true);
}

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
    // create the renderer on the first charts panel show; update_charts_frame
    // attaches it to the native content view and initializes the backend
    m_charts_renderer = std::make_unique<ChartsRenderer>();
    // wire hover readout to update the status bar
    m_charts_renderer->set_hover_callback([this](const std::string& text) {
        if (text.empty())
            m_window->set_status_text(slint::SharedString(m_last_status_text));
        else
            m_window->set_status_text(slint::SharedString(text));
    });
    update_charts_frame();
}

void SlintMainWindowView2::update_charts_frame() {
    // the renderer is created lazily on the first charts panel show
    if (m_charts_renderer == nullptr)
        return;
    // attach and initialize the renderer the first time the native content view
    // exists; when the renderer was created right after the window was shown the
    // native view may not exist yet, so the attach is deferred until this frame
    // recompute runs again (e.g. from the show() single-shot timer)
    if (!m_charts_renderer->attached()) {
        m_charts_renderer->attach(m_window->window());
        m_charts_renderer->initialize();
    }
    // position the renderer in physical pixels below the toolbar
    const auto size = m_window->window().size();
    const auto scale = m_window->window().scale_factor();
    // the content area starts below the 59px toolbar
    const uint32_t toolbar_px = static_cast<uint32_t>(59 * scale);
    uint32_t y = toolbar_px;
    uint32_t height = size.height > toolbar_px ? size.height - toolbar_px : 0;
    // reserve the status bar (24px + 1px divider) at the bottom of the window
    const uint32_t statusbar_px = static_cast<uint32_t>(25 * scale);
    height = height > statusbar_px ? height - statusbar_px : 0;
    // reserve the simulation output panel height when it is visible; the panel
    // is docked at the bottom of the body (below the charts/netlist area), so
    // only the overlay height shrinks, its top stays below the toolbar
    if (m_window->get_simulation_output_visible()) {
        const uint32_t output_px = static_cast<uint32_t>(200 * scale);
        height = height > output_px ? height - output_px : 0;
    }
    // hide the overlay when the charts panel is not shown
    const bool charts_shown = m_window->get_charts_visible();
    m_charts_renderer->set_frame(0, y, charts_shown ? size.width : 0, charts_shown ? height : 0, scale);
    // render the repositioned frame; when the panel is hidden the overlay is
    // sized to zero and render_frame skips the degenerate frame
    if (charts_shown)
        m_charts_renderer->render();
}

bool SlintMainWindowView2::begin_modal_dialog() {
    // a modal dialog is already open, refuse to open another one
    if (m_modal_dialog_open)
        return false;
    // dim the main window while a dialog is open
    m_window->set_modal_active(true);
    // remember the modal state
    m_modal_dialog_open = true;
    return true;
}

void SlintMainWindowView2::end_modal_dialog() {
    // no modal state to release
    if (!m_modal_dialog_open)
        return;
    // restore the main window input
    modal_manager::set_input_blocked(window(), *m_modal_dialog_window, false);
    // restore the main window appearance
    m_window->set_modal_active(false);
    // clear the modal state
    m_modal_dialog_open = false;
}
