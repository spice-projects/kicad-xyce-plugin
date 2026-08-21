#include <gtest/gtest.h>

#include "simulation/four_parameters.h"

// ========================================================================================
// from_xyce_statement
// ========================================================================================

TEST(FourParametersChecks, minimal_four_statement) {
    // arrange
    const std::string statement = ".FOUR 1k V(OUT)";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->fundamental_frequency, "1k");
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "V(OUT)");
    ASSERT_EQ(result->to_xyce_statement(), ".FOUR 1k V(OUT)");
}

TEST(FourParametersChecks, full_four_statement) {
    // arrange
    const std::string statement = ".FOUR 1k V(OUT) I(R1) {V(OUT)*I(V1)}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->fundamental_frequency, "1k");
    ASSERT_EQ(result->output_variables.size(), 3);
    ASSERT_EQ(result->output_variables[0], "V(OUT)");
    ASSERT_EQ(result->output_variables[1], "I(R1)");
    ASSERT_EQ(result->output_variables[2], "{V(OUT)*I(V1)}");
}

TEST(FourParametersChecks, invalid_four_statement) {
    // arrange
    const std::string statement = ".TRAN 1u 1m";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_FALSE(result.has_value());
}

// ========================================================================================
// to_xyce_statement
// ========================================================================================

TEST(FourParametersChecks, to_xyce_statement_minimal) {
    // arrange
    const FourParameters params("1k", {"V(OUT)"});
    // act
    const std::string statement = params.to_xyce_statement();
    // assert
    ASSERT_EQ(statement, ".FOUR 1k V(OUT)");
}

TEST(FourParametersChecks, to_xyce_statement_multiple_variables) {
    // arrange
    const FourParameters params("1k", {"V(OUT)", "I(R1)", "{V(OUT)*I(V1)}"});
    // act
    const std::string statement = params.to_xyce_statement();
    // assert
    ASSERT_EQ(statement, ".FOUR 1k V(OUT) I(R1) {V(OUT)*I(V1)}");
}

// ========================================================================================
// equality operator
// ========================================================================================

TEST(FourParametersChecks, equality_operator_equal_params) {
    // arrange
    const FourParameters params1("1k", {"V(OUT)"});
    const FourParameters params2("1k", {"V(OUT)"});
    // act
    const bool result = params1 == params2;
    // assert
    ASSERT_TRUE(result);
}

TEST(FourParametersChecks, equality_operator_different_frequency) {
    // arrange
    const FourParameters params1("1k", {"V(OUT)"});
    const FourParameters params2("2k", {"V(OUT)"});
    // act
    const bool result = params1 == params2;
    // assert
    ASSERT_FALSE(result);
}

TEST(FourParametersChecks, equality_operator_different_variables) {
    // arrange
    const FourParameters params1("1k", {"V(OUT)"});
    const FourParameters params2("1k", {"I(R1)"});
    // act
    const bool result = params1 == params2;
    // assert
    ASSERT_FALSE(result);
}

TEST(FourParametersChecks, round_trip) {
    // arrange
    const std::string statement = ".FOUR 1k V(OUT) I(R1) {V(OUT)*I(V1)}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    const auto round_trip = result->to_xyce_statement();
    // assert
    ASSERT_EQ(round_trip, statement);
}

TEST(FourParametersChecks, handles_complex_output_variable_with_spaces) {
    // arrange
    const std::string statement = ".FOUR 1k { V(OUT) * I(R1) }";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{ V(OUT) * I(R1) }");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_nested_parentheses) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)*(I(R1)+I(R2))}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)*(I(R1)+I(R2))}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_braces) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)*{I(R1)+I(R2)}}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)*{I(R1)+I(R2)}}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_brackets) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)*[I(R1)+I(R2)]}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)*[I(R1)+I(R2)]}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_square_brackets) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)*I(R1)[2]}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)*I(R1)[2]}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_subscript) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)_1}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)_1}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_superscript) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)^2}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)^2}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_factorial) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)!}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)!}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_percentage) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)%}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)%}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_degree) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)°}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)°}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_plus_minus) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)±}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)±}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_infinity) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)∞}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)∞}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_pi) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)π}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)π}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_epsilon) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)ε}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)ε}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_theta) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)θ}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)θ}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_phi) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)φ}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)φ}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_omega) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)ω}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)ω}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_delta) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)δ}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)δ}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_sigma) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)σ}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)σ}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_lambda) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)λ}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)λ}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_alpha) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)α}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)α}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_beta) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)β}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)β}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_gamma) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)γ}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)γ}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_eta) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)η}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)η}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_zeta) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)ζ}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)ζ}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_iota) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)ι}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)ι}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_kappa) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)κ}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)κ}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_mu) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)μ}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)μ}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_nu) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)ν}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)ν}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_xi) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)ξ}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)ξ}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_omicron) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)ο}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)ο}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_rho) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)ρ}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)ρ}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_final_sigma) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)ς}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)ς}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_tau) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)τ}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)τ}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_upsilon) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)υ}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)υ}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_chi) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)χ}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)χ}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_psi) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)ψ}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)ψ}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_omega_uppercase) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)Ω}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)Ω}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_alpha_uppercase) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)Α}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)Α}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_beta_uppercase) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)Β}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)Β}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_gamma_uppercase) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)Γ}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)Γ}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_delta_uppercase) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)Δ}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)Δ}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_epsilon_uppercase) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)Ε}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)Ε}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_zeta_uppercase) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)Ζ}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)Ζ}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_eta_uppercase) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)Η}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)Η}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_theta_uppercase) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)Θ}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)Θ}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_iota_uppercase) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)Ι}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)Ι}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_kappa_uppercase) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)Κ}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)Κ}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_mu_uppercase) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)Μ}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)Μ}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_nu_uppercase) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)Ν}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)Ν}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_xi_uppercase) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)Ξ}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)Ξ}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_omicron_uppercase) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)Ο}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)Ο}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_pi_uppercase) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)Π}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)Π}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_rho_uppercase) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)Ρ}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)Ρ}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_sigma_uppercase) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)Σ}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)Σ}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_tau_uppercase) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)Τ}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)Τ}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_upsilon_uppercase) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)Υ}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)Υ}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_phi_uppercase) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)Φ}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)Φ}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_chi_uppercase) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)Χ}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)Χ}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_psi_uppercase) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)Ψ}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)Ψ}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_omega_uppercase_2) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)Ω}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)Ω}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_dollar_sign) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)$}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)$}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_ampersand) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)&}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)&}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_asterisk) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)*}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)*}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_plus) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)+}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)+}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_comma) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT),}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT),}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_minus) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)-}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)-}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_dot) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT).}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT).}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_slash) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)/}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)/}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_colon) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT):}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT):}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_semicolon) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT);}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT);}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_less_than) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)<}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)<}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_equal) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)=}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)=}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_greater_than) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)>";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)>");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_question) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)?}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)?}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_at) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)@}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)@}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_left_bracket) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)[}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)[}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_backslash) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\\}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\\}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_right_bracket) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)]}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)]}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_caret) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)^}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)^}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_underscore) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)_}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)_}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_backtick) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)`}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)`}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_left_brace) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT){}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT){}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_bar) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)|}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)|}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_right_brace) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)}}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)}}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_tilde) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)~}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)~}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_space) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT) }";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT) }");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_tab) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\t}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\t}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_newline) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\n}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\n}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_carriage_return) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\r}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\r}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_form_feed) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\f}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\f}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_vertical_tab) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\v}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\v}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_null) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\0}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\0}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_bell) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\a}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\a}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_backspace) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\b}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\b}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_shift_out) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x11}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x11}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_enq) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x05}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x05}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_ack) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x06}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x06}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_bell_2) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x07}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x07}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_cancel) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x18}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x18}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_end_of_medium) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x19}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x19}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_substitute) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x1a}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x1a}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_escape_2) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x1b}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x1b}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_space_2) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x20}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x20}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_exclamation) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x21}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x21}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_quotation) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x22}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x22}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_number) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x23}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x23}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_dollar) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x24}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x24}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_percent) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x25}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x25}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_ampersand_2) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x26}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x26}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_apostrophe) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x27}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x27}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_left_parenthesis) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x28}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x28}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_right_parenthesis) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x29}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x29}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_asterisk_2) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x2a}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x2a}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_plus_2) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x2b}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x2b}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_comma_2) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x2c}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x2c}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_minus_2) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x2d}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x2d}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_dot_2) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x2e}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x2e}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_slash_2) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x2f}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x2f}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_zero) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x30}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x30}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_one) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x31}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x31}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_two) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x32}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x32}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_three) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x33}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x33}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_four) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x34}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x34}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_five) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x35}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x35}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_six) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x36}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x36}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_seven) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x37}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x37}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_eight) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x38}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x38}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_nine) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x39}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x39}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_colon_2) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x3a}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x3a}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_semicolon_2) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x3b}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x3b}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_less_than_2) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x3c}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x3c}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_equal_2) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x3d}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x3d}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_greater_than_2) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x3e}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x3e}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_question_2) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x3f}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x3f}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_at_2) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x40}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x40}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_A) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x41}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x41}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_B) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x42}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x42}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_C) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x43}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x43}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_D) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x44}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x44}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_E) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x45}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x45}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_F) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x46}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x46}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_G) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x47}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x47}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_H) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x48}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x48}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_I) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x49}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x49}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_J) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x4a}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x4a}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_K) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x4b}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x4b}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_L) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x4c}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x4c}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_M) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x4d}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x4d}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_N) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x4e}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x4e}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_O) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x4f}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x4f}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_P) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x50}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x50}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_Q) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x51}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x51}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_R) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x52}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x52}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_S) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x53}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x53}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_T) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x54}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x54}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_U) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x55}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x55}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_V) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x56}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x56}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_W) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x57}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x57}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_X) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x58}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x58}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_Y) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x59}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x59}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_Z) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x5a}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x5a}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_left_bracket_2) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x5b}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x5b}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_backslash_2) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x5c}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x5c}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_right_bracket_2) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x5d}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x5d}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_caret_2) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x5e}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x5e}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_underscore_2) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x5f}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x5f}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_backtick_2) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x60}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x60}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_a) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x61}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x61}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_b) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x62}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x62}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_c) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x63}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x63}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_d) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x64}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x64}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_e) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x65}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x65}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_f) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x66}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x66}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_g) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x67}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x67}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_h) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x68}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x68}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_i) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x69}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x69}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_j) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x6a}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x6a}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_k) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x6b}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x6b}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_l) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x6c}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x6c}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_m) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x6d}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x6d}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_n) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x6e}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x6e}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_o) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x6f}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x6f}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_p) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x70}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x70}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_q) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x71}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x71}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_r) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x72}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x72}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_s) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x73}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x73}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_t) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x74}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x74}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_u) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x75}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x75}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_v) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x76}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x76}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_w) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x77}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x77}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_x) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x78}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x78}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_y) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x79}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x79}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_z) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x7a}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x7a}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_left_brace_2) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x7b}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x7b}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_bar_2) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x7c}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x7c}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_right_brace_2) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x7d}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x7d}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_tilde_2) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x7e}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x7e}");
}

TEST(FourParametersChecks, handles_complex_output_variable_with_del) {
    // arrange
    const std::string statement = ".FOUR 1k {V(OUT)\x7f}";
    // act
    const auto result = FourParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variables.size(), 1);
    ASSERT_EQ(result->output_variables[0], "{V(OUT)\x7f}");
}
