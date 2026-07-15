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

class XyceEvaluator
{
public:
    XyceValue evaluate(const ExpressionNode& expression, const std::unordered_map<std::string, XyceValue>& expressions = {}, const std::unordered_map<std::string, FunctionDefinitionNode>& functions = {}, const std::unordered_map<std::string, XyceValue>& constants = {}, const std::optional<std::vector<std::pair<size_t, size_t>>>& step_slices = std::nullopt);

private:
    XyceValue evaluate(const ExpressionNode& expression, EvaluationContext& context, const std::vector<std::string>& call_stack);

    XyceValue evaluate_unary(const UnaryOperationNode& expression, EvaluationContext& context, const std::vector<std::string>& call_stack);

    XyceValue evaluate_binary(const BinaryOperationNode& expression, EvaluationContext& context, const std::vector<std::string>& call_stack);

    XyceValue evaluate_ternary(const TernaryOperationNode& expression, EvaluationContext& context, const std::vector<std::string>& call_stack);

    XyceValue evaluate_function_call(const FunctionCallNode& expression, EvaluationContext& context, const std::vector<std::string>& call_stack);

    XyceValue evaluate_step_selector(const StepSelectorNode& expression, EvaluationContext& context, const std::vector<std::string>& call_stack);

    XyceValue lookup_name(const std::string& name, EvaluationContext& context);

    XyceValue evaluate_probe(const FunctionCallNode& expression, EvaluationContext& context);

    static bool is_probe_call(const FunctionCallNode& expression);

    static bool has_simple_probe_args(const FunctionCallNode& expression);

    static std::string reconstruct_probe_name(const FunctionCallNode& expression);

    static std::string node_name_from_expr(const ExpressionNode& expression);

    static std::string extract_node_name(const ExpressionNode& expression);

    static bool is_scalar_value(const XyceValue& value);

    static size_t value_size(const XyceValue& value);

    static double scalar_real(const XyceValue& value);

    static std::complex<double> scalar_complex(const XyceValue& value);

    static std::vector<double> boolean_result(const std::vector<double>& values);

    static XyceValue boolean_result(bool value);

    static XyceValue truth_mask(const XyceValue& value);

    static XyceValue apply_unary_numeric(const XyceValue& value, const std::function<std::complex<double>(std::complex<double>)>& fn);

    static XyceValue apply_binary_numeric(const XyceValue& left, const XyceValue& right, const std::function<std::complex<double>(std::complex<double>, std::complex<double>)>& fn);

    static XyceValue apply_binary_real(const XyceValue& left, const XyceValue& right, const std::function<double(double, double)>& fn);

    static XyceValue apply_compare(const XyceValue& left, const XyceValue& right, const std::function<bool(double, double)>& fn);

    static XyceValue apply_elementwise(const XyceValue& left, const XyceValue& right, const std::function<double(double, double)>& fn);

    static XyceValue apply_elementwise_complex(const XyceValue& left, const XyceValue& right, const std::function<std::complex<double>(std::complex<double>, std::complex<double>)>& fn);

    static XyceValue apply_unary_elementwise(const XyceValue& value, const std::function<double(double)>& fn);

    static XyceValue apply_unary_elementwise_complex(const XyceValue& value, const std::function<std::complex<double>(std::complex<double>)>& fn);

    static std::string casefold(std::string text);

    static XyceValue to_array(const XyceValue& value);
};

XyceValue evaluate_expression(const ExpressionNode& expression, const std::unordered_map<std::string, XyceValue>& expressions = {}, const std::unordered_map<std::string, FunctionDefinitionNode>& functions = {}, const std::unordered_map<std::string, XyceValue>& constants = {}, const std::optional<std::vector<std::pair<size_t, size_t>>>& step_slices = std::nullopt);

extern const std::unordered_map<std::string, double> NUMBER_SUFFIXES;
extern const std::unordered_map<std::string, BuiltinCallable> BUILTIN_FUNCTIONS;
extern const std::unordered_map<std::string, XyceValue> BUILTIN_CONSTANTS;

#endif
