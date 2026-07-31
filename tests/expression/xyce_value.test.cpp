#include <gtest/gtest.h>
#include <complex>
#include <memory>
#include <vector>

#include "../src/expression/xyce_value.h"
#include "../src/expression/view.h"
#include "../src/expression/expression.h"

// helper: create an Expression<double> with a single step containing the supplied data
static AnyExpression make_double_expression(const std::vector<double>& data) {
    std::vector<View<double>> steps;
    steps.emplace_back(data);
    Expression<double> expr("test", steps, "V");
    return AnyExpression(std::move(expr));
}

// helper: create an Expression<std::complex<double>> with a single step containing the supplied data
static AnyExpression make_complex_expression(const std::vector<std::complex<double>>& data) {
    std::vector<View<std::complex<double>>> steps;
    steps.emplace_back(data);
    Expression<std::complex<double>> expr("test", steps, "V");
    return AnyExpression(std::move(expr));
}

// test suite for xyce_value utilities
class XyceValueTest : public ::testing::Test {};

TEST_F(XyceValueTest, from_expression_scalar_double) {
    // arrange
    std::vector<double> data = {1.5};
    AnyExpression any = make_double_expression(data);
    // act
    XyceValue val = from_expression(any);
    // assert
    ASSERT_TRUE(std::holds_alternative<std::shared_ptr<View<double>>>(val));
    auto view = std::get<std::shared_ptr<View<double>>>(val);
    ASSERT_NE(view, nullptr);
    EXPECT_DOUBLE_EQ((*view)[0], 1.5);
}

TEST_F(XyceValueTest, from_expression_scalar_complex) {
    // arrange
    std::vector<std::complex<double>> data = {{2.0, -3.0}};
    AnyExpression any = make_complex_expression(data);
    // act
    XyceValue val = from_expression(any);
    // assert
    ASSERT_TRUE(std::holds_alternative<std::shared_ptr<View<std::complex<double>>>>(val));
    auto view = std::get<std::shared_ptr<View<std::complex<double>>>>(val);
    ASSERT_NE(view, nullptr);
    EXPECT_EQ((*view)[0], std::complex<double>(2.0, -3.0));
}

TEST_F(XyceValueTest, type_predicates) {
    // arrange
    XyceValue scalar = 42.0;
    XyceValue vec = std::make_shared<View<double>>(std::vector<double>{1.0, 2.0});
    XyceValue cscalar = std::complex<double>(1.0, 2.0);
    XyceValue cvec = std::make_shared<View<std::complex<double>>>(std::vector<std::complex<double>>{{3.0, 4.0}});
    // assert
    EXPECT_TRUE(is_scalar(scalar));
    EXPECT_TRUE(is_scalar(cscalar));
    EXPECT_FALSE(is_scalar(vec));
    EXPECT_TRUE(is_vector(vec));
    EXPECT_TRUE(is_vector(cvec));
    EXPECT_FALSE(is_vector(scalar));
    EXPECT_TRUE(is_real(scalar));
    EXPECT_TRUE(is_real(vec));
    EXPECT_FALSE(is_real(cscalar));
    EXPECT_FALSE(is_real(cvec));
    EXPECT_TRUE(is_complex(cscalar));
    EXPECT_TRUE(is_complex(cvec));
    EXPECT_FALSE(is_complex(scalar));
    EXPECT_FALSE(is_complex(vec));
}

TEST_F(XyceValueTest, scalar_value_double) {
    // arrange
    XyceValue v1 = 5.0;
    XyceValue v2 = std::complex<double>(-2.5, 7.0);
    XyceValue v3 = std::make_shared<View<double>>(std::vector<double>{10.0, 20.0});
    XyceValue v4 = std::make_shared<View<std::complex<double>>>(std::vector<std::complex<double>>{{3.3, 4.4}, {5.5, 6.6}});
    // assert
    EXPECT_DOUBLE_EQ(scalar_value<double>(v1), 5.0);
    EXPECT_DOUBLE_EQ(scalar_value<double>(v2), -2.5);
    EXPECT_DOUBLE_EQ(scalar_value<double>(v3), 10.0);
    EXPECT_DOUBLE_EQ(scalar_value<double>(v4), 3.3);
}

TEST_F(XyceValueTest, scalar_value_complex) {
    // arrange
    XyceValue v1 = 7.0;
    XyceValue v2 = std::complex<double>(-1.0, 2.0);
    XyceValue v3 = std::make_shared<View<double>>(std::vector<double>{8.0, 9.0});
    XyceValue v4 = std::make_shared<View<std::complex<double>>>(std::vector<std::complex<double>>{{4.4, 5.5}});
    // assert
    EXPECT_EQ(scalar_value<std::complex<double>>(v1), std::complex<double>(7.0, 0.0));
    EXPECT_EQ(scalar_value<std::complex<double>>(v2), std::complex<double>(-1.0, 2.0));
    EXPECT_EQ(scalar_value<std::complex<double>>(v3), std::complex<double>(8.0, 0.0));
    EXPECT_EQ(scalar_value<std::complex<double>>(v4), std::complex<double>(4.4, 5.5));
}

TEST_F(XyceValueTest, to_real_vector) {
    // arrange
    XyceValue d = 3.14;
    XyceValue c = std::complex<double>(2.5, -1.0);
    XyceValue vr = std::make_shared<View<double>>(std::vector<double>{1.0, 2.0, 3.0});
    XyceValue vc = std::make_shared<View<std::complex<double>>>(std::vector<std::complex<double>>{{1.0, 1.0}, {2.0, 2.0}});
    // act / assert scalar double
    auto rv1 = to_real_vector(d);
    ASSERT_NE(rv1, nullptr);
    EXPECT_EQ(rv1->size(), static_cast<std::size_t>(1));
    EXPECT_DOUBLE_EQ((*rv1)[0], 3.14);
    // act / assert scalar complex
    auto rv2 = to_real_vector(c);
    ASSERT_NE(rv2, nullptr);
    EXPECT_EQ(rv2->size(), static_cast<std::size_t>(1));
    EXPECT_DOUBLE_EQ((*rv2)[0], 2.5);
    // act / assert real vector view
    auto rv3 = to_real_vector(vr);
    ASSERT_NE(rv3, nullptr);
    EXPECT_EQ(rv3->size(), static_cast<std::size_t>(3));
    EXPECT_DOUBLE_EQ((*rv3)[2], 3.0);
    // act / assert complex vector view
    auto rv4 = to_real_vector(vc);
    ASSERT_NE(rv4, nullptr);
    EXPECT_EQ(rv4->size(), static_cast<std::size_t>(2));
    EXPECT_DOUBLE_EQ((*rv4)[1], 2.0);
}

TEST_F(XyceValueTest, to_complex_vector) {
    // arrange
    XyceValue d = 4.0;
    XyceValue c = std::complex<double>(-3.0, 5.0);
    XyceValue vr = std::make_shared<View<double>>(std::vector<double>{6.0, 7.0});
    XyceValue vc = std::make_shared<View<std::complex<double>>>(std::vector<std::complex<double>>{{1.0, 2.0}, {3.0, 4.0}});
    // act / assert scalar double
    auto cv1 = to_complex_vector(d);
    ASSERT_NE(cv1, nullptr);
    EXPECT_EQ(cv1->size(), static_cast<std::size_t>(1));
    EXPECT_EQ((*cv1)[0], std::complex<double>(4.0, 0.0));
    // act / assert scalar complex
    auto cv2 = to_complex_vector(c);
    ASSERT_NE(cv2, nullptr);
    EXPECT_EQ(cv2->size(), static_cast<std::size_t>(1));
    EXPECT_EQ((*cv2)[0], std::complex<double>(-3.0, 5.0));
    // act / assert real vector view
    auto cv3 = to_complex_vector(vr);
    ASSERT_NE(cv3, nullptr);
    EXPECT_EQ(cv3->size(), static_cast<std::size_t>(2));
    EXPECT_EQ((*cv3)[0], std::complex<double>(6.0, 0.0));
    // act / assert complex vector view
    auto cv4 = to_complex_vector(vc);
    ASSERT_NE(cv4, nullptr);
    EXPECT_EQ(cv4->size(), static_cast<std::size_t>(2));
    EXPECT_EQ((*cv4)[1], std::complex<double>(3.0, 4.0));
}
