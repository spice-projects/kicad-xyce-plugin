#ifndef KICAD_XYCE_PLUGIN_ARRAYVIEW_H
#define KICAD_XYCE_PLUGIN_ARRAYVIEW_H

#include <vector>
#include <variant>


template <typename T>
class ArrayView {
public:
    // default constructor
    ArrayView()
        : m_type(Type::EMPTY), m_ptr(nullptr)
    {
    }

    // owning constructor
    explicit ArrayView(std::vector<T>&& data)
        : m_type(Type::OWNING), m_owning(std::move(data))
    {
        // assign pointer
        m_ptr = m_owning.data();
        // assign size
        m_size = m_owning.size();
        // assign stride
        m_stride = 1;
    }

    // copy constructor
    ArrayView(const ArrayView& other)
        : m_type(other.m_type), m_owning(other.m_owning), m_ptr(other.m_ptr), m_size(other.m_size), m_stride(other.m_stride) {
        // update pointer if owning
        if (m_type == Type::OWNING) {
            // point to own vector
            m_ptr = m_owning.data();
        }
    }

    // move constructor
    ArrayView(ArrayView&& other) noexcept
        : m_type(other.m_type), m_owning(std::move(other.m_owning)), m_ptr(other.m_ptr), m_size(other.m_size), m_stride(other.m_stride) {
        // update pointer if owning
        if (m_type == Type::OWNING) {
            // point to own vector
            m_ptr = m_owning.data();
        }
        // reset other
        other.m_type = Type::EMPTY;
        // reset pointer
        other.m_ptr = nullptr;
        // reset size
        other.m_size = 0;
        // reset stride
        other.m_stride = 1;
    }

    // copy assignment
    ArrayView& operator=(const ArrayView& other) {
        // check self-assignment
        if (this != &other) {
            // assign fields
            m_type = other.m_type;
            // copy vector
            m_owning = other.m_owning;
            // assign pointer
            m_ptr = other.m_ptr;
            // assign size
            m_size = other.m_size;
            // assign stride
            m_stride = other.m_stride;
            if (m_type == Type::OWNING) {
                // point to own vector
                m_ptr = m_owning.data();
            }
        }
        return *this;
    }

    // move assignment
    ArrayView& operator=(ArrayView&& other) noexcept {
        // check self-assignment
        if (this != &other) {
            // assign fields
            m_type = other.m_type;
            // move vector
            m_owning = std::move(other.m_owning);
            // assign pointer
            m_ptr = other.m_ptr;
            // assign size
            m_size = other.m_size;
            // assign stride
            m_stride = other.m_stride;
            if (m_type == Type::OWNING) {
                // point to own vector
                m_ptr = m_owning.data();
            }
            // reset other
            other.m_type = Type::EMPTY;
            // reset pointer
            other.m_ptr = nullptr;
            // reset size
            other.m_size = 0;
            // reset stride
            other.m_stride = 1;
        }
        return *this;
    }

    // non-owning constructor
    ArrayView(const T* ptr, size_t size, size_t stride)
        : m_type(Type::VIEW), m_ptr(ptr), m_size(size), m_stride(stride) {}

    // element access
    T operator[](size_t index) const {
        // access element at stride
        return m_ptr[index * m_stride];
    }

    // bounds-checked access
    T at(const size_t index) const {
        if (index >= m_size) {
            // out of range
            throw std::out_of_range("ArrayView index out of range");
        }
        // access element at stride
        return m_ptr[index * m_stride];
    }

    // size getter
    [[nodiscard]] size_t size() const {
        // return size
        return m_size;
    }

    // stride getter
    [[nodiscard]] size_t stride() const {
        // return stride
        return m_stride;
    }

    // empty checker
    [[nodiscard]] bool empty() const {
        // check size
        return m_size == 0;
    }

    // convert to vector
    std::vector<T> to_vector() const {
        // create result
        std::vector<T> res;
        // reserve memory
        res.reserve(m_size);
        for (size_t i = 0; i < m_size; ++i) {
            // append element
            res.push_back((*this)[i]);
        }
        // return result
        return res;
    }

private:
    // enum representing data ownership
    enum class Type { EMPTY, OWNING, VIEW };
    // type field
    Type m_type;
    // owning vector
    std::vector<T> m_owning;
    // data pointer
    const T* m_ptr = nullptr;
    // size field
    size_t m_size = 0;
    // stride field
    size_t m_stride = 1;
};


using RealStepData = std::vector<ArrayView<double>>;
using ComplexStepData = std::vector<ArrayView<std::complex<double>>>;
using StepDataVariant = std::variant<RealStepData, ComplexStepData>;

#endif //KICAD_XYCE_PLUGIN_ARRAYVIEW_H