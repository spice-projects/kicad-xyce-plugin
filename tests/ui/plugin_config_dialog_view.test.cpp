#include <gtest/gtest.h>

#include <string>
#include <type_traits>

#include "config/plugin_config.h"
#include "dsp/fft.h"
#include "expression/expression.h"
#include "main_window.h"
#include "simulation/simulation_config.h"
#include "ui/main_window_view_def.h"
#include "ui/plugin_config_dialog_view.h"

#include <slint.h>

TEST(PluginConfigDialogViewChecks, constructor_takes_main_window_handle) {
    // arrange / act
    using ViewT = plugin_config_dialog_view::PluginConfigDialogView;
    // assert
    EXPECT_TRUE((std::is_constructible_v<ViewT, slint::ComponentHandle<main_window::MainWindow>>));
}

TEST(PluginConfigDialogViewChecks, destructor_is_user_declared) {
    // arrange / act
    using ViewT = plugin_config_dialog_view::PluginConfigDialogView;
    // assert
    EXPECT_FALSE(std::is_trivially_destructible_v<ViewT>);
}

TEST(PluginConfigDialogViewChecks, show_method_exists) {
    // arrange / act
    using ViewT = plugin_config_dialog_view::PluginConfigDialogView;
    // assert
    EXPECT_TRUE((std::is_member_function_pointer_v<decltype(&ViewT::show)>));
}

TEST(PluginConfigDialogViewChecks, does_not_implement_main_window_view_def_events) {
    // arrange / act
    using ViewT = plugin_config_dialog_view::PluginConfigDialogView;
    // assert
    EXPECT_FALSE((std::is_base_of_v<MainWindowViewDefEvents, ViewT>));
}
