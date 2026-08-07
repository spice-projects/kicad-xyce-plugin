#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <vector>

#include "../expression/expression_manager.h"
#include "../step_information.h"
#include "xyce_output_file.h"

std::optional<std::vector<std::shared_ptr<XyceOutputFile>>> xyce_fft_file_parser(const std::filesystem::path& file_pattern, const StepInformation& step_information, ExpressionManager* expression_manager = nullptr);
