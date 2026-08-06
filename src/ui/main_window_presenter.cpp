#include <filesystem>
#include <memory>
#include <optional>
#include <string>

#include <spdlog/spdlog.h>

#include "../file/xyce_raw_file.h"
#include "../netlist/netlist.h"
#include "editor_netlist_source.h"
#include "main_window_presenter.h"
#include "xyce_simulation_runner.h"

MainWindowPresenter::MainWindowPresenter(MainWindowView& view, std::unique_ptr<NetlistSource> netlist_source, PluginConfig plugin_config) :
    m_view(view), m_netlist_source(std::move(netlist_source)), m_simulation_config(SimulationConfig::from_xyce_directives({})), m_plugin_config(std::move(plugin_config)) {}

MainWindowPresenter::~MainWindowPresenter() = default;

void MainWindowPresenter::open_netlist_file(const std::filesystem::path& path) {
    // create an editor netlist source backed by the selected file, reading live editor text
    m_netlist_source = std::make_unique<EditorNetlistSource>([this]() { return m_view.netlist_editor_content(); }, path);
    // update the window title from the source
    set_base_title(m_netlist_source->title());
    // the netlist editor is now editable
    m_view.set_netlist_editor_read_only(false);
    // load the netlist content
    const auto [reloaded, content] = m_netlist_source->load_netlist();
    // set the editor content to the loaded netlist
    update_netlist_editor_content(content, false);
    // remove the raw output file reference
    m_xyce_raw_file = std::nullopt;
    // show the netlist view over the charts view
    m_view.show_netlist_view();
    m_charts_shown = false;
    // refresh toolbar/menu states
    refresh_action_states();
}

void MainWindowPresenter::open_raw_file(const std::filesystem::path& path) {
    // parse the raw file and store it in the presenter state
    if (update_xyce_raw_file(xyce_raw_file_parser(path), true)) {
        // update the window title from the raw file
        set_base_title(m_xyce_raw_file.value()->title());
        // clear the netlist editor content
        update_netlist_editor_content("", false);
        // show the charts view over the netlist editor
        m_view.show_charts_view();
        m_charts_shown = true;
        // hide the simulation output panel for the raw-file view
        m_view.hide_simulation_output_panel();
        m_simulation_output_hidden = true;
        // refresh toolbar/menu states
        refresh_action_states();
    }
}

void MainWindowPresenter::load_raw_file(std::shared_ptr<XyceOutputFile> raw_file) {
    // store the already-parsed raw file
    if (update_xyce_raw_file(std::optional<std::shared_ptr<XyceOutputFile>>(std::move(raw_file)), true)) {
        // update the window title from the raw file
        set_base_title(m_xyce_raw_file.value()->title());
        // clear the netlist editor content
        update_netlist_editor_content("", false);
        // show the charts view over the netlist editor
        m_view.show_charts_view();
        m_charts_shown = true;
        // hide the simulation output panel for the raw-file view
        m_view.hide_simulation_output_panel();
        m_simulation_output_hidden = true;
        // refresh toolbar/menu states
        refresh_action_states();
    }
}

void MainWindowPresenter::save_netlist() {
    // save content in the netlist source
    m_netlist_source->save_netlist();
    // reset the dirty flag and refresh states when it changed
    if (set_netlist_editor_dirty(false))
        refresh_action_states();
}

void MainWindowPresenter::show_netlist_view() {
    // hide the charts panel and show the netlist editor
    m_view.show_netlist_view();
    // the charts view is no longer shown
    m_charts_shown = false;
    // refresh toolbar/menu states
    refresh_action_states();
}

void MainWindowPresenter::show_charts_view() {
    // hide the netlist editor and show the charts panel
    m_view.show_charts_view();
    // the charts view is now shown
    m_charts_shown = true;
    // refresh toolbar/menu states
    refresh_action_states();
}

void MainWindowPresenter::show_simulation_output() {
    // re-show the simulation output panel
    m_view.show_simulation_output_panel();
    // the output panel is no longer hidden
    m_simulation_output_hidden = false;
    // refresh toolbar/menu states
    refresh_action_states();
}

void MainWindowPresenter::close_simulation_output() {
    // unsplit the bottom pane to dismiss the simulation output panel
    m_view.hide_simulation_output_panel();
    // the output panel is now hidden
    m_simulation_output_hidden = true;
    // refresh toolbar/menu states
    refresh_action_states();
}

void MainWindowPresenter::configure_simulation() {
    // load the netlist content
    const auto [reloaded, content] = m_netlist_source->load_netlist();
    // check content was reloaded
    if (reloaded)
        update_netlist_editor_content(content, false);
    // parse the netlist and extract the topology
    const auto [sanitized_netlist, topology] = parse_netlist(content);
    // build the simulation config from the parsed directives
    m_simulation_config = SimulationConfig::from_xyce_directives(topology.m_directives);
    // ask the view to show the dialog with the current config
    const auto dialog_config = m_view.show_simulation_parameters_dialog(m_simulation_config);
    // check the dialog was accepted
    if (dialog_config.has_value()) {
        // apply the updated simulation config from the dialog
        m_simulation_config = dialog_config.value();
        // build the directives from the config with topology expansion
        const auto directives = m_simulation_config.to_xyce_directives(topology);
        // merge the directives into the sanitized netlist before .END
        const auto final_netlist = build_final_netlist(sanitized_netlist, directives, topology.m_passthrough_directives);
        // update the editor with the final netlist
        if (update_netlist_editor_content(final_netlist, content != final_netlist))
            refresh_action_states();
        // exit
        return;
    }
    // update states when content was reloaded but the dialog was canceled
    if (reloaded)
        refresh_action_states();
}

void MainWindowPresenter::configure_plugin() {
    // ask the view to show the config dialog with the current plugin config
    const auto dialog_config = m_view.show_plugin_config_dialog(m_plugin_config);
    // apply the updated config when the dialog was accepted
    if (dialog_config.has_value()) {
        // log the update before applying
        spdlog::info("Plugin configuration updated: Xyce path = {}", m_plugin_config.xyce_executable_path());
        // store the updated plugin configuration
        m_plugin_config = dialog_config.value();
    }
}

void MainWindowPresenter::run_simulation() {
    // validate the plugin configuration before launching
    if (!m_plugin_config.is_xyce_executable_valid()) {
        // update the statusbar with an error
        m_view.set_status_text("Configured Xyce executable path is invalid");
        // exit
        return;
    }
    // load the netlist source content
    const auto [reloaded, content] = m_netlist_source->load_netlist();
    // parse the netlist and extract the topology
    const auto [sanitized_netlist, topology] = parse_netlist(content);
    // guard against an empty netlist
    if (sanitized_netlist.empty()) {
        // update the statusbar
        m_view.set_status_text("No netlist content to simulate");
        // update the editor with the final netlist
        if (reloaded && update_netlist_editor_content("", false))
            refresh_action_states();
        // exit
        return;
    }
    // initialize the simulation config from the parsed directives
    m_simulation_config = SimulationConfig::from_xyce_directives(topology.m_directives);
    // prompt the user when no analysis is configured yet
    if (std::holds_alternative<std::monostate>(m_simulation_config.analysis)) {
        // ask the view to show the configure dialog
        const auto dialog_config = m_view.show_simulation_parameters_dialog(m_simulation_config);
        // abort when the dialog was canceled
        if (!dialog_config.has_value()) {
            // restore the editor content
            if (reloaded && update_netlist_editor_content(content, false))
                refresh_action_states();
            // exit
            return;
        }
        // apply the updated simulation config from the dialog
        m_simulation_config = dialog_config.value();
    }
    // build the directives from the config with topology expansion
    const auto directives = m_simulation_config.to_xyce_directives(topology);
    // merge the directives into the sanitized netlist before .END
    const auto final_netlist = build_final_netlist(sanitized_netlist, directives, topology.m_passthrough_directives);
    // update the editor with the final netlist
    if (update_netlist_editor_content(final_netlist, content != final_netlist))
        refresh_action_states();
    // working directory for the netlist source
    const auto working_directory = m_netlist_source->working_directory();
    // create a temporary netlist file for the runner
    const auto temp_path = XyceSimulationRunner::create_temp_netlist(final_netlist);
    // check the temporary netlist was created
    if (temp_path.empty()) {
        // update the statusbar with an error
        m_view.set_status_text("Failed to create temporary netlist file");
        // exit
        return;
    }
    // clear any previous runner
    m_simulation_runner.reset();
    // create a new simulation runner
    m_simulation_runner = std::make_shared<XyceSimulationRunner>();
    // mark the simulation as running
    m_simulation_running = true;
    // show the simulation output panel for this run
    m_view.show_simulation_output_panel();
    m_simulation_output_hidden = false;
    // reset the log for this run
    m_view.clear_simulation_output();
    m_simulation_output_has_content = false;
    // launch the simulation through the view, which wires the wx process events
    m_view.start_simulation_process(m_plugin_config.xyce_executable_path(), temp_path, working_directory);
    // refresh toolbar/menu states
    refresh_action_states();
    // update the statusbar
    m_view.set_status_text("Simulation started...");
}

void MainWindowPresenter::cancel_simulation() {
    // cancel the running simulation when present
    if (m_simulation_runner)
        m_simulation_runner->cancel();
}

void MainWindowPresenter::handle_simulation_finished(int exit_code, bool was_canceled) {
    // mark the simulation as no longer running
    m_simulation_running = false;
    // handle canceled simulations
    if (was_canceled) {
        // update the statusbar
        m_view.set_status_text("Simulation canceled");
        // clear the runner
        m_simulation_runner.reset();
        // refresh toolbar/menu states
        refresh_action_states();
        // exit
        return;
    }
    // check for success
    if (exit_code == 0) {
        // ensure we have a runner reference for accessing paths
        if (!m_simulation_runner) {
            // update the statusbar
            m_view.set_status_text("Simulation finished but runner reference is missing");
            // refresh toolbar/menu states
            refresh_action_states();
            // exit
            return;
        }
        // compute the expected raw output file path
        const auto raw_path = m_simulation_config.raw_output_file_path(m_simulation_runner->working_directory(), m_simulation_runner->netlist_file_path());
        // try to load the raw file when a path was computed and exists
        if (raw_path.has_value() && std::filesystem::exists(*raw_path)) {
            // parse the raw file
            auto raw_file = xyce_raw_file_parser(raw_path->string());
            // check the raw file was parsed
            if (raw_file.has_value()) {
                // update the charts view with the parsed data
                update_xyce_raw_file(std::move(raw_file), true);
                // switch to the charts view
                m_view.show_charts_view();
                m_charts_shown = true;
                // update the window title from the raw file
                set_base_title(m_xyce_raw_file.value()->title());
                // update the statusbar
                m_view.set_status_text("Simulation finished successfully");
                // clear the runner
                m_simulation_runner.reset();
                // refresh toolbar/menu states
                refresh_action_states();
                // exit
                return;
            }
        }
        // raw file not found or failed to parse
        m_view.set_status_text("Simulation finished but output raw file could not be found");
    }
    else {
        // simulation failed with a non-zero exit code
        m_view.set_status_text("Simulation failed (exit code " + std::to_string(exit_code) + ")");
    }
    // clear the runner
    m_simulation_runner.reset();
    // refresh toolbar/menu states
    refresh_action_states();
}

void MainWindowPresenter::handle_simulation_stdout(const std::string& line) {
    // forward the stdout line to the view for display
    m_view.append_simulation_output_line(line);
    // the simulation output log now holds content
    m_simulation_output_has_content = true;
}

void MainWindowPresenter::handle_simulation_stderr(const std::string& line) {
    // log the error line
    spdlog::warn("{}", line);
    // update the statusbar with the latest error line
    m_view.set_status_text("Simulation error: " + line);
}

void MainWindowPresenter::handle_netlist_editor_modified() {
    // track whether the editor still holds content
    m_netlist_has_content = !m_view.netlist_editor_content().empty();
    // mark the editor dirty and refresh states when the flag changed
    if (set_netlist_editor_dirty(true))
        refresh_action_states();
}

void MainWindowPresenter::extract_schematic_netlist() {
    // extract the netlist from the schematic
    const auto [reloaded, content] = m_netlist_source->load_netlist();
    // ensure the editor content matches the schematic netlist
    update_netlist_editor_content(content, false);
    // keep the netlist editor read-only in KiCad plugin mode
    m_view.set_netlist_editor_read_only(true);
    // show the netlist editor over the charts view
    m_view.show_netlist_view();
    m_charts_shown = false;
    // refresh toolbar/menu states
    refresh_action_states();
}

std::shared_ptr<XyceSimulationRunner> MainWindowPresenter::simulation_runner() const {
    // return the active simulation runner
    return m_simulation_runner;
}

const std::optional<std::shared_ptr<XyceOutputFile>>& MainWindowPresenter::raw_file() const {
    // return the current raw file reference
    return m_xyce_raw_file;
}

void MainWindowPresenter::refresh_action_states() {
    // gather the input flags describing the current window state
    ActionStateInput input;
    input.has_netlist = m_netlist_has_content;
    input.has_netlist_file = m_netlist_source != nullptr && !m_netlist_source->is_read_only();
    input.has_raw = m_xyce_raw_file.has_value();
    input.charts_shown = m_charts_shown;
    input.simulation_running = m_simulation_running;
    input.netlist_editor_dirty = m_netlist_editor_dirty;
    input.output_hidden = m_simulation_output_hidden;
    input.log_has_content = m_simulation_output_has_content;
    // derive the current application state
    const AppState state = derive_app_state(input);
    // log state transitions
    if (state != m_app_state) {
        // store the new state
        m_app_state = state;
        // log the transition
        spdlog::debug("Application state changed to {}", app_state_name(state));
    }
    // compute the action enablement for the current state
    const ActionStateEnablement enablement = compute_action_enablement(input);
    // apply the enablement to the view
    m_view.apply_action_enablement(enablement);
}

bool MainWindowPresenter::update_xyce_raw_file(std::optional<std::shared_ptr<XyceOutputFile>> raw_file, bool delete_charts) {
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
        m_view.update_charts(file->expression_manager(), file->step_information(), "", file->abscissa_scale());
        // indicate success
        return true;
    }
    // indicate failure
    return false;
}

void MainWindowPresenter::set_base_title(const std::string& title) {
    // store the clean base title and reset the dirty marker
    m_base_title = title;
    set_netlist_editor_dirty(false);
}

bool MainWindowPresenter::set_netlist_editor_dirty(bool flag) {
    // previous dirty state
    const bool previous_dirty_state = m_netlist_editor_dirty;
    // store the dirty state
    m_netlist_editor_dirty = flag;
    // prefix the base title while the editor holds unsaved changes
    m_view.set_title(flag ? "* " + m_base_title : m_base_title);
    // return true if the dirty state changed, false if it remained the same
    return previous_dirty_state != m_netlist_editor_dirty;
}

bool MainWindowPresenter::update_netlist_editor_content(const std::string& content, bool dirty_flag) {
    // set the editor content through the view and reset the dirty flag
    m_view.set_netlist_editor_content(content, dirty_flag);
    // track whether the editor holds content
    m_netlist_has_content = !content.empty();
    // return whether the dirty flag changed
    return set_netlist_editor_dirty(dirty_flag);
}
