#include <algorithm>
#include <regex>
#include <chrono>
#include <cmath>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "array_view.h"
#include "expression.h"
#include "step_information.h"
#include "xyce_raw_file.h"

// constructor
XyceOutputFile::XyceOutputFile(std::filesystem::path filename, std::string title, bool is_complex, StepInformation step_info, const Expression& abscissa, AbscissaScale abscissa_scale, ExpressionManager expr_mgr, const void* mmap_ptr, size_t mmap_len)
    : m_filename(std::move(filename)), m_title(std::move(title)), m_complex(is_complex), m_step_information(std::move(step_info)), m_abscissa(std::move(abscissa)), m_abscissa_scale(abscissa_scale), m_expression_manager(std::move(expr_mgr)), m_mmap_ptr(mmap_ptr), m_mmap_len(mmap_len)
{
}


// destructor
XyceOutputFile::~XyceOutputFile() {
    // check mmap pointer
    if (m_mmap_ptr && m_mmap_ptr != MAP_FAILED) {
        // unmap mmap memory
        munmap(const_cast<void*>(m_mmap_ptr), m_mmap_len);
    }
}


// move constructor
XyceOutputFile::XyceOutputFile(XyceOutputFile&& other) noexcept
    : m_filename(std::move(other.m_filename)), m_title(std::move(other.m_title)), m_complex(other.m_complex), m_step_information(std::move(other.m_step_information)), m_abscissa(std::move(other.m_abscissa)), m_abscissa_scale(other.m_abscissa_scale), m_expression_manager(std::move(other.m_expression_manager)), m_mmap_ptr(other.m_mmap_ptr), m_mmap_len(other.m_mmap_len)
{
    // invalidate other ptr
    other.m_mmap_ptr = nullptr;
    // reset other length
    other.m_mmap_len = 0;
}


// move assignment
XyceOutputFile& XyceOutputFile::operator=(XyceOutputFile&& other) noexcept {
    // check self-assignment
    if (this != &other) {
        // check existing mmap pointer
        if (m_mmap_ptr && m_mmap_ptr != MAP_FAILED) {
            // unmap existing memory
            munmap(const_cast<void*>(m_mmap_ptr), m_mmap_len);
        }
        // move fields
        m_filename = std::move(other.m_filename);
        // move title
        m_title = std::move(other.m_title);
        // assign complex flag
        m_complex = other.m_complex;
        // move step information
        m_step_information = std::move(other.m_step_information);
        // move abscissa
        m_abscissa = std::move(other.m_abscissa);
        // assign scale
        m_abscissa_scale = other.m_abscissa_scale;
        // move expression manager
        m_expression_manager = std::move(other.m_expression_manager);
        // copy pointer
        m_mmap_ptr = other.m_mmap_ptr;
        // copy length
        m_mmap_len = other.m_mmap_len;
        // invalidate other ptr
        other.m_mmap_ptr = nullptr;
        // reset other length
        other.m_mmap_len = 0;
    }
    return *this;
}


// filename getter
const std::filesystem::path& XyceOutputFile::filename() const {
    // return filename
    return m_filename;
}


// title getter
const std::string& XyceOutputFile::title() const {
    // return title
    return m_title;
}


// complex status getter
bool XyceOutputFile::is_complex() const {
    // return complex status
    return m_complex;
}


// step information getter
const StepInformation& XyceOutputFile::step_information() const {
    // return step information
    return m_step_information;
}


// steps count getter
size_t XyceOutputFile::steps() const {
    // return length
    return m_step_information.length();
}


// abscissa getter
const Expression& XyceOutputFile::abscissa() const {
    // return abscissa
    return m_abscissa;
}


// abscissa scale getter
AbscissaScale XyceOutputFile::abscissa_scale() const {
    // return scale
    return m_abscissa_scale;
}


// chart type getter
std::string XyceOutputFile::chart_type() const {
    if (m_abscissa.unit() == "Hz") {
        // return ac type
        return "AC";
    }
    if (m_abscissa.unit() == "s") {
        // return transient type
        return "TRANSIENT";
    }
    // fallback type
    return "DC";
}


// expression manager getter
const ExpressionManager& XyceOutputFile::expression_manager() const {
    // return manager
    return m_expression_manager;
}


namespace {

// block header scan result struct
struct BlockHeaderScanResult {
    // header map
    std::unordered_map<std::string, std::string> header;
    // variable definitions list
    std::vector<std::tuple<int, std::string, std::optional<VariableType>>> variable_definitions;
    // data offset field
    size_t data_offset = 0;
    // ascii status flag
    bool is_ascii = false;
};


// trim whitespace helper
void trim(std::string& s) {
    // erase leading whitespace
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    // erase trailing whitespace
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
}


// scan block header helper
std::optional<BlockHeaderScanResult> scan_block_header(const char* data, size_t length, size_t start_pos) {
    // initialize result
    BlockHeaderScanResult result;
    // initialize variables status
    bool in_variables = false;
    // initialize position
    size_t pos = start_pos;
    // initialize marker status
    bool found_marker = false;
    while (pos < length) {
        // find next newline
        size_t newline = pos;
        while (newline < length && data[newline] != '\n') {
            // increment newline
            newline++;
        }
        if (newline >= length && pos == newline) {
            // break loop
            break;
        }
        // extract line
        std::string line(data + pos, newline - pos);
        // trim whitespace
        trim(line);
        // advance position
        pos = newline + 1;
        if (in_variables) {
            // check for end of variables
            if (line == "Binary:" || line == "Values:") {
                // record offset
                result.data_offset = pos;
                // record ascii status
                result.is_ascii = (line == "Values:");
                // update status
                found_marker = true;
                // break loop
                break;
            }
            // split by tab characters
            std::vector<std::string> parts;
            // init start
            size_t start = 0;
            // find first tab
            size_t tab = line.find('\t');
            while (tab != std::string::npos) {
                // append part
                parts.push_back(line.substr(start, tab - start));
                // update start
                start = tab + 1;
                // find next tab
                tab = line.find('\t', start);
            }
            // append last part
            parts.push_back(line.substr(start));
            if (parts.size() == 3) {
                // parse index
                int index = std::stoi(parts[0]);
                // parse name
                std::string name = parts[1];
                // parse variable type
                std::optional<VariableType> variable_type = parse_variable_type(parts[2]);
                // insert variable definition
                result.variable_definitions.emplace_back(index, name, variable_type);
            }
            // continue loop
            continue;
        }
        if (line == "Variables:") {
            // update status
            in_variables = true;
            // continue loop
            continue;
        }
        // find colon
        size_t colon = line.find(':');
        if (colon != std::string::npos) {
            // extract key
            std::string key = line.substr(0, colon);
            // extract value
            std::string value = line.substr(colon + 1);
            // trim key
            trim(key);
            // trim value
            trim(value);
            // insert to map
            result.header[key] = value;
        }
    }
    if (!found_marker) {
        // empty result
        return std::nullopt;
    }
    // return result
    return result;
}


// parse ascii variables helper
struct ParseAsciiResult {
    // actual points count
    size_t actual_points = 0;
    // variables list
    std::vector<std::variant<ArrayView<double>, ArrayView<std::complex<double>>>> variables;
};


// get tokens helper
std::vector<std::string> get_tokens(const std::string& line) {
    // initialize tokens
    std::vector<std::string> tokens;
    // initialize start
    size_t start = 0;
    while (start < line.size()) {
        while (start < line.size() && std::isspace(static_cast<unsigned char>(line[start]))) {
            // advance start
            start++;
        }
        if (start >= line.size()) {
            // break loop
            break;
        }
        // initialize end
        size_t end = start;
        while (end < line.size() && !std::isspace(static_cast<unsigned char>(line[end]))) {
            // advance end
            end++;
        }
        // add token
        tokens.push_back(line.substr(start, end - start));
        // update start
        start = end;
    }
    // return tokens
    return tokens;
}


// parse ascii variables helper
std::optional<ParseAsciiResult> parse_ascii_variables(const char* data, size_t length, size_t offset, const std::vector<std::tuple<int, std::string, std::optional<VariableType>>>& variable_definitions, bool is_complex, size_t num_variables, size_t num_points)
{
    // initialize result
    ParseAsciiResult result;
    // compute floats per point
    size_t floats_per_point = is_complex ? num_variables * 2 : num_variables;
    // compute tokens per line
    size_t tokens_per_line = 1 + floats_per_point;
    // initialize floats buffer
    std::vector<double> all_floats;
    // initialize expected index
    size_t expected_index = 0;
    // initialize position
    size_t pos = offset;
    while (pos < length) {
        // find next newline
        size_t newline = pos;
        while (newline < length && data[newline] != '\n') {
            // increment newline
            newline++;
        }
        if (newline >= length && pos == newline) {
            // break loop
            break;
        }
        // extract line
        std::string line(data + pos, newline - pos);
        // advance position
        pos = newline + 1;
        // trim line
        trim(line);
        if (line.empty()) {
            // skip empty
            continue;
        }
        // get tokens
        std::vector<std::string> tokens = get_tokens(line);
        if (tokens.size() != tokens_per_line) {
            // error return
            return std::nullopt;
        }
        try {
            // parse index
            size_t index = std::stoull(tokens[0]);
            if (index != expected_index) {
                // error return
                return std::nullopt;
            }
            for (size_t i = 1; i < tokens.size(); ++i) {
                // append parsed double
                all_floats.push_back(std::stod(tokens[i]));
            }
            // increment expected index
            expected_index++;
        } 
        catch (...) {
            // error return
            return std::nullopt;
        }
    }
    // compute actual points
    size_t actual_points = all_floats.size() / floats_per_point;
    if (num_points > 0 && actual_points != num_points) {
        // error return
        return std::nullopt;
    }
    // assign actual points
    result.actual_points = actual_points;
    // reserve variables size
    result.variables.reserve(variable_definitions.size());
    for (const auto& var_def : variable_definitions) {
        // get variable index
        int idx = std::get<0>(var_def);
        if (is_complex) {
            if (idx == 0) {
                // create abscissa data
                std::vector<double> var_data;
                // reserve memory
                var_data.reserve(actual_points);
                for (size_t r = 0; r < actual_points; ++r) {
                    // append real part
                    var_data.push_back(all_floats[r * floats_per_point + idx * 2]);
                }
                // add owning view
                result.variables.emplace_back(ArrayView(std::move(var_data)));
            } 
            else {
                // create variable data
                std::vector<std::complex<double>> var_data;
                // reserve memory
                var_data.reserve(actual_points);
                for (size_t r = 0; r < actual_points; ++r) {
                    // get real part
                    double real_part = all_floats[r * floats_per_point + idx * 2];
                    // get imag part
                    double imag_part = all_floats[r * floats_per_point + idx * 2 + 1];
                    // append complex value
                    var_data.emplace_back(real_part, imag_part);
                }
                // add owning view
                result.variables.emplace_back(ArrayView<std::complex<double>>(std::move(var_data)));
            }
        } 
        else {
            // create variable data for real
            std::vector<double> var_data;
            // reserve memory
            var_data.reserve(actual_points);
            for (size_t r = 0; r < actual_points; ++r) {
                // append value
                var_data.push_back(all_floats[r * floats_per_point + idx]);
            }
            // add owning view
            result.variables.emplace_back(ArrayView<double>(std::move(var_data)));
        }
    }
    // return result
    return result;
}


// process abscissa scale helper
Expression process_abscissa_scale(const Expression& abscissa, const AbscissaScale scale) {
    if (scale == AbscissaScale::DECADE) {
        // create new steps list
        RealStepData new_steps;
        for (size_t s = 0; s < abscissa.step_count(); ++s) {
            // get step view
            const auto& step = abscissa.step_data_real(s);
            // create log data vector
            std::vector<double> log_data;
            // reserve memory
            log_data.reserve(step.size());
            for (size_t i = 0; i < step.size(); ++i) {
                // compute log10 value
                log_data.push_back(std::log10(step[i]));
            }
            // add to steps
            new_steps.push_back(ArrayView<double>(std::move(log_data)));
        }
        // return new expression
        return Expression(abscissa.name(), new_steps, abscissa.unit(), abscissa.source(), abscissa.variable_type());
    }
    if (scale == AbscissaScale::OCTAVE) {
        // create new steps list
        RealStepData new_steps;
        for (size_t s = 0; s < abscissa.step_count(); ++s) {
            // get step view
            const auto& step = abscissa.step_data_real(s);
            // create log data vector
            std::vector<double> log_data;
            // reserve memory
            log_data.reserve(step.size());
            for (size_t i = 0; i < step.size(); ++i) {
                // compute log2 value
                log_data.push_back(std::log2(step[i]));
            }
            // add to steps
            new_steps.push_back(ArrayView<double>(std::move(log_data)));
        }
        // return new expression
        return Expression(abscissa.name(), new_steps, abscissa.unit(), abscissa.source(), abscissa.variable_type());
    }
    // return copy
    return abscissa;
}


// parsed block struct for scan accumulator
struct ParsedBlock {
    // header map
    std::unordered_map<std::string, std::string> headers;
    // points count
    size_t num_points = 0;
    // complex status flag
    bool is_complex = false;
    // block variables list
    std::vector<std::tuple<std::string, std::variant<ArrayView<double>, ArrayView<std::complex<double>>>, std::string, std::optional<std::string>>> variables;
};


// temporary variable struct for accumulator
struct TempVariable {
    // name field
    std::string name;
    // steps variant field
    StepDataVariant steps;
    // unit field
    std::string unit;
    // variable type field
    std::optional<std::string> variable_type;
};

} // namespace


// parse xyce raw file
std::shared_ptr<XyceOutputFile> xyce_raw_file_parser(const std::filesystem::path& filename) {
    // check if file exists
    if (!std::filesystem::exists(filename))
        return nullptr;
    // open the file
    int fd = open(filename.c_str(), O_RDONLY);
    if (fd < 0)
        return nullptr;
    // stat file size
    struct stat sb{};
    if (fstat(fd, &sb) == -1) {
        // close fd
        close(fd);
        // exit
        return nullptr;
    }
    // assign length
    size_t length = sb.st_size;
    if (length == 0) {
        // close fd
        close(fd);
        // exit
        return nullptr;
    }
    // memory map the file
    void* addr = mmap(nullptr, length, PROT_READ, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        // close fd
        close(fd);
        // exit
        return nullptr;
    }
    // cast to character pointer
    const char* data = reinterpret_cast<const char*>(addr);
    // initialize blocks list
    std::vector<ParsedBlock> blocks;
    // initialize step keys
    std::vector<std::string> step_keys;
    // initialize step values
    std::vector<std::vector<double>> step_values;
    // initialize scan position
    size_t pos = 0;
    // loop until no more blocks
    while (true) {
        // scan next block header
        auto block_result = scan_block_header(data, length, pos);
        if (!block_result)
            break;
        // move headers
        auto headers = std::move(block_result->header);
        // move variable definitions
        auto var_defs = std::move(block_result->variable_definitions);
        // assign data offset
        size_t data_offset = block_result->data_offset;
        // assign ascii status
        bool is_ascii = block_result->is_ascii;
        // initialize complex flag
        bool is_complex = false;
        if (headers.contains("Flags")) {
            // get flag string
            std::string flags = headers.at("Flags");
            // convert to lowercase
            std::transform(flags.begin(), flags.end(), flags.begin(), [](unsigned char c) { return std::tolower(c); });
            if (flags.find("complex") != std::string::npos) {
                // update complex flag
                is_complex = true;
            }
        }
        // initialize variables count
        size_t num_variables = 0;
        if (headers.contains("No. Variables")) {
            // parse count
            num_variables = std::stoull(headers.at("No. Variables"));
        }
        // initialize points count
        size_t num_points = 0;
        if (headers.contains("No. Points")) {
            // parse count
            num_points = std::stoull(headers.at("No. Points"));
        }
        // initialize actual points count
        size_t actual_points = num_points;
        // initialize variables list
        std::vector<std::tuple<std::string, std::variant<ArrayView<double>, ArrayView<std::complex<double>>>, std::string, std::optional<std::string>>> block_variables;
        if (is_ascii) {
            // parse ascii values
            auto ascii_res = parse_ascii_variables(data, length, data_offset, var_defs, is_complex, num_variables, num_points);
            if (!ascii_res) {
                // break loop
                break;
            }
            // update actual points
            actual_points = ascii_res->actual_points;
            for (size_t i = 0; i < var_defs.size(); ++i) {
                // get var def reference
                const auto& var_def = var_defs[i];
                // get variable name
                std::string name = std::get<1>(var_def);
                // get type option
                std::optional<VariableType> vt = std::get<2>(var_def);
                // assign unit
                std::string unit = vt ? get_variable_type_info(*vt).unit : "";
                // assign variable type name
                std::optional<std::string> vtype = vt ? std::optional<std::string>(get_variable_type_info(*vt).name) : std::nullopt;
                // append variable
                block_variables.emplace_back(name, std::move(ascii_res->variables[i]), unit, vtype);
            }
        }
        else {
            if (is_complex) {
                if (actual_points == 0) {
                    // compute row count from size
                    actual_points = (length - data_offset) / (num_variables * 16);
                }
                // cast base pointer
                const auto* base_ptr = reinterpret_cast<const std::complex<double>*>(data + data_offset);
                for (const auto& var_def : var_defs) {
                    // parse variable index
                    int idx = std::get<0>(var_def);
                    // parse variable name
                    std::string name = std::get<1>(var_def);
                    // parse type option
                    std::optional<VariableType> vt = std::get<2>(var_def);
                    // assign unit
                    std::string unit = vt ? get_variable_type_info(*vt).unit : "";
                    // assign type name
                    std::optional<std::string> vtype = vt ? std::optional<std::string>(get_variable_type_info(*vt).name) : std::nullopt;
                    if (idx == 0) {
                        // cast pointer to double for real abscissa
                        const auto* ptr = reinterpret_cast<const double*>(base_ptr);
                        // create strided view
                        ArrayView<double> view(ptr, actual_points, 2 * num_variables);
                        // append variable view
                        block_variables.emplace_back(name, std::move(view), unit, vtype);
                    }
                    else {
                        // cast pointer for complex variables
                        const std::complex<double>* ptr = base_ptr + idx;
                        // create strided view
                        ArrayView view(ptr, actual_points, num_variables);
                        // append variable view
                        block_variables.emplace_back(name, std::move(view), unit, vtype);
                    }
                }
            }
            else {
                if (actual_points == 0) {
                    // compute row count from size
                    actual_points = (length - data_offset) / (num_variables * 8);
                }
                // cast base pointer
                const auto* base_ptr = reinterpret_cast<const double*>(data + data_offset);
                for (const auto& var_def : var_defs) {
                    // parse index
                    int idx = std::get<0>(var_def);
                    // parse name
                    std::string name = std::get<1>(var_def);
                    // parse type option
                    std::optional<VariableType> vt = std::get<2>(var_def);
                    // assign unit
                    std::string unit = vt ? get_variable_type_info(*vt).unit : "";
                    // assign type name
                    std::optional<std::string> vtype = vt ? std::optional<std::string>(get_variable_type_info(*vt).name) : std::nullopt;
                    // cast pointer
                    const double* ptr = base_ptr + idx;
                    // create strided view
                    ArrayView view(ptr, actual_points, num_variables);
                    // append variable view
                    block_variables.emplace_back(name, std::move(view), unit, vtype);
                }
            }
        }
        if (actual_points == 0 || block_variables.empty()) {
            // break loop
            break;
        }
        // initialize parsed block
        ParsedBlock block;
        // assign headers
        block.headers = std::move(headers);
        // assign points
        block.num_points = actual_points;
        // assign complex flag
        block.is_complex = is_complex;
        // assign variables
        block.variables = std::move(block_variables);
        // append block
        blocks.push_back(std::move(block));
        if (is_ascii) {
            // stop scanning after ascii
            break;
        }
        // compute bytes per value
        size_t bytes_per_value = is_complex ? 16 : 8;
        // compute next scan position
        pos = data_offset + actual_points * num_variables * bytes_per_value;
        if (blocks.back().headers.contains("Plotname")) {
            // get plotname
            std::string plotname = blocks.back().headers.at("Plotname");
            // initialize regex
            std::regex pair_re(R"(name\s*=\s*(\S+)\s+value\s*=\s*([\d.eE+-]+))");
            // initialize name list
            std::vector<std::string> param_names;
            // initialize value list
            std::vector<double> param_values;
            // get regex iterators
            auto begin = std::sregex_iterator(plotname.begin(), plotname.end(), pair_re);
            // get end iterator
            auto end = std::sregex_iterator();
            for (auto i = begin; i != end; ++i) {
                // get match
                const std::smatch& match = *i;
                // append parsed name
                param_names.push_back(match[1].str());
                // append parsed value
                param_values.push_back(std::stod(match[2].str()));
            }
            if (!param_names.empty()) {
                // assign step keys
                step_keys = std::move(param_names);
                // append step values
                step_values.push_back(std::move(param_values));
            }
        }
        if (pos >= length) {
            // stop loop
            break;
        }
    }
    // check we found blocks in file
    if (blocks.empty()) {
        // un-map memory
        munmap(addr, length);
        // return null
        return nullptr;
    }
    // get first block reference
    const auto& first_block = blocks[0];
    // get step count
    size_t step_count = blocks.size();
    // initialize slice indices
    std::vector<std::pair<size_t, size_t>> abscissa_indices;
    // append first block slice
    abscissa_indices.emplace_back(0, first_block.num_points);
    // assign scale
    AbscissaScale abscissa_scale = AbscissaScale::LINEAR;
    // initialize value ranges list
    std::vector<std::pair<double, double>> abscissa_value_ranges;
    if (first_block.num_points > 0) {
        // get abscissa variable
        const auto& abscissa_var = first_block.variables[0];
        // get view variant
        const auto& view_var = std::get<1>(abscissa_var);
        // get first value
        double first_val = std::get<ArrayView<double>>(view_var)[0];
        // get last value
        double last_val = std::get<ArrayView<double>>(view_var)[first_block.num_points - 1];
        // append first range
        abscissa_value_ranges.emplace_back(first_val, last_val);
    } 
    else {
        // append empty range
        abscissa_value_ranges.emplace_back(0.0, 0.0);
    }
    // initialize temporary variables list
    std::vector<TempVariable> temp_variables;
    for (const auto& var_tuple : first_block.variables) {
        // get variable name
        std::string name = std::get<0>(var_tuple);
        // get variable unit
        std::string unit = std::get<2>(var_tuple);
        // get variable type name
        std::optional<std::string> vtype = std::get<3>(var_tuple);
        // initialize temp var
        TempVariable temp_var;
        // assign fields
        temp_var.name = std::move(name);
        // assign unit
        temp_var.unit = std::move(unit);
        // assign type
        temp_var.variable_type = std::move(vtype);
        // get variable value view
        const auto& var_val = std::get<1>(var_tuple);
        if (std::holds_alternative<ArrayView<double>>(var_val)) {
            // create steps list
            RealStepData steps;
            // append first step view
            steps.push_back(std::get<ArrayView<double>>(var_val));
            // assign steps variant
            temp_var.steps = std::move(steps);
        } 
        else {
            // create steps list for complex
            ComplexStepData steps;
            // append first step view
            steps.push_back(std::get<ArrayView<std::complex<double>>>(var_val));
            // assign steps variant
            temp_var.steps = std::move(steps);
        }
        // append temp variable
        temp_variables.push_back(std::move(temp_var));
    }
    // initialize abscissa offset
    size_t abscissa_index_offset = first_block.num_points;
    for (size_t b = 1; b < blocks.size(); ++b) {
        // get block reference
        const auto& block = blocks[b];
        for (size_t idx = 0; idx < block.variables.size(); ++idx) {
            // get variable tuple reference
            const auto& var_tuple = block.variables[idx];
            // get name
            std::string name = std::get<0>(var_tuple);
            // get value view
            const auto& var_val = std::get<1>(var_tuple);
            if (idx >= temp_variables.size() || temp_variables[idx].name != name) {
                // munmap memory
                munmap(addr, length);
                // return null
                return nullptr;
            }
            // get temp var reference
            auto& temp_var = temp_variables[idx];
            if (std::holds_alternative<ArrayView<double>>(var_val)) {
                // append real view
                std::get<RealStepData>(temp_var.steps).push_back(std::get<ArrayView<double>>(var_val));
            } 
            else {
                // append complex view
                std::get<ComplexStepData>(temp_var.steps).push_back(std::get<ArrayView<std::complex<double>>>(var_val));
            }
        }
        if (block.num_points > 0) {
            // get abscissa tuple reference
            const auto& abscissa_var = block.variables[0];
            // get view reference
            const auto& view_var = std::get<1>(abscissa_var);
            // get first value
            double first_val = std::get<ArrayView<double>>(view_var)[0];
            // get last value
            double last_val = std::get<ArrayView<double>>(view_var)[block.num_points - 1];
            // append range
            abscissa_value_ranges.emplace_back(first_val, last_val);
        } 
        else {
            // append empty range
            abscissa_value_ranges.emplace_back(0.0, 0.0);
        }
        // append index pair
        abscissa_indices.emplace_back(abscissa_index_offset, abscissa_index_offset + block.num_points);
        // advance offset
        abscissa_index_offset += block.num_points;
    }
    // create step information
    StepInformation step_information(std::move(step_keys), std::move(step_values), std::move(abscissa_value_ranges));
    // initialize expressions list
    std::vector<Expression> expressions;
    // reserve memory
    expressions.reserve(temp_variables.size());
    for (auto&[name, steps, unit, variable_type] : temp_variables) {
        // append expression
        expressions.emplace_back(std::move(name), std::move(steps), std::move(unit), std::nullopt, std::move(variable_type));
    }
    // scale abscissa
    Expression abscissa = process_abscissa_scale(expressions[0], abscissa_scale);
    // update list
    expressions[0] = abscissa;
    // initialize optional slices
    std::optional<std::vector<std::pair<size_t, size_t>>> step_slices = std::nullopt;
    if (step_count > 1) {
        // assign step slices
        step_slices = std::move(abscissa_indices);
    }
    // create expression manager
    ExpressionManager expression_manager(std::move(expressions), std::move(step_slices));
    // initialize title
    std::string title;
    if (first_block.headers.contains("Title")) {
        // assign title
        title = first_block.headers.at("Title");
    }
    // return shared pointer
    return std::make_shared<XyceOutputFile>(filename, std::move(title), first_block.is_complex, std::move(step_information), std::move(abscissa), abscissa_scale, std::move(expression_manager), addr, length);
}
