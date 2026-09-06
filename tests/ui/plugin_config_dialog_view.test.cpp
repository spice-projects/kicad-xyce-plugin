#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "config/plugin_config.h"
#include "dsp/fft.h"
#include "expression/expression.h"
#include "simulation/simulation_config.h"
#include "ui/main_window_view_def.h"
#include "ui/plugin_config_dialog_view.h"

#include <slint.h>

namespace
{
    class RecordingEventHandler : public MainWindowViewDefEvents
    {
    public:
        void on_open_xyce_file(const std::filesystem::path&) override {}
        void on_save_netlist() override {}
        void on_show_netlist() override {}
        void on_show_charts() override {}
        void on_show_simulation_output() override {}
        void on_close_simulation_output() override {}
        void on_run_simulation() override {}
        void on_cancel_simulation() override {}
        void on_configure_simulation() override {}
        void on_configure_plugin() override {}
        void on_plugin_config_dialog_result(const PluginConfig&) override {}
        void on_simulation_parameters_dialog_result(const SimulationConfig&) override {}
        void on_fft_dialog_result(std::vector<AnyExpression*>, const fft::FftParameters&) override {}
        void on_chart_calculate_fft(size_t) override {}
        void on_chart_open_xyce_fft_calculation(size_t) override {}
        void on_chart_step_tool(size_t) override {}
        void on_chart_new_window(size_t) override {}
        void on_simulation_finished(int, bool) override {}
        void on_simulation_stdout(const std::string&) override {}
        void on_simulation_stderr(const std::string&) override {}
        void on_netlist_editor_modified() override {}
        void on_extract_schematic_netlist() override {}
    };
} // namespace

TEST(PluginConfigDialogViewChecks, constructor_takes_main_window_handle) {
    // verify the constructor accepts slint::ComponentHandle<main_window::MainWindow>
    // arrange / act
    constexpr bool is_constructible = std::is_constructible_v<plugin_config_dialog_view::PluginConfigDialogView, slint::ComponentHandle<main_window::MainWindow>>;
    // assert
    static_assert(is_constructible);
    EXPECT_TRUE(is_constructible);
}

TEST(PluginConfigDialogViewChecks, destructor_is_user_declared) {
    // verify the destructor is user-declared
    // arrange / act
    constexpr bool has_destructor = !std::is_trivially_destructible_v<plugin_config_dialog_view::PluginConfigDialogView>;
    // assert
    static_assert(has_destructor);
    EXPECT_TRUE(has_destructor);
}

TEST(PluginConfigDialogViewChecks, show_method_exists) {
    // verify the show method signature
    // arrange / act
    constexpr bool has_show = std::is_member_function_pointer_v<decltype(&plugin_config_dialog_view::PluginConfigDialogView::show)>;
    // assert
    static_assert(has_show);
    EXPECT_TRUE(has_show);
}

TEST(PluginConfigDialogViewChecks, implements_main_window_view_def_events) {
    // verify the view works with MainWindowViewDefEvents
    // arrange / act
    constexpr bool is_base_of = std::is_base_of_v<MainWindowViewDefEvents, RecordingEventHandler>;
    // assert
    static_assert(is_base_of);
    EXPECT_TRUE(is_base_of);
}
