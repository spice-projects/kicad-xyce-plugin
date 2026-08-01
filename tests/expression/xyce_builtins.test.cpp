#include <cmath>
#include <complex>
#include <unordered_map>
#include <vector>

#include <gtest/gtest.h>

#include "expression/xyce_evaluator.h"

namespace
{
    // keep scalar values explicit in the assertions
    XyceValue expression_value(double value) { return value; }

    // keep complex values explicit in the assertions
    XyceValue expression_value(const std::complex<double>& value) { return value; }

    // read the first scalar value from a variant for compact checks
    template <typename T>
    T scalar(const XyceValue& value) {
        return scalar_value<T>(value);
    }
} // namespace

TEST(XyceBuiltinsChecks, constants_match_expected_values) {
    // arrange
    const auto& constants = BUILTIN_CONSTANTS;
    // act
    const auto pi = scalar<double>(constants.at("pi"));
    // assert
    ASSERT_NEAR(pi, std::acos(-1.0), 1e-12);
    ASSERT_NEAR(scalar<double>(constants.at("e")), std::exp(1.0), 1e-12);
    ASSERT_DOUBLE_EQ(scalar<double>(constants.at("meg")), 1e6);
    ASSERT_DOUBLE_EQ(scalar<double>(constants.at("k")), 1e3);
    ASSERT_DOUBLE_EQ(scalar<double>(constants.at("m")), 1e-3);
    ASSERT_DOUBLE_EQ(scalar<double>(constants.at("u")), 1e-6);
    ASSERT_DOUBLE_EQ(scalar<double>(constants.at("n")), 1e-9);
    ASSERT_DOUBLE_EQ(scalar<double>(constants.at("p")), 1e-12);
    ASSERT_DOUBLE_EQ(scalar<double>(constants.at("f")), 1e-15);
    ASSERT_DOUBLE_EQ(scalar<double>(constants.at("g")), 1e9);
    ASSERT_DOUBLE_EQ(scalar<double>(constants.at("t")), 1e12);
    ASSERT_NEAR(scalar<double>(constants.at("mil")), 25.4e-6, 1e-20);
    ASSERT_EQ(std::get<std::complex<double>>(constants.at("j")), std::complex<double>(0.0, 1.0));
    ASSERT_DOUBLE_EQ(scalar<double>(constants.at("mho")), 1.0);
}

TEST(XyceBuiltinsChecks, core_builtin_functions_work) {
    // arrange
    const auto& functions = BUILTIN_FUNCTIONS;
    // act
    const auto log_value = scalar<double>(functions.at("log")({expression_value(100.0)}));
    // assert
    ASSERT_DOUBLE_EQ(log_value, 2.0);
    ASSERT_DOUBLE_EQ(scalar<double>(functions.at("log10")({expression_value(1000.0)})), 3.0);
    ASSERT_DOUBLE_EQ(scalar<double>(functions.at("ln")({expression_value(std::exp(1.0))})), 1.0);
    ASSERT_DOUBLE_EQ(scalar<double>(functions.at("abs")({expression_value(-3.0)})), 3.0);
    ASSERT_DOUBLE_EQ(scalar<double>(functions.at("sqrt")({expression_value(9.0)})), 3.0);
    ASSERT_DOUBLE_EQ(scalar<double>(functions.at("db")({expression_value(10.0)})), 20.0);
    ASSERT_DOUBLE_EQ(scalar<double>(functions.at("real")({expression_value(std::complex<double>(3.0, 4.0))})), 3.0);
    ASSERT_DOUBLE_EQ(scalar<double>(functions.at("imag")({expression_value(std::complex<double>(3.0, 4.0))})), 4.0);
    ASSERT_DOUBLE_EQ(scalar<double>(functions.at("phase")({expression_value(std::complex<double>(0.0, 1.0))})), 90.0);
    ASSERT_DOUBLE_EQ(scalar<double>(functions.at("sin")({expression_value(0.0)})), 0.0);
    ASSERT_DOUBLE_EQ(scalar<double>(functions.at("cos")({expression_value(0.0)})), 1.0);
    ASSERT_DOUBLE_EQ(scalar<double>(functions.at("tan")({expression_value(0.0)})), 0.0);
    ASSERT_DOUBLE_EQ(scalar<double>(functions.at("atan2")({expression_value(1.0), expression_value(1.0)})), std::atan2(1.0, 1.0));
    ASSERT_DOUBLE_EQ(scalar<double>(functions.at("sgn")({expression_value(-3.0)})), -1.0);
    ASSERT_DOUBLE_EQ(scalar<double>(functions.at("sign")({expression_value(-3.0), expression_value(2.0)})), 3.0);
    ASSERT_DOUBLE_EQ(scalar<double>(functions.at("uramp")({expression_value(-5.0)})), 0.0);
    ASSERT_DOUBLE_EQ(scalar<double>(functions.at("stp")({expression_value(1.0)})), 1.0);
    ASSERT_DOUBLE_EQ(scalar<double>(functions.at("round")({expression_value(2.7)})), 3.0);
    ASSERT_DOUBLE_EQ(scalar<double>(functions.at("nint")({expression_value(1.5)})), 2.0);
    ASSERT_DOUBLE_EQ(scalar<double>(functions.at("floor")({expression_value(2.9)})), 2.0);
    ASSERT_DOUBLE_EQ(scalar<double>(functions.at("ceil")({expression_value(2.1)})), 3.0);
    ASSERT_DOUBLE_EQ(scalar<double>(functions.at("int")({expression_value(-2.9)})), -2.0);
    ASSERT_DOUBLE_EQ(scalar<double>(functions.at("pow")({expression_value(2.0), expression_value(10.0)})), 1024.0);
    ASSERT_DOUBLE_EQ(scalar<double>(functions.at("pwr")({expression_value(-2.0), expression_value(3.0)})), 8.0);
    ASSERT_DOUBLE_EQ(scalar<double>(functions.at("pwrs")({expression_value(-2.0), expression_value(3.0)})), -8.0);
    ASSERT_DOUBLE_EQ(scalar<double>(functions.at("fmod")({expression_value(10.0), expression_value(3.0)})), 1.0);
    ASSERT_DOUBLE_EQ(scalar<double>(functions.at("min")({expression_value(3.0), expression_value(1.0), expression_value(2.0)})), 1.0);
    ASSERT_DOUBLE_EQ(scalar<double>(functions.at("max")({expression_value(3.0), expression_value(1.0), expression_value(2.0)})), 3.0);
    ASSERT_DOUBLE_EQ(scalar<double>(functions.at("limit")({expression_value(5.0), expression_value(1.0), expression_value(10.0)})), 5.0);
    ASSERT_DOUBLE_EQ(scalar<double>(functions.at("if")({expression_value(1.0), expression_value(10.0), expression_value(20.0)})), 10.0);
}

TEST(XyceBuiltinsChecks, builtin_function_errors_match_scenarios) {
    // arrange
    const auto& functions = BUILTIN_FUNCTIONS;
    // act
    const auto invalid_abs_args = std::vector<XyceValue>{expression_value(1.0), expression_value(2.0)};
    // assert
    ASSERT_THROW(functions.at("abs")(invalid_abs_args), std::invalid_argument);
    ASSERT_THROW(functions.at("sign")({expression_value(1.0)}), std::invalid_argument);
    ASSERT_THROW(functions.at("fmod")({expression_value(1.0)}), std::invalid_argument);
    ASSERT_THROW(functions.at("ddt")({expression_value(1.0)}), std::logic_error);
    ASSERT_THROW(functions.at("sdt")({expression_value(1.0)}), std::logic_error);
}
