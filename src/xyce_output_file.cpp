#include <algorithm>
#include <chrono>
#include <regex>
#include <unistd.h>
#include <sys/mman.h>

#include "xyce_output_file.h"
#include "expression.h"
#include "step_information.h"

XyceOutputFile::XyceOutputFile(std::filesystem::path filename, std::string title, const bool is_complex, StepInformation&& step_info, const AbscissaScale abscissa_scale, ExpressionManager&& expression_manager, const void* mmap_ptr, const size_t mmap_length)
    : m_filename(std::move(filename)), m_title(std::move(title)), m_is_complex(is_complex), m_step_information(std::move(step_info)), m_abscissa_scale(abscissa_scale), m_expression_manager(std::move(expression_manager)), m_mmap_ptr(mmap_ptr), m_mmap_len(mmap_length) {
}

XyceOutputFile::~XyceOutputFile() {
    // check mmap pointer
    if (m_mmap_ptr && m_mmap_ptr != MAP_FAILED) {
        // unmap mmap memory
        munmap(const_cast<void*>(m_mmap_ptr), m_mmap_len);
    }
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
