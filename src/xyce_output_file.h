#ifndef XYCE_OUTPUT_FILE_H
#define XYCE_OUTPUT_FILE_H

#include <filesystem>
#include <string>

#include "expression.h"
#include "expression_manager.h"
#include "step_information.h"

enum class AbscissaScale
{
    LINEAR,
    DECADE,
    OCTAVE
};

class XyceOutputFile
{
public:
    XyceOutputFile() = delete;

    XyceOutputFile(const XyceOutputFile&) = delete;

    XyceOutputFile(XyceOutputFile&&) noexcept = default;

    XyceOutputFile(std::filesystem::path filename, std::string title, bool is_complex, StepInformation&& step_info, Expression&& abscissa, AbscissaScale abscissa_scale, ExpressionManager&& expression_manager, const void* mmap_ptr, size_t mmap_length);

    ~XyceOutputFile();

    XyceOutputFile& operator=(const XyceOutputFile&) = delete;

    XyceOutputFile& operator=(XyceOutputFile&&) noexcept = default;

    [[nodiscard]] const std::filesystem::path& filename() const;

    [[nodiscard]] const std::string& title() const;

    [[nodiscard]] bool is_complex() const;

    [[nodiscard]] const StepInformation& step_information() const;

    Expression& abscissa();

    [[nodiscard]] AbscissaScale abscissa_scale() const;

    [[nodiscard]] std::string chart_type() const;

    [[nodiscard]] ExpressionManager& expression_manager();

private:
    std::filesystem::path m_filename;
    std::string m_title;
    bool m_is_complex;
    StepInformation m_step_information;
    Expression m_abscissa;
    AbscissaScale m_abscissa_scale;
    ExpressionManager m_expression_manager;

    const void* m_mmap_ptr = nullptr;
    size_t m_mmap_len = 0;
};

#endif
