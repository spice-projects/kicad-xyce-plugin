#include "xyce_output_file.h"
#include "../step_information.h"
#include "../expression/expression.h"

XyceOutputFile::XyceOutputFile(XyceOutputFile&& other) noexcept
    : m_filename(std::move(other.m_filename)), m_title(std::move(other.m_title)), m_is_complex(other.m_is_complex), m_step_information(std::move(other.m_step_information)), m_abscissa_scale(other.m_abscissa_scale), m_expression_manager(std::move(other.m_expression_manager)), m_mapped_file(std::move(other.m_mapped_file)) {
}

XyceOutputFile::XyceOutputFile(std::filesystem::path filename, std::string title, const bool is_complex, StepInformation&& step_info, const AbscissaScale abscissa_scale, ExpressionManager&& expression_manager, std::unique_ptr<MappedFile>&& mapped_file)
    : m_filename(std::move(filename)), m_title(std::move(title)), m_is_complex(is_complex), m_step_information(std::move(step_info)), m_abscissa_scale(abscissa_scale), m_expression_manager(std::move(expression_manager)), m_mapped_file(std::move(mapped_file)) {
}

XyceOutputFile::~XyceOutputFile() = default;

XyceOutputFile& XyceOutputFile::operator=(XyceOutputFile&& other) noexcept {
    // move all field values to this instance
    m_filename = std::move(other.m_filename);
    m_title = std::move(other.m_title);
    m_is_complex = other.m_is_complex;
    m_step_information = std::move(other.m_step_information);
    m_abscissa_scale = other.m_abscissa_scale;
    m_expression_manager = std::move(other.m_expression_manager);
    m_mapped_file = std::move(other.m_mapped_file);
    // exit
    return *this;
}

const std::filesystem::path& XyceOutputFile::filename() const {
    // return filename
    return m_filename;
}

const std::string& XyceOutputFile::title() const {
    // return title
    return m_title;
}

bool XyceOutputFile::is_complex() const {
    // return complex status
    return m_is_complex;
}

const StepInformation& XyceOutputFile::step_information() const {
    // return step information
    return m_step_information;
}

Expression<double>& XyceOutputFile::abscissa() {
    return m_expression_manager.abscissa();
}

AbscissaScale XyceOutputFile::abscissa_scale() const {
    // return scale
    return m_abscissa_scale;
}

ExpressionManager& XyceOutputFile::expression_manager() {
    // return manager
    return m_expression_manager;
}
