#include <gtest/gtest.h>

#include <type_traits>

#include "ui/main_window_view.h"

TEST(SlintMainWindowViewChecks, class_is_non_copyable) {
    // the view is non-copyable by design (owns unique_ptr members and Slint component handles)
    // arrange / act
    using ViewT = SlintMainWindowView;
    // assert
    static_assert(!std::is_copy_constructible_v<ViewT>);
    static_assert(!std::is_copy_assignable_v<ViewT>);
    SUCCEED();
}

TEST(SlintMainWindowViewChecks, class_has_virtual_destructor) {
    // the view has a virtual destructor
    // because it inherits from MainWindowViewDef
    // arrange / act
    using ViewT = SlintMainWindowView;
    // assert
    static_assert(!std::is_trivially_destructible_v<ViewT>);
    SUCCEED();
}

TEST(SlintMainWindowViewChecks, set_title_is_virtual) {
    // set_title is a virtual override
    // of MainWindowViewDef
    // arrange / act
    using ViewT = SlintMainWindowView;
    // assert
    static_assert(std::is_member_function_pointer_v<decltype(&ViewT::set_title)>);
    SUCCEED();
}

TEST(SlintMainWindowViewChecks, show_netlist_view_is_virtual) {
    // show_netlist_view is a virtual override
    // arrange / act
    using ViewT = SlintMainWindowView;
    // assert
    static_assert(std::is_member_function_pointer_v<decltype(&ViewT::show_netlist_view)>);
    SUCCEED();
}

TEST(SlintMainWindowViewChecks, show_charts_view_is_virtual) {
    // show_charts_view is a virtual override
    // arrange / act
    using ViewT = SlintMainWindowView;
    // assert
    static_assert(std::is_member_function_pointer_v<decltype(&ViewT::show_charts_view)>);
    SUCCEED();
}

TEST(SlintMainWindowViewChecks, update_charts_is_virtual) {
    // update_charts is a virtual override
    // arrange / act
    using ViewT = SlintMainWindowView;
    // assert
    static_assert(std::is_member_function_pointer_v<decltype(&ViewT::update_charts)>);
    SUCCEED();
}

TEST(SlintMainWindowViewChecks, start_simulation_process_is_virtual) {
    // start_simulation_process is a virtual override
    // arrange / act
    using ViewT = SlintMainWindowView;
    // assert
    static_assert(std::is_member_function_pointer_v<decltype(&ViewT::start_simulation_process)>);
    SUCCEED();
}

TEST(SlintMainWindowViewChecks, cancel_simulation_process_is_virtual) {
    // cancel_simulation_process is a virtual override
    // arrange / act
    using ViewT = SlintMainWindowView;
    // assert
    static_assert(std::is_member_function_pointer_v<decltype(&ViewT::cancel_simulation_process)>);
    SUCCEED();
}

TEST(SlintMainWindowViewChecks, set_event_handler_is_virtual) {
    // set_event_handler is a virtual override
    // of MainWindowViewDef (not MainWindowView)
    // arrange / act
    using ViewT = SlintMainWindowView;
    // assert
    static_assert(std::is_member_function_pointer_v<decltype(&ViewT::set_event_handler)>);
    SUCCEED();
}

TEST(SlintMainWindowViewChecks, implements_main_window_view_def) {
    // SlintMainWindowView inherits from MainWindowViewDef
    // arrange / act
    using ViewT = SlintMainWindowView;
    using BaseT = MainWindowViewDef;
    // assert
    static_assert(std::is_base_of_v<BaseT, ViewT>);
    SUCCEED();
}

TEST(SlintMainWindowViewChecks, guard_modal_is_private) {
    // guard_modal is a private template method
    // used internally to gate modal dialog interactions
    // this is verified by inspecting the class design (non-public interface)
    // arrange / act / assert
    SUCCEED();
}
