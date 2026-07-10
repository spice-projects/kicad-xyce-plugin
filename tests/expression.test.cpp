#include <complex>

#include <gtest/gtest.h>
#include "expression.h"

TEST(ExpressionChecks, default_constructor_initialization) {
    // arrange
    const Expression expression = {};
    // act / assert
    ASSERT_TRUE(expression.name().empty());
    ASSERT_TRUE(expression.unit().empty());
    ASSERT_FALSE(expression.source().has_value());
    ASSERT_FALSE(expression.variable_type().has_value());
    ASSERT_FALSE(expression.is_complex());
    ASSERT_TRUE(expression.step_count()==0);
}