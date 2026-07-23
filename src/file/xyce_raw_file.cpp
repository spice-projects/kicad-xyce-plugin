#include <algorithm>
#include <complex>
#include <optional>
#include <regex>

#include "../expression/expression.h"
#include "../step_information.h"
#include "mapped_file.h"
#include "xyce_output_file.h"
#include "xyce_raw_file.h"


namespace
{
    struct BlockHeaderScanResult
    {
        std::string title;
        std::string plotname;
        bool is_complex = false;
        size_t data_offset = 0;
        bool is_ascii = false;
        size_t num_points = 0;
        std::vector<std::tuple<int, std::string, VariableType, std::variant<std::monostate, View<double>, View<std::complex<double>>>>> variables;
    };

    void trim(std::string& s) {
        // erase leading whitespace
        s.erase(s.begin(), std::ranges::find_if(s, [](const unsigned char ch) { return !std::isspace(ch); }));
        // erase trailing whitespace
        s.erase(std::find_if(s.rbegin(), s.rend(), [](const unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
    }

    std::vector<std::string> split(const std::string& line) {
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
        // exit
        return parts;
    }

    std::optional<BlockHeaderScanResult> scan_block_header(const char* data, const size_t length, const size_t start_pos) {
        // initialize result
        BlockHeaderScanResult result;
        // headers
        std::unordered_map<std::string, std::string> headers;
        // initialize variables status
        bool in_variables = false;
        // initialize position
        size_t pos = start_pos;
        // initialize marker status
        bool found_marker = false;
        // process buffer
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
            // check we are processing variables
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
                if (std::vector<std::string> parts = split(line); parts.size() == 3) {
                    // parse index
                    int index = std::stoi(parts[0]);
                    // parse name
                    std::string name = parts[1];
                    // parse variable type
                    VariableType variable_type = parse_variable_type(parts[2]);
                    // insert variable definition
                    result.variables.emplace_back(index, name, variable_type, std::monostate());
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
            if (size_t colon = line.find(':'); colon != std::string::npos) {
                // extract key
                std::string key = line.substr(0, colon);
                // extract value
                std::string value = line.substr(colon + 1);
                // trim key
                trim(key);
                // trim value
                trim(value);
                // insert to map
                headers[key] = value;
            }
        }
        if (!found_marker) {
            // empty result
            return std::nullopt;
        }
        // extract Title
        if (headers.contains("Title")) {
            // update result
            result.title = headers.at("Title");
        }
        // extract Plotname
        if (headers.contains("Plotname")) {
            // update result
            result.plotname = headers.at("Plotname");
        }
        // check header Flags
        if (headers.contains("Flags")) {
            // get flag string
            std::string flags = headers.at("Flags");
            // convert to lowercase
            std::ranges::transform(flags, flags.begin(), [](const unsigned char c) { return std::tolower(c); });
            if (flags.find("complex") != std::string::npos) {
                // update complex flag
                result.is_complex = true;
            }
        }
        // check No variables
        if (headers.contains("No. Variables")) {
            // parse count
            if (const size_t num_variables = std::stoull(headers.at("No. Variables")); result.variables.size() != num_variables) {
                // empty result
                return std::nullopt;
            }
        }
        // check points count
        if (headers.contains("No. Points")) {
            // parse count
            result.num_points = std::stoull(headers.at("No. Points"));
        }
        // return result
        return result;
    }

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

    bool parse_ascii_variables(const char* data, const size_t length, const size_t offset, std::vector<std::tuple<int, std::string, VariableType, std::variant<std::monostate, View<double>, View<std::complex<double>>>>>& variables, bool is_complex, const size_t num_points) {
        // number of variables
        const size_t num_variables = variables.size();
        // compute floats per point
        const size_t floats_per_point = is_complex ? num_variables * 2 : num_variables;
        // compute tokens per line
        const size_t tokens_per_line = 1 + floats_per_point;
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
                return false;
            }
            try {
                // parse index
                const size_t index = std::stoull(tokens[0]);
                if (index != expected_index) {
                    // error return
                    return false;
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
                return false;
            }
        }
        // compute actual points
        const size_t actual_points = all_floats.size() / floats_per_point;
        if (num_points > 0 && actual_points != num_points) {
            // error return
            return false;
        }
        // process variable definitions
        for (auto& variable : variables) {
            // get variable index
            const int idx = std::get<0>(variable);
            // check complex variables are expected
            if (is_complex) {
                // abscissa is always real
                if (idx == 0) {
                    // create abscissa data
                    std::vector<double> variable_data;
                    // reserve memory
                    variable_data.reserve(actual_points);
                    // loop expected points
                    for (size_t r = 0; r < actual_points; ++r) {
                        // append real part
                        variable_data.push_back(all_floats[r * floats_per_point + idx * 2]);
                    }
                    // add owning view
                    std::get<3>(variable) = View(variable_data);
                    // next
                    continue;
                }
                // create variable data
                std::vector<std::complex<double>> variable_data;
                // reserve memory
                variable_data.reserve(actual_points);
                // loop expected points
                for (size_t r = 0; r < actual_points; ++r) {
                    // get real part
                    double real_part = all_floats[r * floats_per_point + idx * 2];
                    // get imag part
                    double imag_part = all_floats[r * floats_per_point + idx * 2 + 1];
                    // append complex value
                    variable_data.emplace_back(real_part, imag_part);
                }
                // add owning view
                std::get<3>(variable) = View(variable_data);
                // next
                continue;
            }
            // create variable data for real
            std::vector<double> variable_data;
            // reserve memory
            variable_data.reserve(actual_points);
            // loop expected points
            for (size_t r = 0; r < actual_points; ++r) {
                // append value
                variable_data.push_back(all_floats[r * floats_per_point + idx]);
            }
            // add owning view
            std::get<3>(variable) = View(variable_data);
        }
        return true;
    }

    bool parse_binary_variables(const char* data, const size_t length, const size_t offset, std::vector<std::tuple<int, std::string, VariableType, std::variant<std::monostate, View<double>, View<std::complex<double>>>>>& variables, bool is_complex, const size_t num_points) {
        // number of variables
        const size_t num_variables = variables.size();
        // check we need to read complex numbers from file
        if (is_complex) {
            // validate file length
            if (offset + num_points * num_variables * sizeof(std::complex<double>) > length)
                return false;
            // cast base pointer
            auto* base_ptr = reinterpret_cast<const std::complex<double>*>(data + offset);
            // loop variable definitions
            for (auto& variable : variables) {
                // parse variable index
                int idx = std::get<0>(variable);
                // parse variable name
                std::string name = std::get<1>(variable);
                // extract variable type info
                auto [vtype, unit] = get_variable_type_info(std::get<2>(variable));
                // check this is the abscissa (always a real number)
                if (idx == 0) {
                    // cast pointer to double for real abscissa
                    auto ptr = reinterpret_cast<const double*>(base_ptr);
                    // create stride view
                    std::get<3>(variable) = View(ptr, num_points, 2 * num_variables);
                }
                else {
                    // cast pointer for complex variables
                    auto ptr = base_ptr + idx;
                    // create stride view
                    std::get<3>(variable) = View(ptr, num_points, num_variables);
                }
            }
            return true;
        }
        // validate file length
        if (offset + num_variables * num_points * sizeof(double) > length)
            return false;
        // cast base pointer
        auto* base_ptr = reinterpret_cast<const double*>(data + offset);
        // loop variable definitions
        for (auto& variable : variables) {
            // parse index
            int idx = std::get<0>(variable);
            // parse name
            std::string name = std::get<1>(variable);
            // extract variable type info
            auto [vtype, unit] = get_variable_type_info(std::get<2>(variable));
            // cast pointer
            auto ptr = base_ptr + idx;
            // create stride view
            std::get<3>(variable) = View(ptr, num_points, num_variables);
        }
        return true;
    }

    // temporary variable struct for accumulator
    struct TempVariable
    {
        bool is_complex;
        std::string name;
        std::variant<std::vector<View<double>>, std::vector<View<std::complex<double>>>> steps;
        VariableType variable_type;
    };
} // namespace

std::optional<XyceOutputFile> xyce_raw_file_parser(const std::filesystem::path& filename) {
    // check if file exists
    if (!std::filesystem::exists(filename))
        return {};

    MappedFile mapped_file(filename);
    if (!mapped_file.is_valid())
        return {};

    const auto data = mapped_file.data();
    const size_t length = mapped_file.size();
    if (length == 0) {
        return {};
    }
    // initialize blocks list
    std::vector<BlockHeaderScanResult> blocks;
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
        // assign data offset
        size_t data_offset = block_result->data_offset;
        // assign ascii status
        bool is_ascii = block_result->is_ascii;
        // number of variables in this block
        const size_t num_variables = block_result->variables.size();
        // check ascii file
        if (is_ascii) {
            // parse ascii values
            if (!parse_ascii_variables(data, length, data_offset, block_result->variables, block_result->is_complex, block_result->num_points))
                break;
            // ascii blocks are parsed from the remaining text section
            pos = length;
        }
        else {
            // parse binary file
            if (!parse_binary_variables(data, length, data_offset, block_result->variables, block_result->is_complex, block_result->num_points))
                break;
            // compute bytes per point
            const size_t bytes_per_point = block_result->is_complex ? num_variables * sizeof(std::complex<double>) : num_variables * sizeof(double);
            // compute next scan position
            pos = data_offset + block_result->num_points * bytes_per_point;
        }
        // append parsed block
        blocks.push_back(std::move(*block_result));
        // process plotname
        if (!blocks.back().plotname.empty()) {
            // initialize regex
            std::regex pair_re(R"(name\s*=\s*(\S+)\s+value\s*=\s*([\d.eE+-]+))");
            // initialize name list
            std::vector<std::string> param_names;
            // initialize value list
            std::vector<double> param_values;
            // get regex iterators
            auto begin = std::sregex_iterator(blocks.back().plotname.begin(), blocks.back().plotname.end(), pair_re);
            // get end iterator
            auto end = std::sregex_iterator();
            // loop matches
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
        return {};
    }
    // get first block reference
    auto& first_block = blocks[0];
    // get step count
    const size_t step_count = blocks.size();
    // step related data
    std::vector<std::pair<size_t, size_t>> abscissa_indices;
    std::vector<std::pair<double, double>> abscissa_value_ranges;
    // reserve memory for steps
    abscissa_indices.reserve(step_count);
    abscissa_value_ranges.reserve(step_count);
    // assign scale
    AbscissaScale abscissa_scale = AbscissaScale::LINEAR;
    // check first block number of points in expressions
    if (first_block.num_points > 0) {
        // get abscissa variable
        const auto& abscissa_view = std::get<View<double>>(std::get<3>(first_block.variables[0]));
        // get first & last values
        double first_val = abscissa_view[0];
        double last_val = abscissa_view[abscissa_view.size() - 1];
        // append first indices and range
        abscissa_indices.emplace_back(0, first_block.num_points);
        abscissa_value_ranges.emplace_back(first_val, last_val);
    }
    else {
        // append empty range
        abscissa_indices.emplace_back(0, 0);
        abscissa_value_ranges.emplace_back(0.0, 0.0);
    }
    // initialize temporary variables list
    std::vector<TempVariable> temp_variables;
    // allocate it
    temp_variables.reserve(first_block.variables.size());
    // loop first block variables
    for (auto& [idx, name, variable_type, view] : first_block.variables) {
        // check view type
        if (std::holds_alternative<View<double>>(view)) {
            // create vector
            std::vector<View<double>> steps;
            // allocate memory
            steps.reserve(step_count);
            // append first value
            steps.push_back(std::move(std::get<View<double>>(view)));
            // append variable
            temp_variables.emplace_back(false, name, std::move(steps), variable_type);
            // next
            continue;
        }
        // check view type
        if (std::holds_alternative<View<std::complex<double>>>(view)) {
            // create vector
            std::vector<View<std::complex<double>>> steps;
            // allocate memory
            steps.reserve(step_count);
            // append first value
            steps.push_back(std::move(std::get<View<std::complex<double>>>(view)));
            // append variable
            temp_variables.emplace_back(true, name, std::move(steps), variable_type);
        }
    }
    // initialize abscissa offset
    size_t abscissa_index_offset = first_block.num_points;
    // loop from second block
    for (size_t b = 1; b < blocks.size(); ++b) {
        // get block reference
        auto& block = blocks[b];
        // loop variables
        for (auto& [idx, name, variable_type, view] : block.variables) {
            // validate variable index and metadata consistency across steps
            if (idx < 0 || static_cast<size_t>(idx) >= temp_variables.size() || temp_variables[idx].name != name || temp_variables[idx].variable_type != variable_type) {
                return {};
            }
            // variable at index
            auto& variable = temp_variables[idx];
            // check it is a complex variable
            if (variable.is_complex) {
                // append view to steps
                std::get<std::vector<View<std::complex<double>>>>(variable.steps).push_back(std::move(std::get<View<std::complex<double>>>(view)));
            }
            else {
                // append view to steps
                std::get<std::vector<View<double>>>(variable.steps).push_back(std::move(std::get<View<double>>(view)));
            }
        }
        if (block.num_points > 0) {
            // get abscissa variable
            const auto& abscissa_view = std::get<View<double>>(std::get<3>(block.variables[0]));
            // get first & last values
            double first_val = abscissa_view[0];
            double last_val = abscissa_view[abscissa_view.size() - 1];
            // append first range
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
    std::vector<AnyExpression> expressions;
    // reserve memory
    expressions.reserve(temp_variables.size());
    // loop variables
    for (auto&& [idx, name, steps, variable_type] : temp_variables) {
        // extract variable type info
        auto [vtype, unit] = get_variable_type_info(variable_type);
        // create expressions
        auto l = [&expressions, &name, &vtype, &unit]<typename T0>(T0& s) {
            // actual parameter type
            using TX = std::decay_t<T0>;
            // double
            if constexpr (std::is_same_v<TX, std::vector<View<double>>>)
                expressions.emplace_back(Expression<double>(name, s, unit, "", vtype));
            // complex
            if constexpr (std::is_same_v<TX, std::vector<View<std::complex<double>>>>)
                expressions.emplace_back(Expression<std::complex<double>>(name, s, unit, "", vtype));
        };
        // process list
        std::visit(l, steps);
    }
    // // transform abscissa if required
    // if (abscissa_scale != AbscissaScale::LINEAR) {
    //     // abscissa
    //     auto& abscissa = std::get<Expression<double>>(expressions.at(0));
    //     // transform it
    //     abscissa.transform(abscissa_scale == AbscissaScale::DECADE ? [](const double value)-> double { return std::log10(value); } : [](const double value)-> double { return std::log2(value); });
    // }
    // create expression manager
    ExpressionManager expression_manager(expressions, abscissa_indices);
    auto shared_mapped_file = std::make_shared<MappedFile>(std::move(mapped_file));
    // return file
    return XyceOutputFile(filename, first_block.title, first_block.is_complex, std::move(step_information), abscissa_scale, std::move(expression_manager), shared_mapped_file);
}
