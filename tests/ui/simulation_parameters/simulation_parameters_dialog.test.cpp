#include <gtest/gtest.h>

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/frame.h>
#endif

#include "simulation_parameters/ac_simulation_parameters.h"
#include "simulation_parameters/data_block.h"
#include "simulation_parameters/dc_simulation_parameters.h"
#include "simulation_parameters/hb_simulation_parameters.h"
#include "simulation_parameters/lin_simulation_parameters.h"
#include "simulation_parameters/noise_simulation_parameters.h"
#include "simulation_parameters/op_simulation_parameters.h"
#include "simulation_parameters/option_parameters.h"
#include "simulation_parameters/print_parameters.h"
#include "simulation_parameters/sens_parameter.h"
#include "simulation_parameters/simulation_config.h"
#include "simulation_parameters/step_parameters.h"
#include "simulation_parameters/transient_simulation_parameters.h"
#include "ui/simulation_parameters/simulation_parameters_dialog.h"

namespace
{
    // parent frame fixture for wxDialog-based tests
    class UiSimulationParametersDialogTest : public ::testing::Test
    {
    protected:
        void SetUp() override { m_parent = new wxFrame(nullptr, wxID_ANY, "test"); }

        void TearDown() override { delete m_parent; }

        wxFrame* m_parent = nullptr;
    };

    // empty option parameters used as default in all test configs
    const OptionParameters EMPTY_OPTIONS({}, {}, {}, {}, {});
} // namespace

// ========================================================================================
// constructor — basic smoke tests
// ========================================================================================

TEST_F(UiSimulationParametersDialogTest, constructor_creates_dialog) {
    // arrange / act
    OpSimulationParameters op_params(false, false, false, std::vector<std::string>{}, "", "", false, "NODESET", "", {}, {}, std::nullopt);
    SimulationConfig config("OP", op_params, {}, {}, EMPTY_OPTIONS, {});
    SimulationParametersDialog dialog(m_parent, config);
    // assert
    SUCCEED();
}

// ========================================================================================
// constructor — initial page selection
// ========================================================================================

TEST_F(UiSimulationParametersDialogTest, constructor_selects_op_page) {
    // arrange / act
    SimulationConfig config("OP", std::monostate{}, {}, {}, EMPTY_OPTIONS, {});
    SimulationParametersDialog dialog(m_parent, config);
    auto output = dialog.get_config();
    // assert
    EXPECT_EQ(output.analysis_type, "OP");
}

TEST_F(UiSimulationParametersDialogTest, constructor_selects_tran_page) {
    // arrange / act
    SimulationConfig config("TRAN", std::monostate{}, {}, {}, EMPTY_OPTIONS, {});
    SimulationParametersDialog dialog(m_parent, config);
    auto output = dialog.get_config();
    // assert
    EXPECT_EQ(output.analysis_type, "TRAN");
}

TEST_F(UiSimulationParametersDialogTest, constructor_selects_dc_page) {
    // arrange / act
    SimulationConfig config("DC", std::monostate{}, {}, {}, EMPTY_OPTIONS, {});
    SimulationParametersDialog dialog(m_parent, config);
    auto output = dialog.get_config();
    // assert
    EXPECT_EQ(output.analysis_type, "DC");
}

TEST_F(UiSimulationParametersDialogTest, constructor_selects_ac_page) {
    // arrange / act
    SimulationConfig config("AC", std::monostate{}, {}, {}, EMPTY_OPTIONS, {});
    SimulationParametersDialog dialog(m_parent, config);
    auto output = dialog.get_config();
    // assert
    EXPECT_EQ(output.analysis_type, "AC");
}

TEST_F(UiSimulationParametersDialogTest, constructor_selects_noise_page) {
    // arrange / act
    SimulationConfig config("NOISE", std::monostate{}, {}, {}, EMPTY_OPTIONS, {});
    SimulationParametersDialog dialog(m_parent, config);
    auto output = dialog.get_config();
    // assert
    EXPECT_EQ(output.analysis_type, "NOISE");
}

TEST_F(UiSimulationParametersDialogTest, constructor_selects_hb_page) {
    // arrange / act
    SimulationConfig config("HB", std::monostate{}, {}, {}, EMPTY_OPTIONS, {});
    SimulationParametersDialog dialog(m_parent, config);
    auto output = dialog.get_config();
    // assert
    EXPECT_EQ(output.analysis_type, "HB");
}

TEST_F(UiSimulationParametersDialogTest, constructor_selects_lin_page) {
    // arrange / act
    SimulationConfig config("LIN", std::monostate{}, {}, {}, EMPTY_OPTIONS, {});
    SimulationParametersDialog dialog(m_parent, config);
    auto output = dialog.get_config();
    // assert
    EXPECT_EQ(output.analysis_type, "LIN");
}

// ========================================================================================
// get_config — round-trip with default parameters
// ========================================================================================

TEST_F(UiSimulationParametersDialogTest, get_config_roundtrip_op_defaults) {
    // arrange
    OpSimulationParameters op_params(false, false, false, std::vector<std::string>{}, "", "", false, "NODESET", "", {}, {}, std::nullopt);
    SimulationConfig input("OP", op_params, {}, {}, EMPTY_OPTIONS, {});
    SimulationParametersDialog dialog(m_parent, input);
    // act
    auto output = dialog.get_config();
    // assert
    EXPECT_EQ(output.analysis_type, input.analysis_type);
    ASSERT_TRUE(std::holds_alternative<OpSimulationParameters>(output.analysis));
    EXPECT_EQ(std::get<OpSimulationParameters>(output.analysis), op_params);
    EXPECT_TRUE(output.steps.empty());
}

TEST_F(UiSimulationParametersDialogTest, get_config_roundtrip_tran_defaults) {
    // arrange
    TransientSimulationParameters tran_params("", "", "", "", "", {}, std::nullopt, {}, {}, {}, std::nullopt);
    SimulationConfig input("TRAN", tran_params, {}, {}, EMPTY_OPTIONS, {});
    SimulationParametersDialog dialog(m_parent, input);
    // act
    auto output = dialog.get_config();
    // assert
    EXPECT_EQ(output.analysis_type, input.analysis_type);
    ASSERT_TRUE(std::holds_alternative<TransientSimulationParameters>(output.analysis));
    EXPECT_EQ(std::get<TransientSimulationParameters>(output.analysis), tran_params);
    EXPECT_TRUE(output.steps.empty());
}

TEST_F(UiSimulationParametersDialogTest, get_config_roundtrip_dc_defaults) {
    // arrange — panel defaults use sweep_mode="LIN" when not specified
    DCSimulationParameters dc_params("LIN", "", "", "", "", "", {}, "", "", "", "", "", "", std::nullopt, {}, std::nullopt);
    SimulationConfig input("DC", dc_params, {}, {}, EMPTY_OPTIONS, {});
    SimulationParametersDialog dialog(m_parent, input);
    // act
    auto output = dialog.get_config();
    // assert
    EXPECT_EQ(output.analysis_type, input.analysis_type);
    ASSERT_TRUE(std::holds_alternative<DCSimulationParameters>(output.analysis));
    EXPECT_EQ(std::get<DCSimulationParameters>(output.analysis), dc_params);
    EXPECT_TRUE(output.steps.empty());
}

TEST_F(UiSimulationParametersDialogTest, get_config_roundtrip_ac_defaults) {
    // arrange — panel defaults use sweep_mode="LIN" when not specified
    AcSimulationParameters ac_params("LIN", "", "", "", "", std::nullopt, {}, std::nullopt);
    SimulationConfig input("AC", ac_params, {}, {}, EMPTY_OPTIONS, {});
    SimulationParametersDialog dialog(m_parent, input);
    // act
    auto output = dialog.get_config();
    // assert
    EXPECT_EQ(output.analysis_type, input.analysis_type);
    ASSERT_TRUE(std::holds_alternative<AcSimulationParameters>(output.analysis));
    EXPECT_EQ(std::get<AcSimulationParameters>(output.analysis), ac_params);
    EXPECT_TRUE(output.steps.empty());
}

TEST_F(UiSimulationParametersDialogTest, get_config_roundtrip_noise_defaults) {
    // arrange — panel defaults use sweep_type="LIN" when not specified
    NoiseSimulationParameters noise_params("", "", "", "", "", "", "LIN", {}, "", std::nullopt);
    SimulationConfig input("NOISE", noise_params, {}, {}, EMPTY_OPTIONS, {});
    SimulationParametersDialog dialog(m_parent, input);
    // act
    auto output = dialog.get_config();
    // assert
    EXPECT_EQ(output.analysis_type, input.analysis_type);
    ASSERT_TRUE(std::holds_alternative<NoiseSimulationParameters>(output.analysis));
    EXPECT_EQ(std::get<NoiseSimulationParameters>(output.analysis), noise_params);
    EXPECT_TRUE(output.steps.empty());
}

TEST_F(UiSimulationParametersDialogTest, get_config_roundtrip_hb_defaults) {
    // arrange
    HbSimulationParameters hb_params({}, {}, std::nullopt, std::nullopt, std::nullopt, std::nullopt, {}, {});
    SimulationConfig input("HB", hb_params, {}, {}, EMPTY_OPTIONS, {});
    SimulationParametersDialog dialog(m_parent, input);
    // act
    auto output = dialog.get_config();
    // assert
    EXPECT_EQ(output.analysis_type, input.analysis_type);
    ASSERT_TRUE(std::holds_alternative<HbSimulationParameters>(output.analysis));
    EXPECT_EQ(std::get<HbSimulationParameters>(output.analysis), hb_params);
    EXPECT_TRUE(output.steps.empty());
}

TEST_F(UiSimulationParametersDialogTest, get_config_roundtrip_lin_defaults) {
    // arrange — panel defaults use sparcalc=true, format="TOUCHSTONE2",
    //           lintype="S", dataformat="RI", sweep_mode="LIN"
    LinSimulationParameters lin_params(true, "TOUCHSTONE2", "S", "RI", "", "", "", "LIN", "", "", "", "", std::nullopt);
    SimulationConfig input("LIN", lin_params, {}, {}, EMPTY_OPTIONS, {});
    SimulationParametersDialog dialog(m_parent, input);
    // act
    auto output = dialog.get_config();
    // assert
    EXPECT_EQ(output.analysis_type, input.analysis_type);
    ASSERT_TRUE(std::holds_alternative<LinSimulationParameters>(output.analysis));
    EXPECT_EQ(std::get<LinSimulationParameters>(output.analysis), lin_params);
    EXPECT_TRUE(output.steps.empty());
}

// ========================================================================================
// get_config — round-trip with custom values and step parameters
// ========================================================================================

TEST_F(UiSimulationParametersDialogTest, get_config_roundtrip_op_with_step) {
    // arrange
    OpSimulationParameters op_params(false, false, false, std::vector<std::string>{}, "", "", true, "IC", "save.dat", {}, {}, std::nullopt);
    StepParameters step("LIN", "TEMP", "0", "100", "1", "", {}, "", true);
    SimulationConfig input("OP", op_params, {step}, {}, EMPTY_OPTIONS, {});
    SimulationParametersDialog dialog(m_parent, input);
    // act
    auto output = dialog.get_config();
    // assert
    EXPECT_EQ(output.analysis_type, "OP");
    ASSERT_TRUE(std::holds_alternative<OpSimulationParameters>(output.analysis));
    EXPECT_EQ(std::get<OpSimulationParameters>(output.analysis), op_params);
    ASSERT_EQ(output.steps.size(), 1);
    EXPECT_EQ(output.steps[0], step);
}

TEST_F(UiSimulationParametersDialogTest, get_config_roundtrip_tran_with_sensitivity_and_step) {
    // arrange
    std::vector<TransientSchedulePoint> schedule = {TransientSchedulePoint("1u", "10n")};
    auto print_params = PrintParameters("TRAN", "RAW", "tran.raw", {"V(1)"}, {});
    auto sens = SensParameter("TRAN", "objvars", {"V(1)"}, {"R1"}, true, false, std::nullopt);
    TransientSimulationParameters tran_params("1u", "1m", "0", "5u", "NOOP", schedule, print_params, {}, {}, {}, sens);
    StepParameters step("LIN", "TEMP", "0", "100", "1", "", {}, "", true);
    SimulationConfig input("TRAN", tran_params, {step}, {}, EMPTY_OPTIONS, {});
    SimulationParametersDialog dialog(m_parent, input);
    // act
    auto output = dialog.get_config();
    // assert
    EXPECT_EQ(output.analysis_type, "TRAN");
    ASSERT_TRUE(std::holds_alternative<TransientSimulationParameters>(output.analysis));
    EXPECT_EQ(std::get<TransientSimulationParameters>(output.analysis), tran_params);
    ASSERT_EQ(output.steps.size(), 1);
    EXPECT_EQ(output.steps[0], step);
}

TEST_F(UiSimulationParametersDialogTest, get_config_roundtrip_dc_with_sensitivity_and_step) {
    // arrange
    auto print_params = PrintParameters("DC", "RAW", "dc.raw", {"V(1)"}, {});
    auto sens = SensParameter("DC", "objfunc", {}, {"R1"}, false, true, std::nullopt);
    DCSimulationParameters dc_params("LIN", "V1", "0", "5", "0.1", "", {}, "", "V2", "0", "10", "1", "", print_params, {}, sens);
    StepParameters step("DEC", "R1", "10", "1k", "", "5", {}, "", true);
    SimulationConfig input("DC", dc_params, {step}, {}, EMPTY_OPTIONS, {});
    SimulationParametersDialog dialog(m_parent, input);
    // act
    auto output = dialog.get_config();
    // assert
    EXPECT_EQ(output.analysis_type, "DC");
    ASSERT_TRUE(std::holds_alternative<DCSimulationParameters>(output.analysis));
    EXPECT_EQ(std::get<DCSimulationParameters>(output.analysis), dc_params);
    ASSERT_EQ(output.steps.size(), 1);
    EXPECT_EQ(output.steps[0], step);
}

TEST_F(UiSimulationParametersDialogTest, get_config_roundtrip_ac_with_sensitivity_and_step) {
    // arrange
    auto print_params = PrintParameters("AC", "CSV", "ac.csv", {"V(out)"}, {});
    auto sens = SensParameter("AC", "objvars", {"V(out)"}, {"C1", "R1"}, true, false, std::nullopt);
    AcSimulationParameters ac_params("DEC", "100", "1", "1MEG", "", print_params, {}, sens);
    StepParameters step("OCT", "C1", "1n", "1u", "", "5", {}, "", true);
    SimulationConfig input("AC", ac_params, {step}, {}, EMPTY_OPTIONS, {});
    SimulationParametersDialog dialog(m_parent, input);
    // act
    auto output = dialog.get_config();
    // assert
    EXPECT_EQ(output.analysis_type, "AC");
    ASSERT_TRUE(std::holds_alternative<AcSimulationParameters>(output.analysis));
    EXPECT_EQ(std::get<AcSimulationParameters>(output.analysis), ac_params);
    ASSERT_EQ(output.steps.size(), 1);
    EXPECT_EQ(output.steps[0], step);
}

TEST_F(UiSimulationParametersDialogTest, get_config_roundtrip_noise_with_step) {
    // arrange
    std::vector<DeviceNoiseOperator> dno;
    dno.emplace_back("DNI", "OUT", "V1");
    auto print_params = PrintParameters("NOISE", "RAW", "noise.raw", {"V(OUT)"}, {});
    NoiseSimulationParameters noise_params("OUT", "0", "V1", "10", "100k", "100", "DEC", dno, "", print_params);
    StepParameters step("LIN", "TEMP", "-50", "150", "5", "", {}, "", true);
    SimulationConfig input("NOISE", noise_params, {step}, {}, EMPTY_OPTIONS, {});
    SimulationParametersDialog dialog(m_parent, input);
    // act
    auto output = dialog.get_config();
    // assert
    EXPECT_EQ(output.analysis_type, "NOISE");
    ASSERT_TRUE(std::holds_alternative<NoiseSimulationParameters>(output.analysis));
    EXPECT_EQ(std::get<NoiseSimulationParameters>(output.analysis), noise_params);
    ASSERT_EQ(output.steps.size(), 1);
    EXPECT_EQ(output.steps[0], step);
}

TEST_F(UiSimulationParametersDialogTest, get_config_roundtrip_hb_with_step) {
    // arrange
    std::map<std::string, std::string> nonlin_opts;
    nonlin_opts["MAXITER"] = "100";
    std::map<std::string, std::string> linsol_opts;
    linsol_opts["METHOD"] = "KINSOL";
    auto print_params = PrintParameters("HB_FD", "CSV", "hb.csv", {"V(1)"}, {});
    HbSimulationParameters hb_params({"1e6", "2e6"}, {5, 7}, 10, "ALL", 10, print_params, nonlin_opts, linsol_opts);
    StepParameters step("LIST", "R1", "", "", "", "", {"1k", "2k", "5k"}, "", true);
    SimulationConfig input("HB", hb_params, {step}, {}, EMPTY_OPTIONS, {});
    SimulationParametersDialog dialog(m_parent, input);
    // act
    auto output = dialog.get_config();
    // assert
    EXPECT_EQ(output.analysis_type, "HB");
    ASSERT_TRUE(std::holds_alternative<HbSimulationParameters>(output.analysis));
    EXPECT_EQ(std::get<HbSimulationParameters>(output.analysis), hb_params);
    ASSERT_EQ(output.steps.size(), 1);
    EXPECT_EQ(output.steps[0], step);
}

TEST_F(UiSimulationParametersDialogTest, get_config_roundtrip_lin_with_step) {
    // arrange
    auto print_params = PrintParameters("AC", "RAW", "lin.raw", {"V(*)"}, {});
    LinSimulationParameters lin_params(true, "CITIFILE", "Z", "DB", "out.s2p", "10", "6", "DEC", "100", "1k", "1M", "", print_params);
    StepParameters step("DATA", "", "", "", "", "", {}, "myTable", true);
    SimulationConfig input("LIN", lin_params, {step}, {}, EMPTY_OPTIONS, {});
    SimulationParametersDialog dialog(m_parent, input);
    // act
    auto output = dialog.get_config();
    // assert
    EXPECT_EQ(output.analysis_type, "LIN");
    ASSERT_TRUE(std::holds_alternative<LinSimulationParameters>(output.analysis));
    EXPECT_EQ(std::get<LinSimulationParameters>(output.analysis), lin_params);
    ASSERT_EQ(output.steps.size(), 1);
    EXPECT_EQ(output.steps[0], step);
}

// ========================================================================================
// get_config — no step returned when step is disabled
// ========================================================================================

TEST_F(UiSimulationParametersDialogTest, get_config_returns_no_step_when_disabled) {
    // arrange
    OpSimulationParameters op_params(false, false, false, std::vector<std::string>{}, "", "", false, "NODESET", "", {}, {}, std::nullopt);
    StepParameters step("LIN", "TEMP", "0", "100", "1", "", {}, "", false);
    SimulationConfig input("OP", op_params, {step}, {}, EMPTY_OPTIONS, {});
    SimulationParametersDialog dialog(m_parent, input);
    // act
    auto output = dialog.get_config();
    // assert
    EXPECT_TRUE(output.steps.empty());
}

// ========================================================================================
// get_config — data blocks are preserved
// ========================================================================================

TEST_F(UiSimulationParametersDialogTest, get_config_preserves_data_blocks) {
    // arrange
    std::vector<std::string> params = {"TIME", "V(1)"};
    std::vector<std::vector<std::string>> records = {{"0.0", "0.0"}, {"1e-6", "1.5"}};
    std::vector<DataBlock> data_blocks;
    data_blocks.emplace_back("sweep1", params, records);

    OpSimulationParameters op_params(false, false, false, std::vector<std::string>{}, "", "", false, "NODESET", "", {}, {}, std::nullopt);
    SimulationConfig input("OP", op_params, {}, data_blocks, EMPTY_OPTIONS, {});
    SimulationParametersDialog dialog(m_parent, input);
    // act
    auto output = dialog.get_config();
    // assert
    ASSERT_EQ(output.data_blocks.size(), 1);
    EXPECT_EQ(output.data_blocks[0].name, "sweep1");
    ASSERT_EQ(output.data_blocks[0].parameters.size(), 2);
    EXPECT_EQ(output.data_blocks[0].parameters[0], "TIME");
    EXPECT_EQ(output.data_blocks[0].parameters[1], "V(1)");
    ASSERT_EQ(output.data_blocks[0].records.size(), 2);
    EXPECT_EQ(output.data_blocks[0].records[0][0], "0.0");
    EXPECT_EQ(output.data_blocks[0].records[0][1], "0.0");
    EXPECT_EQ(output.data_blocks[0].records[1][0], "1e-6");
    EXPECT_EQ(output.data_blocks[0].records[1][1], "1.5");
}

// ========================================================================================
// get_config — unassociated prints are preserved
// ========================================================================================

TEST_F(UiSimulationParametersDialogTest, get_config_preserves_unassociated_prints) {
    // arrange
    PrintParameters print_params("AC", "", "", {"V(1)"}, {});
    std::vector<PrintParameters> unassociated_prints = {print_params};
    OpSimulationParameters op_params(false, false, false, std::vector<std::string>{}, "", "", false, "NODESET", "", {}, {}, std::nullopt);
    SimulationConfig input("OP", op_params, {}, {}, EMPTY_OPTIONS, unassociated_prints);
    SimulationParametersDialog dialog(m_parent, input);
    // act
    auto output = dialog.get_config();
    // assert
    ASSERT_EQ(output.unassociated_prints.size(), 1);
    EXPECT_EQ(output.unassociated_prints[0].print_type, "AC");
    ASSERT_EQ(output.unassociated_prints[0].output_variables.size(), 1);
    EXPECT_EQ(output.unassociated_prints[0].output_variables[0], "V(1)");
}

// ========================================================================================
// get_config — transient with full print section and sensitivity
// ========================================================================================

TEST_F(UiSimulationParametersDialogTest, get_config_roundtrip_tran_full) {
    // arrange
    std::vector<TransientSchedulePoint> schedule = {TransientSchedulePoint("1u", "10n"), TransientSchedulePoint("10u", "100n")};
    std::vector<PrintParameters> print_params;
    auto sens = SensParameter("TRAN", "objvars", {"V(1)"}, {"R1", "C1"}, true, true, std::nullopt);
    TransientSimulationParameters tran_params("1u", "1m", "0", "5u", "UIC", schedule, std::nullopt, {}, {}, {}, sens);
    StepParameters step("LIN", "TEMP", "0", "100", "1", "", {}, "", true);
    SimulationConfig input("TRAN", tran_params, {step}, {}, EMPTY_OPTIONS, {});
    SimulationParametersDialog dialog(m_parent, input);
    // act
    auto output = dialog.get_config();
    // assert
    EXPECT_EQ(output.analysis_type, "TRAN");
    ASSERT_TRUE(std::holds_alternative<TransientSimulationParameters>(output.analysis));
    auto& out_params = std::get<TransientSimulationParameters>(output.analysis);
    EXPECT_EQ(out_params.initial_step_value, "1u");
    EXPECT_EQ(out_params.final_time_value, "1m");
    EXPECT_EQ(out_params.start_time_value, "0");
    EXPECT_EQ(out_params.step_ceiling_value, "5u");
    EXPECT_EQ(out_params.op_keyword, "UIC");
    ASSERT_EQ(out_params.schedule_points.size(), 2);
    EXPECT_EQ(out_params.schedule_points[0].time_value, "1u");
    EXPECT_EQ(out_params.schedule_points[0].max_time_step_value, "10n");
    EXPECT_EQ(out_params.schedule_points[1].time_value, "10u");
    EXPECT_EQ(out_params.schedule_points[1].max_time_step_value, "100n");
    ASSERT_TRUE(out_params.sensitivity.has_value());
    EXPECT_EQ(out_params.sensitivity->analysis_context, "TRAN");
    EXPECT_EQ(out_params.sensitivity->objective_mode, "objvars");
    ASSERT_EQ(out_params.sensitivity->objective_values.size(), 1);
    EXPECT_EQ(out_params.sensitivity->objective_values[0], "V(1)");
    ASSERT_EQ(out_params.sensitivity->parameter_list.size(), 2);
    EXPECT_EQ(out_params.sensitivity->parameter_list[0], "R1");
    EXPECT_EQ(out_params.sensitivity->parameter_list[1], "C1");
    EXPECT_TRUE(out_params.sensitivity->direct);
    EXPECT_TRUE(out_params.sensitivity->adjoint);
    ASSERT_EQ(output.steps.size(), 1);
    EXPECT_EQ(output.steps[0], step);
}

// ========================================================================================
// get_config — dc with full print section and sensitivity
// ========================================================================================

TEST_F(UiSimulationParametersDialogTest, get_config_roundtrip_dc_full) {
    // arrange
    std::vector<std::string> list_vals = {"0.5", "1.0", "1.5"};
    auto sens = SensParameter("DC", "objfunc", {"V(1)", "I(V1)"}, {"R1"}, true, false, std::nullopt);
    DCSimulationParameters dc_params("LIST", "V1", "", "", "", "", list_vals, "", "", "", "", "", "", std::nullopt, {}, sens);
    StepParameters step("DEC", "R1", "10", "1k", "", "5", {}, "", true);
    SimulationConfig input("DC", dc_params, {step}, {}, EMPTY_OPTIONS, {});
    SimulationParametersDialog dialog(m_parent, input);
    // act
    auto output = dialog.get_config();
    // assert
    EXPECT_EQ(output.analysis_type, "DC");
    ASSERT_TRUE(std::holds_alternative<DCSimulationParameters>(output.analysis));
    auto& out_params = std::get<DCSimulationParameters>(output.analysis);
    EXPECT_EQ(out_params.sweep_mode, "LIST");
    EXPECT_EQ(out_params.primary_variable, "V1");
    ASSERT_EQ(out_params.list_values.size(), 3);
    EXPECT_EQ(out_params.list_values[0], "0.5");
    EXPECT_EQ(out_params.list_values[1], "1.0");
    EXPECT_EQ(out_params.list_values[2], "1.5");
    ASSERT_TRUE(out_params.sensitivity.has_value());
    EXPECT_EQ(out_params.sensitivity->analysis_context, "DC");
    EXPECT_EQ(out_params.sensitivity->objective_mode, "objfunc");
    ASSERT_EQ(out_params.sensitivity->objective_values.size(), 2);
    EXPECT_EQ(out_params.sensitivity->objective_values[0], "V(1)");
    EXPECT_EQ(out_params.sensitivity->objective_values[1], "I(V1)");
    EXPECT_TRUE(out_params.sensitivity->direct);
    EXPECT_FALSE(out_params.sensitivity->adjoint);
    ASSERT_EQ(output.steps.size(), 1);
    EXPECT_EQ(output.steps[0], step);
}
