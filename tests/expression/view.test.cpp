#include <algorithm>
#include <complex>
#include <concepts>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "expression/view.h"

static_assert(std::is_default_constructible_v<View<double>>);
static_assert(!std::is_copy_constructible_v<View<double>>);
static_assert(!std::is_copy_assignable_v<View<double>>);
static_assert(std::is_nothrow_move_constructible_v<View<double>>);
static_assert(std::is_nothrow_move_assignable_v<View<double>>);
static_assert(std::is_nothrow_destructible_v<View<double>>);

static_assert(std::is_default_constructible_v<View<double>::Iterator>);
static_assert(std::is_copy_constructible_v<View<double>::Iterator>);
static_assert(std::is_copy_assignable_v<View<double>::Iterator>);

static_assert(std::same_as<View<double>::Iterator::value_type, double>);
static_assert(std::same_as<View<double>::Iterator::difference_type, std::ptrdiff_t>);
static_assert(std::same_as<View<double>::Iterator::pointer, const double*>);
static_assert(std::same_as<View<double>::Iterator::reference, const double&>);
static_assert(std::same_as<View<double>::Iterator::iterator_concept, std::forward_iterator_tag>);
static_assert(std::same_as<View<double>::Iterator::iterator_category, std::forward_iterator_tag>);

TEST(ViewChecks, default_constructed_view_is_move_assignable) {
    // arrange
    View<double> view;
    View<double> source(std::vector<double>{1, 2, 3});
    // act
    view = std::move(source);
    // assert
    ASSERT_EQ(view.size(), 3);
    ASSERT_EQ(view.stride(), 1);
    ASSERT_EQ(view[0], 1);
    ASSERT_EQ(view[1], 2);
    ASSERT_EQ(view[2], 3);
}

TEST(ViewChecks, move_constructor_transfers_data_and_ownership) {
    // arrange
    View<double> source(std::vector<double>{1, 2, 3});
    // act
    const View<double> moved(std::move(source));
    // assert
    ASSERT_EQ(moved.size(), 3);
    ASSERT_EQ(moved.stride(), 1);
    ASSERT_EQ(moved[0], 1);
    ASSERT_EQ(moved[1], 2);
    ASSERT_EQ(moved[2], 3);
}

TEST(ViewChecks, pointer_constructor_creates_view_over_external_buffer) {
    // arrange
    const std::vector<double> buffer = {1, 2, 3, 4, 5};
    // act
    const View<double> view(buffer.data(), 3);
    // assert
    ASSERT_EQ(view.size(), 3);
    ASSERT_EQ(view.stride(), 1);
    ASSERT_EQ(view.data(), buffer.data());
    ASSERT_FALSE(view.empty());
    ASSERT_EQ(view[0], 1);
    ASSERT_EQ(view[1], 2);
    ASSERT_EQ(view[2], 3);
}

TEST(ViewChecks, pointer_constructor_with_offset_views_suffix_of_buffer) {
    // arrange
    const std::vector<double> buffer = {1, 2, 3, 4, 5};
    // act
    const View<double> view(buffer.data() + 2, 3);
    // assert
    ASSERT_EQ(view.data(), buffer.data() + 2);
    ASSERT_EQ(view.size(), 3);
    ASSERT_EQ(view[0], 3);
    ASSERT_EQ(view[1], 4);
    ASSERT_EQ(view[2], 5);
}

TEST(ViewChecks, view_index_operator_respects_stride) {
    // arrange
    const std::vector<double> buffer = {1, 10, 2, 20, 3, 30};
    const View<double> view(buffer.data(), 3, 2);
    // act
    const std::vector<double> values = {view[0], view[1], view[2]};
    // assert
    ASSERT_EQ(values[0], 1);
    ASSERT_EQ(values[1], 2);
    ASSERT_EQ(values[2], 3);
}

TEST(ViewChecks, view_size_returns_constructor_size) {
    // arrange
    const std::vector<double> buffer = {1, 10, 2, 20, 3, 30};
    const View<double> view(buffer.data(), 3, 2);
    // act
    const auto size = view.size();
    // assert
    ASSERT_EQ(size, 3);
}

TEST(ViewChecks, stride_returns_constructor_stride) {
    // arrange
    const std::vector<double> buffer = {1, 10, 2, 20, 3, 30};
    // act
    const View<double> view(buffer.data(), 3, 2);
    // assert
    ASSERT_EQ(view.stride(), 2);
}

TEST(ViewChecks, empty_returns_true_for_zero_size_view) {
    // arrange
    const std::vector<double> buffer = {1, 2, 3};
    // act
    const View<double> view(buffer.data(), 0);
    // assert
    ASSERT_TRUE(view.empty());
    ASSERT_EQ(view.size(), 0);
}

TEST(ViewChecks, empty_returns_false_for_non_empty_view) {
    // arrange
    const std::vector<double> buffer = {1, 2, 3};
    // act
    const View<double> view(buffer.data(), 3);
    // assert
    ASSERT_FALSE(view.empty());
}

TEST(ViewChecks, view_vector_constructor_owns_data) {
    // act
    const View<double> view(std::vector<double>{4, 5, 6});
    // assert
    ASSERT_EQ(view.size(), 3);
    ASSERT_EQ(view.stride(), 1);
    ASSERT_EQ(view.data(), &view[0]);
    ASSERT_EQ(view[0], 4);
    ASSERT_EQ(view[1], 5);
    ASSERT_EQ(view[2], 6);
}

TEST(ViewChecks, data_owner_keeps_underlying_data_alive) {
    // arrange
    std::shared_ptr<View<double>> owner = std::make_shared<View<double>>(std::vector<double>{10, 20, 30});
    View<double> view(owner->data(), 3, 1, owner);
    // act
    owner.reset();
    // assert
    ASSERT_EQ(view.size(), 3);
    ASSERT_EQ(view[0], 10);
    ASSERT_EQ(view[1], 20);
    ASSERT_EQ(view[2], 30);
}

TEST(ViewChecks, begin_end_support_range_for_over_all_elements) {
    // arrange
    const std::vector<double> buffer = {1, 10, 2, 20, 3, 30};
    const View<double> view(buffer.data(), 3, 2);
    // act
    std::vector<double> values;
    for (double value : view)
        values.push_back(value);
    // assert
    ASSERT_EQ(values, (std::vector<double>{1, 2, 3}));
}

TEST(ViewChecks, iterator_pre_increment_walks_strided_elements) {
    // arrange
    const std::vector<double> buffer = {1, 10, 2, 20, 3, 30};
    const View<double> view(buffer.data(), 3, 2);
    // act
    auto it = view.begin();
    // assert
    ASSERT_EQ(*it, 1);
    ++it;
    ASSERT_EQ(*it, 2);
    ++it;
    ASSERT_EQ(*it, 3);
    ++it;
    ASSERT_EQ(it, view.end());
}

TEST(ViewChecks, iterator_post_increment_returns_previous_position) {
    // arrange
    const std::vector<double> buffer = {1, 2, 3};
    const View<double> view(buffer.data(), 3);
    // act
    auto it = view.begin();
    auto previous = it++;
    // assert
    ASSERT_EQ(*previous, 1);
    ASSERT_EQ(*it, 2);
}

TEST(ViewChecks, iterator_arrow_operator_dereferences_pointer) {
    // arrange
    const std::vector<double> buffer = {42.0};
    const View<double> view(buffer.data(), 1);
    // act
    auto it = view.begin();
    // assert
    ASSERT_EQ(*(it.operator->()), 42.0);
}

TEST(ViewChecks, iterator_equality_compares_positions) {
    // arrange
    const std::vector<double> buffer = {1, 2, 3};
    const View<double> view(buffer.data(), 3);
    // act
    auto begin = view.begin();
    auto end = view.end();
    // assert
    ASSERT_EQ(begin, view.begin());
    ASSERT_NE(begin, end);
    ASSERT_NE(end, view.begin());
}

TEST(ViewChecks, iterator_default_construction_yields_null_iterator) {
    // act
    View<double>::Iterator first;
    View<double>::Iterator second;
    // assert
    ASSERT_EQ(first, second);
}

TEST(ViewChecks, empty_range_begin_equals_end) {
    // arrange
    const std::vector<double> buffer = {1, 2, 3};
    // act
    const View<double> view(buffer.data(), 0);
    // assert
    ASSERT_EQ(view.begin(), view.end());
}

TEST(ViewChecks, default_constructed_view_is_empty) {
    // act
    const View<double> view;
    // assert
    ASSERT_EQ(view.size(), 0);
    ASSERT_TRUE(view.empty());
    ASSERT_EQ(view.stride(), 1);
    ASSERT_EQ(view.data(), nullptr);
    ASSERT_EQ(view.begin(), view.end());
}

TEST(ViewChecks, moved_from_view_is_empty) {
    // arrange
    View<double> source(std::vector<double>{1, 2, 3});
    // act
    View<double> moved(std::move(source));
    // assert
    ASSERT_EQ(moved.size(), 3);
    ASSERT_EQ(moved[0], 1);
    ASSERT_EQ(moved[2], 3);
    ASSERT_EQ(source.size(), 0);
    ASSERT_TRUE(source.empty());
    ASSERT_EQ(source.data(), nullptr);
}

TEST(ViewChecks, move_assigned_to_source_becomes_empty) {
    // arrange
    View<double> source(std::vector<double>{1, 2, 3});
    View<double> target(std::vector<double>{9, 9, 9, 9});
    // act
    target = std::move(source);
    // assert
    ASSERT_EQ(target.size(), 3);
    ASSERT_EQ(target[0], 1);
    ASSERT_EQ(target[2], 3);
    ASSERT_EQ(source.size(), 0);
    ASSERT_TRUE(source.empty());
    ASSERT_EQ(source.data(), nullptr);
}

TEST(ViewChecks, move_assignment_onto_owning_view_releases_old_data) {
    // arrange
    View<double> source(std::vector<double>{1, 2, 3});
    // act
    View<double> target(std::vector<double>{9, 9, 9, 9});
    target = std::move(source);
    // assert
    ASSERT_EQ(target.size(), 3);
    ASSERT_EQ(target[0], 1);
    ASSERT_EQ(target[2], 3);
    ASSERT_TRUE(source.empty());
}

TEST(ViewChecks, move_assignment_onto_strided_view_overrides_state) {
    // arrange
    const std::vector<double> buffer = {1, 10, 2, 20, 3, 30};
    // act
    View<double> target(buffer.data(), 3, 2);
    View<double> source(std::vector<double>{4, 5, 6});
    target = std::move(source);
    // assert
    ASSERT_EQ(target.size(), 3);
    ASSERT_EQ(target.stride(), 1);
    ASSERT_EQ(target[0], 4);
    ASSERT_EQ(target[2], 6);
}

TEST(ViewChecks, moved_from_owning_view_does_not_dangle) {
    // arrange
    // act
    View<double> source(std::vector<double>{1, 2, 3});
    View<double> target(std::move(source));
    // assert
    ASSERT_EQ(target.data(), &target[0]);
    ASSERT_NE(target.data(), nullptr);
}

TEST(ViewChecks, data_owner_transfers_across_move) {
    // arrange
    std::shared_ptr<View<double>> owner = std::make_shared<View<double>>(std::vector<double>{10, 20, 30});
    View<double> source(owner->data(), 3, 1, owner);
    owner.reset();
    // act
    View<double> target(std::move(source));
    // assert
    ASSERT_EQ(target.size(), 3);
    ASSERT_EQ(target[0], 10);
    ASSERT_EQ(target[2], 30);
}

TEST(ViewChecks, data_owner_transfers_across_move_assignment) {
    // arrange
    std::shared_ptr<View<double>> owner = std::make_shared<View<double>>(std::vector<double>{10, 20, 30});
    View<double> source(owner->data(), 3, 1, owner);
    owner.reset();
    // act
    View<double> target(std::vector<double>{9});
    target = std::move(source);
    // assert
    ASSERT_EQ(target.size(), 3);
    ASSERT_EQ(target[0], 10);
    ASSERT_EQ(target[2], 30);
}

TEST(ViewChecks, data_returns_base_pointer_for_strided_view) {
    // arrange
    const std::vector<double> buffer = {1, 10, 2, 20, 3, 30};
    // act
    const View<double> view(buffer.data(), 3, 2);
    // assert
    ASSERT_EQ(view.data(), buffer.data());
    ASSERT_EQ(view[0], 1);
    ASSERT_EQ(view[2], 3);
}

TEST(ViewChecks, stride_zero_view_is_rejected_by_begin_end) {
    // arrange
    const std::vector<double> buffer = {1, 2, 3};
    // act
    const View<double> view(buffer.data(), 3, 0);
    // assert: a zero stride makes end() equal begin(), so the view is not iterable
    ASSERT_EQ(view.size(), 3);
    ASSERT_EQ(view[0], 1);
    ASSERT_EQ(view.begin(), view.end());
}

TEST(ViewChecks, empty_rvalue_vector_constructs_empty_view) {
    // act
    const View<double> view(std::vector<double>{});
    // assert
    ASSERT_TRUE(view.empty());
    ASSERT_EQ(view.size(), 0);
    ASSERT_EQ(view.begin(), view.end());
}

TEST(ViewChecks, iterator_copy_is_independent) {
    // arrange
    const std::vector<double> buffer = {1, 10, 2, 20, 3, 30};
    const View<double> view(buffer.data(), 3, 2);
    // act
    auto it = view.begin();
    auto copy = it;
    ++it;
    // assert
    ASSERT_EQ(*copy, 1);
    ASSERT_EQ(*it, 2);
}

TEST(ViewChecks, std_distance_matches_size) {
    // arrange
    const std::vector<double> buffer = {1, 10, 2, 20, 3, 30};
    const View<double> view(buffer.data(), 3, 2);
    // act
    const auto distance = std::distance(view.begin(), view.end());
    // assert
    ASSERT_EQ(distance, 3);
    ASSERT_EQ(static_cast<size_t>(distance), view.size());
}

TEST(ViewChecks, std_find_works_over_strided_view) {
    // arrange
    const std::vector<double> buffer = {1, 10, 2, 20, 3, 30};
    const View<double> view(buffer.data(), 3, 2);
    // act
    const auto found = std::find(view.begin(), view.end(), 3.0);
    const auto missing = std::find(view.begin(), view.end(), 10.0);
    // assert
    ASSERT_NE(found, view.end());
    ASSERT_EQ(*found, 3.0);
    ASSERT_EQ(missing, view.end());
}

TEST(ViewChecks, const_buffer_view_reads_values) {
    // arrange
    const double buffer[] = {1.0, 2.0, 3.0};
    // act
    const View<double> view(buffer, 3);
    // assert
    ASSERT_EQ(view.size(), 3);
    ASSERT_EQ(view[0], 1.0);
    ASSERT_EQ(view[2], 3.0);
}

TEST(ViewChecks, strided_data_owner_view_reads_values) {
    // arrange
    std::shared_ptr<View<double>> owner = std::make_shared<View<double>>(std::vector<double>{10, 20, 30, 40, 50, 60});
    // act
    View<double> view(owner->data(), 3, 2, owner);
    owner.reset();
    // assert
    ASSERT_EQ(view.size(), 3);
    ASSERT_EQ(view.stride(), 2);
    ASSERT_EQ(view[0], 10);
    ASSERT_EQ(view[1], 30);
    ASSERT_EQ(view[2], 50);
}

template <typename T>
class ViewTypedTest : public ::testing::Test
{
};

using ViewTypes = ::testing::Types<double, std::complex<double>, float, int>;
TYPED_TEST_SUITE(ViewTypedTest, ViewTypes);

TYPED_TEST(ViewTypedTest, operator_index_respects_stride) {
    // arrange
    const std::vector<TypeParam> buffer = {TypeParam(1), TypeParam(2), TypeParam(3), TypeParam(4)};
    // act
    const View<TypeParam> view(buffer.data(), 3, 1);
    // assert
    ASSERT_EQ(view.size(), 3);
    ASSERT_EQ(view.stride(), 1);
    ASSERT_FALSE(view.empty());
    ASSERT_EQ(view[0], TypeParam(1));
    ASSERT_EQ(view[1], TypeParam(2));
    ASSERT_EQ(view[2], TypeParam(3));
}

TYPED_TEST(ViewTypedTest, begin_end_empty_and_stride) {
    // arrange
    const std::vector<TypeParam> buffer = {TypeParam(1), TypeParam(2), TypeParam(3), TypeParam(4)};
    const View<TypeParam> view(buffer.data(), 2, 2);
    // act
    std::vector<TypeParam> values;
    for (const auto& value : view)
        values.push_back(value);
    // assert
    ASSERT_EQ(values.size(), 2);
    ASSERT_EQ(values[0], TypeParam(1));
    ASSERT_EQ(values[1], TypeParam(3));
    ASSERT_FALSE(view.empty());
    ASSERT_EQ(std::next(view.begin(), 2), view.end());
}

TYPED_TEST(ViewTypedTest, empty_view_begin_equals_end) {
    // arrange
    const std::vector<TypeParam> buffer = {TypeParam(1)};
    // act
    const View<TypeParam> view(buffer.data(), 0);
    // assert
    ASSERT_TRUE(view.empty());
    ASSERT_EQ(view.size(), 0);
    ASSERT_EQ(view.begin(), view.end());
}
