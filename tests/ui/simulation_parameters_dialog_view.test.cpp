#include <gtest/gtest.h>

#include <functional>

#include "ui/simulation_parameters_dialog_view.h"

TEST(SimulationParametersDialogViewChecks, copy_constructor_is_deleted) {
    // verify SimulationParametersDialogView cannot be copied
    // arrange / act
    using ViewT = simulation_parameters_dialog_view::SimulationParametersDialogView;
    constexpr bool is_copy_constructible = std::is_copy_constructible_v<ViewT>;
    constexpr bool is_copy_assignable = std::is_copy_assignable_v<ViewT>;
    // assert
    static_assert(!is_copy_constructible);
    static_assert(!is_copy_assignable);
    EXPECT_FALSE(is_copy_constructible);
    EXPECT_FALSE(is_copy_assignable);
}

TEST(SimulationParametersDialogViewChecks, constructor_takes_main_window_handle) {
    // verify the constructor signature accepts a slint ComponentHandle
    // arrange / act
    using ViewT = simulation_parameters_dialog_view::SimulationParametersDialogView;
    constexpr bool is_constructible = std::is_constructible_v<ViewT, slint::ComponentHandle<main_window::MainWindow>>;
    // assert
    static_assert(is_constructible);
    EXPECT_TRUE(is_constructible);
}

TEST(SimulationParametersDialogViewChecks, destructor_is_user_declared) {
    // verify the destructor is user-declared (not implicitly generated)
    // arrange / act
    using ViewT = simulation_parameters_dialog_view::SimulationParametersDialogView;
    constexpr bool has_destructor = !std::is_trivially_destructible_v<ViewT>;
    // assert
    static_assert(has_destructor);
    EXPECT_TRUE(has_destructor);
}

TEST(SimulationParametersDialogViewChecks, show_method_exists) {
    // verify the show method signature is callable
    // arrange / act
    using ViewT = simulation_parameters_dialog_view::SimulationParametersDialogView;
    constexpr bool has_show = std::is_member_function_pointer_v<decltype(&ViewT::show)>;
    // assert
    static_assert(has_show);
    EXPECT_TRUE(has_show);
}

TEST(SimulationParametersDialogViewChecks, show_method_signature_matches_interface) {
    // verify show() accepts SimulationConfig, MainWindowViewDefEvents&, and std::function<void()>
    // arrange / act
    using ViewT = simulation_parameters_dialog_view::SimulationParametersDialogView;
    constexpr bool has_correct_signature = std::is_invocable_v<decltype(&ViewT::show), ViewT*, const SimulationConfig&, MainWindowViewDefEvents&, const std::function<void()>&>;
    // assert
    static_assert(has_correct_signature);
    EXPECT_TRUE(has_correct_signature);
}
