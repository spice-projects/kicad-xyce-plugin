#pragma once

#include <filesystem>
#include <memory>
#include <string>

#include "../expression/expression.h"
#include "../expression/expression_manager.h"
#include "../step_information.h"
#include "mapped_file.h"

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

    XyceOutputFile(XyceOutputFile&&) noexcept;

    XyceOutputFile(std::filesystem::path filename, std::string title, bool is_complex, StepInformation&& step_info, AbscissaScale abscissa_scale, ExpressionManager&& expression_manager, std::unique_ptr<MappedFile>&& mapped_file);

    ~XyceOutputFile();

    XyceOutputFile& operator=(const XyceOutputFile&) = delete;

    XyceOutputFile& operator=(XyceOutputFile&&) noexcept;

    [[nodiscard]] const std::filesystem::path& filename() const;

    [[nodiscard]] const std::string& title() const;

    [[nodiscard]] bool is_complex() const;

    [[nodiscard]] const StepInformation& step_information() const;

    [[nodiscard]] Expression<double>& abscissa();

    [[nodiscard]] AbscissaScale abscissa_scale() const;

    [[nodiscard]] ExpressionManager& expression_manager();

private:
    std::filesystem::path m_filename;
    std::string m_title;
    bool m_is_complex;
    StepInformation m_step_information;
    AbscissaScale m_abscissa_scale;
    ExpressionManager m_expression_manager;

    std::unique_ptr<MappedFile> m_mapped_file;
};
