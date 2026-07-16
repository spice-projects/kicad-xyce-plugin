#ifndef XYCE_EVALUATOR_H
#define XYCE_EVALUATOR_H

#include <complex>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "xyce_parser.h"

using XyceValue = std::variant<double, std::complex<double>, std::vector<double>, std::vector<std::complex<double>>>;

using BuiltinCallable = std::function<XyceValue(const std::vector<XyceValue>&)>;

struct EvaluationContext
{
    std::unordered_map<std::string, const XyceValue*> expressions;
    std::unordered_map<std::string, XyceValue> variables;
    std::unordered_map<std::string, const FunctionDefinitionNode*> functions;
    std::unordered_map<std::string, const XyceValue*> constants;
    std::optional<std::vector<std::pair<size_t, size_t>>> step_slices;
};

XyceValue evaluate_expression(const ExpressionNode& expression, const std::unordered_map<std::string, XyceValue>& expressions = {}, const std::unordered_map<std::string, FunctionDefinitionNode>& functions = {}, const std::unordered_map<std::string, XyceValue>& constants = {}, const std::optional<std::vector<std::pair<size_t, size_t>>>& step_slices = std::nullopt);

extern const std::unordered_map<std::string, double> NUMBER_SUFFIXES;
extern const std::unordered_map<std::string, BuiltinCallable> BUILTIN_FUNCTIONS;
extern const std::unordered_map<std::string, XyceValue> BUILTIN_CONSTANTS;

#endif
