#include <filesystem>
#include <memory>
#include <string>
#include <utility>

#include <spdlog/spdlog.h>

#include "../../file/xyce_raw_file.h"
#include "../../simulation_parameters/simulation_config.h"
#include "../main_window_state.h"
#include "main_window_presenter.h"

SlintMainWindowPresenter2::SlintMainWindowPresenter2(MainWindowViewDef& view, std::unique_ptr<NetlistSource> netlist_source, PluginConfig plugin_config) :
    m_view(view), m_netlist_source(std::move(netlist_source)), m_simulation_config(SimulationConfig::from_xyce_directives({})), m_plugin_config(std::move(plugin_config)) {
    spdlog::info("slint presenter2 created");
}

SlintMainWindowPresenter2::~SlintMainWindowPresenter2() = default;

void SlintMainWindowPresenter2::on_open_xyce_file(const std::filesystem::path& path) {
    // analyze the file extension
    const auto extension = path.extension().string();
    // netlist file extension
    if (extension == ".cir") {
        // forward the netlist file to the open handler
        spdlog::info("presenter2: open netlist file (stub)");
        return;
    }
    // raw file extension
    if (extension == ".raw") {
        // parse the raw file and store it in the presenter state
        if (update_xyce_raw_file(xyce_raw_file_parser(path), true))
            show_raw_file_view();
    }
}

void SlintMainWindowPresenter2::on_save_netlist() {
    // editor content is forwarded to the netlist source
    spdlog::info("presenter2: save netlist (stub)");
}

void SlintMainWindowPresenter2::on_show_netlist() {
    // switch to the netlist view
    m_view.show_netlist_view();
    // refresh state
    refresh_action_states();
}

void SlintMainWindowPresenter2::on_show_charts() {
    // switch to the charts view
    m_view.show_charts_view();
    // refresh state
    refresh_action_states();
}

void SlintMainWindowPresenter2::on_show_simulation_output() {
    // show the simulation output panel
    m_view.show_simulation_output_panel();
    // refresh state
    refresh_action_states();
}

void SlintMainWindowPresenter2::on_run_simulation() {
    // simulation process launch wiring pending
    spdlog::info("presenter2: run simulation (stub)");
}

void SlintMainWindowPresenter2::on_configure_simulation() {
    // dialog wiring pending; presenter opens the simulation parameters dialog
    spdlog::info("presenter2: configure simulation (stub)");
}

void SlintMainWindowPresenter2::on_configure_plugin() {
    // dialog wiring pending; presenter opens the plugin configuration dialog
    spdlog::info("presenter2: configure plugin (stub)");
}

void SlintMainWindowPresenter2::on_chart_zoom_to_fit() { spdlog::info("presenter2: chart zoom to fit (stub)"); }

void SlintMainWindowPresenter2::on_chart_autorange() { spdlog::info("presenter2: chart autorange (stub)"); }

void SlintMainWindowPresenter2::on_chart_zoom_abscissa_extent() { spdlog::info("presenter2: chart zoom abscissa extent (stub)"); }

void SlintMainWindowPresenter2::on_chart_add_remove_plots() { spdlog::info("presenter2: chart add/remove plots (stub)"); }

void SlintMainWindowPresenter2::on_chart_delete_all_plots() { spdlog::info("presenter2: chart delete all plots (stub)"); }

void SlintMainWindowPresenter2::on_chart_calculate_fft() { spdlog::info("presenter2: chart calculate fft (stub)"); }

void SlintMainWindowPresenter2::on_chart_open_xyce_fft_calculation() { spdlog::info("presenter2: chart open xyce fft calculation (stub)"); }

void SlintMainWindowPresenter2::on_chart_step_tool() { spdlog::info("presenter2: chart step tool (stub)"); }

void SlintMainWindowPresenter2::on_chart_add_chart() { spdlog::info("presenter2: chart add chart (stub)"); }

void SlintMainWindowPresenter2::on_chart_delete_chart() { spdlog::info("presenter2: chart delete chart (stub)"); }

void SlintMainWindowPresenter2::on_chart_new_window() { spdlog::info("presenter2: chart new window (stub)"); }

void SlintMainWindowPresenter2::on_simulation_finished(int /*exit_code*/, bool /*was_canceled*/) { spdlog::info("presenter2: simulation finished (stub)"); }

void SlintMainWindowPresenter2::on_simulation_stdout(const std::string& /*line*/) { spdlog::info("presenter2: simulation stdout (stub)"); }

void SlintMainWindowPresenter2::on_simulation_stderr(const std::string& /*line*/) { spdlog::info("presenter2: simulation stderr (stub)"); }

void SlintMainWindowPresenter2::on_netlist_editor_modified() {
    // dirty state tracking wiring pending
    spdlog::info("presenter2: netlist editor modified (stub)");
}

void SlintMainWindowPresenter2::on_extract_schematic_netlist() {
    // kiCad integration wiring pending
    spdlog::info("presenter2: extract schematic netlist (stub)");
}

std::shared_ptr<XyceSimulationRunner> SlintMainWindowPresenter2::simulation_runner() const {
    spdlog::info("presenter2: simulation runner access (stub)");
    return nullptr;
}

const std::optional<std::shared_ptr<XyceOutputFile>>& SlintMainWindowPresenter2::raw_file() const {
    // return the current raw file reference
    return m_xyce_raw_file;
}

const std::vector<std::shared_ptr<XyceOutputFile>>& SlintMainWindowPresenter2::fft_files() const {
    // return the parsed FFT calculation output files
    return m_fft_files;
}

void SlintMainWindowPresenter2::refresh_action_states() {
    // gather the input flags describing the current window state
    ActionStateInput input;
    input.has_netlist = m_netlist_has_content;
    input.has_netlist_file = m_netlist_source != nullptr && !m_netlist_source->is_read_only();
    input.has_raw = m_xyce_raw_file.has_value();
    input.charts_shown = m_view.charts_shown();
    input.netlist_editor_dirty = m_netlist_editor_dirty;
    input.output_hidden = m_view.simulation_output_panel_hidden();
    input.log_has_content = m_view.simulation_output_has_content();
    // compute the action enablement for the current state
    const ActionStateEnablement enablement = compute_action_enablement(input);
    // apply the enablement to the view
    m_view.apply_action_enablement(enablement);
    // forward the parsed FFT calculation files so the charts context menu can expose the action
    m_view.set_open_fft_calculation_files(m_fft_files);
}

bool SlintMainWindowPresenter2::update_xyce_raw_file(std::optional<std::shared_ptr<XyceOutputFile>> raw_file, bool delete_charts) {
    // store the raw file reference
    m_xyce_raw_file = std::move(raw_file);
    // check the file is present
    if (m_xyce_raw_file.has_value()) {
        // file instance
        auto& file = m_xyce_raw_file.value();
        // delete all charts when the file may differ from the previous one
        if (delete_charts)
            m_view.delete_all_charts();
        // update the charts with the parsed data
        m_view.update_charts(file->expression_manager(), file->step_information(), file->abscissa_scale(), file->suggested_plots());
        // indicate success
        return true;
    }
    // indicate failure
    return false;
}

void SlintMainWindowPresenter2::show_raw_file_view() {
    // update the window title from the raw file
    set_base_title(m_xyce_raw_file.value()->title());
    // clear the netlist editor content
    update_netlist_editor_content("", false);
    // show the charts view over the netlist editor
    m_view.show_charts_view();
    // hide the simulation output panel for the raw-file view
    m_view.hide_simulation_output_panel();
    // refresh toolbar/menu states
    refresh_action_states();
}

void SlintMainWindowPresenter2::set_base_title(const std::string& title) {
    // store the clean base title
    m_base_title = title;
    // reset the dirty marker
    set_netlist_editor_dirty(false);
}

bool SlintMainWindowPresenter2::set_netlist_editor_dirty(bool flag) {
    // previous dirty state
    const bool previous_dirty_state = m_netlist_editor_dirty;
    // store the dirty state
    m_netlist_editor_dirty = flag;
    // prefix the base title while the editor holds unsaved changes
    m_view.set_title(flag ? "* " + m_base_title : m_base_title);
    // return true if the dirty state changed, false if it remained the same
    return previous_dirty_state != m_netlist_editor_dirty;
}

bool SlintMainWindowPresenter2::update_netlist_editor_content(const std::string& content, bool dirty_flag) {
    // set the editor content through the view
    m_view.set_netlist_editor_content(content);
    // track whether the editor holds content
    m_netlist_has_content = !content.empty();
    // return whether the dirty flag changed
    return set_netlist_editor_dirty(dirty_flag);
}
