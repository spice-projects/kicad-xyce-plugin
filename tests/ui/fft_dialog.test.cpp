#include <span>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/event.h>
#include <wx/frame.h>
#include <wx/textctrl.h>
#endif

#include "expression/expression.h"
#include "expression/expression_manager.h"
#include "fft/fft.h"
#include "ui/fft_dialog.h"

namespace
{
    // number of preset np options exposed by the dialog; the next index is the custom entry
    const int CUSTOM_NP_INDEX = 7;

    Expression<double> make_real_expression(const std::string& name, const std::vector<double>& values, const std::string& unit = "V", const std::string& type = "") {
        std::vector<double> data(values);
        std::vector<std::span<const double>> steps = {{data.data(), data.size()}};
        return {name, std::move(data), std::move(steps), unit, "", type};
    }

    // fixture providing a wx parent frame for the dialog
    class UiFftDialogFixture : public ::testing::Test
    {
    protected:
        void SetUp() override { m_parent = new wxFrame(nullptr, wxID_ANY, "test"); }

        void TearDown() override { delete m_parent; }

        wxFrame* m_parent = nullptr;
    };
} // namespace

// friend access gateway, declared as friend in FftDialog; must live in the global namespace to match the friend declaration
class FftDialogTest
{
public:
    static void select_all_range(FftDialog& dialog) {
        dialog.m_range_mode_all->SetValue(true);
        dialog.on_range_changed();
    }

    static void select_zoom_range(FftDialog& dialog) {
        dialog.m_range_mode_zoom->SetValue(true);
        dialog.on_range_changed();
    }

    static void select_custom_range(FftDialog& dialog) {
        dialog.m_range_mode_custom->SetValue(true);
        dialog.on_range_changed();
    }

    static void set_custom_range(FftDialog& dialog, const std::string& from, const std::string& to) {
        dialog.m_custom_from_input->SetValue(from);
        dialog.m_custom_to_input->SetValue(to);
    }

    static void select_custom_np(FftDialog& dialog) { dialog.m_np_choice->SetSelection(CUSTOM_NP_INDEX); }

    static void set_custom_np(FftDialog& dialog, const std::string& np) { dialog.m_custom_np_input->SetValue(np); }

    static void select_window(FftDialog& dialog, int index) { dialog.m_window_choice->SetSelection(index); }

    static void select_output(FftDialog& dialog, int index) { dialog.m_output_choice->SetSelection(index); }

    static void select_format(FftDialog& dialog, int index) { dialog.m_format_choice->SetSelection(index); }

    static void set_keep_dc(FftDialog& dialog, bool keep) { dialog.m_keep_dc_checkbox->SetValue(keep); }

    static bool custom_inputs_enabled(const FftDialog& dialog) { return dialog.m_custom_from_input->IsEnabled(); }

    static std::string custom_np_text(const FftDialog& dialog) { return dialog.m_custom_np_input->GetValue().ToStdString(); }

    static void simulate_ok(FftDialog& dialog) {
        wxCommandEvent event(wxEVT_BUTTON, wxID_OK);
        dialog.on_ok(event);
    }
};

// ========================================================================================
// constructor defaults
// ========================================================================================

TEST_F(UiFftDialogFixture, constructor_sets_default_parameters) {
    // arrange
    std::vector<AnyExpression> expressions;
    expressions.emplace_back(make_real_expression("V(1)", {1.0, 2.0, 3.0, 4.0}, "V", "voltage"));
    std::vector<std::pair<size_t, size_t>> slices = {{0, 4}};
    ExpressionManager manager(expressions, slices);
    std::vector<AnyExpression*> selected = {manager.expressions()[0]};
    // act
    FftDialog dialog(m_parent, &manager, selected, 0.0, 10.0, 2.0, 8.0);
    // assert
    const auto params = dialog.parameters();
    EXPECT_EQ(params.np, 1024);
    EXPECT_EQ(params.window, fft::WindowFunction::HANNING);
    EXPECT_EQ(params.format, fft::FftFormat::NORM);
    EXPECT_EQ(params.output, fft::FftOutput::MAGNITUDE);
    EXPECT_TRUE(params.keep_dc);
    EXPECT_DOUBLE_EQ(params.start, 2.0);
    EXPECT_DOUBLE_EQ(params.stop, 8.0);
    EXPECT_DOUBLE_EQ(dialog.from_index(), 2.0);
    EXPECT_DOUBLE_EQ(dialog.to_index(), 8.0);
}

// ========================================================================================
// range mode selection
// ========================================================================================

TEST_F(UiFftDialogFixture, select_all_range_uses_full_abscissa_bounds) {
    // arrange
    std::vector<AnyExpression> expressions;
    expressions.emplace_back(make_real_expression("V(1)", {1.0, 2.0, 3.0, 4.0}, "V", "voltage"));
    std::vector<std::pair<size_t, size_t>> slices = {{0, 4}};
    ExpressionManager manager(expressions, slices);
    std::vector<AnyExpression*> selected = {manager.expressions()[0]};
    FftDialog dialog(m_parent, &manager, selected, 0.0, 10.0, 2.0, 8.0);
    // act
    FftDialogTest::select_all_range(dialog);
    // assert
    EXPECT_DOUBLE_EQ(dialog.from_index(), 0.0);
    EXPECT_DOUBLE_EQ(dialog.to_index(), 10.0);
}

TEST_F(UiFftDialogFixture, select_zoom_range_uses_zoomed_bounds) {
    // arrange
    std::vector<AnyExpression> expressions;
    expressions.emplace_back(make_real_expression("V(1)", {1.0, 2.0, 3.0, 4.0}, "V", "voltage"));
    std::vector<std::pair<size_t, size_t>> slices = {{0, 4}};
    ExpressionManager manager(expressions, slices);
    std::vector<AnyExpression*> selected = {manager.expressions()[0]};
    FftDialog dialog(m_parent, &manager, selected, 0.0, 10.0, 2.0, 8.0);
    // act
    FftDialogTest::select_zoom_range(dialog);
    // assert
    EXPECT_DOUBLE_EQ(dialog.from_index(), 2.0);
    EXPECT_DOUBLE_EQ(dialog.to_index(), 8.0);
}

TEST_F(UiFftDialogFixture, select_custom_range_enables_custom_inputs) {
    // arrange
    std::vector<AnyExpression> expressions;
    expressions.emplace_back(make_real_expression("V(1)", {1.0, 2.0, 3.0, 4.0}, "V", "voltage"));
    std::vector<std::pair<size_t, size_t>> slices = {{0, 4}};
    ExpressionManager manager(expressions, slices);
    std::vector<AnyExpression*> selected = {manager.expressions()[0]};
    FftDialog dialog(m_parent, &manager, selected, 0.0, 10.0, 2.0, 8.0);
    // act
    FftDialogTest::select_custom_range(dialog);
    // assert
    EXPECT_TRUE(FftDialogTest::custom_inputs_enabled(dialog));
}

// ========================================================================================
// on_ok parameter building
// ========================================================================================

TEST_F(UiFftDialogFixture, ok_with_all_range_builds_parameters) {
    // arrange
    std::vector<AnyExpression> expressions;
    expressions.emplace_back(make_real_expression("V(1)", {1.0, 2.0, 3.0, 4.0}, "V", "voltage"));
    std::vector<std::pair<size_t, size_t>> slices = {{0, 4}};
    ExpressionManager manager(expressions, slices);
    std::vector<AnyExpression*> selected = {manager.expressions()[0]};
    FftDialog dialog(m_parent, &manager, selected, 0.0, 10.0, 2.0, 8.0);
    // act
    FftDialogTest::simulate_ok(dialog);
    // assert
    const auto params = dialog.parameters();
    EXPECT_DOUBLE_EQ(params.start, 0.0);
    EXPECT_DOUBLE_EQ(params.stop, 10.0);
    EXPECT_EQ(params.np, 1024);
    EXPECT_EQ(params.window, fft::WindowFunction::HANNING);
}

TEST_F(UiFftDialogFixture, ok_with_custom_range_parses_text_values) {
    // arrange
    std::vector<AnyExpression> expressions;
    expressions.emplace_back(make_real_expression("V(1)", {1.0, 2.0, 3.0, 4.0}, "V", "voltage"));
    std::vector<std::pair<size_t, size_t>> slices = {{0, 4}};
    ExpressionManager manager(expressions, slices);
    std::vector<AnyExpression*> selected = {manager.expressions()[0]};
    FftDialog dialog(m_parent, &manager, selected, 0.0, 10.0, 2.0, 8.0);
    FftDialogTest::select_custom_range(dialog);
    FftDialogTest::set_custom_range(dialog, "1.5", "7.5");
    // act
    FftDialogTest::simulate_ok(dialog);
    // assert
    EXPECT_DOUBLE_EQ(dialog.parameters().start, 1.5);
    EXPECT_DOUBLE_EQ(dialog.parameters().stop, 7.5);
}

TEST_F(UiFftDialogFixture, ok_with_custom_np_canonicalizes_to_power_of_two) {
    // arrange
    std::vector<AnyExpression> expressions;
    expressions.emplace_back(make_real_expression("V(1)", {1.0, 2.0, 3.0, 4.0}, "V", "voltage"));
    std::vector<std::pair<size_t, size_t>> slices = {{0, 4}};
    ExpressionManager manager(expressions, slices);
    std::vector<AnyExpression*> selected = {manager.expressions()[0]};
    FftDialog dialog(m_parent, &manager, selected, 0.0, 10.0, 2.0, 8.0);
    FftDialogTest::select_custom_np(dialog);
    FftDialogTest::set_custom_np(dialog, "1000");
    // act
    FftDialogTest::simulate_ok(dialog);
    // assert
    EXPECT_EQ(dialog.parameters().np, 1024);
    EXPECT_EQ(FftDialogTest::custom_np_text(dialog), "1024");
}

TEST_F(UiFftDialogFixture, ok_with_custom_np_rounds_down_when_closer_to_lower) {
    // arrange
    std::vector<AnyExpression> expressions;
    expressions.emplace_back(make_real_expression("V(1)", {1.0, 2.0, 3.0, 4.0}, "V", "voltage"));
    std::vector<std::pair<size_t, size_t>> slices = {{0, 4}};
    ExpressionManager manager(expressions, slices);
    std::vector<AnyExpression*> selected = {manager.expressions()[0]};
    FftDialog dialog(m_parent, &manager, selected, 0.0, 10.0, 2.0, 8.0);
    FftDialogTest::select_custom_np(dialog);
    FftDialogTest::set_custom_np(dialog, "3000");
    // act
    FftDialogTest::simulate_ok(dialog);
    // assert
    EXPECT_EQ(dialog.parameters().np, 2048);
}

TEST_F(UiFftDialogFixture, ok_applies_choice_selections) {
    // arrange
    std::vector<AnyExpression> expressions;
    expressions.emplace_back(make_real_expression("V(1)", {1.0, 2.0, 3.0, 4.0}, "V", "voltage"));
    std::vector<std::pair<size_t, size_t>> slices = {{0, 4}};
    ExpressionManager manager(expressions, slices);
    std::vector<AnyExpression*> selected = {manager.expressions()[0]};
    FftDialog dialog(m_parent, &manager, selected, 0.0, 10.0, 2.0, 8.0);
    // select window index 1 (Hamming), output index 2 (Phase), format index 1 (UNORM), keep dc off
    FftDialogTest::select_window(dialog, 1);
    FftDialogTest::select_output(dialog, 2);
    FftDialogTest::select_format(dialog, 1);
    FftDialogTest::set_keep_dc(dialog, false);
    // act
    FftDialogTest::simulate_ok(dialog);
    // assert
    const auto params = dialog.parameters();
    EXPECT_EQ(params.window, fft::WindowFunction::HAMMING);
    EXPECT_EQ(params.output, fft::FftOutput::PHASE);
    EXPECT_EQ(params.format, fft::FftFormat::UNORM);
    EXPECT_FALSE(params.keep_dc);
}

// ========================================================================================
// on_ok validation
// ========================================================================================

TEST_F(UiFftDialogFixture, ok_without_selection_does_not_update_range) {
    // arrange
    std::vector<AnyExpression> expressions;
    expressions.emplace_back(make_real_expression("V(1)", {1.0, 2.0, 3.0, 4.0}, "V", "voltage"));
    std::vector<std::pair<size_t, size_t>> slices = {{0, 4}};
    ExpressionManager manager(expressions, slices);
    FftDialog dialog(m_parent, &manager, {}, 0.0, 10.0, 2.0, 8.0);
    // attempt to set a custom range; it must not be applied because validation fails first
    FftDialogTest::select_custom_range(dialog);
    FftDialogTest::set_custom_range(dialog, "1.5", "7.5");
    // act
    FftDialogTest::simulate_ok(dialog);
    // assert (the from/to indices remain at the initial zoomed bounds)
    EXPECT_DOUBLE_EQ(dialog.from_index(), 2.0);
    EXPECT_DOUBLE_EQ(dialog.to_index(), 8.0);
}
