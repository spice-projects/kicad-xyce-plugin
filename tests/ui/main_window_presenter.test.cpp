#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "config/plugin_config.h"
#include "expression/expression_manager.h"
#include "file/xyce_output_file.h"
#include "step_information.h"
#include "ui/editor_netlist_source.h"
#include "ui/main_window_presenter.h"

namespace
{
    // a small netlist backed by an on-disk file, editable through the fake view
    const std::string kNetlist = "R1 1 0 100\n.END\n";

    // guaranteed-executable path used to build a valid plugin config
    const std::string kExecutablePath =
#ifdef _WIN32
        "C:\\Windows\\System32\\cmd.exe";
#else
        "/bin/sh";
#endif

    // fake netlist source tracking the live editor text, backed by a temp file
    std::unique_ptr<NetlistSource> make_source(const std::filesystem::path& path, const std::string& initial) {
        // write the initial content to the backing file
        std::ofstream out(path, std::ios::out | std::ios::trunc);
        out << initial;
        // return an editor source reading the fake editor text
        return std::make_unique<EditorNetlistSource>([]() { return std::string(); }, path);
    }

    std::filesystem::path temp_file_path(const std::string& name) {
        // return a unique temp path for the test
        return std::filesystem::temp_directory_path() / name;
    }
} // namespace

// fake view recording presenter interactions, used by all tests in this file
class FakeMainWindowView : public MainWindowView
{
public:
    void set_title(const std::string& value) override { title = value; }

    void set_status_text(const std::string& value) override { status_text = value; }

    void apply_action_enablement(const ActionStateEnablement& enablement) override {
        // record the enablement and bump the call count
        applied_enablement = enablement;
        apply_count++;
    }

    void set_simulation_running(bool running) override { simulation_running = running; }

    void show_netlist_view() override {
        // switch to the netlist view
        netlist_view_shown = true;
        charts_view_shown = false;
    }

    void show_charts_view() override {
        // switch to the charts view
        charts_view_shown = true;
        netlist_view_shown = false;
    }

    void set_netlist_editor_content(const std::string& value) override { editor_content = value; }

    std::string netlist_editor_content() const override { return editor_content; }

    void set_netlist_editor_read_only(bool read_only) override { editor_read_only = read_only; }

    bool charts_shown() const override { return charts_view_shown; }

    void show_simulation_output_panel() override { output_panel_hidden = false; }

    void hide_simulation_output_panel() override { output_panel_hidden = true; }

    void clear_simulation_output() override { output_content.clear(); }

    void append_simulation_output_line(const std::string& line) override { output_content += line + "\n"; }

    bool simulation_output_panel_hidden() const override { return output_panel_hidden; }

    bool simulation_output_has_content() const override { return !output_content.empty(); }

    void update_charts(ExpressionManager&, const StepInformation&, AbscissaScale, const std::vector<std::vector<std::string>>& suggested_plots) override {
        // record the suggested plots and count the chart updates
        update_charts_suggested_plots = suggested_plots;
        update_charts_calls++;
    }

    void delete_all_charts() override {
        // count the chart deletions
        delete_all_charts_calls++;
    }

    void set_open_fft_calculation_files(const std::vector<std::shared_ptr<XyceOutputFile>>& files) override {
        // record the forwarded FFT calculation files
        open_fft_calculation_files = files;
    }

    std::optional<SimulationConfig> show_simulation_parameters_dialog(const SimulationConfig&) override { return simulation_parameters_result; }

    std::optional<PluginConfig> show_plugin_config_dialog(const PluginConfig&) override { return plugin_config_result; }

    void start_simulation_process(const std::string& program, const std::filesystem::path&, const std::filesystem::path&) override {
        // record the launch without starting a real process
        start_process_calls++;
        start_program = program;
    }

    void cancel_simulation_process() override {
        // count the cancel requests
        cancel_process_calls++;
    }

    void spawn_raw_file_window(std::shared_ptr<XyceOutputFile>) override {
        // count the spawned raw-file windows
        spawn_raw_file_calls++;
    }

    // configurable canned dialog results
    std::optional<SimulationConfig> simulation_parameters_result;
    std::optional<PluginConfig> plugin_config_result;

    // recorded state
    std::string title;
    std::string status_text;
    std::string editor_content;
    std::string output_content;
    std::string start_program;
    bool editor_read_only = false;
    bool netlist_view_shown = false;
    bool charts_view_shown = false;
    bool output_panel_hidden = true;
    bool simulation_running = false;
    int apply_count = 0;
    int update_charts_calls = 0;
    int delete_all_charts_calls = 0;
    int start_process_calls = 0;
    int cancel_process_calls = 0;
    int spawn_raw_file_calls = 0;
    ActionStateEnablement applied_enablement;
    std::vector<std::vector<std::string>> update_charts_suggested_plots;
    std::vector<std::shared_ptr<XyceOutputFile>> open_fft_calculation_files;
};

// ========================================================================================
// opening files
// ========================================================================================

TEST(MainWindowPresenterChecks, open_cir_updates_title_shows_netlist_and_stays_clean) {
    // arrange
    const auto path = temp_file_path("presenter_open.cir");
    std::ofstream(path) << kNetlist;
    FakeMainWindowView view;
    MainWindowPresenter presenter(view, nullptr, PluginConfig());
    // act
    presenter.open_netlist_file(path);
    // assert
    EXPECT_EQ(view.title, path.filename().string());
    EXPECT_TRUE(view.netlist_view_shown);
    EXPECT_FALSE(view.charts_view_shown);
    EXPECT_EQ(view.editor_content, kNetlist);
    EXPECT_EQ(view.title.find("* "), std::string::npos);
    EXPECT_FALSE(view.applied_enablement.save);
    EXPECT_TRUE(view.applied_enablement.open);
    EXPECT_TRUE(view.applied_enablement.run_simulation);
    EXPECT_TRUE(view.applied_enablement.configure_simulation);
    // cleanup
    std::filesystem::remove(path);
}

// ========================================================================================
// saving / dirty tracking
// ========================================================================================

TEST(MainWindowPresenterChecks, edit_then_save_clears_dirty_and_writes_file) {
    // arrange
    const auto path = temp_file_path("presenter_save.cir");
    FakeMainWindowView view;
    MainWindowPresenter presenter(view, make_source(path, kNetlist), PluginConfig());
    presenter.open_netlist_file(path);
    // simulate an edit through the view
    view.editor_content = "R1 1 0 200\n.END\n";
    view.title = path.filename().string();
    // act
    presenter.handle_netlist_editor_modified();
    // assert dirty marker is applied
    EXPECT_EQ(view.title, "* " + path.filename().string());
    EXPECT_TRUE(view.applied_enablement.save);
    // act
    presenter.save_netlist();
    // assert the file was written and the dirty marker cleared
    {
        std::ifstream in(path);
        std::string written((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        EXPECT_EQ(written, "R1 1 0 200\n.END\n");
    }
    EXPECT_EQ(view.title, path.filename().string());
    EXPECT_EQ(view.title.find("* "), std::string::npos);
    EXPECT_FALSE(view.applied_enablement.save);
    // cleanup
    std::filesystem::remove(path);
}

// ========================================================================================
// configure simulation
// ========================================================================================

TEST(MainWindowPresenterChecks, configure_simulation_accepts_dialog_and_rebuilds_netlist) {
    // arrange
    const auto path = temp_file_path("presenter_configure.cir");
    FakeMainWindowView view;
    MainWindowPresenter presenter(view, make_source(path, kNetlist), PluginConfig());
    presenter.open_netlist_file(path);
    const auto accepted = SimulationConfig::from_xyce_directives({".TRAN 1u 1m"});
    view.simulation_parameters_result = accepted;
    // act
    presenter.configure_simulation();
    // assert
    EXPECT_TRUE(view.editor_content.find("TRAN") != std::string::npos);
    EXPECT_EQ(view.title, "* " + path.filename().string());
    // cleanup
    std::filesystem::remove(path);
}

TEST(MainWindowPresenterChecks, configure_simulation_canceled_leaves_netlist_unchanged) {
    // arrange
    const auto path = temp_file_path("presenter_configure_cancel.cir");
    FakeMainWindowView view;
    MainWindowPresenter presenter(view, make_source(path, kNetlist), PluginConfig());
    presenter.open_netlist_file(path);
    view.simulation_parameters_result = std::nullopt;
    // act
    presenter.configure_simulation();
    // assert
    EXPECT_EQ(view.editor_content, kNetlist);
    EXPECT_EQ(view.start_process_calls, 0);
    // cleanup
    std::filesystem::remove(path);
}

// ========================================================================================
// run simulation
// ========================================================================================

TEST(MainWindowPresenterChecks, run_simulation_with_invalid_executable_sets_status_and_does_not_start) {
    // arrange
    FakeMainWindowView view;
    MainWindowPresenter presenter(view, nullptr, PluginConfig());
    // act
    presenter.run_simulation();
    // assert
    EXPECT_EQ(view.status_text, "Configured Xyce executable path is invalid");
    EXPECT_EQ(view.start_process_calls, 0);
}

TEST(MainWindowPresenterChecks, run_simulation_with_no_analysis_prompts_and_aborts_when_canceled) {
    // arrange
    const auto path = temp_file_path("presenter_no_analysis.cir");
    FakeMainWindowView view;
    MainWindowPresenter presenter(view, make_source(path, kNetlist), PluginConfig(kExecutablePath));
    view.simulation_parameters_result = std::nullopt;
    // act
    presenter.run_simulation();
    // assert
    EXPECT_EQ(view.start_process_calls, 0);
    EXPECT_TRUE(view.status_text.empty());
    // cleanup
    std::filesystem::remove(path);
}

// ========================================================================================
// simulation finished
// ========================================================================================

TEST(MainWindowPresenterChecks, simulation_finished_canceled_sets_status) {
    // arrange
    FakeMainWindowView view;
    MainWindowPresenter presenter(view, nullptr, PluginConfig());
    // act
    presenter.handle_simulation_finished(0, true);
    // assert
    EXPECT_EQ(view.status_text, "Simulation canceled");
}

TEST(MainWindowPresenterChecks, simulation_finished_nonzero_sets_failure_status) {
    // arrange
    FakeMainWindowView view;
    MainWindowPresenter presenter(view, nullptr, PluginConfig());
    // act
    presenter.handle_simulation_finished(3, false);
    // assert
    EXPECT_EQ(view.status_text, "Simulation failed (exit code 3)");
}

TEST(MainWindowPresenterChecks, simulation_finished_success_without_runner_sets_missing_status) {
    // arrange
    FakeMainWindowView view;
    MainWindowPresenter presenter(view, nullptr, PluginConfig());
    // act
    presenter.handle_simulation_finished(0, false);
    // assert
    EXPECT_EQ(view.status_text, "Simulation finished but runner reference is missing");
}

// ========================================================================================
// raw file loading
// ========================================================================================

TEST(MainWindowPresenterChecks, load_raw_file_shows_charts_and_sets_title) {
    // arrange
    std::vector<std::string> keys;
    std::vector<std::vector<double>> values;
    std::vector<std::pair<double, double>> ranges;
    StepInformation step_info(keys, values, ranges);
    ExpressionManager manager;
    auto raw_file = std::make_shared<XyceOutputFile>("/tmp/presenter.raw", "Raw Title", false, std::move(step_info), PlotType::UNKNOWN, AbscissaScale::LINEAR, std::move(manager), nullptr);
    FakeMainWindowView view;
    MainWindowPresenter presenter(view, nullptr, PluginConfig());
    // act
    presenter.load_raw_file(raw_file);
    // assert
    EXPECT_TRUE(view.charts_view_shown);
    EXPECT_FALSE(view.netlist_view_shown);
    EXPECT_EQ(view.title, "Raw Title");
    EXPECT_EQ(view.update_charts_calls, 1);
    EXPECT_EQ(view.delete_all_charts_calls, 1);
    EXPECT_TRUE(presenter.raw_file().has_value());
}

TEST(MainWindowPresenterChecks, load_raw_file_forwards_suggested_plots_to_charts) {
    // arrange
    const std::vector<std::vector<std::string>> expected_suggested_plots = {{"V(out)", "I(R1)"}, {"V(in)"}};
    std::vector<std::string> keys;
    std::vector<std::vector<double>> values;
    std::vector<std::pair<double, double>> ranges;
    StepInformation step_info(keys, values, ranges);
    ExpressionManager manager;
    auto raw_file = std::make_shared<XyceOutputFile>("/tmp/presenter.raw", "Raw Title", false, std::move(step_info), PlotType::UNKNOWN, AbscissaScale::LINEAR, std::move(manager), nullptr, expected_suggested_plots);
    FakeMainWindowView view;
    MainWindowPresenter presenter(view, nullptr, PluginConfig());
    // act
    presenter.load_raw_file(raw_file);
    // assert
    EXPECT_EQ(view.update_charts_calls, 1);
    EXPECT_EQ(view.update_charts_suggested_plots, expected_suggested_plots);
}

// ========================================================================================
// action enablement propagation
// ========================================================================================

TEST(MainWindowPresenterChecks, editing_after_open_enables_save_via_enablement) {
    // arrange
    const auto path = temp_file_path("presenter_state.cir");
    FakeMainWindowView view;
    MainWindowPresenter presenter(view, make_source(path, kNetlist), PluginConfig());
    presenter.open_netlist_file(path);
    view.editor_content = "edited";
    view.title = path.filename().string();
    // act
    presenter.handle_netlist_editor_modified();
    // assert
    EXPECT_TRUE(view.applied_enablement.save);
    EXPECT_FALSE(view.applied_enablement.show_charts);
    EXPECT_TRUE(view.applied_enablement.configure_simulation);
    EXPECT_TRUE(view.applied_enablement.run_simulation);
    // cleanup
    std::filesystem::remove(path);
}

// ========================================================================================
// configure plugin
// ========================================================================================

TEST(MainWindowPresenterChecks, configure_plugin_accepting_valid_executable_is_used_by_run) {
    // arrange
    const auto path = temp_file_path("presenter_plugin.cir");
    FakeMainWindowView view;
    MainWindowPresenter presenter(view, make_source(path, kNetlist), PluginConfig());
    view.plugin_config_result = PluginConfig(kExecutablePath);
    // act
    presenter.configure_plugin();
    // run now passes the executable guard and reaches the analysis prompt instead
    presenter.run_simulation();
    // assert
    EXPECT_NE(view.status_text, "Configured Xyce executable path is invalid");
    EXPECT_EQ(view.start_process_calls, 0);
    // cleanup
    std::filesystem::remove(path);
}

// ========================================================================================
// run simulation — empty netlist
// ========================================================================================

TEST(MainWindowPresenterChecks, run_simulation_with_empty_netlist_prompts_and_aborts) {
    // arrange
    const auto path = temp_file_path("presenter_empty.cir");
    FakeMainWindowView view;
    MainWindowPresenter presenter(view, make_source(path, ""), PluginConfig(kExecutablePath));
    // act
    presenter.run_simulation();
    // assert (empty content parses to no analysis directive, so the run prompts and aborts without starting)
    EXPECT_EQ(view.start_process_calls, 0);
    EXPECT_TRUE(view.status_text.empty());
    // cleanup
    std::filesystem::remove(path);
}

// ========================================================================================
// extract schematic netlist
// ========================================================================================

TEST(MainWindowPresenterChecks, extract_schematic_netlist_sets_read_only_editor) {
    // arrange
    const auto path = temp_file_path("presenter_extract.cir");
    std::ofstream(path) << kNetlist;
    FakeMainWindowView view;
    MainWindowPresenter presenter(view, make_source(path, kNetlist), PluginConfig());
    // act
    presenter.extract_schematic_netlist();
    // assert
    EXPECT_TRUE(view.editor_read_only);
    EXPECT_TRUE(view.netlist_view_shown);
    EXPECT_FALSE(view.charts_view_shown);
    EXPECT_EQ(view.editor_content, kNetlist);
    // cleanup
    std::filesystem::remove(path);
}

// ========================================================================================
// simulation output forwarding
// ========================================================================================

TEST(MainWindowPresenterChecks, simulation_stdout_forwarded_to_output) {
    // arrange
    FakeMainWindowView view;
    MainWindowPresenter presenter(view, nullptr, PluginConfig());
    // act
    presenter.handle_simulation_stdout("line one");
    presenter.handle_simulation_stdout("line two");
    // assert
    EXPECT_EQ(view.output_content, "line one\nline two\n");
}

TEST(MainWindowPresenterChecks, simulation_stderr_sets_status) {
    // arrange
    FakeMainWindowView view;
    MainWindowPresenter presenter(view, nullptr, PluginConfig());
    // act
    presenter.handle_simulation_stderr("boom");
    // assert
    EXPECT_EQ(view.status_text, "Simulation error: boom");
}

// ========================================================================================
// raw file state reset
// ========================================================================================

TEST(MainWindowPresenterChecks, open_netlist_file_clears_previous_raw_file) {
    // arrange
    std::vector<std::string> keys;
    std::vector<std::vector<double>> values;
    std::vector<std::pair<double, double>> ranges;
    StepInformation step_info(keys, values, ranges);
    ExpressionManager manager;
    auto raw_file = std::make_shared<XyceOutputFile>("/tmp/presenter_reset.raw", "Raw Title", false, std::move(step_info), PlotType::UNKNOWN, AbscissaScale::LINEAR, std::move(manager), nullptr);
    const auto path = temp_file_path("presenter_reset.cir");
    std::ofstream(path) << kNetlist;
    FakeMainWindowView view;
    MainWindowPresenter presenter(view, make_source(path, kNetlist), PluginConfig());
    presenter.load_raw_file(raw_file);
    ASSERT_TRUE(view.charts_view_shown);
    ASSERT_TRUE(presenter.raw_file().has_value());
    // act
    presenter.open_netlist_file(path);
    // assert
    EXPECT_TRUE(view.netlist_view_shown);
    EXPECT_FALSE(view.charts_view_shown);
    EXPECT_FALSE(presenter.raw_file().has_value());
    // cleanup
    std::filesystem::remove(path);
}
