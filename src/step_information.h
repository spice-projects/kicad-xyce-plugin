#ifndef STEP_INFORMATION_H
#define STEP_INFORMATION_H

#include <string>
#include <vector>

// step information class
class StepInformation {
public:
    // default constructor
    StepInformation();

    // parameterized constructor
    StepInformation(std::vector<std::string> keys, std::vector<std::vector<double>> values, std::vector<std::pair<double, double>> abscissa_value_ranges);

    // keys getter
    [[nodiscard]] const std::vector<std::string>& keys() const;

    // values getter
    [[nodiscard]] const std::vector<std::vector<double>>& values() const;

    // length getter
    [[nodiscard]] size_t length() const;

    // abscissa left value getter
    [[nodiscard]] double abscissa_left_value() const;

    // abscissa right value getter
    [[nodiscard]] double abscissa_right_value() const;

    // abscissa ascending status getter
    [[nodiscard]] bool abscissa_ascending() const;

    // step abscissa left value getter
    [[nodiscard]] double step_abscissa_left_value(size_t step_index) const;

    // step abscissa right value getter
    [[nodiscard]] double step_abscissa_right_value(size_t step_index) const;

private:
    // keys list
    std::vector<std::string> m_keys;
    // values list
    std::vector<std::vector<double>> m_values;
    // abscissa value ranges list
    std::vector<std::pair<double, double>> m_abscissa_value_ranges;
    // step count field
    size_t m_step_count;
    // abscissa ascending flag
    bool m_abscissa_ascending;
    // abscissa left value field
    double m_abscissa_left_value;
    // abscissa right value field
    double m_abscissa_right_value;
};

#endif
