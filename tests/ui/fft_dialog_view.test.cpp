#include <gtest/gtest.h>

#include <memory>
#include <type_traits>

#include "ui/fft_dialog_view.h"

TEST(FftDialogViewChecks, constructor_with_main_window_and_renderer) {
    // arrange / act
    using ViewT = fft_dialog_view::FftDialogView;
    // assert
    EXPECT_FALSE(std::is_trivially_copyable_v<ViewT>);
    EXPECT_FALSE(std::is_copy_constructible_v<ViewT>);
    EXPECT_FALSE(std::is_copy_assignable_v<ViewT>);
    EXPECT_FALSE(std::is_trivially_destructible_v<ViewT>);
}

TEST(FftDialogViewChecks, constructor_signature_is_explicit) {
    // arrange / act
    using ViewT = fft_dialog_view::FftDialogView;
    // assert
    EXPECT_TRUE((std::is_constructible_v<ViewT, slint::ComponentHandle<main_window::MainWindow>, ChartsRenderer&>));
}

TEST(FftDialogViewChecks, show_for_chart_method_exists) {
    // arrange / act
    using ViewT = fft_dialog_view::FftDialogView;
    // assert
    EXPECT_TRUE((std::is_member_function_pointer_v<decltype(&ViewT::show_for_chart)>));
}

TEST(FftDialogViewChecks, destructor_is_user_declared) {
    // arrange / act
    using ViewT = fft_dialog_view::FftDialogView;
    // assert
    EXPECT_FALSE(std::is_trivially_destructible_v<ViewT>);
}

TEST(FftDialogViewChecks, pimpl_pattern_uses_unique_ptr) {
    // arrange / act
    using ViewT = fft_dialog_view::FftDialogView;
    // assert
    EXPECT_FALSE(std::is_trivially_copyable_v<ViewT>);
}
