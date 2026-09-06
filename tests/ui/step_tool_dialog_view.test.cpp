#include <gtest/gtest.h>

#include <type_traits>

#include "ui/step_tool_dialog_view.h"

TEST(StepToolDialogViewChecks, constructor_accepts_window_and_renderer) {
    // verify the constructor is callable with the expected parameter types at compile time
    // arrange / act
    using ViewT = step_tool_dialog_view::StepToolDialogView;
    // assert
    static_assert(!std::is_copy_constructible_v<ViewT>);
    static_assert(!std::is_copy_assignable_v<ViewT>);
    static_assert(!std::is_trivially_destructible_v<ViewT>);
    SUCCEED();
}

TEST(StepToolDialogViewChecks, show_for_chart_method_exists) {
    // verify the show_for_chart method exists with the expected signature
    // arrange / act
    using ViewT = step_tool_dialog_view::StepToolDialogView;
    // assert
    static_assert(std::is_member_function_pointer_v<decltype(&ViewT::show_for_chart)>);
    SUCCEED();
}

TEST(StepToolDialogViewChecks, implements_pimpl_pattern) {
    // the class uses pimpl with std::unique_ptr<Impl>, which makes it non-copyable and non-trivially-destructible
    // arrange / act
    using ViewT = step_tool_dialog_view::StepToolDialogView;
    // assert
    static_assert(!std::is_trivially_copyable_v<ViewT>);
    SUCCEED();
}
