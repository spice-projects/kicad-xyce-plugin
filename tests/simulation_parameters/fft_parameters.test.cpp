#include <gtest/gtest.h>

#include "simulation_parameters/fft_parameters.h"

// ========================================================================================
// from_xyce_statement
// ========================================================================================

TEST(FftParametersChecks, minimal_fft_statement) {
    // arrange
    const std::string statement = ".FFT V(OUT)";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "V(OUT)");
    ASSERT_EQ(result->to_xyce_statement(), ".FFT V(OUT)");
}

TEST(FftParametersChecks, full_fft_statement) {
    // arrange
    const std::string statement = ".FFT V(OUT) NP=1024 WINDOW=HANN ALFA=1.0 FORMAT=NORM START=0 STOP=10m FREQ=1k FMIN=0 FMAX=10k";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "V(OUT)");
    ASSERT_EQ(result->np, "1024");
    ASSERT_EQ(result->window, "HANN");
    ASSERT_EQ(result->alfa, "1.0");
    ASSERT_EQ(result->fft_format, "NORM");
    ASSERT_EQ(result->start, "0");
    ASSERT_EQ(result->stop, "10m");
    ASSERT_EQ(result->freq, "1k");
    ASSERT_EQ(result->fmin, "0");
    ASSERT_EQ(result->fmax, "10k");
}

TEST(FftParametersChecks, synonyms_from_to) {
    // arrange
    const std::string statement = ".FFT V(OUT) FROM=1m TO=5m";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->start, "1m");
    ASSERT_EQ(result->stop, "5m");
    ASSERT_EQ(result->to_xyce_statement(), ".FFT V(OUT) START=1m STOP=5m");
}

TEST(FftParametersChecks, synonym_triangular) {
    // arrange
    const std::string statement = ".FFT V(OUT) WINDOW=TRIANGULAR";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->window, "TRIANGULAR");
    ASSERT_EQ(result->to_xyce_statement(), ".FFT V(OUT) WINDOW=TRIANGULAR");
}

TEST(FftParametersChecks, invalid_fft_statement) {
    // arrange
    const std::string statement = ".TRAN 1u 1m";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_FALSE(result.has_value());
}

TEST(FftParametersChecks, handles_complex_output_variable) {
    // arrange
    const std::string statement = ".FFT {V(OUT)*I(R1)} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)*I(R1)}");
}

// ========================================================================================
// to_xyce_statement
// ========================================================================================

TEST(FftParametersChecks, to_xyce_statement_only_required_field) {
    // arrange
    const FftParameters params("V(OUT)", "", "", "", "", "", "", "", "", "");
    // act
    const std::string result = params.to_xyce_statement();
    // assert
    ASSERT_EQ(result, ".FFT V(OUT)");
}

TEST(FftParametersChecks, to_xyce_statement_all_fields) {
    // arrange
    const FftParameters params("V(OUT)", "1024", "HANN", "1.0", "NORM", "0", "10m", "1k", "0", "10k");
    // act
    const std::string result = params.to_xyce_statement();
    // assert
    ASSERT_EQ(result, ".FFT V(OUT) NP=1024 WINDOW=HANN ALFA=1.0 FORMAT=NORM START=0 STOP=10m FREQ=1k FMIN=0 FMAX=10k");
}

TEST(FftParametersChecks, round_trip) {
    // arrange
    const std::string statement = ".FFT V(OUT) NP=1024 WINDOW=HANN ALFA=1.0 FORMAT=NORM START=0 STOP=10m FREQ=1k FMIN=0 FMAX=10k";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    const auto round_trip = result->to_xyce_statement();
    // assert
    ASSERT_EQ(round_trip, statement);
}

TEST(FftParametersChecks, handles_complex_output_variable_with_spaces) {
    // arrange
    const std::string statement = ".FFT { V(OUT) * I(R1) } WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{ V(OUT) * I(R1) }");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_nested_parentheses) {
    // arrange
    const std::string statement = ".FFT {V(OUT)*(I(R1)+I(R2))} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)*(I(R1)+I(R2))}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_braces) {
    // arrange
    const std::string statement = ".FFT {V(OUT)*{I(R1)+I(R2)}} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)*{I(R1)+I(R2)}}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_brackets) {
    // arrange
    const std::string statement = ".FFT {V(OUT)*[I(R1)+I(R2)]} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)*[I(R1)+I(R2)]}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_square_brackets) {
    // arrange
    const std::string statement = ".FFT {V(OUT)*I(R1)[2]} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)*I(R1)[2]}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_subscript) {
    // arrange
    const std::string statement = ".FFT {V(OUT)_1} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)_1}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_superscript) {
    // arrange
    const std::string statement = ".FFT {V(OUT)^2} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)^2}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_factorial) {
    // arrange
    const std::string statement = ".FFT {V(OUT)!} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)!}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_percentage) {
    // arrange
    const std::string statement = ".FFT {V(OUT)%} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)%}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_degree) {
    // arrange
    const std::string statement = ".FFT {V(OUT)°} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)°}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_plus_minus) {
    // arrange
    const std::string statement = ".FFT {V(OUT)±} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)±}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_infinity) {
    // arrange
    const std::string statement = ".FFT {V(OUT)∞} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)∞}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_pi) {
    // arrange
    const std::string statement = ".FFT {V(OUT)π} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)π}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_epsilon) {
    // arrange
    const std::string statement = ".FFT {V(OUT)ε} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)ε}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_theta) {
    // arrange
    const std::string statement = ".FFT {V(OUT)θ} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)θ}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_phi) {
    // arrange
    const std::string statement = ".FFT {V(OUT)φ} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)φ}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_omega) {
    // arrange
    const std::string statement = ".FFT {V(OUT)ω} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)ω}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_delta) {
    // arrange
    const std::string statement = ".FFT {V(OUT)δ} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)δ}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_sigma) {
    // arrange
    const std::string statement = ".FFT {V(OUT)σ} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)σ}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_lambda) {
    // arrange
    const std::string statement = ".FFT {V(OUT)λ} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)λ}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_alpha) {
    // arrange
    const std::string statement = ".FFT {V(OUT)α} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)α}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_beta) {
    // arrange
    const std::string statement = ".FFT {V(OUT)β} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)β}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_gamma) {
    // arrange
    const std::string statement = ".FFT {V(OUT)γ} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)γ}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_eta) {
    // arrange
    const std::string statement = ".FFT {V(OUT)η} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)η}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_zeta) {
    // arrange
    const std::string statement = ".FFT {V(OUT)ζ} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)ζ}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_iota) {
    // arrange
    const std::string statement = ".FFT {V(OUT)ι} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)ι}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_kappa) {
    // arrange
    const std::string statement = ".FFT {V(OUT)κ} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)κ}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_mu) {
    // arrange
    const std::string statement = ".FFT {V(OUT)μ} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)μ}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_nu) {
    // arrange
    const std::string statement = ".FFT {V(OUT)ν} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)ν}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_xi) {
    // arrange
    const std::string statement = ".FFT {V(OUT)ξ} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)ξ}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_omicron) {
    // arrange
    const std::string statement = ".FFT {V(OUT)ο} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)ο}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_rho) {
    // arrange
    const std::string statement = ".FFT {V(OUT)ρ} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)ρ}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_final_sigma) {
    // arrange
    const std::string statement = ".FFT {V(OUT)ς} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)ς}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_tau) {
    // arrange
    const std::string statement = ".FFT {V(OUT)τ} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)τ}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_upsilon) {
    // arrange
    const std::string statement = ".FFT {V(OUT)υ} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)υ}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_chi) {
    // arrange
    const std::string statement = ".FFT {V(OUT)χ} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)χ}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_psi) {
    // arrange
    const std::string statement = ".FFT {V(OUT)ψ} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)ψ}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_omega_uppercase) {
    // arrange
    const std::string statement = ".FFT {V(OUT)Ω} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)Ω}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_alpha_uppercase) {
    // arrange
    const std::string statement = ".FFT {V(OUT)Α} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)Α}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_beta_uppercase) {
    // arrange
    const std::string statement = ".FFT {V(OUT)Β} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)Β}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_gamma_uppercase) {
    // arrange
    const std::string statement = ".FFT {V(OUT)Γ} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)Γ}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_delta_uppercase) {
    // arrange
    const std::string statement = ".FFT {V(OUT)Δ} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)Δ}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_epsilon_uppercase) {
    // arrange
    const std::string statement = ".FFT {V(OUT)Ε} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)Ε}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_zeta_uppercase) {
    // arrange
    const std::string statement = ".FFT {V(OUT)Ζ} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)Ζ}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_eta_uppercase) {
    // arrange
    const std::string statement = ".FFT {V(OUT)Η} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)Η}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_theta_uppercase) {
    // arrange
    const std::string statement = ".FFT {V(OUT)Θ} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)Θ}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_iota_uppercase) {
    // arrange
    const std::string statement = ".FFT {V(OUT)Ι} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)Ι}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_kappa_uppercase) {
    // arrange
    const std::string statement = ".FFT {V(OUT)Κ} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)Κ}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_mu_uppercase) {
    // arrange
    const std::string statement = ".FFT {V(OUT)Μ} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)Μ}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_nu_uppercase) {
    // arrange
    const std::string statement = ".FFT {V(OUT)Ν} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)Ν}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_xi_uppercase) {
    // arrange
    const std::string statement = ".FFT {V(OUT)Ξ} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)Ξ}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_omicron_uppercase) {
    // arrange
    const std::string statement = ".FFT {V(OUT)Ο} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)Ο}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_pi_uppercase) {
    // arrange
    const std::string statement = ".FFT {V(OUT)Π} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)Π}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_rho_uppercase) {
    // arrange
    const std::string statement = ".FFT {V(OUT)Ρ} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)Ρ}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_sigma_uppercase) {
    // arrange
    const std::string statement = ".FFT {V(OUT)Σ} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)Σ}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_tau_uppercase) {
    // arrange
    const std::string statement = ".FFT {V(OUT)Τ} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)Τ}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_upsilon_uppercase) {
    // arrange
    const std::string statement = ".FFT {V(OUT)Υ} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)Υ}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_phi_uppercase) {
    // arrange
    const std::string statement = ".FFT {V(OUT)Φ} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)Φ}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_chi_uppercase) {
    // arrange
    const std::string statement = ".FFT {V(OUT)Χ} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)Χ}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_psi_uppercase) {
    // arrange
    const std::string statement = ".FFT {V(OUT)Ψ} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)Ψ}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_omega_uppercase_2) {
    // arrange
    const std::string statement = ".FFT {V(OUT)Ω} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)Ω}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_dollar_sign) {
    // arrange
    const std::string statement = ".FFT {V(OUT)$} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)$}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_ampersand) {
    // arrange
    const std::string statement = ".FFT {V(OUT)&} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)&}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_asterisk) {
    // arrange
    const std::string statement = ".FFT {V(OUT)*} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)*}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_plus) {
    // arrange
    const std::string statement = ".FFT {V(OUT)+} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)+}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_comma) {
    // arrange
    const std::string statement = ".FFT {V(OUT),} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT),}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_minus) {
    // arrange
    const std::string statement = ".FFT {V(OUT)-} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)-}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_dot) {
    // arrange
    const std::string statement = ".FFT {V(OUT).} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT).}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_slash) {
    // arrange
    const std::string statement = ".FFT {V(OUT)/} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)/}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_colon) {
    // arrange
    const std::string statement = ".FFT {V(OUT):} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT):}");
}



TEST(FftParametersChecks, handles_complex_output_variable_with_tilde) {
    // arrange
    const std::string statement = ".FFT {V(OUT)~} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)~}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_space) {
    // arrange
    const std::string statement = ".FFT {V(OUT) } WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT) }");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_tab) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\t} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\t}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_newline) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\n} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\n}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_carriage_return) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\r} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\r}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_form_feed) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\f} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\f}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_vertical_tab) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\v} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\v}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_null) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\0} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\0}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_bell) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\a} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\a}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_backspace) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\b} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\b}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_escape) {
    // arrange
    const std::string statement = ".FFT {V(OUT)} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_shift_in) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x10} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x10}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_shift_out) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x11} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x11}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_enq) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x05} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x05}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_ack) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x06} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x06}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_bell_2) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x07} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x07}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_cancel) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x18} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x18}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_end_of_medium) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x19} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x19}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_substitute) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x1a} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x1a}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_escape_2) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x1b} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x1b}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_space_2) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x20} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x20}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_exclamation) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x21} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x21}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_quotation) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x22} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x22}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_number) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x23} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x23}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_dollar) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x24} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x24}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_percent) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x25} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x25}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_ampersand_2) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x26} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x26}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_apostrophe) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x27} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x27}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_left_parenthesis) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x28} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x28}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_right_parenthesis) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x29} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x29}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_asterisk_2) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x2a} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x2a}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_plus_2) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x2b} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x2b}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_comma_2) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x2c} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x2c}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_minus_2) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x2d} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x2d}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_dot_2) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x2e} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x2e}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_slash_2) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x2f} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x2f}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_zero) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x30} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x30}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_one) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x31} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x31}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_two) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x32} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x32}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_three) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x33} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x33}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_four) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x34} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x34}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_five) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x35} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x35}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_six) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x36} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x36}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_seven) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x37} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x37}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_eight) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x38} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x38}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_nine) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x39} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x39}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_colon_2) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x3a} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x3a}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_semicolon_2) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x3b} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x3b}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_less_than_2) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x3c} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x3c}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_equal_2) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x3d} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x3d}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_greater_than_2) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x3e} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x3e}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_question_2) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x3f} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x3f}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_at_2) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x40} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x40}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_A) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x41} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x41}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_B) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x42} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x42}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_C) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x43} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x43}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_D) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x44} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x44}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_E) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x45} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x45}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_F) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x46} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x46}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_G) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x47} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x47}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_H) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x48} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x48}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_I) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x49} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x49}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_J) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x4a} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x4a}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_K) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x4b} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x4b}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_L) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x4c} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x4c}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_M) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x4d} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x4d}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_N) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x4e} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x4e}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_O) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x4f} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x4f}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_P) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x50} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x50}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_Q) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x51} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x51}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_R) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x52} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x52}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_S) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x53} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x53}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_T) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x54} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x54}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_U) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x55} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x55}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_V) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x56} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x56}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_W) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x57} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x57}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_X) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x58} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x58}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_Y) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x59} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x59}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_Z) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x5a} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x5a}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_left_bracket_2) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x5b} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x5b}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_backslash_2) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x5c} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x5c}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_right_bracket_2) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x5d} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x5d}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_caret_2) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x5e} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x5e}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_underscore_2) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x5f} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x5f}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_backtick_2) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x60} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x60}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_a) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x61} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x61}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_b) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x62} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x62}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_c) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x63} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x63}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_d) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x64} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x64}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_e) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x65} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x65}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_f) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x66} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x66}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_g) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x67} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x67}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_h) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x68} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x68}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_i) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x69} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x69}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_j) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x6a} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x6a}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_k) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x6b} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x6b}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_l) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x6c} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x6c}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_m) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x6d} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x6d}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_n) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x6e} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x6e}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_o) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x6f} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x6f}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_p) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x70} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x70}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_q) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x71} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x71}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_r) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x72} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x72}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_s) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x73} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x73}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_t) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x74} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x74}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_u) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x75} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x75}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_v) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x76} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x76}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_w) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x77} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x77}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_x) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x78} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x78}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_y) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x79} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x79}");
}

TEST(FftParametersChecks, handles_complex_output_variable_with_z) {
    // arrange
    const std::string statement = ".FFT {V(OUT)\x7a} WINDOW=RECT";
    // act
    const auto result = FftParameters::from_xyce_statement(statement);
    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->output_variable, "{V(OUT)\x7a}");
}



TEST(FftParametersChecks, to_xyce_statement_preserves_case_of_valid_window) {
    // arrange
    const FftParameters params("V(OUT)", "", "BLACKMAN", "", "", "", "", "", "", "");
    // act
    const std::string result = params.to_xyce_statement();
    // assert
    ASSERT_EQ(result, ".FFT V(OUT) WINDOW=BLACKMAN");
}

TEST(FftParametersChecks, to_xyce_statement_preserves_case_of_valid_format) {
    // arrange
    const FftParameters params("V(OUT)", "", "", "", "UNORM", "", "", "", "", "");
    // act
    const std::string result = params.to_xyce_statement();
    // assert
    ASSERT_EQ(result, ".FFT V(OUT) FORMAT=UNORM");
}
