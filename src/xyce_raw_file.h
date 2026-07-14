#ifndef XYCE_RAW_FILE_H
#define XYCE_RAW_FILE_H

#include <filesystem>
#include <memory>
#include <string>

#include "xyce_output_file.h"

enum class VariableType
{
    FREQUENCY,
    VOLTAGE,
    CURRENT,
    TIME,
    POWER,
    PARAMETER,
    PHASE,
    UNKNOWN
};

inline std::tuple<std::string, std::string> get_variable_type_info(const VariableType vt) {
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

inline VariableType parse_variable_type(const std::string& type_str) {
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
    // unknown
    return VariableType::UNKNOWN;
}

std::shared_ptr<XyceOutputFile> xyce_raw_file_parser(const std::filesystem::path& filename);

#endif // XYCE_RAW_FILE_H
