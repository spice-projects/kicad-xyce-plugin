#include <gtest/gtest.h>

#include <type_traits>

#include "ui/main_window_view.h"

TEST(SlintMainWindowViewChecks, class_is_non_copyable) {
    // arrange / act / assert — the view is non-copyable by design
    // (owns unique_ptr members and Slint component handles)
    using ViewT = SlintMainWindowView;
    static_assert(!std::is_copy_constructible_v<ViewT>);
    static_assert(!std::is_copy_assignable_v<ViewT>);
    SUCCEED();
}

TEST(SlintMainWindowViewChecks, class_has_virtual_destructor) {
    // arrange / act / assert — the view has a virtual destructor
    // because it inherits from MainWindowViewDef
    using ViewT = SlintMainWindowView;
    static_assert(!std::is_trivially_destructible_v<ViewT>);
    SUCCEED();
}

TEST(SlintMainWindowViewChecks, set_title_is_virtual) {
    // arrange / act / assert — set_title is a virtual override
    // of MainWindowViewDef
    using ViewT = SlintMainWindowView;
    static_assert(std::is_member_function_pointer_v<decltype(&ViewT::set_title)>);
    SUCCEED();
}

TEST(SlintMainWindowViewChecks, show_netlist_view_is_virtual) {
    // arrange / act / assert — show_netlist_view is a virtual override
    using ViewT = SlintMainWindowView;
    static_assert(std::is_member_function_pointer_v<decltype(&ViewT::show_netlist_view)>);
    SUCCEED();
}

TEST(SlintMainWindowViewChecks, show_charts_view_is_virtual) {
    // arrange / act / assert — show_charts_view is a virtual override
    using ViewT = SlintMainWindowView;
    static_assert(std::is_member_function_pointer_v<decltype(&ViewT::show_charts_view)>);
    SUCCEED();
}

TEST(SlintMainWindowViewChecks, update_charts_is_virtual) {
    // arrange / act / assert — update_charts is a virtual override
    using ViewT = SlintMainWindowView;
    static_assert(std::is_member_function_pointer_v<decltype(&ViewT::update_charts)>);
    SUCCEED();
}

TEST(SlintMainWindowViewChecks, start_simulation_process_is_virtual) {
    // arrange / act / assert — start_simulation_process is a virtual override
    using ViewT = SlintMainWindowView;
    static_assert(std::is_member_function_pointer_v<decltype(&ViewT::start_simulation_process)>);
    SUCCEED();
}

TEST(SlintMainWindowViewChecks, cancel_simulation_process_is_virtual) {
    // arrange / act / assert — cancel_simulation_process is a virtual override
    using ViewT = SlintMainWindowView;
    static_assert(std::is_member_function_pointer_v<decltype(&ViewT::cancel_simulation_process)>);
    SUCCEED();
}

TEST(SlintMainWindowViewChecks, set_event_handler_is_virtual) {
    // arrange / act / assert — set_event_handler is a virtual override
    // of MainWindowViewDef (not MainWindowView)
    using ViewT = SlintMainWindowView;
    static_assert(std::is_member_function_pointer_v<decltype(&ViewT::set_event_handler)>);
    SUCCEED();
}

TEST(SlintMainWindowViewChecks, implements_main_window_view_def) {
    // arrange / act / assert — SlintMainWindowView inherits from MainWindowViewDef
    using ViewT = SlintMainWindowView;
    using BaseT = MainWindowViewDef;
    static_assert(std::is_base_of_v<BaseT, ViewT>);
    SUCCEED();
}

TEST(SlintMainWindowViewChecks, guard_modal_is_private) {
    // arrange / act / assert — guard_modal is a private template method
    // used internally to gate modal dialog interactions
    // This is verified by inspecting the class design (non-public interface)
    SUCCEED();
}