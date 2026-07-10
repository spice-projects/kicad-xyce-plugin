//
// Created by Rogelio J. Baucells on 7/9/26.
//

#include <complex>
#include "array_view.h"
#include "expression.h"

// parameterized constructor
Expression::Expression(std::string name, StepDataVariant steps, std::string unit, std::optional<std::string> source, std::optional<std::string> variable_type)
    : m_name(std::move(name)), m_steps(std::move(steps)), m_unit(std::move(unit)), m_source(std::move(source)), m_variable_type(std::move(variable_type))
{
}


// name getter
const std::string& Expression::name() const {
    // return name
    return m_name;
}


// unit getter
const std::string& Expression::unit() const {
    // return unit
    return m_unit;
}


// source getter
const std::optional<std::string>& Expression::source() const {
    // return source
    return m_source;
}


// variable type getter
const std::optional<std::string>& Expression::variable_type() const {
    // return type
    return m_variable_type;
}


// complex status checker
bool Expression::is_complex() const {
    // check variant type
    return std::holds_alternative<ComplexStepData>(m_steps);
}


// step count getter
size_t Expression::step_count() const {
    if (is_complex()) {
        // return complex step count
        return std::get<ComplexStepData>(m_steps).size();
    }
    // return real step count
    return std::get<RealStepData>(m_steps).size();
}


// step data real getter
const ArrayView<double>& Expression::step_data_real(size_t step_index) const {
    // return real view
    return std::get<RealStepData>(m_steps).at(step_index);
}


// step data complex getter
const ArrayView<std::complex<double>>& Expression::step_data_complex(size_t step_index) const {
    // return complex view
    return std::get<ComplexStepData>(m_steps).at(step_index);
}


// full data real view getter
ArrayView<double> Expression::data_real() const {
    if (is_complex()) {
        // error out
        throw std::runtime_error("Expression is complex, cannot get real data");
    }
    // get steps
    const auto& steps = std::get<RealStepData>(m_steps);
    if (steps.empty()) {
        // empty view
        return {};
    }
    if (steps.size() == 1) {
        // return first step directly
        return steps[0];
    }
    if (!std::holds_alternative<std::vector<double>>(m_cached_data)) {
        // create new vector
        std::vector<double> concatenated;
        // init total size
        size_t total_size = 0;
        for (const auto& step : steps) {
            // accumulate size
            total_size += step.size();
        }
        // reserve memory
        concatenated.reserve(total_size);
        for (const auto& step : steps) {
            for (size_t i = 0; i < step.size(); ++i) {
                // append value
                concatenated.push_back(step[i]);
            }
        }
        // cache data
        m_cached_data = std::move(concatenated);
    }
    // get cached vector
    const auto& cached = std::get<std::vector<double>>(m_cached_data);
    // return view of cached
    return {&cached[0], cached.size(), 1};
}


// full data complex view getter
ArrayView<std::complex<double>> Expression::data_complex() const {
    if (!is_complex()) {
        // error out
        throw std::runtime_error("Expression is real, cannot get complex data");
    }
    // get steps
    const auto& steps = std::get<ComplexStepData>(m_steps);
    if (steps.empty()) {
        // empty view
        return {};
    }
    if (steps.size() == 1) {
        // return first step directly
        return steps[0];
    }
    if (!std::holds_alternative<std::vector<std::complex<double>>>(m_cached_data)) {
        // create new vector
        std::vector<std::complex<double>> concatenated;
        // init total size
        size_t total_size = 0;
        for (const auto& step : steps) {
            // accumulate size
            total_size += step.size();
        }
        // reserve memory
        concatenated.reserve(total_size);
        for (const auto& step : steps) {
            for (size_t i = 0; i < step.size(); ++i) {
                // append value
                concatenated.push_back(step[i]);
            }
        }
        // cache data
        m_cached_data = std::move(concatenated);
    }
    // get cached vector
    const auto& cached = std::get<std::vector<std::complex<double>>>(m_cached_data);
    // return view of cached
    return {&cached[0], cached.size(), 1};
}
