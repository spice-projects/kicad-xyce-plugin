#include <algorithm>
#include <cmath>
#include <complex>
#include <numbers>
#include <stdexcept>
#include <type_traits>

#include "xyce_evaluator.h"

namespace
{
    // read the scalar value from a builtin argument
    template <typename T>
    T scalar_value(const XyceValue& value) {
        // processor
        auto l = []<typename T0>(T0& arg) -> T {
            // actual parameter type
            using TX = std::decay_t<T0>;
            // real scalar
            if constexpr (std::is_same_v<T, double>) {
                // scalar double
                if constexpr (std::is_same_v<TX, double>) {
                    return arg;
                }
                else if constexpr (std::is_same_v<TX, std::complex<double>>) {
                    return arg.real();
                }
                else if constexpr (std::is_same_v<TX, std::vector<double>>) {
                    return arg.at(0);
                }
                else {
                    return arg.at(0).real();
                }
            }
            // complex scalar
            else if constexpr (std::is_same_v<TX, double>) {
                return T(arg, 0.0);
            }
            // scalar complex
            else if constexpr (std::is_same_v<TX, std::complex<double>>) {
                return arg;
            }
            // vector<double>
            else if constexpr (std::is_same_v<TX, std::vector<double>>) {
                return T(arg.at(0), 0.0);
            }
            else {
                return arg.at(0);
            }
        };
        // exit
        return std::visit(l, value);
    }

    // fail when a builtin receives the wrong arity
    void expect_arity(const std::string& name, const std::vector<XyceValue>& args, const size_t count) {
        if (args.size() != count) {
            throw std::invalid_argument("Function '" + name + "' expects " + std::to_string(count) + " arguments, got " + std::to_string(args.size()));
        }
    }

    // fail when a builtin receives too few arguments
    void expect_min_arity(const std::string& name, const std::vector<XyceValue>& args, const size_t count) {
        if (args.size() < count) {
            throw std::invalid_argument("Function '" + name + "' expects at least " + std::to_string(count) + " arguments, got " + std::to_string(args.size()));
        }
    }

    // normalize a value to a real vector for broadcasting
    std::vector<double> to_real_vector(const XyceValue& value) {
        // processor
        auto l = []<typename T0>(T0& arg) -> std::vector<double> {
            // actual parameter type
            using TX = std::decay_t<T0>;
            // scalar double
            if constexpr (std::is_same_v<TX, double>) {
                return {arg};
            }
            // scalar complex
            else if constexpr (std::is_same_v<TX, std::complex<double>>) {
                return {arg.real()};
            }
            // vector<double>
            else if constexpr (std::is_same_v<TX, std::vector<double>>) {
                return arg;
            }
            // vector<complex>
            else {
                // create output vector
                std::vector<double> out;
                out.reserve(arg.size());
                // append real values
                std::ranges::transform(arg.begin(), arg.end(), std::back_inserter(out), [](auto v) { return v.real(); });
                // exit
                return out;
            }
        };
        // exit
        return std::visit(l, value);
    }

    // tell whether the value should be treated as a vector
    bool is_vector(const XyceValue& value) {
        // processor
        auto l = []<typename T0>(T0&) {
            // actual parameter type
            using TX = std::decay_t<T0>;
            // vector<?>
            if constexpr (std::is_same_v<TX, std::vector<double>> || std::is_same_v<TX, std::vector<std::complex<double>>>) {
                return true;
            }
            return false;
        };
        // exit
        return std::visit(l, value);
    }

    // collapse a complex scalar to a real when possible
    XyceValue make_scalar(const std::complex<double>& value) {
        // convert to real if imaginary part is very small
        if (std::abs(value.imag()) < 1e-15) {
            return value.real();
        }
        return value;
    }

    // collapse a complex vector to a real vector when possible
    XyceValue make_vector(const std::vector<std::complex<double>>& values) {
        // execution flag
        bool all_real = true;
        // check we can eliminate the imaginary part
        for (const auto& value : values) {
            if (std::abs(value.imag()) >= 1e-15) {
                all_real = false;
                break;
            }
        }
        // we can convert vector
        if (all_real) {
            // allocate vector
            std::vector<double> out;
            out.reserve(values.size());
            // transform data
            std::ranges::transform(values.begin(), values.end(), std::back_inserter(out), [](auto v) { return v.real(); });
            // exit
            return out;
        }
        return values;
    }

    // apply a unary real function across a scalar or vector
    XyceValue map_unary_real(const XyceValue& value, const std::function<double(double)>& fn) {
        // processor
        auto l = [&fn]<typename T0>(T0& arg) -> XyceValue {
            // actual parameter type
            using TX = std::decay_t<T0>;
            // vector<double>
            if constexpr (std::is_same_v<TX, std::vector<double>>) {
                // create and allocate output vector
                std::vector<double> out;
                out.reserve(arg.size());
                // append mapped values
                std::ranges::transform(arg.begin(), arg.end(), std::back_inserter(out), fn);
                // exit
                return out;
            }
            // vector<complex>
            if constexpr (std::is_same_v<TX, std::vector<std::complex<double>>>) {
                // create and allocate output vector
                std::vector<double> out;
                out.reserve(arg.size());
                // append mapped values
                std::ranges::transform(arg.begin(), arg.end(), std::back_inserter(out), [&fn](auto v) { return fn(v.real()); });
                // exit
                return out;
            }
            // scalar
            return fn(scalar_value<double>(arg));
        };
        // exit
        return std::visit(l, value);
    }

    // apply a unary complex function across a scalar or vector
    XyceValue map_unary_complex(const XyceValue& value, const std::function<std::complex<double>(std::complex<double>)>& fn) {
        // processor
        auto l = [&fn]<typename T0>(T0& arg) -> XyceValue {
            // actual parameter type
            using TX = std::decay_t<T0>;
            // vector<double>
            if constexpr (std::is_same_v<TX, std::vector<double>>) {
                // create and allocate output vector
                std::vector<std::complex<double>> out;
                out.reserve(arg.size());
                // append mapped values
                std::ranges::transform(arg.begin(), arg.end(), std::back_inserter(out), [&fn](auto v) { return fn(std::complex<double>(v, 0.0)); });
                // exit
                return make_vector(out);
            }
            // vector<complex>
            if constexpr (std::is_same_v<TX, std::vector<std::complex<double>>>) {
                // create and allocate output vector
                std::vector<std::complex<double>> out;
                out.reserve(arg.size());
                // append mapped values
                std::ranges::transform(arg.begin(), arg.end(), std::back_inserter(out), fn);
                // exit
                return make_vector(out);
            }
            // scalar
            return make_scalar(fn(scalar_value<std::complex<double>>(arg)));
        };
        // exit
        return std::visit(l, value);
    }

    // absolute value builtin
    XyceValue builtin_abs(const std::vector<XyceValue>& args) {
        // one value argument
        expect_arity("abs", args, 1);
        // processor
        auto l = []<typename T0>(T0& arg) -> XyceValue {
            // actual parameter type
            using TX = std::decay_t<T0>;
            // vector<double>
            if constexpr (std::is_same_v<TX, std::vector<double>>) {
                // create and allocate output vector
                std::vector<double> out;
                out.reserve(arg.size());
                // append absolute values
                std::ranges::transform(arg.begin(), arg.end(), std::back_inserter(out), [](auto v) { return std::abs(v); });
                // exit
                return out;
            }
            // vector<complex>
            else if constexpr (std::is_same_v<TX, std::vector<std::complex<double>>>) {
                // create and allocate output vector
                std::vector<double> out;
                out.reserve(arg.size());
                // append absolute values
                std::ranges::transform(arg.begin(), arg.end(), std::back_inserter(out), [](auto v) { return std::abs(v); });
                // exit
                return out;
            }
            // scalar
            else
                return std::abs(arg);
        };
        // exit
        return std::visit(l, args[0]);
    }

    // square root builtin
    XyceValue builtin_sqrt(const std::vector<XyceValue>& args) {
        // one value argument
        expect_arity("sqrt", args, 1);
        // apply function, returns vector or scalar
        return map_unary_complex(args[0], [](const std::complex<double> value) { return std::sqrt(value); });
    }

    // base-10 logarithm builtin
    XyceValue builtin_log10(const std::vector<XyceValue>& args) {
        // one value argument
        expect_arity("log10", args, 1);
        // apply function, returns vector or scalar
        return map_unary_complex(args[0], [](const std::complex<double> value) { return std::log10(value); });
    }

    // Xyce log maps to base-10 log
    XyceValue builtin_log(const std::vector<XyceValue>& args) {
        return builtin_log10(args);
    }

    // natural logarithm builtin
    XyceValue builtin_ln(const std::vector<XyceValue>& args) {
        // one value argument
        expect_arity("ln", args, 1);
        // apply function, returns vector or scalar
        return map_unary_complex(args[0], [](const std::complex<double> value) { return std::log(value); });
    }

    // dB conversion builtin
    XyceValue builtin_db(const std::vector<XyceValue>& args) {
        // one value argument
        expect_arity("db", args, 1);
        // apply function, returns vector or scalar
        return map_unary_real(args[0], [](const double value) { return 20.0 * std::log10(std::abs(value)); });
    }

    // real part builtin
    XyceValue builtin_real(const std::vector<XyceValue>& args) {
        // one real argument
        expect_arity("real", args, 1);
        // processor
        auto l = []<typename T0>(T0& arg) -> XyceValue {
            // actual parameter type
            using TX = std::decay_t<T0>;
            // vector<complex>
            if constexpr (std::is_same_v<TX, std::vector<std::complex<double>>>) {
                // create and allocate output vector
                std::vector<double> out;
                out.reserve(arg.size());
                // append real values
                std::ranges::transform(arg.begin(), arg.end(), std::back_inserter(out), [](auto v) { return v.real(); });
                // exit
                return out;
            }
            // vector<double>
            if constexpr (std::is_same_v<TX, std::vector<double>>)
                return arg;
            // scalar
            return scalar_value<double>(arg);
        };
        // exit
        return std::visit(l, args[0]);
    }

    // imaginary part builtin
    XyceValue builtin_imag(const std::vector<XyceValue>& args) {
        // one value argument
        expect_arity("imag", args, 1);
        // processor
        auto l = []<typename T0>(T0& arg) -> XyceValue {
            // actual parameter type
            using TX = std::decay_t<T0>;
            // vector<complex>
            if constexpr (std::is_same_v<TX, std::vector<std::complex<double>>>) {
                // create and allocate output vector
                std::vector<double> out;
                out.reserve(arg.size());
                // append imag values
                std::ranges::transform(arg.begin(), arg.end(), std::back_inserter(out), [](auto v) { return v.imag(); });
                // exit
                return out;
            }
            // vector<double>
            if constexpr (std::is_same_v<TX, std::vector<double>>)
                return std::vector<double>(arg.size(), 0.0);
            // scalar
            if constexpr (std::is_same_v<TX, std::complex<double>>) {
                return arg.imag();
            }
            return 0.0;
        };
        // exit
        return std::visit(l, args[0]);
    }

    // phase angle builtin
    XyceValue builtin_angle(const std::vector<XyceValue>& args) {
        // one value argument
        expect_arity("angle", args, 1);
        // processor
        auto l = []<typename T0>(T0& arg) -> XyceValue {
            // actual parameter type
            using TX = std::decay_t<T0>;
            // vector<complex>
            if constexpr (std::is_same_v<TX, std::vector<std::complex<double>>>) {
                // create and allocate output vector
                std::vector<double> out;
                out.reserve(arg.size());
                // append angle values
                std::ranges::transform(arg.begin(), arg.end(), std::back_inserter(out), [](auto v) { return std::arg(v) * 180.0 / std::numbers::pi; });
                // exit
                return out;
            }
            // vector<double>
            else if constexpr (std::is_same_v<TX, std::vector<double>>) {
                // create and allocate output vector
                std::vector<double> out;
                out.reserve(arg.size());
                // append angle values
                std::ranges::transform(arg.begin(), arg.end(), std::back_inserter(out), [](auto v) { return std::arg(std::complex<double>(v, 0.0)) * 180.0 / std::numbers::pi; });
                // exit
                return out;
            }
            // complex
            else if constexpr (std::is_same_v<TX, std::complex<double>>) {
                return std::arg(arg) * 180.0 / std::numbers::pi;
            }
            // double
            else {
                return std::arg(std::complex<double>(arg, 0.0)) * 180.0 / std::numbers::pi;
            }
        };
        // exit
        return std::visit(l, args[0]);
    }

    // sine builtin
    XyceValue builtin_sin(const std::vector<XyceValue>& args) {
        // one value argument
        expect_arity("sin", args, 1);
        // apply function, returns vector or scalar
        return map_unary_complex(args[0], [](const std::complex<double> value) { return std::sin(value); });
    }

    // cosine builtin
    XyceValue builtin_cos(const std::vector<XyceValue>& args) {
        // one value argument
        expect_arity("cos", args, 1);
        // apply function, returns vector or scalar
        return map_unary_complex(args[0], [](const std::complex<double> value) { return std::cos(value); });
    }

    // tangent builtin
    XyceValue builtin_tan(const std::vector<XyceValue>& args) {
        // one value argument
        expect_arity("tan", args, 1);
        // apply function, returns vector or scalar
        return map_unary_real(args[0], [](const double value) { return std::tan(value); });
    }

    // arcsine builtin
    XyceValue builtin_asin(const std::vector<XyceValue>& args) {
        // one value argument
        expect_arity("asin", args, 1);
        // apply function, returns vector or scalar
        return std::asin(scalar_value<double>(args[0]));
    }

    // arccosine builtin
    XyceValue builtin_acos(const std::vector<XyceValue>& args) {
        // one value argument
        expect_arity("acos", args, 1);
        // apply function, returns vector or scalar
        return std::acos(scalar_value<double>(args[0]));
    }

    // arctangent builtin
    XyceValue builtin_atan(const std::vector<XyceValue>& args) {
        // one value argument
        expect_arity("atan", args, 1);
        // apply function, returns vector or scalar
        return std::atan(scalar_value<double>(args[0]));
    }

    // arctangent alias
    XyceValue builtin_arctan(const std::vector<XyceValue>& args) {
        return builtin_atan(args);
    }

    // atan2 builtin
    XyceValue builtin_atan2(const std::vector<XyceValue>& args) {
        // one value argument
        expect_arity("atan2", args, 2);
        // apply function, returns vector or scalar
        return std::atan2(scalar_value<double>(args[0]), scalar_value<double>(args[1]));
    }

    // hyperbolic sine builtin
    XyceValue builtin_sinh(const std::vector<XyceValue>& args) {
        // one value argument
        expect_arity("sinh", args, 1);
        // apply function, returns vector or scalar
        return std::sinh(scalar_value<double>(args[0]));
    }

    // hyperbolic cosine builtin
    XyceValue builtin_cosh(const std::vector<XyceValue>& args) {
        // one value argument
        expect_arity("cosh", args, 1);
        // apply function, returns vector or scalar
        return std::cosh(scalar_value<double>(args[0]));
    }

    // hyperbolic tangent builtin
    XyceValue builtin_tanh(const std::vector<XyceValue>& args) {
        // one value argument
        expect_arity("tanh", args, 1);
        // apply function, returns vector or scalar
        return std::tanh(scalar_value<double>(args[0]));
    }

    // inverse hyperbolic sine builtin
    XyceValue builtin_asinh(const std::vector<XyceValue>& args) {
        // one value argument
        expect_arity("asinh", args, 1);
        // apply function, returns vector or scalar
        return std::asinh(scalar_value<double>(args[0]));
    }

    // inverse hyperbolic cosine builtin
    XyceValue builtin_acosh(const std::vector<XyceValue>& args) {
        // one value argument
        expect_arity("acosh", args, 1);
        // apply function, returns vector or scalar
        return std::acosh(scalar_value<double>(args[0]));
    }

    // inverse hyperbolic tangent builtin
    XyceValue builtin_atanh(const std::vector<XyceValue>& args) {
        // one value argument
        expect_arity("atanh", args, 1);
        // apply function, returns vector or scalar
        return std::atanh(scalar_value<double>(args[0]));
    }

    // exponential builtin
    XyceValue builtin_exp(const std::vector<XyceValue>& args) {
        // one value argument
        expect_arity("exp", args, 1);
        // apply function, returns vector or scalar
        return std::exp(scalar_value<double>(args[0]));
    }

    // complex conjugate builtin
    XyceValue builtin_conj(const std::vector<XyceValue>& args) {
        // one value argument
        expect_arity("conj", args, 1);
        // processor
        auto l = []<typename T0>(T0& arg) -> XyceValue {
            // actual parameter type
            using TX = std::decay_t<T0>;
            // complex
            if constexpr (std::is_same_v<TX, std::complex<double>>) {
                return std::conj(arg);
            }
            return std::conj(std::complex<double>(scalar_value<double>(arg), 0.0));
        };
        // exit
        return std::visit(l, args[0]);
    }

    // square builtin
    XyceValue builtin_sqr(const std::vector<XyceValue>& args) {
        // one value argument
        expect_arity("sqr", args, 1);
        // apply function, returns vector or scalar
        const auto x = scalar_value<double>(args[0]);
        // multiply values
        return x * x;
    }

    // signum builtin
    XyceValue builtin_sgn(const std::vector<XyceValue>& args) {
        // one value argument
        expect_arity("sgn", args, 1);
        const auto x = scalar_value<double>(args[0]);
        return (x > 0.0) - (x < 0.0);
    }

    // sign-with-magnitude builtin
    XyceValue builtin_sign(const std::vector<XyceValue>& args) {
        // two values argument
        expect_arity("sign", args, 2);
        const auto x = std::abs(scalar_value<double>(args[0]));
        const auto y = scalar_value<double>(args[1]);
        return y < 0.0 ? -x : x;
    }

    // positive ramp builtin
    XyceValue builtin_uramp(const std::vector<XyceValue>& args) {
        // one value argument
        expect_arity("uramp", args, 1);
        // apply function, returns vector or scalar
        return std::max(scalar_value<double>(args[0]), 0.0);
    }

    // step builtin
    XyceValue builtin_stp(const std::vector<XyceValue>& args) {
        // one value argument
        expect_arity("stp", args, 1);
        // apply function, returns vector or scalar
        return scalar_value<double>(args[0]) > 0.0 ? 1.0 : 0.0;
    }

    // round builtin
    XyceValue builtin_round(const std::vector<XyceValue>& args) {
        // one value argument
        expect_arity("round", args, 1);
        // apply function, returns vector or scalar
        return std::round(scalar_value<double>(args[0]));
    }

    // nearest-integer builtin
    XyceValue builtin_nint(const std::vector<XyceValue>& args) {
        // one value argument
        expect_arity("nint", args, 1);
        // apply function, returns vector or scalar
        return std::round(scalar_value<double>(args[0]));
    }

    // floor builtin
    XyceValue builtin_floor(const std::vector<XyceValue>& args) {
        // one value argument
        expect_arity("floor", args, 1);
        // apply function, returns vector or scalar
        return std::floor(scalar_value<double>(args[0]));
    }

    // ceiling builtin
    XyceValue builtin_ceil(const std::vector<XyceValue>& args) {
        // one value argument
        expect_arity("ceil", args, 1);
        // apply function, returns vector or scalar
        return std::ceil(scalar_value<double>(args[0]));
    }

    // truncate builtin
    XyceValue builtin_int(const std::vector<XyceValue>& args) {
        // one value argument
        expect_arity("int", args, 1);
        // apply function, returns vector or scalar
        return std::trunc(scalar_value<double>(args[0]));
    }

    // power builtin
    XyceValue builtin_pow(const std::vector<XyceValue>& args) {
        expect_arity("pow", args, 2);
        return std::pow(scalar_value<double>(args[0]), scalar_value<double>(args[1]));
    }

    // absolute-power builtin
    XyceValue builtin_pwr(const std::vector<XyceValue>& args) {
        expect_arity("pwr", args, 2);
        return std::pow(std::abs(scalar_value<double>(args[0])), scalar_value<double>(args[1]));
    }

    // signed-power builtin
    XyceValue builtin_pwrs(const std::vector<XyceValue>& args) {
        expect_arity("pwrs", args, 2);
        const auto x = scalar_value<double>(args[0]);
        const auto y = scalar_value<double>(args[1]);
        return (x < 0.0 ? -1.0 : 1.0) * std::pow(std::abs(x), y);
    }

    // floating remainder builtin
    XyceValue builtin_fmod(const std::vector<XyceValue>& args) {
        expect_arity("fmod", args, 2);
        return std::fmod(scalar_value<double>(args[0]), scalar_value<double>(args[1]));
    }

    // minimum builtin
    XyceValue builtin_min(const std::vector<XyceValue>& args) {
        expect_min_arity("min", args, 1);
        auto result = scalar_value<double>(args[0]);
        for (size_t i = 1; i < args.size(); ++i) {
            result = std::min(result, scalar_value<double>(args[i]));
        }
        return result;
    }

    // maximum builtin
    XyceValue builtin_max(const std::vector<XyceValue>& args) {
        expect_min_arity("max", args, 1);
        auto result = scalar_value<double>(args[0]);
        for (size_t i = 1; i < args.size(); ++i) {
            result = std::max(result, scalar_value<double>(args[i]));
        }
        return result;
    }

    // clamp builtin
    XyceValue builtin_limit(const std::vector<XyceValue>& args) {
        expect_arity("limit", args, 3);
        return std::clamp(scalar_value<double>(args[0]), scalar_value<double>(args[1]), scalar_value<double>(args[2]));
    }

    // conditional builtin
    XyceValue builtin_if(const std::vector<XyceValue>& args) {
        expect_arity("if", args, 3);
        if (!is_vector(args[0]) && !is_vector(args[1]) && !is_vector(args[2])) {
            return scalar_value<double>(args[0]) != 0.0 ? args[1] : args[2];
        }

        const auto condition = to_real_vector(args[0]);
        const auto if_true = to_real_vector(args[1]);
        const auto if_false = to_real_vector(args[2]);

        std::vector<double> out;
        out.reserve(condition.size());
        for (size_t index = 0; index < condition.size(); ++index) {
            const auto t = if_true.size() == 1 ? if_true[0] : if_true.at(index);
            const auto f = if_false.size() == 1 ? if_false[0] : if_false.at(index);
            out.push_back(condition[index] != 0.0 ? t : f);
        }

        return out;
    }

    // derivative builtin placeholder
    XyceValue builtin_ddt(const std::vector<XyceValue>&) {
        throw std::logic_error("DDT() requires time-domain simulation context and cannot be evaluated in post-processing");
    }

    // integral builtin placeholder
    XyceValue builtin_sdt(const std::vector<XyceValue>&) {
        throw std::logic_error("SDT() requires time-domain simulation context and cannot be evaluated in post-processing");
    }
}

const std::unordered_map<std::string, double> NUMBER_SUFFIXES{
    {"T", 1e12},
    {"G", 1e9},
    {"MEG", 1e6},
    {"K", 1e3},
    {"M", 1e-3},
    {"U", 1e-6},
    {"N", 1e-9},
    {"P", 1e-12},
    {"F", 1e-15},
    {"MIL", 25.4e-6},
};

const std::unordered_map<std::string, XyceValue> BUILTIN_CONSTANTS{
    {"e", std::exp(1.0)},
    {"f", 1e-15},
    {"g", 1e9},
    {"j", std::complex<double>(0.0, 1.0)},
    {"k", 1e3},
    {"m", 1e-3},
    {"meg", 1e6},
    {"mho", 1.0},
    {"mil", 25.4e-6},
    {"n", 1e-9},
    {"p", 1e-12},
    {"pi", std::acos(-1.0)},
    {"s", 1.0},
    {"t", 1e12},
    {"u", 1e-6},
};

const std::unordered_map<std::string, BuiltinCallable> BUILTIN_FUNCTIONS{
    {"abs", builtin_abs},
    {"acos", builtin_acos},
    {"acosh", builtin_acosh},
    {"arctan", builtin_arctan},
    {"asin", builtin_asin},
    {"asinh", builtin_asinh},
    {"atan", builtin_atan},
    {"atanh", builtin_atanh},
    {"atan2", builtin_atan2},
    {"ceil", builtin_ceil},
    {"conj", builtin_conj},
    {"cos", builtin_cos},
    {"cosh", builtin_cosh},
    {"db", builtin_db},
    {"ddt", builtin_ddt},
    {"exp", builtin_exp},
    {"floor", builtin_floor},
    {"fmod", builtin_fmod},
    {"if", builtin_if},
    {"img", builtin_imag},
    {"imag", builtin_imag},
    {"int", builtin_int},
    {"limit", builtin_limit},
    {"ln", builtin_ln},
    {"log", builtin_log},
    {"log10", builtin_log10},
    {"m", builtin_abs},
    {"mag", builtin_abs},
    {"max", builtin_max},
    {"min", builtin_min},
    {"nint", builtin_nint},
    {"ph", builtin_angle},
    {"phase", builtin_angle},
    {"pow", builtin_pow},
    {"pwr", builtin_pwr},
    {"pwrs", builtin_pwrs},
    {"r", builtin_real},
    {"re", builtin_real},
    {"real", builtin_real},
    {"round", builtin_round},
    {"sdt", builtin_sdt},
    {"sgn", builtin_sgn},
    {"sign", builtin_sign},
    {"sin", builtin_sin},
    {"sinh", builtin_sinh},
    {"sqr", builtin_sqr},
    {"sqrt", builtin_sqrt},
    {"stp", builtin_stp},
    {"tan", builtin_tan},
    {"tanh", builtin_tanh},
    {"uramp", builtin_uramp},
};
