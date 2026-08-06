// #include <span>
// #include <string>
// #include <utility>
// #include <vector>

// #include <gtest/gtest.h>

// #include <wx/wxprec.h>

// #ifndef WX_PRECOMP
// #include <wx/app.h>
// #include <wx/stattext.h>
// #include <wx/textctrl.h>
// #endif

// #include "ui/add_plot_dialog.h"

// namespace
// {
//     Expression<double> make_real_expression(const std::string& name, const std::vector<double>& values, const std::string& unit = "V", const std::string& type = "") {
//         std::vector<double> data(values);
//         std::vector<std::span<const double>> steps = {{data.data(), data.size()}};
//         return {name, data, steps, unit, "", type};
//     }
// } // namespace

// // friend access gateway — declared as friend in AddPlotDialog
// class AddPlotDialogTest
// {
// public:
//     static const std::vector<DisplayedExpressionItem>& get_all_expressions(const AddPlotDialog& dialog) {
//         return dialog.m_all_expressions;
//     }

//     static wxTextCtrl* get_custom_input(const AddPlotDialog& dialog) {
//         return dialog.m_custom_input;
//     }

//     static wxStaticText* get_error_label(const AddPlotDialog& dialog) {
//         return dialog.m_error_label;
//     }

//     static void toggle_selection(AddPlotDialog& dialog, size_t index) {
//         dialog.toggle_expression_selection(index);
//     }

//     static void simulate_add_custom(AddPlotDialog& dialog, const std::string& text) {
//         dialog.m_custom_input->SetValue(text);
//         wxCommandEvent event(wxEVT_BUTTON, wxID_ANY);
//         dialog.on_add_custom(event);
//     }

//     static void simulate_ok(AddPlotDialog& dialog) {
//         wxCommandEvent event(wxEVT_BUTTON, wxID_OK);
//         dialog.on_ok(event);
//     }
// };

// // ========================================================================================
// // constructor
// // ========================================================================================

// TEST(AddPlotDialogChecks, constructor_populates_grid_with_filtered_expressions) {
//     // arrange
//     std::vector<AnyExpression> expressions;
//     expressions.emplace_back(make_real_expression("V(out)", {1.0, 2.0}, "V", "voltage"));
//     expressions.emplace_back(make_real_expression("I(R1)", {0.1, 0.2}, "A", "current"));
//     std::vector<std::pair<size_t, size_t>> slices = {{0, 2}, {2, 4}};
//     ExpressionManager manager(expressions, slices);
//     // act
//     const AddPlotDialog dialog(nullptr, &manager, {}, true, [](const AnyExpression* expr) { return std::visit([](const auto& e) { return e.variable_type() == "voltage"; }, *expr); });
//     // assert
//     const auto& items = AddPlotDialogTest::get_all_expressions(dialog);
//     ASSERT_EQ(items.size(), 1);
//     ASSERT_EQ(items[0].name, "V(out)");
//     ASSERT_EQ(items[0].type, "voltage");
// }

// TEST(AddPlotDialogChecks, constructor_preselects_specified_expressions) {
//     // arrange
//     std::vector<AnyExpression> expressions;
//     expressions.emplace_back(make_real_expression("V(out)", {1.0}, "V", "voltage"));
//     expressions.emplace_back(make_real_expression("I(R1)", {0.1}, "A", "current"));
//     std::vector<std::pair<size_t, size_t>> slices = {{0, 1}, {1, 2}};
//     ExpressionManager manager(expressions, slices);
//     std::vector<AnyExpression*> selected = {manager.evaluate("V(out)", "V(out)")};
//     // act
//     const AddPlotDialog dialog(nullptr, &manager, selected);
//     // assert
//     const auto& items = AddPlotDialogTest::get_all_expressions(dialog);
//     ASSERT_EQ(items.size(), 2);
//     ASSERT_TRUE(items[0].selected);
//     ASSERT_FALSE(items[1].selected);
// }

// // ========================================================================================
// // add custom expression
// // ========================================================================================

// TEST(AddPlotDialogChecks, add_custom_adds_and_selects_valid_expression) {
//     // arrange
//     std::vector<AnyExpression> expressions;
//     expressions.emplace_back(make_real_expression("V(out)", {1.0}, "V", "voltage"));
//     std::vector<std::pair<size_t, size_t>> slices = {{0, 1}};
//     ExpressionManager manager(expressions, slices);
//     AddPlotDialog dialog(nullptr, &manager, {});
//     // act
//     AddPlotDialogTest::simulate_add_custom(dialog, "V(out)");
//     // assert
//     const auto& items = AddPlotDialogTest::get_all_expressions(dialog);
//     ASSERT_EQ(items.size(), 1);
//     ASSERT_TRUE(items[0].selected);
//     ASSERT_EQ(AddPlotDialogTest::get_custom_input(dialog)->GetValue().ToStdString(), "");
//     ASSERT_FALSE(AddPlotDialogTest::get_error_label(dialog)->IsShown());
// }

// TEST(AddPlotDialogChecks, add_custom_sets_error_for_invalid_expression) {
//     // arrange
//     std::vector<AnyExpression> expressions;
//     std::vector<std::pair<size_t, size_t>> slices = {{0, 1}};
//     ExpressionManager manager(expressions, slices);
//     AddPlotDialog dialog(nullptr, &manager, {});
//     // act
//     AddPlotDialogTest::simulate_add_custom(dialog, "invalid_expr");
//     // assert
//     ASSERT_TRUE(AddPlotDialogTest::get_all_expressions(dialog).empty());
//     ASSERT_EQ(AddPlotDialogTest::get_error_label(dialog)->GetLabel().ToStdString(), "Invalid expression");
//     ASSERT_TRUE(AddPlotDialogTest::get_error_label(dialog)->IsShown());
// }

// // ========================================================================================
// // ok button
// // ========================================================================================

// TEST(AddPlotDialogChecks, ok_button_updates_selected_expressions) {
//     // arrange
//     std::vector<AnyExpression> expressions;
//     expressions.emplace_back(make_real_expression("V(out)", {1.0}, "V", "voltage"));
//     expressions.emplace_back(make_real_expression("I(R1)", {0.1}, "A", "current"));
//     std::vector<std::pair<size_t, size_t>> slices = {{0, 1}, {1, 2}};
//     ExpressionManager manager(expressions, slices);
//     AddPlotDialog dialog(nullptr, &manager, {});
//     // act
//     AddPlotDialogTest::toggle_selection(dialog, 1);
//     AddPlotDialogTest::simulate_ok(dialog);
//     // assert
//     const auto result = dialog.selected_expressions();
//     ASSERT_EQ(result.size(), 1);
//     ASSERT_EQ(std::visit([](const auto& e) { return e.name(); }, **result.begin()), "I(R1)");
// }
