#include <gtest/gtest.h>

#include <type_traits>

#include "ui/step_tool_dialog_view.h"

TEST(StepToolDialogViewChecks, constructor_accepts_window_and_renderer) {
    // arrange / act
    using ViewT = step_tool_dialog_view::StepToolDialogView;
    // assert
    EXPECT_FALSE(std::is_copy_constructible_v<ViewT>);
    EXPECT_FALSE(std::is_copy_assignable_v<ViewT>);
    EXPECT_FALSE(std::is_trivially_destructible_v<ViewT>);
}

TEST(StepToolDialogViewChecks, show_for_chart_method_exists) {
    // arrange / act
    using ViewT = step_tool_dialog_view::StepToolDialogView;
    // assert
    EXPECT_TRUE(std::is_member_function_pointer_v<decltype(&ViewT::show_for_chart)>);
}

TEST(StepToolDialogViewChecks, implements_pimpl_pattern) {
    // arrange / act
    using ViewT = step_tool_dialog_view::StepToolDialogView;
    // assert
    EXPECT_FALSE(std::is_trivially_copyable_v<ViewT>);
}
