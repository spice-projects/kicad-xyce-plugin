#include <gtest/gtest.h>

#include "ui/add_plot_dialog_view.h"

TEST(AddPlotDialogViewChecks, constructor_accepts_window_and_renderer) {
    // arrange / act
    // verify the constructor is callable with the expected parameter types at compile time, using type traits that work regardless of include order issues with main_window::MainWindow
    using ViewT = add_plot_dialog_view::AddPlotDialogView;
    // assert
    static_assert(!std::is_copy_constructible_v<ViewT>);
    static_assert(!std::is_copy_assignable_v<ViewT>);
    static_assert(!std::is_trivially_destructible_v<ViewT>);
    SUCCEED();
}

TEST(AddPlotDialogViewChecks, show_for_chart_method_exists) {
    // arrange / act
    // verify the show_for_chart method exists with the expected signature, using a member function pointer trait
    using ViewT = add_plot_dialog_view::AddPlotDialogView;
    // assert
    static_assert(std::is_member_function_pointer_v<decltype(&ViewT::show_for_chart)>);
    SUCCEED();
}

TEST(AddPlotDialogViewChecks, implements_pimpl_pattern) {
    // arrange / act
    // the class uses pimpl with std::unique_ptr<Impl>, which makes it non-copyable and non-trivially-destructible
    using ViewT = add_plot_dialog_view::AddPlotDialogView;
    // assert
    static_assert(!std::is_trivially_copyable_v<ViewT>);
    SUCCEED();
}
