#include <set>
#include <vector>

#include <gtest/gtest.h>

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/button.h>
#include <wx/listctrl.h>
#include <wx/stattext.h>
#endif

#include "step_information.h"
#include "ui/step_tool_dialog.h"

// ========================================================================================
// constructor
// ========================================================================================

TEST(UiStepToolDialogChecks, constructor_returns_empty_selection_for_empty_info) {
    // arrange
    StepInformation step_info({}, {}, {});
    // act
    StepToolDialog dialog(nullptr, &step_info, {});
    // assert
    EXPECT_EQ(dialog.selected_steps(), std::set<size_t>{});
}

TEST(UiStepToolDialogChecks, constructor_returns_empty_selection_when_none_initialized) {
    // arrange
    StepInformation step_info({"R1", "TEMP"}, {{1000.0, 27.0}, {2000.0, 85.0}, {3000.0, 125.0}}, {{0.0, 4.0}, {0.0, 4.0}, {0.0, 4.0}});
    // act
    StepToolDialog dialog(nullptr, &step_info, {});
    // assert
    EXPECT_EQ(dialog.selected_steps(), std::set<size_t>{});
}

TEST(UiStepToolDialogChecks, constructor_returns_initial_selection_set) {
    // arrange
    StepInformation step_info({"R1", "TEMP"}, {{1000.0, 27.0}, {2000.0, 85.0}, {3000.0, 125.0}}, {{0.0, 4.0}, {0.0, 4.0}, {0.0, 4.0}});
    std::set<size_t> initial = {0, 2};
    // act
    StepToolDialog dialog(nullptr, &step_info, initial);
    // assert
    EXPECT_EQ(dialog.selected_steps(), initial);
}

TEST(UiStepToolDialogChecks, constructor_selects_all_when_full_set_given) {
    // arrange
    StepInformation step_info({"R1", "TEMP"}, {{1000.0, 27.0}, {2000.0, 85.0}, {3000.0, 125.0}}, {{0.0, 4.0}, {0.0, 4.0}, {0.0, 4.0}});
    std::set<size_t> initial = {0, 1, 2};
    // act
    StepToolDialog dialog(nullptr, &step_info, initial);
    // assert
    EXPECT_EQ(dialog.selected_steps(), initial);
}

TEST(UiStepToolDialogChecks, constructor_creates_correct_number_of_rows) {
    // arrange
    StepInformation step_info({"R1", "TEMP"}, {{1000.0, 27.0}, {2000.0, 85.0}, {3000.0, 125.0}}, {{0.0, 4.0}, {0.0, 4.0}, {0.0, 4.0}});
    StepToolDialog dialog(nullptr, &step_info, {});
    // act
    wxListView* step_list = nullptr;
    for (wxWindow* child : dialog.GetChildren()) {
        step_list = dynamic_cast<wxListView*>(child);
        if (step_list != nullptr) {
            break;
        }
    }
    // assert
    ASSERT_NE(step_list, nullptr);
    EXPECT_EQ(step_list->GetItemCount(), 3);
}

TEST(UiStepToolDialogChecks, constructor_creates_column_for_each_key_plus_checkbox) {
    // arrange
    StepInformation step_info({"R1", "TEMP"}, {{1000.0, 27.0}, {2000.0, 85.0}, {3000.0, 125.0}}, {{0.0, 4.0}, {0.0, 4.0}, {0.0, 4.0}});
    StepToolDialog dialog(nullptr, &step_info, {});
    // act
    wxListView* step_list = nullptr;
    for (wxWindow* child : dialog.GetChildren()) {
        step_list = dynamic_cast<wxListView*>(child);
        if (step_list != nullptr) {
            break;
        }
    }
    // assert
    ASSERT_NE(step_list, nullptr);
    EXPECT_EQ(step_list->GetColumnCount(), 3);
}

// ========================================================================================
// select all
// ========================================================================================

TEST(UiStepToolDialogChecks, select_all_selects_all_steps) {
    // arrange
    StepInformation step_info({"R1", "TEMP"}, {{1000.0, 27.0}, {2000.0, 85.0}, {3000.0, 125.0}}, {{0.0, 4.0}, {0.0, 4.0}, {0.0, 4.0}});
    StepToolDialog dialog(nullptr, &step_info, {});
    // act
    wxButton* select_all_btn = dynamic_cast<wxButton*>(dialog.FindWindowByLabel("Select All"));
    ASSERT_NE(select_all_btn, nullptr);
    wxCommandEvent click_event(wxEVT_BUTTON, select_all_btn->GetId());
    click_event.SetEventObject(select_all_btn);
    select_all_btn->GetEventHandler()->ProcessEvent(click_event);
    // assert
    std::set<size_t> expected = {0, 1, 2};
    EXPECT_EQ(dialog.selected_steps(), expected);
}

TEST(UiStepToolDialogChecks, select_all_when_some_selected_selects_all) {
    // arrange
    StepInformation step_info({"R1", "TEMP"}, {{1000.0, 27.0}, {2000.0, 85.0}, {3000.0, 125.0}}, {{0.0, 4.0}, {0.0, 4.0}, {0.0, 4.0}});
    StepToolDialog dialog(nullptr, &step_info, {0});
    // act
    wxButton* select_all_btn = dynamic_cast<wxButton*>(dialog.FindWindowByLabel("Select All"));
    ASSERT_NE(select_all_btn, nullptr);
    wxCommandEvent click_event(wxEVT_BUTTON, select_all_btn->GetId());
    click_event.SetEventObject(select_all_btn);
    select_all_btn->GetEventHandler()->ProcessEvent(click_event);
    // assert
    std::set<size_t> expected = {0, 1, 2};
    EXPECT_EQ(dialog.selected_steps(), expected);
}

// ========================================================================================
// clear all
// ========================================================================================

TEST(UiStepToolDialogChecks, clear_all_deselects_all_steps) {
    // arrange
    StepInformation step_info({"R1", "TEMP"}, {{1000.0, 27.0}, {2000.0, 85.0}, {3000.0, 125.0}}, {{0.0, 4.0}, {0.0, 4.0}, {0.0, 4.0}});
    StepToolDialog dialog(nullptr, &step_info, {0, 1, 2});
    // act
    wxButton* clear_all_btn = dynamic_cast<wxButton*>(dialog.FindWindowByLabel("Clear All"));
    ASSERT_NE(clear_all_btn, nullptr);
    wxCommandEvent click_event(wxEVT_BUTTON, clear_all_btn->GetId());
    click_event.SetEventObject(clear_all_btn);
    clear_all_btn->GetEventHandler()->ProcessEvent(click_event);
    // assert
    EXPECT_EQ(dialog.selected_steps(), std::set<size_t>{});
}

TEST(UiStepToolDialogChecks, clear_all_when_none_selected_stays_empty) {
    // arrange
    StepInformation step_info({"R1", "TEMP"}, {{1000.0, 27.0}, {2000.0, 85.0}, {3000.0, 125.0}}, {{0.0, 4.0}, {0.0, 4.0}, {0.0, 4.0}});
    StepToolDialog dialog(nullptr, &step_info, {});
    // act
    wxButton* clear_all_btn = dynamic_cast<wxButton*>(dialog.FindWindowByLabel("Clear All"));
    ASSERT_NE(clear_all_btn, nullptr);
    wxCommandEvent click_event(wxEVT_BUTTON, clear_all_btn->GetId());
    click_event.SetEventObject(clear_all_btn);
    clear_all_btn->GetEventHandler()->ProcessEvent(click_event);
    // assert
    EXPECT_EQ(dialog.selected_steps(), std::set<size_t>{});
}

// ========================================================================================
// invert selection
// ========================================================================================

TEST(UiStepToolDialogChecks, invert_empty_selects_all) {
    // arrange
    StepInformation step_info({"R1", "TEMP"}, {{1000.0, 27.0}, {2000.0, 85.0}, {3000.0, 125.0}}, {{0.0, 4.0}, {0.0, 4.0}, {0.0, 4.0}});
    StepToolDialog dialog(nullptr, &step_info, {});
    // act
    wxButton* invert_btn = dynamic_cast<wxButton*>(dialog.FindWindowByLabel("Invert"));
    ASSERT_NE(invert_btn, nullptr);
    wxCommandEvent click_event(wxEVT_BUTTON, invert_btn->GetId());
    click_event.SetEventObject(invert_btn);
    invert_btn->GetEventHandler()->ProcessEvent(click_event);
    // assert
    std::set<size_t> expected = {0, 1, 2};
    EXPECT_EQ(dialog.selected_steps(), expected);
}

TEST(UiStepToolDialogChecks, invert_full_deselects_all) {
    // arrange
    StepInformation step_info({"R1", "TEMP"}, {{1000.0, 27.0}, {2000.0, 85.0}, {3000.0, 125.0}}, {{0.0, 4.0}, {0.0, 4.0}, {0.0, 4.0}});
    StepToolDialog dialog(nullptr, &step_info, {0, 1, 2});
    // act
    wxButton* invert_btn = dynamic_cast<wxButton*>(dialog.FindWindowByLabel("Invert"));
    ASSERT_NE(invert_btn, nullptr);
    wxCommandEvent click_event(wxEVT_BUTTON, invert_btn->GetId());
    click_event.SetEventObject(invert_btn);
    invert_btn->GetEventHandler()->ProcessEvent(click_event);
    // assert
    EXPECT_EQ(dialog.selected_steps(), std::set<size_t>{});
}

TEST(UiStepToolDialogChecks, invert_partial_flips_selection) {
    // arrange
    StepInformation step_info({"R1", "TEMP"}, {{1000.0, 27.0}, {2000.0, 85.0}, {3000.0, 125.0}}, {{0.0, 4.0}, {0.0, 4.0}, {0.0, 4.0}});
    StepToolDialog dialog(nullptr, &step_info, {1});
    // act
    wxButton* invert_btn = dynamic_cast<wxButton*>(dialog.FindWindowByLabel("Invert"));
    ASSERT_NE(invert_btn, nullptr);
    wxCommandEvent click_event(wxEVT_BUTTON, invert_btn->GetId());
    click_event.SetEventObject(invert_btn);
    invert_btn->GetEventHandler()->ProcessEvent(click_event);
    // assert
    std::set<size_t> expected = {0, 2};
    EXPECT_EQ(dialog.selected_steps(), expected);
}

TEST(UiStepToolDialogChecks, invert_twice_restores_original) {
    // arrange
    StepInformation step_info({"R1", "TEMP"}, {{1000.0, 27.0}, {2000.0, 85.0}, {3000.0, 125.0}}, {{0.0, 4.0}, {0.0, 4.0}, {0.0, 4.0}});
    std::set<size_t> original = {0, 2};
    StepToolDialog dialog(nullptr, &step_info, original);
    // act
    wxButton* invert_btn = dynamic_cast<wxButton*>(dialog.FindWindowByLabel("Invert"));
    ASSERT_NE(invert_btn, nullptr);
    wxCommandEvent click_event(wxEVT_BUTTON, invert_btn->GetId());
    click_event.SetEventObject(invert_btn);
    invert_btn->GetEventHandler()->ProcessEvent(click_event);
    invert_btn->GetEventHandler()->ProcessEvent(click_event);
    // assert
    EXPECT_EQ(dialog.selected_steps(), original);
}

// ========================================================================================
// item toggle via checkbox
// ========================================================================================

TEST(UiStepToolDialogChecks, item_checked_adds_to_selection) {
    // arrange
    StepInformation step_info({"R1", "TEMP"}, {{1000.0, 27.0}, {2000.0, 85.0}, {3000.0, 125.0}}, {{0.0, 4.0}, {0.0, 4.0}, {0.0, 4.0}});
    StepToolDialog dialog(nullptr, &step_info, {});
    // act
    wxListView* step_list = nullptr;
    for (wxWindow* child : dialog.GetChildren()) {
        step_list = dynamic_cast<wxListView*>(child);
        if (step_list != nullptr) {
            break;
        }
    }
    ASSERT_NE(step_list, nullptr);
    step_list->CheckItem(1, true);
    // assert
    EXPECT_TRUE(dialog.selected_steps().contains(1));
}

TEST(UiStepToolDialogChecks, item_unchecked_removes_from_selection) {
    // arrange
    StepInformation step_info({"R1", "TEMP"}, {{1000.0, 27.0}, {2000.0, 85.0}, {3000.0, 125.0}}, {{0.0, 4.0}, {0.0, 4.0}, {0.0, 4.0}});
    StepToolDialog dialog(nullptr, &step_info, {0, 1, 2});
    // act
    wxListView* step_list = nullptr;
    for (wxWindow* child : dialog.GetChildren()) {
        step_list = dynamic_cast<wxListView*>(child);
        if (step_list != nullptr) {
            break;
        }
    }
    ASSERT_NE(step_list, nullptr);
    step_list->CheckItem(1, false);
    // assert
    EXPECT_FALSE(dialog.selected_steps().contains(1));
}

TEST(UiStepToolDialogChecks, item_toggle_preserves_other_selections) {
    // arrange
    StepInformation step_info({"R1", "TEMP"}, {{1000.0, 27.0}, {2000.0, 85.0}, {3000.0, 125.0}}, {{0.0, 4.0}, {0.0, 4.0}, {0.0, 4.0}});
    StepToolDialog dialog(nullptr, &step_info, {0, 2});
    // act
    wxListView* step_list = nullptr;
    for (wxWindow* child : dialog.GetChildren()) {
        step_list = dynamic_cast<wxListView*>(child);
        if (step_list != nullptr) {
            break;
        }
    }
    ASSERT_NE(step_list, nullptr);
    step_list->CheckItem(0, false);
    // assert
    EXPECT_FALSE(dialog.selected_steps().contains(0));
    EXPECT_TRUE(dialog.selected_steps().contains(2));
}

TEST(UiStepToolDialogChecks, item_toggle_multiple_checks_accumulate) {
    // arrange
    StepInformation step_info({"R1", "TEMP"}, {{1000.0, 27.0}, {2000.0, 85.0}, {3000.0, 125.0}}, {{0.0, 4.0}, {0.0, 4.0}, {0.0, 4.0}});
    StepToolDialog dialog(nullptr, &step_info, {});
    // act
    wxListView* step_list = nullptr;
    for (wxWindow* child : dialog.GetChildren()) {
        step_list = dynamic_cast<wxListView*>(child);
        if (step_list != nullptr) {
            break;
        }
    }
    ASSERT_NE(step_list, nullptr);
    step_list->CheckItem(0, true);
    step_list->CheckItem(2, true);
    // assert
    std::set<size_t> expected = {0, 2};
    EXPECT_EQ(dialog.selected_steps(), expected);
}

// ========================================================================================
// selection count label
// ========================================================================================

TEST(UiStepToolDialogChecks, selection_count_label_shows_initial_count) {
    // arrange
    StepInformation step_info({"R1", "TEMP"}, {{1000.0, 27.0}, {2000.0, 85.0}, {3000.0, 125.0}}, {{0.0, 4.0}, {0.0, 4.0}, {0.0, 4.0}});
    StepToolDialog dialog(nullptr, &step_info, {1});
    // act
    wxStaticText* count_label = nullptr;
    for (wxWindow* child : dialog.GetChildren()) {
        auto* st = dynamic_cast<wxStaticText*>(child);
        if (st != nullptr && st->GetLabel().StartsWith("Selected ")) {
            count_label = st;
            break;
        }
    }
    // assert
    ASSERT_NE(count_label, nullptr);
    EXPECT_EQ(count_label->GetLabel().ToStdString(), "Selected 1 / 3");
}

TEST(UiStepToolDialogChecks, selection_count_label_updates_after_select_all) {
    // arrange
    StepInformation step_info({"R1", "TEMP"}, {{1000.0, 27.0}, {2000.0, 85.0}, {3000.0, 125.0}}, {{0.0, 4.0}, {0.0, 4.0}, {0.0, 4.0}});
    StepToolDialog dialog(nullptr, &step_info, {});
    // act
    wxButton* select_all_btn = dynamic_cast<wxButton*>(dialog.FindWindowByLabel("Select All"));
    ASSERT_NE(select_all_btn, nullptr);
    wxCommandEvent click_event(wxEVT_BUTTON, select_all_btn->GetId());
    click_event.SetEventObject(select_all_btn);
    select_all_btn->GetEventHandler()->ProcessEvent(click_event);
    wxStaticText* count_label = nullptr;
    for (wxWindow* child : dialog.GetChildren()) {
        auto* st = dynamic_cast<wxStaticText*>(child);
        if (st != nullptr && st->GetLabel().StartsWith("Selected ")) {
            count_label = st;
            break;
        }
    }
    // assert
    ASSERT_NE(count_label, nullptr);
    EXPECT_EQ(count_label->GetLabel().ToStdString(), "Selected 3 / 3");
}

TEST(UiStepToolDialogChecks, selection_count_label_updates_after_clear_all) {
    // arrange
    StepInformation step_info({"R1", "TEMP"}, {{1000.0, 27.0}, {2000.0, 85.0}, {3000.0, 125.0}}, {{0.0, 4.0}, {0.0, 4.0}, {0.0, 4.0}});
    StepToolDialog dialog(nullptr, &step_info, {0, 1, 2});
    // act
    wxButton* clear_all_btn = dynamic_cast<wxButton*>(dialog.FindWindowByLabel("Clear All"));
    ASSERT_NE(clear_all_btn, nullptr);
    wxCommandEvent click_event(wxEVT_BUTTON, clear_all_btn->GetId());
    click_event.SetEventObject(clear_all_btn);
    clear_all_btn->GetEventHandler()->ProcessEvent(click_event);
    wxStaticText* count_label = nullptr;
    for (wxWindow* child : dialog.GetChildren()) {
        auto* st = dynamic_cast<wxStaticText*>(child);
        if (st != nullptr && st->GetLabel().StartsWith("Selected ")) {
            count_label = st;
            break;
        }
    }
    // assert
    ASSERT_NE(count_label, nullptr);
    EXPECT_EQ(count_label->GetLabel().ToStdString(), "Selected 0 / 3");
}

TEST(UiStepToolDialogChecks, selection_count_label_updates_after_invert) {
    // arrange
    StepInformation step_info({"R1", "TEMP"}, {{1000.0, 27.0}, {2000.0, 85.0}, {3000.0, 125.0}}, {{0.0, 4.0}, {0.0, 4.0}, {0.0, 4.0}});
    StepToolDialog dialog(nullptr, &step_info, {0, 2});
    // act
    wxButton* invert_btn = dynamic_cast<wxButton*>(dialog.FindWindowByLabel("Invert"));
    ASSERT_NE(invert_btn, nullptr);
    wxCommandEvent click_event(wxEVT_BUTTON, invert_btn->GetId());
    click_event.SetEventObject(invert_btn);
    invert_btn->GetEventHandler()->ProcessEvent(click_event);
    wxStaticText* count_label = nullptr;
    for (wxWindow* child : dialog.GetChildren()) {
        auto* st = dynamic_cast<wxStaticText*>(child);
        if (st != nullptr && st->GetLabel().StartsWith("Selected ")) {
            count_label = st;
            break;
        }
    }
    // assert
    ASSERT_NE(count_label, nullptr);
    EXPECT_EQ(count_label->GetLabel().ToStdString(), "Selected 1 / 3");
}

TEST(UiStepToolDialogChecks, selection_count_label_zero_for_empty_info) {
    // arrange
    StepInformation step_info({}, {}, {});
    StepToolDialog dialog(nullptr, &step_info, {});
    // act
    wxStaticText* count_label = nullptr;
    for (wxWindow* child : dialog.GetChildren()) {
        auto* st = dynamic_cast<wxStaticText*>(child);
        if (st != nullptr && st->GetLabel().StartsWith("Selected ")) {
            count_label = st;
            break;
        }
    }
    // assert
    ASSERT_NE(count_label, nullptr);
    EXPECT_EQ(count_label->GetLabel().ToStdString(), "Selected 0 / 0");
}

// ========================================================================================
// selected_steps accessor returns a copy
// ========================================================================================

TEST(UiStepToolDialogChecks, selected_steps_returns_detached_copy) {
    // arrange
    StepInformation step_info({"R1", "TEMP"}, {{1000.0, 27.0}, {2000.0, 85.0}, {3000.0, 125.0}}, {{0.0, 4.0}, {0.0, 4.0}, {0.0, 4.0}});
    std::set<size_t> initial = {0, 2};
    StepToolDialog dialog(nullptr, &step_info, initial);
    // act
    std::set<size_t> result = dialog.selected_steps();
    result.insert(1);
    // assert
    EXPECT_NE(result, dialog.selected_steps());
}

// ========================================================================================
// single step edge case
// ========================================================================================

TEST(UiStepToolDialogChecks, single_step_constructor_with_selection) {
    // arrange
    StepInformation step_info({"R1"}, {{4700.0}}, {{0.0, 1.0}});
    // act
    StepToolDialog dialog(nullptr, &step_info, {0});
    // assert
    EXPECT_EQ(dialog.selected_steps(), std::set<size_t>({0}));
}

TEST(UiStepToolDialogChecks, single_step_select_all_works) {
    // arrange
    StepInformation step_info({"R1"}, {{4700.0}}, {{0.0, 1.0}});
    StepToolDialog dialog(nullptr, &step_info, {});
    // act
    wxButton* select_all_btn = dynamic_cast<wxButton*>(dialog.FindWindowByLabel("Select All"));
    ASSERT_NE(select_all_btn, nullptr);
    wxCommandEvent click_event(wxEVT_BUTTON, select_all_btn->GetId());
    click_event.SetEventObject(select_all_btn);
    select_all_btn->GetEventHandler()->ProcessEvent(click_event);
    // assert
    EXPECT_EQ(dialog.selected_steps(), std::set<size_t>({0}));
}

TEST(UiStepToolDialogChecks, single_step_clear_all_works) {
    // arrange
    StepInformation step_info({"R1"}, {{4700.0}}, {{0.0, 1.0}});
    StepToolDialog dialog(nullptr, &step_info, {0});
    // act
    wxButton* clear_all_btn = dynamic_cast<wxButton*>(dialog.FindWindowByLabel("Clear All"));
    ASSERT_NE(clear_all_btn, nullptr);
    wxCommandEvent click_event(wxEVT_BUTTON, clear_all_btn->GetId());
    click_event.SetEventObject(clear_all_btn);
    clear_all_btn->GetEventHandler()->ProcessEvent(click_event);
    // assert
    EXPECT_EQ(dialog.selected_steps(), std::set<size_t>{});
}

TEST(UiStepToolDialogChecks, single_step_invert_works) {
    // arrange
    StepInformation step_info({"R1"}, {{4700.0}}, {{0.0, 1.0}});
    StepToolDialog dialog(nullptr, &step_info, {});
    // act
    wxButton* invert_btn = dynamic_cast<wxButton*>(dialog.FindWindowByLabel("Invert"));
    ASSERT_NE(invert_btn, nullptr);
    wxCommandEvent click_event(wxEVT_BUTTON, invert_btn->GetId());
    click_event.SetEventObject(invert_btn);
    invert_btn->GetEventHandler()->ProcessEvent(click_event);
    std::set<size_t> expected = {0};
    EXPECT_EQ(dialog.selected_steps(), expected);
    invert_btn->GetEventHandler()->ProcessEvent(click_event);
    // assert
    EXPECT_EQ(dialog.selected_steps(), std::set<size_t>{});
}

// ========================================================================================
// chained operations
// ========================================================================================

TEST(UiStepToolDialogChecks, select_all_then_clear_all) {
    // arrange
    StepInformation step_info({"R1", "TEMP"}, {{1000.0, 27.0}, {2000.0, 85.0}, {3000.0, 125.0}}, {{0.0, 4.0}, {0.0, 4.0}, {0.0, 4.0}});
    StepToolDialog dialog(nullptr, &step_info, {});
    // act
    wxButton* select_all_btn = dynamic_cast<wxButton*>(dialog.FindWindowByLabel("Select All"));
    ASSERT_NE(select_all_btn, nullptr);
    wxCommandEvent click_event(wxEVT_BUTTON, select_all_btn->GetId());
    click_event.SetEventObject(select_all_btn);
    select_all_btn->GetEventHandler()->ProcessEvent(click_event);
    wxButton* clear_all_btn = dynamic_cast<wxButton*>(dialog.FindWindowByLabel("Clear All"));
    ASSERT_NE(clear_all_btn, nullptr);
    wxCommandEvent clear_event(wxEVT_BUTTON, clear_all_btn->GetId());
    clear_event.SetEventObject(clear_all_btn);
    clear_all_btn->GetEventHandler()->ProcessEvent(clear_event);
    // assert
    EXPECT_EQ(dialog.selected_steps(), std::set<size_t>{});
}

TEST(UiStepToolDialogChecks, clear_all_then_select_all) {
    // arrange
    StepInformation step_info({"R1", "TEMP"}, {{1000.0, 27.0}, {2000.0, 85.0}, {3000.0, 125.0}}, {{0.0, 4.0}, {0.0, 4.0}, {0.0, 4.0}});
    StepToolDialog dialog(nullptr, &step_info, {0});
    // act
    wxButton* clear_all_btn = dynamic_cast<wxButton*>(dialog.FindWindowByLabel("Clear All"));
    ASSERT_NE(clear_all_btn, nullptr);
    wxCommandEvent clear_event(wxEVT_BUTTON, clear_all_btn->GetId());
    clear_event.SetEventObject(clear_all_btn);
    clear_all_btn->GetEventHandler()->ProcessEvent(clear_event);
    wxButton* select_all_btn = dynamic_cast<wxButton*>(dialog.FindWindowByLabel("Select All"));
    ASSERT_NE(select_all_btn, nullptr);
    wxCommandEvent click_event(wxEVT_BUTTON, select_all_btn->GetId());
    click_event.SetEventObject(select_all_btn);
    select_all_btn->GetEventHandler()->ProcessEvent(click_event);
    // assert
    std::set<size_t> expected = {0, 1, 2};
    EXPECT_EQ(dialog.selected_steps(), expected);
}

TEST(UiStepToolDialogChecks, invert_then_item_toggle) {
    // arrange
    StepInformation step_info({"R1", "TEMP"}, {{1000.0, 27.0}, {2000.0, 85.0}, {3000.0, 125.0}}, {{0.0, 4.0}, {0.0, 4.0}, {0.0, 4.0}});
    StepToolDialog dialog(nullptr, &step_info, {0});
    // act
    wxButton* invert_btn = dynamic_cast<wxButton*>(dialog.FindWindowByLabel("Invert"));
    ASSERT_NE(invert_btn, nullptr);
    wxCommandEvent click_event(wxEVT_BUTTON, invert_btn->GetId());
    click_event.SetEventObject(invert_btn);
    invert_btn->GetEventHandler()->ProcessEvent(click_event);
    wxListView* step_list = nullptr;
    for (wxWindow* child : dialog.GetChildren()) {
        step_list = dynamic_cast<wxListView*>(child);
        if (step_list != nullptr) {
            break;
        }
    }
    ASSERT_NE(step_list, nullptr);
    step_list->CheckItem(0, true);
    // assert
    std::set<size_t> expected = {0, 1, 2};
    EXPECT_EQ(dialog.selected_steps(), expected);
}
