// #include <span>
// #include <string>
// #include <utility>
// #include <vector>

// #include <gtest/gtest.h>
// #include <wx/app.h>
// #include <wx/listbox.h>
// #include <wx/stattext.h>
// #include <wx/textctrl.h>

// #include "ui/add_plot_dialog.h"

// // helper to create expressions
// namespace
// {
//     Expression<double> make_real_expression(const std::string& name, const std::vector<double>& values, const std::string& unit = "V", const std::string& type = "") {
//         std::vector<double> data(values);
//         std::vector<std::span<const double>> steps = {{data.data(), data.size()}};
//         return {name, data, steps, unit, "", type};
//     }
// } // namespace

// // add plot dialog test fixture
// class AddPlotDialogTest : public ::testing::Test
// {
// protected:
//     wxListBox* get_list_box(const AddPlotDialog& dialog) {
//         // return list box
//         return dialog.m_list_box;
//     }

//     wxTextCtrl* get_custom_input(const AddPlotDialog& dialog) {
//         // return custom input
//         return dialog.m_custom_input;
//     }

//     wxStaticText* get_error_label(const AddPlotDialog& dialog) {
//         // return error label
//         return dialog.m_error_label;
//     }

//     void toggle_selection(AddPlotDialog& dialog, size_t index) {
//         dialog.toggle_expression_selection(index);
//     }

//     void simulate_add_custom(AddPlotDialog& dialog, const std::string& text) {
//         // set value in input field
//         dialog.m_custom_input->SetValue(text);
//         // create command event
//         wxCommandEvent event(wxEVT_BUTTON, wxID_ANY);
//         // invoke event handler directly
//         dialog.on_add_custom(event);
//     }

//     void simulate_ok(AddPlotDialog& dialog) {
//         // create command event
//         wxCommandEvent event(wxEVT_BUTTON, wxID_OK);
//         // invoke event handler directly
//         dialog.on_ok(event);
//     }
// };


// TEST_F(AddPlotDialogTest, constructor_populates_list_box_with_filtered_expressions) {
//     // arrange
//     std::vector<AnyExpression> expressions;
//     expressions.emplace_back(make_real_expression("V(out)", {1.0, 2.0}, "V", "voltage"));
//     expressions.emplace_back(make_real_expression("I(R1)", {0.1, 0.2}, "A", "current"));
//     std::vector<std::pair<size_t, size_t>> slices = {{0, 2}, {2, 4}};
//     ExpressionManager manager(expressions, slices);
//     // act
//     const AddPlotDialog dialog(nullptr, &manager, {}, true, [](const AnyExpression* expr) { return std::visit([](const auto& e) { return e.variable_type() == "voltage"; }, *expr); });
//     wxListBox* list_box = get_list_box(dialog);
//     // assert
//     ASSERT_NE(list_box, nullptr);
//     ASSERT_EQ(list_box->GetCount(), 1);
//     ASSERT_EQ(list_box->GetString(0).ToStdString(), "V(out) [voltage]");
// }

// TEST_F(AddPlotDialogTest, constructor_preselects_specified_expressions) {
//     // arrange
//     std::vector<AnyExpression> expressions;
//     expressions.emplace_back(make_real_expression("V(out)", {1.0}, "V", "voltage"));
//     expressions.emplace_back(make_real_expression("I(R1)", {0.1}, "A", "current"));
//     std::vector<std::pair<size_t, size_t>> slices = {{0, 1}, {1, 2}};
//     ExpressionManager manager(expressions, slices);
//     std::vector<AnyExpression*> selected = {manager.evaluate("V(out)", "V(out)")};
//     // act
//     const AddPlotDialog dialog(nullptr, &manager, selected);
//     wxListBox* list_box = get_list_box(dialog);
//     // assert
//     ASSERT_NE(list_box, nullptr);
//     ASSERT_TRUE(list_box->IsSelected(0));
//     ASSERT_FALSE(list_box->IsSelected(1));
// }

// TEST_F(AddPlotDialogTest, add_custom_adds_and_selects_valid_expression) {
//     // arrange
//     std::vector<AnyExpression> expressions;
//     expressions.emplace_back(make_real_expression("V(out)", {1.0}, "V", "voltage"));
//     std::vector<std::pair<size_t, size_t>> slices = {{0, 1}};
//     ExpressionManager manager(expressions, slices);
//     AddPlotDialog dialog(nullptr, &manager, {});
//     // act
//     simulate_add_custom(dialog, "V(out)");
//     wxListBox* list_box = get_list_box(dialog);
//     wxTextCtrl* custom_input = get_custom_input(dialog);
//     wxStaticText* error_label = get_error_label(dialog);
//     // assert
//     ASSERT_EQ(list_box->GetCount(), 1);
//     ASSERT_TRUE(list_box->IsSelected(0));
//     ASSERT_EQ(custom_input->GetValue().ToStdString(), "");
//     ASSERT_EQ(error_label->GetLabel().ToStdString(), "");
// }

// TEST_F(AddPlotDialogTest, add_custom_sets_error_for_invalid_expression) {
//     // arrange
//     std::vector<AnyExpression> expressions;
//     std::vector<std::pair<size_t, size_t>> slices;
//     ExpressionManager manager(expressions, slices);
//     AddPlotDialog dialog(nullptr, &manager, {});
//     // act
//     simulate_add_custom(dialog, "invalid_expr");
//     wxListBox* list_box = get_list_box(dialog);
//     wxStaticText* error_label = get_error_label(dialog);
//     // assert
//     ASSERT_EQ(list_box->GetCount(), 0);
//     ASSERT_EQ(error_label->GetLabel().ToStdString(), "Invalid expression");
// }

// TEST_F(AddPlotDialogTest, ok_button_updates_selected_expressions) {
//     // arrange
//     std::vector<AnyExpression> expressions;
//     expressions.emplace_back(make_real_expression("V(out)", {1.0}, "V", "voltage"));
//     expressions.emplace_back(make_real_expression("I(R1)", {0.1}, "A", "current"));
//     std::vector<std::pair<size_t, size_t>> slices = {{0, 1}, {1, 2}};
//     ExpressionManager manager(expressions, slices);
//     AddPlotDialog dialog(nullptr, &manager, {});
//     // act
//     toggle_selection(dialog, 1);
//     simulate_ok(dialog);
//     const auto result = dialog.selected_expressions();
//     // assert
//     ASSERT_EQ(result.size(), 1);
//     ASSERT_EQ(std::visit([](const auto& e) { return e.name(); }, *result[0]), "I(R1)");
// }

