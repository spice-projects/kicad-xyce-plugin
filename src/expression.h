#ifndef EXPRESSION_H
#define EXPRESSION_H

#include <complex>
#include <functional>
#include <ranges>
#include <span>
#include <string>
#include <variant>
#include <vector>

template <typename T>
class View
{
public:
    View() = delete;

    View(const View&) = delete;

    View(View&&) noexcept = default;

    View(T* pointer, const size_t size, const size_t stride) :
        m_pointer(pointer), m_size(size), m_stride(stride) {
    }

    explicit View(std::vector<T>& vector) :
        m_pointer(vector.data()), m_size(vector.size()), m_stride(1), data(std::move(vector)) {
    }

    ~View() = default;

    View& operator=(const View&) = delete;

    View& operator=(View&&) noexcept = default;

    const T& operator [](const size_t index) const {
        // value at index
        return m_pointer[index * m_stride];
    }

    [[nodiscard]] size_t size() const {
        return m_size;
    }

private:
    T* m_pointer;
    size_t m_size;
    size_t m_stride;

    std::vector<T> data;
};

using Steps = std::variant<std::vector<View<double>>, std::vector<View<std::complex<double>>>, std::vector<std::span<const double>>, std::vector<std::span<const std::complex<double>>>>;

class Expression
{
public:
    Expression() = delete;

    Expression(const Expression&) = delete;

    Expression(Expression&&) noexcept = default;

    Expression(std::string name, std::vector<View<double>>& steps, std::string unit, std::string source = "", std::string variable_type = "");

    Expression(std::string name, std::vector<View<std::complex<double>>>& steps, std::string unit, std::string source = "", std::string variable_type = "");

    Expression(std::string name, std::vector<double>& data, std::vector<std::span<const double>>& steps, std::string unit, std::string source = "", std::string variable_type = "");

    Expression(std::string name, std::vector<std::complex<double>>& data, std::vector<std::span<const std::complex<double>>>& steps, std::string unit, std::string source = "", std::string variable_type = "");

    ~Expression() = default;

    Expression& operator=(const Expression&) = delete;

    Expression& operator=(Expression&&) noexcept = default;

    const std::string& name() const;

    const std::string& unit() const;

    const std::string& source() const;

    const std::string& variable_type() const;

    bool is_complex() const;

    size_t step_count() const;

    std::variant<std::span<const double>, std::span<const std::complex<double>>> step_data(size_t step_index);

    std::variant<std::span<const double>, std::span<const std::complex<double>>> data();

    std::vector<std::pair<size_t, size_t>> step_indices() const;

    template <typename I, typename O> requires (std::same_as<I, double> || std::same_as<I, std::complex<double>>) && (std::same_as<O, double> || std::same_as<O, std::complex<double>>)
    Expression transform(std::string& name, std::string& unit, std::string& variable_type, const std::function<O(I)>& f) {
        // check data has been initialized for this expression
        if (std::holds_alternative<std::monostate>(m_cached_data)) {
            // initialize expression
            initialize_expression_data();
        }
        // create out data vector
        std::vector<O> out_data;
        // process expression data
        auto l = [&out_data, &f]<typename T0>(T0& d) -> void {
            // actual parameter type
            using T = std::decay_t<T0>;
            // process input data
            if constexpr (std::is_same_v<T, std::vector<I>>) {
                // reserve storage
                out_data.reserve(d.size());
                // transform data // [&f](const I& v) -> O { return f(v); }
                std::ranges::transform(d.begin(), d.end(), std::back_inserter(out_data), f);
                // exit
                return;
            }
            throw std::runtime_error("Unexpected state detected");
        };
        // data
        const auto& in_data = std::get<std::variant<std::vector<double>, std::vector<std::complex<double>>>>(m_cached_data);
        // process data
        std::visit(l, in_data);
        // data pointer
        const auto pointer = out_data.data();
        // recreate the step spans
        std::vector<std::span<const double>> views;
        // step indices
        auto indices = step_indices();
        // allocate vector
        views.reserve(indices.size());
        // loop indices
        for (const auto& [begin, end] : indices)
            views.emplace_back(pointer + begin, end - begin);
        // recreate expression
        return {name, out_data, views, unit, variable_type};
    }

private:
    std::string m_name;
    Steps m_steps;
    std::string m_unit;
    std::string m_source;
    std::string m_variable_type;
    bool m_is_complex{};
    std::variant<std::monostate, std::variant<std::vector<double>, std::vector<std::complex<double>>>> m_cached_data;

    void initialize_expression_data();
};

#endif
