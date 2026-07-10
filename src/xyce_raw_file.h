#ifndef XYCE_RAW_FILE_H
#define XYCE_RAW_FILE_H

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <memory>
#include <filesystem>
#include <complex>
#include <stdexcept>

#include "expression.h"
#include "expression_manager.h"
#include "step_information.h"

// abscissa scale enumeration
enum class AbscissaScale {
    LINEAR,
    DECADE,
    OCTAVE
};


// variable type enumeration
enum class VariableType {
    FREQUENCY,
    VOLTAGE,
    CURRENT,
    TIME,
    POWER,
    PARAMETER,
    PHASE,
    UNKNOWN
};


// variable type information structure
struct VariableTypeInformation {
    // name field
    std::string name;
    // unit field
    std::string unit;
};


// get variable type info helper
inline VariableTypeInformation get_variable_type_info(const VariableType vt) {
    switch (vt) {
        // frequency case
        case VariableType::FREQUENCY:
            return {"frequency", "Hz"};
        // voltage case
        case VariableType::VOLTAGE:
            return {"voltage", "V"};
        // current case
        case VariableType::CURRENT:
            return {"current", "A"};
        // time case
        case VariableType::TIME:
            return {"time", "s"};
        // power case
        case VariableType::POWER:
            return {"power", "W"};
        // parameter case
        case VariableType::PARAMETER:
            return {"parameter", ""};
        // phase case
        case VariableType::PHASE:
            return {"phase", "°"};
        // unknown case
        case VariableType::UNKNOWN:
        default:
            return {"unknown", ""};
    }
}


// parse variable type from string
inline std::optional<VariableType> parse_variable_type(const std::string& type_str) {
    // check frequency
    if (type_str == "frequency") {
        return VariableType::FREQUENCY;
    }
    // check voltage
    if (type_str == "voltage") {
        return VariableType::VOLTAGE;
    }
    // check current
    if (type_str == "current") {
        return VariableType::CURRENT;
    }
    // check time
    if (type_str == "time") {
        return VariableType::TIME;
    }
    // check power
    if (type_str == "power") {
        return VariableType::POWER;
    }
    // check parameter
    if (type_str == "parameter") {
        return VariableType::PARAMETER;
    }
    // check phase
    if (type_str == "phase") {
        return VariableType::PHASE;
    }
    // check unknown
    if (type_str == "unknown") {
        return VariableType::UNKNOWN;
    }
    // fallback
    return std::nullopt;
}



// xyce output file class
class XyceOutputFile {
public:
    // constructor
    XyceOutputFile(std::filesystem::path filename, std::string title, bool is_complex, StepInformation step_info, const Expression& abscissa, AbscissaScale abscissa_scale, ExpressionManager expr_mgr, const void* mmap_ptr, size_t mmap_len);

    // destructor
    ~XyceOutputFile();

    // prevent copy construction
    XyceOutputFile(const XyceOutputFile&) = delete;

    // prevent copy assignment
    XyceOutputFile& operator=(const XyceOutputFile&) = delete;

    // move constructor
    XyceOutputFile(XyceOutputFile&& other) noexcept;

    // move assignment
    XyceOutputFile& operator=(XyceOutputFile&& other) noexcept;

    // filename getter
    [[nodiscard]] const std::filesystem::path& filename() const;

    // title getter
    [[nodiscard]] const std::string& title() const;

    // complex status getter
    [[nodiscard]] bool is_complex() const;

    // step information getter
    [[nodiscard]] const StepInformation& step_information() const;

    // steps count getter
    [[nodiscard]] size_t steps() const;

    // abscissa getter
    [[nodiscard]] const Expression& abscissa() const;

    // abscissa scale getter
    [[nodiscard]] AbscissaScale abscissa_scale() const;

    // chart type getter
    [[nodiscard]] std::string chart_type() const;

    // expression manager getter
    [[nodiscard]] const ExpressionManager& expression_manager() const;

private:
    // filename field
    std::filesystem::path m_filename;
    // title field
    std::string m_title;
    // complex status field
    bool m_complex;
    // step information field
    StepInformation m_step_information;
    // abscissa expression field
    Expression m_abscissa;
    // abscissa scale field
    AbscissaScale m_abscissa_scale;
    // expression manager field
    ExpressionManager m_expression_manager;
    // memory map pointer
    const void* m_mmap_ptr = nullptr;
    // memory map length
    size_t m_mmap_len = 0;
};


// parse xyce raw file
std::shared_ptr<XyceOutputFile> xyce_raw_file_parser(const std::filesystem::path& filename);

#endif // XYCE_RAW_FILE_H
