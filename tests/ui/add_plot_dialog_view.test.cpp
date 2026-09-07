#include <type_traits>

#include <gtest/gtest.h>

#include "ui/add_plot_dialog_view.h"

TEST(AddPlotDialogViewChecks, constructor_accepts_window_and_renderer) {
    // arrange / act
    using ViewT = add_plot_dialog_view::AddPlotDialogView;
    // assert
    EXPECT_FALSE(std::is_copy_constructible_v<ViewT>);
    EXPECT_FALSE(std::is_copy_assignable_v<ViewT>);
    EXPECT_FALSE(std::is_trivially_destructible_v<ViewT>);
}

TEST(AddPlotDialogViewChecks, show_for_chart_method_exists) {
    // arrange / act
    using ViewT = add_plot_dialog_view::AddPlotDialogView;
    // assert
    EXPECT_TRUE(std::is_member_function_pointer_v<decltype(&ViewT::show_for_chart)>);
}

TEST(AddPlotDialogViewChecks, implements_pimpl_pattern) {
    // arrange / act
    using ViewT = add_plot_dialog_view::AddPlotDialogView;
    // assert
    EXPECT_FALSE(std::is_trivially_copyable_v<ViewT>);
}
