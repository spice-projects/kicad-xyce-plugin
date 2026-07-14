#include <complex>
#include <span>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

#include "expression.h"

template <typename T>
std::tuple<std::variant<std::vector<double>, std::vector<std::complex<double>>>, Steps> process_steps(const std::vector<View<T>>& steps) {
    // total vector size
    size_t total_size = 0;
    // loop steps
    for (const auto& step : steps) {
        // accumulate size
        total_size += step.size();
    }
    // create data vector (pre-allocate size)
    std::vector<T> concatenated(total_size);
    // create views vector
    std::vector<std::span<const T>> views(steps.size());
    // data pointer and offset
    const T* pointer = concatenated.data();
    size_t offset = 0;
    // write position within concatenated vector
    size_t position = 0;
    // loop steps
    for (size_t i = 0; i < steps.size(); i++) {
        // step
        const auto& step = steps[i];
        // step size
        const auto length = step.size();
        // loop step data
        for (size_t j = 0; j < length; ++j)
            concatenated[position++] = step[j];
        // update view
        views[i] = std::span(pointer + offset, length);
        // move offset
        offset += length;
    }
    // return data (zero copy), copying variant only
    return {std::move(concatenated), std::move(views)};
}

Expression::Expression(std::string name, std::vector<View<double>>& steps, std::string unit, std::string source, std::string variable_type)
    : m_name(std::move(name)), m_steps(std::move(steps)), m_unit(std::move(unit)), m_source(std::move(source)), m_variable_type(std::move(variable_type)) {
    // initialize complex
    m_is_complex = false;
}

Expression::Expression(std::string name, std::vector<View<std::complex<double>>>& steps, std::string unit, std::string source, std::string variable_type)
    : m_name(std::move(name)), m_steps(std::move(steps)), m_unit(std::move(unit)), m_source(std::move(source)), m_variable_type(std::move(variable_type)) {
    // initialize complex
    m_is_complex = true;
}

Expression::Expression(std::string name, std::vector<double>& data, std::vector<std::span<const double>>& steps, std::string unit, std::string source, std::string variable_type)
    : m_name(std::move(name)), m_steps(std::move(steps)), m_unit(std::move(unit)), m_source(std::move(source)), m_variable_type(std::move(variable_type)), m_cached_data(std::move(data)) {
    // initialize complex
    m_is_complex = false;
}

Expression::Expression(std::string name, std::vector<std::complex<double>>& data, std::vector<std::span<const std::complex<double>>>& steps, std::string unit, std::string source, std::string variable_type)
    : m_name(std::move(name)), m_steps(std::move(steps)), m_unit(std::move(unit)), m_source(std::move(source)), m_variable_type(std::move(variable_type)), m_cached_data(std::move(data)) {
    // initialize complex
    m_is_complex = true;
}

const std::string& Expression::name() const {
    // return name
    return m_name;
}

const std::string& Expression::unit() const {
    // return unit
    return m_unit;
}

const std::string& Expression::source() const {
    // return source
    return m_source;
}

const std::string& Expression::variable_type() const {
    // return type
    return m_variable_type;
}

bool Expression::is_complex() const {
    // return complex
    return m_is_complex;
}

size_t Expression::step_count() const {
    // number of steps
    return std::visit([](auto&& v) { return v.size(); }, m_steps);
}

std::variant<std::span<const double>, std::span<const std::complex<double>>> Expression::step_data(const size_t step_index) {
    // check data has been initialized for this expression
    if (std::holds_alternative<std::monostate>(m_cached_data)) {
        // initialize expression
        initialize_expression_data();
    }
    // initialize processor
    auto l = [&step_index]<typename T0>(const T0& steps) -> std::variant<std::span<const double>, std::span<const std::complex<double>>> {
        // actual parameter type
        using T = std::decay_t<T0>;
        // process views only
        if constexpr (std::is_same_v<T, std::vector<std::span<const double>>>) {
            // return item at index
            return steps.at(step_index);
        }
        if constexpr (std::is_same_v<T, std::vector<std::span<const std::complex<double>>>>) {
            // return item at index
            return steps.at(step_index);
        }
        throw std::runtime_error("Unexpected state detected");
    };
    // visit steps, return data (copy a variant on output, but that is cheap (16–24 bytes))
    return std::visit(l, m_steps);
}

std::variant<std::span<const double>, std::span<const std::complex<double>>> Expression::data() {
    // check data has been initialized for this expression
    if (std::holds_alternative<std::monostate>(m_cached_data)) {
        // initialize expression
        initialize_expression_data();
    }
    // return data (create a span over the whole vector, copying a variant on return)
    return std::visit([](auto& v) -> std::variant<std::span<const double>, std::span<const std::complex<double>>> { return std::span(v); }, std::get<std::variant<std::vector<double>, std::vector<std::complex<double>>>>(m_cached_data));
}

std::vector<std::pair<size_t, size_t>> Expression::step_indices() const {
    // result
    std::vector<std::pair<size_t, size_t>> result;
    // lambda
    auto l = [&result](auto& steps) -> void {
        // reserve capacity
        result.reserve(steps.size());
        // offset
        size_t offset = 0;
        // loop steps
        for (auto& step : steps) {
            // end index for step
            size_t end = offset + step.size();
            // append slice
            result.emplace_back(offset, end);
            // update offset
            offset = end;
        }
    };
    // compute step slices
    std::visit(l, m_steps);
    // return slices
    return result;
}

void Expression::initialize_expression_data() {
    // initialize processor
    auto l = []<typename T0>(const T0& steps) -> std::tuple<std::variant<std::vector<double>, std::vector<std::complex<double>>>, Steps> {
        // actual parameter type
        using T = std::decay_t<T0>;
        // process views only
        if constexpr (std::is_same_v<T, std::vector<View<double>>>) {
            // process double
            return process_steps<double>(steps);
        }
        if constexpr (std::is_same_v<T, std::vector<View<std::complex<double>>>>) {
            // process complex
            return process_steps<std::complex<double>>(steps);
        }
        throw std::runtime_error("Unexpected state detected");
    };
    // visit steps
    auto [concatenated, views] = std::visit(l, m_steps);
    // update cache field (at the end of the processing since this moves the data into the field)
    m_cached_data.emplace<std::variant<std::vector<double>, std::vector<std::complex<double>>>>(std::move(concatenated));
    // update steps with views on the contiguous memory data
    m_steps = std::move(views);
}
