#pragma once

#include <optional>
#include <string>
#include <vector>

#include "../netlist/netlist.h"
#include "measure_parameters.h"
#include "print_parameters.h"
#include "sens_parameter.h"

// DC simulation parameters class — parses and serializes Xyce .DC directives
class DCSimulationParameters
{
public:
    // construct a DC simulation parameters instance from individual fields
    DCSimulationParameters(std::string sweep_mode, std::string primary_variable, std::string start, std::string stop, std::string step, std::string points, std::vector<std::string> list_values, std::string data_table_name, std::string secondary_variable, std::string secondary_start, std::string secondary_stop, std::string secondary_step, std::string secondary_points, bool replace_ground, std::optional<PrintParameters> print_parameters, std::vector<MeasureEntry> measure_parameters, std::optional<SensParameter> sensitivity);

    // parse all directives into a DCSimulationParameters instance;
    // returns nullopt when no .DC directive is found
    [[nodiscard]] static std::optional<DCSimulationParameters> from_xyce_directives(const std::vector<std::string>& directives);

    // serialize this instance to a list of Xyce directive strings
    [[nodiscard]] std::vector<std::string> to_xyce_directives(const NetlistTopology* topology = nullptr) const;

    // equality operator
    [[nodiscard]] bool operator==(const DCSimulationParameters& other) const;

    // sweep mode: "LIN", "DEC", "OCT", "LIST", "DATA"
    std::string sweep_mode;
    // primary swept variable name
    std::string primary_variable;
    // start value of the primary sweep
    std::string start;
    // stop value of the primary sweep
    std::string stop;
    // step size (LIN sweep only)
    std::string step;
    // number of points (DEC/OCT sweep only)
    std::string points;
    // explicit list of values (LIST sweep only)
    std::vector<std::string> list_values;
    // data table name (DATA sweep only)
    std::string data_table_name;
    // secondary swept variable name
    std::string secondary_variable;
    // start value of the secondary sweep
    std::string secondary_start;
    // stop value of the secondary sweep
    std::string secondary_stop;
    // step size (LIN sweep only)
    std::string secondary_step;
    // number of points (DEC/OCT sweep only)
    std::string secondary_points;
    // whether to apply replaceground preprocessing
    bool replace_ground;
    // optional print parameters
    std::optional<PrintParameters> print_parameters;
    // measure directives
    std::vector<MeasureEntry> measure_parameters;
    // optional sensitivity parameters
    std::optional<SensParameter> sensitivity;
};
