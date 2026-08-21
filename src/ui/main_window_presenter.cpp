#include <algorithm>
#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <utility>

#include <spdlog/spdlog.h>

#include "../dsp/fft.h"
#include "../io/xyce_fft_file.h"
#include "../io/xyce_raw_file.h"
#include "../kicad/kicad_session.h"
#include "../netlist/editor_netlist_source.h"
#include "../netlist/netlist.h"
#include "../simulation/simulation_config.h"
#include "main_window_presenter.h"
#include "main_window_state.h"
#include "simulation_runner.h"

SlintMainWindowPresenter2::SlintMainWindowPresenter2(MainWindowViewDef& view, std::unique_ptr<NetlistSource> netlist_source, PluginConfig plugin_config, std::shared_ptr<KiCadSession> kicad_session) :
    m_view(view), m_kicad_session(std::move(kicad_session)), m_netlist_source(std::move(netlist_source)), m_simulation_config(SimulationConfig::from_xyce_directives({})), m_plugin_config(std::move(plugin_config)) {
    // initialize the toolbar action states before the window is shown
    refresh_action_states();
}

SlintMainWindowPresenter2::~SlintMainWindowPresenter2() = default;

void SlintMainWindowPresenter2::on_open_xyce_file(const std::filesystem::path& path) {
    // analyze the file extension
    const auto extension = path.extension().string();
    // netlist file extension
    if (extension == ".cir") {
        // create an editor netlist source backed by the selected file, reading
        // live editor text
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
        // remove the parsed FFT calculation files, they belong to a previous run
        m_fft_files.clear();
        // show the netlist view over the charts view
        m_view.show_netlist_view();
        // refresh toolbar/menu states
        refresh_action_states();
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
    // save content in the netlist source
    m_netlist_source->save_netlist();
    // reset the dirty flag and refresh states when it changed
    if (set_netlist_editor_dirty(false))
        refresh_action_states();
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

void SlintMainWindowPresenter2::on_close_simulation_output() {
    // hide the simulation output panel
    m_view.hide_simulation_output_panel();
    // refresh toolbar/menu states
    refresh_action_states();
}

void SlintMainWindowPresenter2::on_run_simulation() {
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
    // remember the parse result for the launch
    m_pending_sanitized_netlist = sanitized_netlist;
    m_pending_topology = topology;
    m_pending_original_netlist = content;
    // prompt the user when no analysis is configured yet
    if (std::holds_alternative<std::monostate>(m_simulation_config.analysis)) {
        // mark the pending dialog as a simulation run
        m_run_pending = true;
        // ask the view to show the configure dialog; the accepted configuration
        // is delivered through on_simulation_parameters_dialog_result
        static_cast<void>(m_view.show_simulation_parameters_dialog(m_simulation_config));
        // exit
        return;
    }
    // launch the simulation with the configured analysis
    launch_simulation();
}

void SlintMainWindowPresenter2::on_cancel_simulation() {
    // request the view to cancel the running simulation process
    m_view.cancel_simulation_process();
}

void SlintMainWindowPresenter2::launch_simulation() {
    // build the directives from the config with topology expansion
    const auto directives = m_simulation_config.to_xyce_directives(m_pending_topology);
    // merge the directives into the sanitized netlist before .END
    const auto final_netlist = build_final_netlist(m_pending_sanitized_netlist, directives, m_pending_topology.m_passthrough_directives);
    // update the editor with the final netlist
    if (update_netlist_editor_content(final_netlist, m_pending_original_netlist != final_netlist))
        refresh_action_states();
    // working directory for the netlist source
    const auto working_directory = m_netlist_source->working_directory();
    // create a temporary netlist file for the runner
    const auto temp_path = SimulationRunner::create_temp_netlist(final_netlist);
    // check the temporary netlist was created
    if (temp_path.empty()) {
        // update the statusbar with an error
        m_view.set_status_text("Failed to create temporary netlist file");
        // exit
        return;
    }
    // clear the parsed FFT calculation files, they belong to the previous run
    m_fft_files.clear();
    // remember the run paths for the finished handler; the view owns the runner
    m_simulation_working_directory = working_directory;
    m_simulation_netlist_path = temp_path;
    // mark the simulation as running
    m_simulation_running = true;
    // show the simulation output panel for this run
    m_view.show_simulation_output_panel();
    // reset the log for this run
    m_view.clear_simulation_output();
    // launch the simulation through the view, which wires the runner
    m_view.start_simulation_process(m_plugin_config.xyce_executable_path(), temp_path, working_directory);
    // refresh toolbar/menu states
    refresh_action_states();
    // update the statusbar
    m_view.set_status_text("Simulation started...");
}

void SlintMainWindowPresenter2::on_configure_simulation() {
    // load the netlist content
    const auto [reloaded, content] = m_netlist_source->load_netlist();
    // check content was reloaded
    if (reloaded)
        update_netlist_editor_content(content, false);
    // parse the netlist and extract the topology
    const auto [sanitized_netlist, topology] = parse_netlist(content);
    // build the simulation config from the parsed directives
    m_simulation_config = SimulationConfig::from_xyce_directives(topology.m_directives);
    // remember the parse result so the accepted config can rebuild the netlist
    m_pending_sanitized_netlist = sanitized_netlist;
    m_pending_topology = topology;
    m_pending_original_netlist = content;
    // this dialog is a configure operation, not a pending simulation run
    m_run_pending = false;
    // ask the view to show the dialog with the current config; the accepted
    // configuration is delivered through on_simulation_parameters_dialog_result
    static_cast<void>(m_view.show_simulation_parameters_dialog(m_simulation_config));
}

void SlintMainWindowPresenter2::on_configure_plugin() {
    // the view owns the config dialog; it seeds it with the current config and
    // reports the accepted result back through on_plugin_config_dialog_result
    static_cast<void>(m_view.show_plugin_config_dialog(m_plugin_config));
}

void SlintMainWindowPresenter2::on_plugin_config_dialog_result(const PluginConfig& config) {
    // log the update before applying
    spdlog::info("Plugin configuration updated: Xyce path = {}", m_plugin_config.xyce_executable_path());
    // store the updated plugin configuration
    m_plugin_config = config;
}

void SlintMainWindowPresenter2::on_simulation_parameters_dialog_result(const SimulationConfig& config) {
    // store the updated simulation configuration
    m_simulation_config = config;
    // resume a pending simulation run when the dialog was opened from run
    if (m_run_pending) {
        // launch the simulation with the configured analysis
        launch_simulation();
        // exit
        return;
    }
    // build the directives from the config with topology expansion
    const auto directives = m_simulation_config.to_xyce_directives(m_pending_topology);
    // merge the directives into the sanitized netlist before .END
    const auto final_netlist = build_final_netlist(m_pending_sanitized_netlist, directives, m_pending_topology.m_passthrough_directives);
    // update the editor with the final netlist
    if (update_netlist_editor_content(final_netlist, m_pending_original_netlist != final_netlist))
        refresh_action_states();
}

void SlintMainWindowPresenter2::on_fft_dialog_result(std::vector<AnyExpression*> selected_expressions, const fft::FftParameters& fft_params) {
    // the transform needs the charts data (expression manager and step
    // information) from the raw file loaded in this window
    if (!m_xyce_raw_file.has_value())
        return;
    auto& file = m_xyce_raw_file.value();
    ExpressionManager& expression_manager = file->expression_manager();
    const StepInformation& step_information = file->step_information();
    // validate the expression selection
    if (selected_expressions.empty()) {
        m_view.set_status_text("No expressions selected for FFT");
        return;
    }
    // from/to abscissa values chosen in the dialog
    const double from_abscissa_value = fft_params.start;
    const double to_abscissa_value = fft_params.stop;
    // list of frequency bins for each step, to be concatenated across steps later
    std::vector<std::vector<double>> frequency_chunks;
    // fft data chunks for each expression, to be concatenated across steps later
    std::vector<std::vector<std::vector<double>>> fft_chunks(selected_expressions.size());
    // processed step indices and abscissa slices
    std::vector<size_t> fft_steps;
    std::vector<std::pair<size_t, size_t>> fft_abscissa_indices;
    std::vector<std::pair<double, double>> fft_abscissa_value_ranges;
    // fft step index offset
    size_t fft_offset = 0;
    // loop steps
    for (size_t step = 0; step < step_information.length(); ++step) {
        // abscissa values for this step — zero copy per-step view
        std::span<const double> step_abscissa = expression_manager.abscissa().step_data(step);
        // find the indices corresponding to the selected abscissa range
        auto it_left = std::lower_bound(step_abscissa.begin(), step_abscissa.end(), from_abscissa_value);
        auto it_right = std::upper_bound(step_abscissa.begin(), step_abscissa.end(), to_abscissa_value);
        // from and to indices for the selected abscissa range (inclusive of from, exclusive of to)
        const size_t from_index = static_cast<size_t>(std::distance(step_abscissa.begin(), it_left));
        const size_t to_index = static_cast<size_t>(std::distance(step_abscissa.begin(), it_right));
        // require at least 2 samples
        if (to_index - from_index < 2) {
            spdlog::warn("Skipping FFT for step {}: selected range has fewer than 2 samples", step);
            continue;
        }
        // expressions in this step
        std::vector<std::span<const double>> y_matrix;
        y_matrix.reserve(selected_expressions.size());
        for (AnyExpression* expression : selected_expressions) {
            // only real-valued expressions are eligible for the transform
            if (std::holds_alternative<Expression<double>>(*expression)) {
                auto& double_expr = std::get<Expression<double>>(*expression);
                const auto y_data = double_expr.step_data(step);
                y_matrix.push_back(y_data.subspan(from_index, to_index - from_index));
            }
        }
        // skip empty matrices (should not happen since the dialog filters real expressions)
        if (y_matrix.empty())
            continue;
        // extract the x interval for the FFT
        const auto x_interval = step_abscissa.subspan(from_index, to_index - from_index);
        try {
            // compute the FFT; all expressions in y_matrix are processed together for this step
            auto result = fft::compute_fft_many(x_interval, y_matrix, fft_params.np, fft_params.window, fft_params.format, 0, x_interval.size() - 1, fft_params.output, fft_params.keep_dc);
            // check the frequency axis is not empty
            if (result.frequencies.empty()) {
                spdlog::error("FFT computation returned an empty frequency axis for step {}", step);
                m_view.set_status_text("FFT computation failed");
                return;
            }
            // remember the first and last frequency bins before moving the chunks
            const double first_frequency = result.frequencies.front();
            const double last_frequency = result.frequencies.back();
            const size_t chunk_size = result.frequencies.size();
            // store the step output slice
            fft_abscissa_indices.emplace_back(fft_offset, fft_offset + chunk_size);
            // update the offset
            fft_offset += chunk_size;
            // store the frequency bins and per-expression values
            frequency_chunks.push_back(std::move(result.frequencies));
            for (size_t i = 0; i < result.values.size(); ++i)
                fft_chunks[i].push_back(std::move(result.values[i]));
            // append the step and its abscissa value range
            fft_steps.push_back(step);
            fft_abscissa_value_ranges.emplace_back(first_frequency, last_frequency);
        }
        catch (const std::exception& e) {
            spdlog::error("FFT computation failed for step {}: {}", step, e.what());
            m_view.set_status_text("FFT computation failed");
            return;
        }
    }
    // require at least one processed step
    if (fft_steps.empty()) {
        spdlog::warn("FFT computation skipped: no step has at least 2 samples in the selected range");
        m_view.set_status_text("FFT computation skipped: no data in the selected range");
        return;
    }
    // build the expression name for the title
    std::string fft_title = "FFT - ";
    for (size_t i = 0; i < selected_expressions.size(); ++i) {
        // append separator
        if (i > 0)
            fft_title += ", ";
        // append the expression name
        fft_title += std::get<Expression<double>>(*selected_expressions[i]).name();
    }
    // build FFT expressions using the Expression<double> constructor with step slices
    std::vector<AnyExpression> fft_expressions;
    {
        // create flat frequency data
        std::vector<double> freq_data;
        freq_data.reserve(fft_offset);
        // concatenate the frequency chunks across steps
        for (const auto& chunk : frequency_chunks)
            freq_data.insert(freq_data.end(), chunk.begin(), chunk.end());
        // append the expression for the frequency abscissa with unit "Hz"
        fft_expressions.emplace_back(Expression<double>("Frequency", std::move(freq_data), fft_abscissa_indices, "Hz"));
    }
    // suggested plots
    std::vector<std::vector<std::string>> suggested_plots;
    for (size_t i = 0; i < selected_expressions.size(); ++i) {
        // current expression
        auto& real_expression = std::get<Expression<double>>(*selected_expressions[i]);
        // determine the unit
        std::string unit;
        if (fft_params.output == fft::FftOutput::PHASE)
            unit = "°";
        else if (fft_params.output == fft::FftOutput::MAGNITUDE_DB)
            unit = "dB";
        else
            unit = real_expression.unit();
        // expression name for the FFT result
        const auto expr_name = "FFT(" + real_expression.name() + ")";
        // create flat data
        std::vector<double> data;
        data.reserve(fft_offset);
        // concatenate the FFT chunks across steps
        for (const auto& chunk : fft_chunks[i])
            data.insert(data.end(), chunk.begin(), chunk.end());
        // suggest a maximum of three expressions for plotting
        if (suggested_plots.size() < 3)
            suggested_plots.push_back({expr_name});
        // append the expression for the FFT result with the step slices
        fft_expressions.emplace_back(Expression<double>(expr_name, std::move(data), fft_abscissa_indices, unit));
    }
    // step information for the FFT output (only include the processed steps)
    StepInformation fft_step_information(step_information.keys(), step_information.values(), fft_abscissa_value_ranges);
    // build the expression manager (moves from the expression list)
    ExpressionManager fft_expression_manager(fft_expressions, fft_abscissa_indices);
    // create a raw file with the FFT results and spawn a result window
    auto fft_raw = std::make_shared<XyceOutputFile>("", fft_title, false, std::move(fft_step_information), PlotType::FFT, AbscissaScale::LINEAR, std::move(fft_expression_manager), nullptr, suggested_plots);
    m_view.spawn_raw_file_window(std::move(fft_raw));
}

void SlintMainWindowPresenter2::on_chart_calculate_fft(size_t chart_index) {
    // the FFT dialog needs the charts data (expression manager and step
    // information) from the raw file loaded in this window
    if (!m_xyce_raw_file.has_value())
        return;
    // ask the view to show the FFT setup dialog for the chart; the accepted
    // expressions and parameters are delivered through on_fft_dialog_result
    m_view.show_fft_dialog(chart_index);
}

void SlintMainWindowPresenter2::on_chart_open_xyce_fft_calculation(size_t) {
    // open a new window for each parsed FFT calculation output file
    for (const auto& fft_file : m_fft_files)
        m_view.spawn_raw_file_window(fft_file);
}

void SlintMainWindowPresenter2::on_chart_step_tool(size_t chart_index) {
    // the step tool needs the charts data (step information) from the raw file
    // loaded in this window
    if (!m_xyce_raw_file.has_value())
        return;
    // ask the view to show the step tool dialog for the chart; the accepted
    // step selection is applied back to the chart through the renderer
    m_view.show_step_tool_dialog(chart_index);
}

void SlintMainWindowPresenter2::on_chart_new_window(size_t) {
    // spawn a new window seeded with the current raw file
    if (m_xyce_raw_file.has_value())
        m_view.spawn_raw_file_window(m_xyce_raw_file.value());
}

void SlintMainWindowPresenter2::load_raw_file(std::shared_ptr<XyceOutputFile> raw_file) {
    // store the already-parsed raw file; the parsed FFT calculation files belong
    // to the previous run
    m_fft_files.clear();
    // update the charts with the raw file data and switch to the charts view
    if (update_xyce_raw_file(std::optional<std::shared_ptr<XyceOutputFile>>(std::move(raw_file)), true))
        show_raw_file_view();
}

void SlintMainWindowPresenter2::on_simulation_finished(int exit_code, bool was_canceled) {
    // mark the simulation as no longer running
    m_simulation_running = false;
    // handle canceled simulations
    if (was_canceled) {
        // update the statusbar
        m_view.set_status_text("Simulation canceled");
        // refresh toolbar/menu states
        refresh_action_states();
        // exit
        return;
    }
    // check for success
    if (exit_code == 0) {
        // compute the expected raw output file path
        const auto raw_path = m_simulation_config.raw_output_file_path(m_simulation_working_directory, m_simulation_netlist_path);
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
                // update the window title from the raw file
                set_base_title(m_xyce_raw_file.value()->title());
                // update the statusbar
                m_view.set_status_text("Simulation finished successfully");
                // parse the FFT calculation output files produced by this run, derived from the analysis config
                m_fft_files.clear();
                if (const auto fft_pattern = m_simulation_config.fft_output_file_path_pattern(m_simulation_netlist_path); fft_pattern.has_value()) {
                    // raw file instance
                    const auto& raw_file_instance = m_xyce_raw_file.value();
                    // parse the matching FFT output files
                    if (auto parsed_files = xyce_fft_file_parser(*fft_pattern, raw_file_instance->step_information(), &raw_file_instance->expression_manager()))
                        m_fft_files = std::move(*parsed_files);
                    // log the number of loaded FFT files
                    spdlog::info("Loaded {} Xyce FFT calculation file(s)", m_fft_files.size());
                }
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
    // refresh toolbar/menu states
    refresh_action_states();
}

void SlintMainWindowPresenter2::on_simulation_stdout(const std::string& line) {
    // forward the stdout line to the view for display
    m_view.append_simulation_output_line(line);
}

void SlintMainWindowPresenter2::on_simulation_stderr(const std::string& line) {
    // log the error line
    spdlog::warn("{}", line);
    // append the error line to the simulation output log so the full Xyce log
    // (stdout and stderr) is visible in the output panel
    m_view.append_simulation_output_line(line);
    // update the statusbar with the latest error line
    m_view.set_status_text("Simulation error: " + line);
}

void SlintMainWindowPresenter2::on_netlist_editor_modified() {
    // track whether the editor still holds content
    m_netlist_has_content = !m_view.netlist_editor_content().empty();
    // mark the editor dirty and refresh states when the flag changed
    if (set_netlist_editor_dirty(true))
        refresh_action_states();
}

void SlintMainWindowPresenter2::on_extract_schematic_netlist() {
    // extract the netlist from the schematic through the session-backed source
    const auto [reloaded, content] = m_netlist_source->load_netlist();
    // ensure the editor content matches the schematic netlist
    update_netlist_editor_content(content, false);
    // keep the netlist editor read-only in KiCad plugin mode
    m_view.set_netlist_editor_read_only(true);
    // show the netlist editor over the charts view
    m_view.show_netlist_view();
    // refresh toolbar/menu states
    refresh_action_states();
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
    input.simulation_running = m_simulation_running;
    input.netlist_editor_dirty = m_netlist_editor_dirty;
    input.output_hidden = m_view.simulation_output_panel_hidden();
    input.log_has_content = m_view.simulation_output_has_content();
    // chart context tools are tied to the loaded raw output
    input.abscissa_is_time = m_xyce_raw_file.has_value() && m_xyce_raw_file.value()->expression_manager().abscissa().unit() == "s";
    input.has_steps = m_xyce_raw_file.has_value() && m_xyce_raw_file.value()->step_information().length() > 1;
    // compute the action enablement for the current state
    ActionStateEnablement enablement = compute_action_enablement(input);
    // file actions are only available in standalone mode; KiCad provides the
    // netlist, so there is no file to load/save when connected
    if (m_kicad_session != nullptr) {
        enablement.open = false;
        enablement.save = false;
    }
    // apply the enablement to the view
    m_view.apply_action_enablement(enablement);
    // drive the Run/Stop toolbar toggle from the simulation state
    m_view.set_simulation_running(m_simulation_running);
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
