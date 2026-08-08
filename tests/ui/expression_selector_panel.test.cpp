#include <set>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/event.h>
#include <wx/frame.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#endif

#include "expression/expression.h"
#include "expression/expression_manager.h"
#include "ui/expression_selector_panel.h"

namespace
{
    Expression<double> make_real_expression(const std::string& name, const std::vector<double>& values, const std::string& unit = "V", const std::string& type = "") {
        std::vector<double> data(values);
        std::vector<std::span<const double>> steps = {{data.data(), data.size()}};
        return {name, std::move(data), std::move(steps), unit, "", type};
    }

    // fixture providing a wx parent frame for the panel
    class ExpressionSelectorPanelFixture : public ::testing::Test
    {
    protected:
        void SetUp() override { m_parent = new wxFrame(nullptr, wxID_ANY, "test"); }

        void TearDown() override { delete m_parent; }

        wxFrame* m_parent = nullptr;
    };
} // namespace

// friend access gateway, declared as friend in ExpressionSelectorPanel; must live in the global namespace to match the friend declaration
class ExpressionSelectorPanelTest
{
public:
    static const std::vector<ExpressionItem>& all_expressions(const ExpressionSelectorPanel& panel) { return panel.m_all_expressions; }

    static void toggle_selection(ExpressionSelectorPanel& panel, size_t index) { panel.toggle_expression_selection(index); }

    static void simulate_filter(ExpressionSelectorPanel& panel, const std::string& text) {
        panel.m_filter_input->SetValue(text);
        wxCommandEvent event(wxEVT_TEXT, wxID_ANY);
        panel.on_filter_text_changed(event);
    }

    static void simulate_add_custom(ExpressionSelectorPanel& panel, const std::string& text) {
        panel.m_custom_input->SetValue(text);
        wxCommandEvent event(wxEVT_BUTTON, wxID_ANY);
        panel.on_add_custom(event);
    }

    static bool custom_input_shown(const ExpressionSelectorPanel& panel) { return panel.m_custom_input != nullptr; }

    static wxStaticText* error_label(const ExpressionSelectorPanel& panel) { return panel.m_error_label; }

    static wxTextCtrl* custom_input(const ExpressionSelectorPanel& panel) { return panel.m_custom_input; }
};

// ========================================================================================
// constructor
// ========================================================================================

TEST_F(ExpressionSelectorPanelFixture, constructor_populates_all_filtered_expressions) {
    // arrange
    std::vector<AnyExpression> expressions;
    expressions.emplace_back(make_real_expression("V(out)", {1.0, 2.0}, "V", "voltage"));
    expressions.emplace_back(make_real_expression("I(R1)", {0.1, 0.2}, "A", "current"));
    std::vector<std::pair<size_t, size_t>> slices = {{0, 2}, {2, 4}};
    ExpressionManager manager(expressions, slices);
    // act
    ExpressionSelectorPanel panel(m_parent, &manager, {}, ExpressionSelectorConfig{});
    // assert
    const auto& items = ExpressionSelectorPanelTest::all_expressions(panel);
    ASSERT_EQ(items.size(), 2);
    EXPECT_EQ(items[0].name, "V(out)");
    EXPECT_EQ(items[0].type, "voltage");
    EXPECT_EQ(items[1].name, "I(R1)");
    EXPECT_EQ(items[1].type, "current");
    EXPECT_FALSE(items[0].selected);
    EXPECT_FALSE(items[1].selected);
}

TEST_F(ExpressionSelectorPanelFixture, constructor_applies_expression_filter) {
    // arrange
    std::vector<AnyExpression> expressions;
    expressions.emplace_back(make_real_expression("V(out)", {1.0, 2.0}, "V", "voltage"));
    expressions.emplace_back(make_real_expression("I(R1)", {0.1, 0.2}, "A", "current"));
    std::vector<std::pair<size_t, size_t>> slices = {{0, 2}, {2, 4}};
    ExpressionManager manager(expressions, slices);
    // act
    ExpressionSelectorPanel panel(m_parent, &manager, {}, ExpressionSelectorConfig{}, [](const AnyExpression* expr) { return std::visit([](const auto& e) { return e.variable_type() == "voltage"; }, *expr); });
    // assert
    const auto& items = ExpressionSelectorPanelTest::all_expressions(panel);
    ASSERT_EQ(items.size(), 1);
    EXPECT_EQ(items[0].name, "V(out)");
}

TEST_F(ExpressionSelectorPanelFixture, constructor_preselects_specified_expressions) {
    // arrange
    std::vector<AnyExpression> expressions;
    expressions.emplace_back(make_real_expression("V(out)", {1.0, 2.0}, "V", "voltage"));
    expressions.emplace_back(make_real_expression("I(R1)", {0.1, 0.2}, "A", "current"));
    std::vector<std::pair<size_t, size_t>> slices = {{0, 2}, {2, 4}};
    ExpressionManager manager(expressions, slices);
    std::vector<AnyExpression*> selected = {manager.expressions()[0]};
    // act
    ExpressionSelectorPanel panel(m_parent, &manager, selected, ExpressionSelectorConfig{});
    // assert
    const auto& items = ExpressionSelectorPanelTest::all_expressions(panel);
    ASSERT_EQ(items.size(), 2);
    EXPECT_TRUE(items[0].selected);
    EXPECT_FALSE(items[1].selected);
}

// ========================================================================================
// selection
// ========================================================================================

TEST_F(ExpressionSelectorPanelFixture, selected_expressions_returns_selected_set) {
    // arrange
    std::vector<AnyExpression> expressions;
    expressions.emplace_back(make_real_expression("V(out)", {1.0, 2.0}, "V", "voltage"));
    expressions.emplace_back(make_real_expression("I(R1)", {0.1, 0.2}, "A", "current"));
    std::vector<std::pair<size_t, size_t>> slices = {{0, 2}, {2, 4}};
    ExpressionManager manager(expressions, slices);
    std::vector<AnyExpression*> selected = {manager.expressions()[0]};
    ExpressionSelectorPanel panel(m_parent, &manager, selected, ExpressionSelectorConfig{});
    // act
    const auto result = panel.selected_expressions();
    // assert
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result, std::set<AnyExpression*>{manager.expressions()[0]});
}

TEST_F(ExpressionSelectorPanelFixture, toggle_selection_updates_selected_set) {
    // arrange
    std::vector<AnyExpression> expressions;
    expressions.emplace_back(make_real_expression("V(out)", {1.0, 2.0}, "V", "voltage"));
    expressions.emplace_back(make_real_expression("I(R1)", {0.1, 0.2}, "A", "current"));
    std::vector<std::pair<size_t, size_t>> slices = {{0, 2}, {2, 4}};
    ExpressionManager manager(expressions, slices);
    ExpressionSelectorPanel panel(m_parent, &manager, {}, ExpressionSelectorConfig{});
    // act
    ExpressionSelectorPanelTest::toggle_selection(panel, 1);
    // assert
    const auto result = panel.selected_expressions();
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result, std::set<AnyExpression*>{manager.expressions()[1]});
}

// ========================================================================================
// filter text
// ========================================================================================

TEST_F(ExpressionSelectorPanelFixture, filter_text_hides_nonmatching_expressions) {
    // arrange
    std::vector<AnyExpression> expressions;
    expressions.emplace_back(make_real_expression("V(out)", {1.0, 2.0}, "V", "voltage"));
    expressions.emplace_back(make_real_expression("I(R1)", {0.1, 0.2}, "A", "current"));
    std::vector<std::pair<size_t, size_t>> slices = {{0, 2}, {2, 4}};
    ExpressionManager manager(expressions, slices);
    ExpressionSelectorConfig config;
    config.show_filter = true;
    ExpressionSelectorPanel panel(m_parent, &manager, {}, config);
    // act
    ExpressionSelectorPanelTest::simulate_filter(panel, "out");
    // assert
    const auto result = panel.selected_expressions();
    const auto& items = ExpressionSelectorPanelTest::all_expressions(panel);
    EXPECT_EQ(result.size(), 0);
    ASSERT_EQ(items.size(), 2);
    EXPECT_TRUE(items[0].name.find("out") != std::string::npos);
}

TEST_F(ExpressionSelectorPanelFixture, filter_text_clears_to_show_all) {
    // arrange
    std::vector<AnyExpression> expressions;
    expressions.emplace_back(make_real_expression("V(out)", {1.0, 2.0}, "V", "voltage"));
    expressions.emplace_back(make_real_expression("I(R1)", {0.1, 0.2}, "A", "current"));
    std::vector<std::pair<size_t, size_t>> slices = {{0, 2}, {2, 4}};
    ExpressionManager manager(expressions, slices);
    ExpressionSelectorConfig config;
    config.show_filter = true;
    ExpressionSelectorPanel panel(m_parent, &manager, {}, config);
    ExpressionSelectorPanelTest::simulate_filter(panel, "R1");
    // act
    ExpressionSelectorPanelTest::simulate_filter(panel, "");
    // assert
    const auto& items = ExpressionSelectorPanelTest::all_expressions(panel);
    ASSERT_EQ(items.size(), 2);
}

// ========================================================================================
// validation
// ========================================================================================

TEST_F(ExpressionSelectorPanelFixture, validate_selection_fails_when_empty_and_not_allowed) {
    // arrange
    std::vector<AnyExpression> expressions;
    expressions.emplace_back(make_real_expression("V(out)", {1.0, 2.0}, "V", "voltage"));
    std::vector<std::pair<size_t, size_t>> slices = {{0, 2}};
    ExpressionManager manager(expressions, slices);
    ExpressionSelectorConfig config;
    config.allow_empty_selection = false;
    ExpressionSelectorPanel panel(m_parent, &manager, {}, config);
    // act
    std::string error_message;
    const bool valid = panel.validate_selection(&error_message);
    // assert
    EXPECT_FALSE(valid);
    EXPECT_FALSE(error_message.empty());
}

TEST_F(ExpressionSelectorPanelFixture, validate_selection_passes_when_selected) {
    // arrange
    std::vector<AnyExpression> expressions;
    expressions.emplace_back(make_real_expression("V(out)", {1.0, 2.0}, "V", "voltage"));
    std::vector<std::pair<size_t, size_t>> slices = {{0, 2}};
    ExpressionManager manager(expressions, slices);
    std::vector<AnyExpression*> selected = {manager.expressions()[0]};
    ExpressionSelectorConfig config;
    config.allow_empty_selection = false;
    ExpressionSelectorPanel panel(m_parent, &manager, selected, config);
    // act
    const bool valid = panel.validate_selection();
    // assert
    EXPECT_TRUE(valid);
}

TEST_F(ExpressionSelectorPanelFixture, validate_selection_allows_empty_when_configured) {
    // arrange
    std::vector<AnyExpression> expressions;
    expressions.emplace_back(make_real_expression("V(out)", {1.0, 2.0}, "V", "voltage"));
    std::vector<std::pair<size_t, size_t>> slices = {{0, 2}};
    ExpressionManager manager(expressions, slices);
    ExpressionSelectorConfig config;
    config.allow_empty_selection = true;
    ExpressionSelectorPanel panel(m_parent, &manager, {}, config);
    // act
    const bool valid = panel.validate_selection();
    // assert
    EXPECT_TRUE(valid);
}

// ========================================================================================
// custom expression input
// ========================================================================================

TEST_F(ExpressionSelectorPanelFixture, add_custom_marks_existing_expression_selected) {
    // arrange
    std::vector<AnyExpression> expressions;
    expressions.emplace_back(make_real_expression("V(out)", {1.0, 2.0}, "V", "voltage"));
    expressions.emplace_back(make_real_expression("I(R1)", {0.1, 0.2}, "A", "current"));
    std::vector<std::pair<size_t, size_t>> slices = {{0, 2}, {2, 4}};
    ExpressionManager manager(expressions, slices);
    ExpressionSelectorConfig config;
    config.show_custom_input = true;
    ExpressionSelectorPanel panel(m_parent, &manager, {}, config);
    ASSERT_TRUE(ExpressionSelectorPanelTest::custom_input_shown(panel));
    // act
    ExpressionSelectorPanelTest::simulate_add_custom(panel, "I(R1)");
    // assert
    const auto result = panel.selected_expressions();
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result, std::set<AnyExpression*>{manager.expressions()[1]});
    EXPECT_EQ(ExpressionSelectorPanelTest::custom_input(panel)->GetValue().ToStdString(), "");
}

TEST_F(ExpressionSelectorPanelFixture, add_custom_invalid_expression_shows_error) {
    // arrange
    std::vector<AnyExpression> expressions;
    expressions.emplace_back(make_real_expression("V(out)", {1.0, 2.0}, "V", "voltage"));
    std::vector<std::pair<size_t, size_t>> slices = {{0, 2}};
    ExpressionManager manager(expressions, slices);
    ExpressionSelectorConfig config;
    config.show_custom_input = true;
    ExpressionSelectorPanel panel(m_parent, &manager, {}, config);
    // act
    ExpressionSelectorPanelTest::simulate_add_custom(panel, "invalid_expr_###");
    // assert
    EXPECT_TRUE(ExpressionSelectorPanelTest::error_label(panel)->IsShown());
    EXPECT_EQ(ExpressionSelectorPanelTest::error_label(panel)->GetLabel().ToStdString(), "Invalid expression");
    EXPECT_TRUE(panel.selected_expressions().empty());
}
