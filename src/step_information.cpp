#include "step_information.h"

// default constructor
StepInformation::StepInformation()
    : m_step_count(0), m_abscissa_ascending(true), m_abscissa_left_value(0.0), m_abscissa_right_value(0.0)
{
}


// parameterized constructor
StepInformation::StepInformation(std::vector<std::string> keys, std::vector<std::vector<double>> values, std::vector<std::pair<double, double>> abscissa_value_ranges)
    : m_keys(std::move(keys)), m_values(std::move(values)), m_abscissa_value_ranges(std::move(abscissa_value_ranges))
{
    // assign step count
    m_step_count = m_abscissa_value_ranges.size();
    // determine direction
    m_abscissa_ascending = m_step_count > 0 ? (m_abscissa_value_ranges[0].first <= m_abscissa_value_ranges[0].second) : true;
    if (m_step_count > 0) {
        if (m_abscissa_ascending) {
            // initialize left and right values
            double left = m_abscissa_value_ranges[0].first;
            // initialize right
            double right = m_abscissa_value_ranges[0].second;
            for (const auto&[first, second] : m_abscissa_value_ranges) {
                // compute minimum
                left = std::min(left, first);
                // compute maximum
                right = std::max(right, second);
            }
            // assign left value
            m_abscissa_left_value = left;
            // assign right value
            m_abscissa_right_value = right;
        }
        else {
            // initialize left and right values for descending
            double left = m_abscissa_value_ranges[0].first;
            // initialize right
            double right = m_abscissa_value_ranges[0].second;
            for (const auto&[first, second] : m_abscissa_value_ranges) {
                // compute maximum for left value
                left = std::max(left, first);
                // compute minimum for right value
                right = std::min(right, second);
            }
            // assign left value
            m_abscissa_left_value = left;
            // assign right value
            m_abscissa_right_value = right;
        }
    }
    else {
        // default left value
        m_abscissa_left_value = 0.0;
        // default right value
        m_abscissa_right_value = 0.0;
    }
}


// keys getter
const std::vector<std::string>& StepInformation::keys() const {
    // return keys
    return m_keys;
}


// values getter
const std::vector<std::vector<double>>& StepInformation::values() const {
    // return values
    return m_values;
}


// length getter
size_t StepInformation::length() const {
    // return count
    return m_step_count;
}


// abscissa left value getter
double StepInformation::abscissa_left_value() const {
    // return left
    return m_abscissa_left_value;
}


// abscissa right value getter
double StepInformation::abscissa_right_value() const {
    // return right
    return m_abscissa_right_value;
}


// abscissa ascending status getter
bool StepInformation::abscissa_ascending() const {
    // return ascending
    return m_abscissa_ascending;
}


// step abscissa left value getter
double StepInformation::step_abscissa_left_value(size_t step_index) const {
    // return left value for index
    return m_abscissa_value_ranges.at(step_index).first;
}


// step abscissa right value getter
double StepInformation::step_abscissa_right_value(size_t step_index) const {
    // return right value for index
    return m_abscissa_value_ranges.at(step_index).second;
}
