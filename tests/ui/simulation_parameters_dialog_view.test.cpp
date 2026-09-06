#include <gtest/gtest.h>

#include <functional>

#include "ui/simulation_parameters_dialog_view.h"

TEST(SimulationParametersDialogViewChecks, copy_constructor_is_deleted) {
    // arrange / act
    using ViewT = simulation_parameters_dialog_view::SimulationParametersDialogView;
    constexpr bool is_copy_constructible = std::is_copy_constructible_v<ViewT>;
    constexpr bool is_copy_assignable = std::is_copy_assignable_v<ViewT>;
    // assert
    EXPECT_FALSE(is_copy_constructible);
    EXPECT_FALSE(is_copy_assignable);
}

TEST(SimulationParametersDialogViewChecks, constructor_takes_main_window_handle) {
    // arrange / act
    using ViewT = simulation_parameters_dialog_view::SimulationParametersDialogView;
    // assert
    EXPECT_TRUE((std::is_constructible_v<ViewT, slint::ComponentHandle<main_window::MainWindow>>));
}

TEST(SimulationParametersDialogViewChecks, destructor_is_user_declared) {
    // arrange / act
    using ViewT = simulation_parameters_dialog_view::SimulationParametersDialogView;
    // assert
    EXPECT_FALSE(std::is_trivially_destructible_v<ViewT>);
}

TEST(SimulationParametersDialogViewChecks, show_method_exists) {
    // arrange / act
    using ViewT = simulation_parameters_dialog_view::SimulationParametersDialogView;
    // assert
    EXPECT_TRUE((std::is_member_function_pointer_v<decltype(&ViewT::show)>));
}

TEST(SimulationParametersDialogViewChecks, show_method_signature_matches_interface) {
    // arrange / act
    using ViewT = simulation_parameters_dialog_view::SimulationParametersDialogView;
    // assert
    EXPECT_TRUE((std::is_invocable_v<decltype(&ViewT::show), ViewT*, const SimulationConfig&, MainWindowViewDefEvents&, const std::function<void()>&>));
}
