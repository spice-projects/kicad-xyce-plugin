#pragma once

#include <algorithm>
#include <complex>
#include <functional>
#include <ranges>
#include <span>
#include <string>
#include <utility>
#include <variant>
#include <vector>

template <typename T>
class View
{
public:
    class Iterator {
    public:
        using iterator_concept  = std::forward_iterator_tag;
        using iterator_category = std::forward_iterator_tag;
        using value_type        = T;
        using difference_type   = std::ptrdiff_t;
        using pointer           = const T*;
        using reference         = const T&;

        Iterator()
            : m_ptr(nullptr), m_stride(1) {
        }

        Iterator(const T* ptr, size_t stride)
            : m_ptr(ptr), m_stride(stride) {
        }

        reference operator*() const { 
            return *m_ptr; 
        }
        
        pointer operator->() const { 
            return m_ptr; 
        }

        Iterator& operator++() { 
            m_ptr += m_stride; 
            return *this; 
        }
        
        Iterator operator++(int) { 
            Iterator tmp = *this;
            m_ptr += m_stride; 
            return tmp;
        }

        friend bool operator==(const Iterator& lhs, const Iterator& rhs) { 
            return lhs.m_ptr == rhs.m_ptr; 
        }
        
        friend bool operator!=(const Iterator& lhs, const Iterator& rhs) { 
            return lhs.m_ptr != rhs.m_ptr; 
        }

    private:
        const T* m_ptr;
        size_t m_stride;
    };

    View() = default;

    View(const View&) = delete;

    View(View&&) noexcept = default;

    View(const T* pointer, const size_t size) 
        : m_pointer(const_cast<T*>(pointer)), m_size(size), m_stride(1) {
    }

    View(const T* pointer, const size_t size, const size_t stride) 
        : m_pointer(const_cast<T*>(pointer)), m_size(size), m_stride(stride) {
    }

    explicit View(std::vector<T>& vector)
        : m_data(std::move(vector)), m_stride(1) {
        // pointer to vector data
        m_pointer = m_data.data();
        m_size = m_data.size();
    }

    explicit View(std::span<const T> span) 
        : m_pointer(const_cast<T*>(span.data())), m_size(span.size()), m_stride(1) {
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

    [[nodiscard]] const T* data() const {
        return m_pointer;
    }

    [[nodiscard]] bool empty() const {
        return m_size == 0;
    }

    Iterator begin() const { 
        return Iterator(m_pointer, m_stride); 
    }
    
    Iterator end() const { 
        return Iterator(m_pointer + (m_size * m_stride), m_stride); 
    }

private:
    T* m_pointer;
    size_t m_size;
    size_t m_stride;

    std::vector<T> m_data;
};

template <typename T>
class Expression
{
public:
    Expression() = delete;

    Expression(const Expression&) = delete;

    Expression(Expression&&) noexcept = default;

    Expression(std::string name, std::vector<View<T>>& steps, std::string unit, std::string source = "", std::string variable_type = "")
        : m_name(std::move(name)), m_steps(std::move(steps)), m_unit(std::move(unit)), m_source(std::move(source)), m_variable_type(std::move(variable_type)) {
    }

    Expression(std::string name, std::vector<T>& data, std::vector<std::span<const T>>& steps, std::string unit, std::string source = "", std::string variable_type = "")
        : m_name(std::move(name)), m_steps(std::move(steps)), m_unit(std::move(unit)), m_source(std::move(source)), m_variable_type(std::move(variable_type)), m_cached_data(std::move(data)) {
    }

    ~Expression() = default;

    Expression& operator=(const Expression&) = delete;

    Expression& operator=(Expression&&) noexcept = default;

    [[nodiscard]] const std::string& name() const {
        return m_name;
    }

    [[nodiscard]] const std::string& unit() const {
        return m_unit;
    }

    [[nodiscard]] const std::string& source() const {
        return m_source;
    }

    [[nodiscard]] const std::string& variable_type() const {
        return m_variable_type;
    }

    [[nodiscard]] size_t step_count() const {
        return std::visit([](auto&& v) { return v.size(); }, m_steps);
    }

    const std::span<const T>& step_data(size_t step_index) {
        // check data has been initialized for this expression
        if (std::holds_alternative<std::monostate>(m_cached_data)) {
            // initialize expression
            initialize_expression_data();
        }
        // at this point only span is available
        const auto& steps = std::get<std::vector<std::span<const T>>>(m_steps);
        // return step at index
        return steps.at(step_index);
    }

    std::span<const T> data() {
        // check data has been initialized for this expression
        if (std::holds_alternative<std::monostate>(m_cached_data)) {
            // initialize expression
            initialize_expression_data();
        }
        // return data (create a span over the whole vector)
        return std::span<const T>(std::get<std::vector<T>>(m_cached_data));
    }

    [[nodiscard]] std::vector<std::pair<size_t, size_t>> step_indices() const {
        // result
        std::vector<std::pair<size_t, size_t>> result;
        // lambda
        auto l = [&result](auto& steps) -> void {
            // reserve capacity
            result.reserve(steps.size());
            // offset
            size_t offset = 0;
            // loop steps
            for (auto& step : steps) {
                // end index for step
                size_t end = offset + step.size();
                // append slice
                result.emplace_back(offset, end);
                // update offset
                offset = end;
            }
        };
        // compute step slices
        std::visit(l, m_steps);
        // return slices
        return result;
    }

    void transform(const std::function<T(T)>& f) {
        // check data has been initialized for this expression
        if (std::holds_alternative<std::monostate>(m_cached_data)) {
            // initialize expression
            initialize_expression_data();
        }
        // at this moment vector contains data
        auto& data = std::get<std::vector<T>>(m_cached_data);
        // transform data in-place
        std::ranges::transform(data, data.begin(), f);
    }

private:
    std::string m_name;
    std::variant<std::vector<View<T>>, std::vector<std::span<const T>>> m_steps;
    std::string m_unit;
    std::string m_source;
    std::string m_variable_type;
    std::variant<std::monostate, std::vector<T>> m_cached_data;

    void initialize_expression_data() {
        // check steps
        if (std::holds_alternative<std::vector<View<T>>>(m_steps)) {
            // steps
            const auto& steps = std::get<std::vector<View<T>>>(m_steps);
            // total vector size
            size_t total_size = 0;
            // loop steps
            for (const auto& step : steps) {
                // accumulate size
                total_size += step.size();
            }
            // create data & step vectors
            std::vector<T> concatenated;
            std::vector<std::span<const T>> spans;
            // allocate vectors
            concatenated.reserve(total_size);
            // loop steps
            for (const View<T>& step : steps) {
                // loop step data, append it to buffer
                for (size_t j = 0; j < step.size(); ++j)
                    concatenated.emplace_back(step[j]);
            }
            // data pointer and offset
            const T* pointer = concatenated.data();
            size_t offset = 0;
            // allocate spans
            spans.reserve(steps.size());
            // loop steps
            for (const auto& step : steps) {
                // step size
                const auto length = step.size();
                // update view
                spans.emplace_back(pointer + offset, length);
                // move offset
                offset += length;
            }
            // update cache field (at the end of the processing since this moves the data into the field)
            m_cached_data = std::move(concatenated);
            // update steps with views on the contiguous memory data
            m_steps = std::move(spans);
        }
    }
};

using AnyExpression = std::variant<Expression<double>, Expression<std::complex<double>>>;
