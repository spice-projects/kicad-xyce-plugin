#include <memory>
#include <wx/datetime.h>

#include "xyce_value.h"

XyceValue from_expression(AnyExpression& expression) {
    // processor
    auto l = []<typename T0>(T0& arg) -> XyceValue {
        // actual parameter type
        using TX = std::decay_t<T0>;
        // double
        if constexpr (std::is_same_v<TX, Expression<double>>) {
            // data span
            auto span = arg.data();
            // create View
            return std::make_shared<View<double>>(span);
        }
        // complex
        if constexpr (std::is_same_v<TX, Expression<std::complex<double>>>) {
            // data span
            auto span = arg.data();
            // create View
            return std::make_shared<View<std::complex<double>>>(span);
        }
        // not possible value type
        throw std::invalid_argument("unsupported type");
    };
    // exit
    return std::visit(l, expression);
}

bool is_scalar(const XyceValue& value) {
    // check it is a double or complex<double> scalar
    return std::holds_alternative<double>(value) || std::holds_alternative<std::complex<double>>(value);
}

bool is_vector(const XyceValue& value) {
    // check it is a vector
    return std::holds_alternative<std::shared_ptr<View<double>>>(value) || std::holds_alternative<std::shared_ptr<View<std::complex<double>>>>(value);
}

bool is_real(const XyceValue& value) {
    // check it is a real vector
    return std::holds_alternative<double>(value) || std::holds_alternative<std::shared_ptr<View<double>>>(value);
}

bool is_complex(const XyceValue& value) {
    // check it is a complex scalar or vector
    return std::holds_alternative<std::complex<double>>(value) || std::holds_alternative<std::shared_ptr<View<std::complex<double>>>>(value);
}

std::shared_ptr<View<double>> to_real_vector(const XyceValue& value) {
    // processor
    auto l = []<typename T0>(T0& arg) -> std::shared_ptr<View<double>> {
        // actual parameter type
        using TX = std::decay_t<T0>;
        // scalar double
        if constexpr (std::is_same_v<TX, double>) {
            // create vector with a single element
            std::vector<double> v = {{arg}};
            // create view (owning the vector)
            return std::make_shared<View<double>>(v);
        }
        // scalar complex
        else if constexpr (std::is_same_v<TX, std::complex<double>>) {
            // create vector with a single element
            std::vector<double> v = {{arg.real()}};
            // create view (owning the vector)
            return std::make_shared<View<double>>(v);
        }
        // View<double>
        else if constexpr (std::is_same_v<TX, std::shared_ptr<View<double>>>) {
            // use argument directly
            return arg;
        }
        // vector<complex>
        else if constexpr (std::is_same_v<TX, std::shared_ptr<View<std::complex<double>>>>) {
            // vector
            std::vector<double> out;
            // allocate space
            out.reserve(arg->size());
            // loop view, append real values
            for (const auto& v : *arg)
                out.emplace_back(v.real());
            // create vector (owning the data)
            return std::make_shared<View<double>>(out);
        }
        // not possible value type
        throw std::invalid_argument("unsupported type");
    };
    // exit
    return std::visit(l, value);
}

std::shared_ptr<View<std::complex<double>>> to_complex_vector(const XyceValue& value) {
    // processor
    auto l = []<typename T0>(T0& arg) -> std::shared_ptr<View<std::complex<double>>> {
        // actual parameter type
        using TX = std::decay_t<T0>;
        // scalar double
        if constexpr (std::is_same_v<TX, double>) {
            // create vector with a single element
            std::vector<std::complex<double>> v = {{arg, 0.0}};
            // create view (owning the vector)
            return std::make_shared<View<std::complex<double>>>(v);
        }
        // scalar complex
        else if constexpr (std::is_same_v<TX, std::complex<double>>) {
            // create vector with a single element
            std::vector<std::complex<double>> v = {{arg}};
            // create view (owning the vector)
            return std::make_shared<View<std::complex<double>>>(v);
        }
        // View<double>
        else if constexpr (std::is_same_v<TX, std::shared_ptr<View<double>>>) {
            // vector
            std::vector<std::complex<double>> out;
            // allocate space
            out.reserve(arg->size());
            // loop view, append real values
            for (const auto& v : *arg)
                out.emplace_back(v, 0.0);
            // create vector (owning the data)
            return std::make_shared<View<std::complex<double>>>(out);
        }
        // vector<complex>
        else if constexpr (std::is_same_v<TX, std::shared_ptr<View<std::complex<double>>>>) {
            // use argument directly
            return arg;
        }
        // not possible value type
        throw std::invalid_argument("unsupported type");
    };
    // exit
    return std::visit(l, value);
}
