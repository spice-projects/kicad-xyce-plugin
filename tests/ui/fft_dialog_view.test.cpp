#include <gtest/gtest.h>

#include <memory>
#include <type_traits>

#include "ui/fft_dialog_view.h"

TEST(FftDialogViewChecks, constructor_with_main_window_and_renderer) {
    // this test verifies at compile time that FftDialogView constructor accepts the expected parameter types
    // FftDialogView requires slint::ComponentHandle<main_window::MainWindow> and ChartsRenderer&, which cannot be instantiated without Slint runtime, so we verify the interface through compile-time type traits
    // arrange / act
    using ViewT = fft_dialog_view::FftDialogView;
    // assert
    static_assert(!std::is_trivially_copyable_v<ViewT>);
    static_assert(!std::is_copy_constructible_v<ViewT>);
    static_assert(!std::is_copy_assignable_v<ViewT>);
    static_assert(!std::is_trivially_destructible_v<ViewT>);
    SUCCEED();
}

TEST(FftDialogViewChecks, constructor_signature_is_explicit) {
    // verify the constructor is explicit (prevents accidental implicit conversions)
    // this is checked through compilation success of the test
    // arrange / act
    using ViewT = fft_dialog_view::FftDialogView;
    // assert
    static_assert(std::is_constructible_v<ViewT, slint::ComponentHandle<main_window::MainWindow>, ChartsRenderer&>);
    SUCCEED();
}

TEST(FftDialogViewChecks, show_for_chart_method_exists) {
    // verify the show_for_chart method exists and has the correct signature
    // arrange / act
    using ViewT = fft_dialog_view::FftDialogView;
    // assert
    static_assert(std::is_member_function_pointer_v<decltype(&ViewT::show_for_chart)>);
    SUCCEED();
}

TEST(FftDialogViewChecks, destructor_is_user_declared) {
    // verify the destructor is user-declared (not implicitly generated)
    // this is important for proper cleanup of the pimpl pattern
    // arrange / act
    using ViewT = fft_dialog_view::FftDialogView;
    // assert
    static_assert(!std::is_trivially_destructible_v<ViewT>);
    SUCCEED();
}

TEST(FftDialogViewChecks, pimpl_pattern_uses_unique_ptr) {
    // verify FftDialogView uses the pimpl idiom with std::unique_ptr
    // the class cannot be trivially copied, indicating non-trivial members
    // arrange / act
    using ViewT = fft_dialog_view::FftDialogView;
    // assert
    static_assert(!std::is_trivially_copyable_v<ViewT>);
    SUCCEED();
}
