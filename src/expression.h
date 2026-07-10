#ifndef KICAD_XYCE_PLUGIN_EXPRESSION_H
#define KICAD_XYCE_PLUGIN_EXPRESSION_H

#include <string>
#include <vector>
#include <optional>

#include "array_view.h"

// expression class
class Expression {
public:
    // default constructor
    Expression() = default;

    // parameterized constructor
    Expression(std::string name, StepDataVariant steps, std::string unit, std::optional<std::string> source = std::nullopt, std::optional<std::string> variable_type = std::nullopt);

    // name getter
    const std::string& name() const;

    // unit getter
    const std::string& unit() const;

    // source getter
    const std::optional<std::string>& source() const;

    // variable type getter
    const std::optional<std::string>& variable_type() const;

    // complex status checker
    bool is_complex() const;

    // step count getter
    size_t step_count() const;

    // step data real getter
    const ArrayView<double>& step_data_real(size_t step_index) const;

    // step data complex getter
    const ArrayView<std::complex<double>>& step_data_complex(size_t step_index) const;

    // full data real view getter
    ArrayView<double> data_real() const;

    // full data complex view getter
    ArrayView<std::complex<double>> data_complex() const;

private:
    // name field
    std::string m_name;
    // steps field
    StepDataVariant m_steps;
    // unit field
    std::string m_unit;
    // source field
    std::optional<std::string> m_source;
    // variable type field
    std::optional<std::string> m_variable_type;
    // cached data field
    mutable std::variant<std::monostate, std::vector<double>, std::vector<std::complex<double>>> m_cached_data;
};

#endif