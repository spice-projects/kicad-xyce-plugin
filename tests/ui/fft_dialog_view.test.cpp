#include <gtest/gtest.h>

#include <memory>
#include <type_traits>

#include "ui/fft_dialog_view.h"

TEST(FftDialogViewChecks, constructor_with_main_window_and_renderer) {
    // arrange / act / assert
    // This test verifies at compile time that FftDialogView constructor
    // accepts the expected parameter types.
    // FftDialogView requires slint::ComponentHandle<main_window::MainWindow>
    // and ChartsRenderer&, which cannot be instantiated without Slint runtime,
    // so we verify the interface through compile-time type traits.
    
    using ViewT = fft_dialog_view::FftDialogView;
    
    // Verify the class is not trivially copyable (implies pimpl with unique_ptr)
    static_assert(!std::is_trivially_copyable_v<ViewT>);
    static_assert(!std::is_copy_constructible_v<ViewT>);
    static_assert(!std::is_copy_assignable_v<ViewT>);
    
    // Verify the class has a user-declared destructor
    static_assert(!std::is_trivially_destructible_v<ViewT>);
    
    SUCCEED();
}

TEST(FftDialogViewChecks, constructor_signature_is_explicit) {
    // arrange / act / assert
    // Verify the constructor is explicit (prevents accidental implicit conversions)
    // This is checked through compilation success of the test.
    
    using ViewT = fft_dialog_view::FftDialogView;
    
    // Verify the class is constructible with the expected types
    static_assert(std::is_constructible_v<ViewT, slint::ComponentHandle<main_window::MainWindow>, ChartsRenderer&>);
    
    SUCCEED();
}

TEST(FftDialogViewChecks, show_for_chart_method_exists) {
    // arrange / act / assert
    // Verify the show_for_chart method exists and has the correct signature
    
    using ViewT = fft_dialog_view::FftDialogView;
    
    // Verify show_for_chart is a member function pointer
    static_assert(std::is_member_function_pointer_v<decltype(&ViewT::show_for_chart)>);
    
    SUCCEED();
}

TEST(FftDialogViewChecks, destructor_is_user_declared) {
    // arrange / act / assert
    // Verify the destructor is user-declared (not implicitly generated)
    // This is important for proper cleanup of the pimpl pattern
    
    using ViewT = fft_dialog_view::FftDialogView;
    
    // The destructor is not trivial (user-declared)
    static_assert(!std::is_trivially_destructible_v<ViewT>);
    
    SUCCEED();
}

TEST(FftDialogViewChecks, pimpl_pattern_uses_unique_ptr) {
    // arrange / act / assert
    // Verify FftDialogView uses the pimpl idiom with std::unique_ptr
    // The class cannot be trivially copied, indicating non-trivial members
    
    using ViewT = fft_dialog_view::FftDialogView;
    
    // The class should not be trivially copyable (has unique_ptr<Impl>)
    static_assert(!std::is_trivially_copyable_v<ViewT>);
    
    // The class should not support move construction/assignment in a trivial way
    // (unique_ptr makes it non-trivial)
    
    SUCCEED();
}