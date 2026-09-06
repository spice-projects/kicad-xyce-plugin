#include <filesystem>
#include <memory>
#include <optional>
#include <string>

#include <gtest/gtest.h>

#include "app/app.h"
#include "config/plugin_config.h"
#include "core/step_information.h"
#include "dsp/fft.h"
#include "expression/expression.h"
#include "expression/expression_manager.h"
#include "io/xyce_output_file.h"
#include "netlist/editor_netlist_source.h"
#include "netlist/netlist_source.h"
#include "simulation/simulation_config.h"
#include "ui/main_window_presenter.h"
#include "ui/main_window_view_def.h"

namespace
{
    // RecordingView captures presenter interactions without requiring
    // a Slint runtime
    class RecordingView : public MainWindowViewDef
    {
    public:
        void set_title(const std::string&) override {}
        void set_status_text(const std::string&) override {}
        void apply_action_enablement(const ActionStateEnablement&) override {}
        void set_simulation_running(bool) override {}
        void show_netlist_view() override {}
        void show_charts_view() override {}
        void set_netlist_editor_content(const std::string&) override {}
        std::string netlist_editor_content() const override { return {}; }
        void set_netlist_editor_read_only(bool) override {}
        bool charts_shown() const override { return false; }
        void show_simulation_output_panel() override {}
        void hide_simulation_output_panel() override {}
        void clear_simulation_output() override {}
        void append_simulation_output_line(const std::string&) override {}
        bool simulation_output_panel_hidden() const override { return true; }
        bool simulation_output_has_content() const override { return false; }
        void update_charts(ExpressionManager&, const StepInformation&, AbscissaScale, const std::vector<std::vector<std::string>>&) override {}
        void delete_all_charts() override {}
        void set_open_fft_calculation_files(const std::vector<std::shared_ptr<XyceOutputFile>>&) override {}
        void show_fft_dialog(size_t) override {}
        void show_step_tool_dialog(size_t) override {}
        std::optional<SimulationConfig> show_simulation_parameters_dialog(const SimulationConfig&) override { return std::nullopt; }
        std::optional<PluginConfig> show_plugin_config_dialog(const PluginConfig&) override { return std::nullopt; }
        void start_simulation_process(const std::string&, const std::filesystem::path&, const std::filesystem::path&) override {}
        void cancel_simulation_process() override {}
        void spawn_raw_file_window(std::shared_ptr<XyceOutputFile>) override {}
        void set_event_handler(MainWindowViewDefEvents&) override {}
    };
} // namespace

// ========================================================================================
// App singleton
// ========================================================================================

TEST(AppChecks, instance_returns_same_reference) {
    // arrange / act
    App& a = App::instance();
    App& b = App::instance();
    // assert
    EXPECT_EQ(&a, &b);
}

// ========================================================================================
// initialize() CLI parsing
// ========================================================================================

TEST(AppChecks, initialize_parses_log_level_long_form) {
    // arrange
    const char* argv[] = {"test", "--log-level", "debug"};
    int argc = 3;
    App& app = App::instance();
    // act
    app.initialize(argc, const_cast<char**>(argv));
    // assert
    EXPECT_EQ(app.log_level(), "debug");
}

TEST(AppChecks, initialize_parses_log_level_equals_form) {
    // arrange
    const char* argv[] = {"test", "--log-level=warn"};
    int argc = 2;
    App& app = App::instance();
    // act
    app.initialize(argc, const_cast<char**>(argv));
    // assert
    EXPECT_EQ(app.log_level(), "warn");
}

TEST(AppChecks, initialize_parses_log_level_short_form) {
    // arrange
    const char* argv[] = {"test", "-l", "error"};
    int argc = 3;
    App& app = App::instance();
    // act
    app.initialize(argc, const_cast<char**>(argv));
    // assert
    EXPECT_EQ(app.log_level(), "error");
}

TEST(AppChecks, initialize_normalizes_to_lowercase) {
    // arrange
    const char* argv[] = {"test", "--log-level", "DEBUG"};
    int argc = 3;
    App& app = App::instance();
    // act
    app.initialize(argc, const_cast<char**>(argv));
    // assert
    EXPECT_EQ(app.log_level(), "debug");
}
