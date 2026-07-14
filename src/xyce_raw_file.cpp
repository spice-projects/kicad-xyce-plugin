// #include <algorithm>
// #include <chrono>
// #include <cmath>
// #include <complex>
// #include <fcntl.h>
// #include <regex>
// #include <unistd.h>
// #include <sys/mman.h>
// #include <sys/stat.h>
//
// #include "xyce_raw_file.h"
// #include "expression.h"
// #include "step_information.h"
//
// namespace
// {
//     struct BlockHeaderScanResult
//     {
//         // complex
//         bool is_complex = false;
//         // data offset field
//         size_t data_offset = 0;
//         // ascii status flag
//         bool is_ascii = false;
//         // number of points in variables
//         size_t num_points = 0;
//         // variable definitions list
//         std::vector<std::tuple<int, std::string, VariableType>> variable_definitions;
//     };
//
//     void trim(std::string& s) {
//         // erase leading whitespace
//         s.erase(s.begin(), std::ranges::find_if(s, [](const unsigned char ch) { return !std::isspace(ch); }));
//         // erase trailing whitespace
//         s.erase(std::find_if(s.rbegin(), s.rend(), [](const unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
//     }
//
//     std::vector<std::string> split(const std::string& line) {
//         // split by tab characters
//         std::vector<std::string> parts;
//         // init start
//         size_t start = 0;
//         // find first tab
//         size_t tab = line.find('\t');
//         while (tab != std::string::npos) {
//             // append part
//             parts.push_back(line.substr(start, tab - start));
//             // update start
//             start = tab + 1;
//             // find next tab
//             tab = line.find('\t', start);
//         }
//         // append last part
//         parts.push_back(line.substr(start));
//         // exit
//         return parts;
//     }
//
//     std::optional<BlockHeaderScanResult> scan_block_header(const char* data, const size_t length, const size_t start_pos) {
//         // initialize result
//         BlockHeaderScanResult result;
//         // headers
//         std::unordered_map<std::string, std::string> headers;
//         // initialize variables status
//         bool in_variables = false;
//         // initialize position
//         size_t pos = start_pos;
//         // initialize marker status
//         bool found_marker = false;
//         // process buffer
//         while (pos < length) {
//             // find next newline
//             size_t newline = pos;
//             while (newline < length && data[newline] != '\n') {
//                 // increment newline
//                 newline++;
//             }
//             if (newline >= length && pos == newline) {
//                 // break loop
//                 break;
//             }
//             // extract line
//             std::string line(data + pos, newline - pos);
//             // trim whitespace
//             trim(line);
//             // advance position
//             pos = newline + 1;
//             // check we are processing variables
//             if (in_variables) {
//                 // check for end of variables
//                 if (line == "Binary:" || line == "Values:") {
//                     // record offset
//                     result.data_offset = pos;
//                     // record ascii status
//                     result.is_ascii = (line == "Values:");
//                     // update status
//                     found_marker = true;
//                     // break loop
//                     break;
//                 }
//                 // split by tab characters
//                 if (std::vector<std::string> parts = split(line); parts.size() == 3) {
//                     // parse index
//                     int index = std::stoi(parts[0]);
//                     // parse name
//                     std::string name = parts[1];
//                     // parse variable type
//                     VariableType variable_type = parse_variable_type(parts[2]);
//                     // insert variable definition
//                     result.variable_definitions.emplace_back(index, name, variable_type);
//                 }
//                 // continue loop
//                 continue;
//             }
//             if (line == "Variables:") {
//                 // update status
//                 in_variables = true;
//                 // continue loop
//                 continue;
//             }
//             // find colon
//             if (size_t colon = line.find(':'); colon != std::string::npos) {
//                 // extract key
//                 std::string key = line.substr(0, colon);
//                 // extract value
//                 std::string value = line.substr(colon + 1);
//                 // trim key
//                 trim(key);
//                 // trim value
//                 trim(value);
//                 // insert to map
//                 headers[key] = value;
//             }
//         }
//         if (!found_marker) {
//             // empty result
//             return std::nullopt;
//         }
//         // check header Flags
//         if (headers.contains("Flags")) {
//             // get flag string
//             std::string flags = headers.at("Flags");
//             // convert to lowercase
//             std::ranges::transform(flags, flags.begin(), [](const unsigned char c) { return std::tolower(c); });
//             if (flags.find("complex") != std::string::npos) {
//                 // update complex flag
//                 result.is_complex = true;
//             }
//         }
//         // check No variables
//         if (headers.contains("No. Variables")) {
//             // parse count
//             const size_t num_variables = std::stoull(headers.at("No. Variables"));
//             // compare it with actual variables
//             if (result.variable_definitions.size() != num_variables) {
//                 // empty result
//                 return std::nullopt;
//             }
//         }
//         // check points count
//         if (headers.contains("No. Points")) {
//             // parse count
//             result.num_points = std::stoull(headers.at("No. Points"));
//         }
//         // return result
//         return result;
//     }
//
//     struct ParseAsciiResult
//     {
//         // actual points count
//         size_t actual_points = 0;
//         // variables list
//         std::vector<std::variant<View<double>, View<std::complex<double>>>> variables;
//     };
//
//     std::vector<std::string> get_tokens(const std::string& line) {
//         // initialize tokens
//         std::vector<std::string> tokens;
//         // initialize start
//         size_t start = 0;
//         while (start < line.size()) {
//             while (start < line.size() && std::isspace(static_cast<unsigned char>(line[start]))) {
//                 // advance start
//                 start++;
//             }
//             if (start >= line.size()) {
//                 // break loop
//                 break;
//             }
//             // initialize end
//             size_t end = start;
//             while (end < line.size() && !std::isspace(static_cast<unsigned char>(line[end]))) {
//                 // advance end
//                 end++;
//             }
//             // add token
//             tokens.push_back(line.substr(start, end - start));
//             // update start
//             start = end;
//         }
//         // return tokens
//         return tokens;
//     }
//
//
//     // parse ascii variables helper
//     std::optional<ParseAsciiResult> parse_ascii_variables(const char* data, size_t length, size_t offset, const std::vector<std::tuple<int, std::string, VariableType>>& variable_definitions, bool is_complex, size_t num_points) {
//         // initialize result
//         ParseAsciiResult result;
//         // number of variables
//         const size_t num_variables = variable_definitions.size();
//         // compute floats per point
//         const size_t floats_per_point = is_complex ? num_variables * 2 : num_variables;
//         // compute tokens per line
//         const size_t tokens_per_line = 1 + floats_per_point;
//         // initialize floats buffer
//         std::vector<double> all_floats;
//         // initialize expected index
//         size_t expected_index = 0;
//         // initialize position
//         size_t pos = offset;
//         while (pos < length) {
//             // find next newline
//             size_t newline = pos;
//             while (newline < length && data[newline] != '\n') {
//                 // increment newline
//                 newline++;
//             }
//             if (newline >= length && pos == newline) {
//                 // break loop
//                 break;
//             }
//             // extract line
//             std::string line(data + pos, newline - pos);
//             // advance position
//             pos = newline + 1;
//             // trim line
//             trim(line);
//             if (line.empty()) {
//                 // skip empty
//                 continue;
//             }
//             // get tokens
//             std::vector<std::string> tokens = get_tokens(line);
//             if (tokens.size() != tokens_per_line) {
//                 // error return
//                 return std::nullopt;
//             }
//             try {
//                 // parse index
//                 const size_t index = std::stoull(tokens[0]);
//                 if (index != expected_index) {
//                     // error return
//                     return std::nullopt;
//                 }
//                 for (size_t i = 1; i < tokens.size(); ++i) {
//                     // append parsed double
//                     all_floats.push_back(std::stod(tokens[i]));
//                 }
//                 // increment expected index
//                 expected_index++;
//             }
//             catch (...) {
//                 // error return
//                 return std::nullopt;
//             }
//         }
//         // compute actual points
//         const size_t actual_points = all_floats.size() / floats_per_point;
//         if (num_points > 0 && actual_points != num_points) {
//             // error return
//             return std::nullopt;
//         }
//         // assign actual points
//         result.actual_points = actual_points;
//         // reserve variables size
//         result.variables.reserve(num_variables);
//         // process variable definitions
//         for (const auto& var_def : variable_definitions) {
//             // get variable index
//             const int idx = std::get<0>(var_def);
//             // check complex variables are expected
//             if (is_complex) {
//                 // abscissa is always real
//                 if (idx == 0) {
//                     // create abscissa data
//                     std::vector<double> var_data;
//                     // reserve memory
//                     var_data.reserve(actual_points);
//                     // loop expected points
//                     for (size_t r = 0; r < actual_points; ++r) {
//                         // append real part
//                         var_data.push_back(all_floats[r * floats_per_point + idx * 2]);
//                     }
//                     // add owning view
//                     result.variables.emplace_back(View(var_data));
//                 }
//                 else {
//                     // create variable data
//                     std::vector<std::complex<double>> var_data;
//                     // reserve memory
//                     var_data.reserve(actual_points);
//                     // loop expected points
//                     for (size_t r = 0; r < actual_points; ++r) {
//                         // get real part
//                         double real_part = all_floats[r * floats_per_point + idx * 2];
//                         // get imag part
//                         double imag_part = all_floats[r * floats_per_point + idx * 2 + 1];
//                         // append complex value
//                         var_data.emplace_back(real_part, imag_part);
//                     }
//                     // add owning view
//                     result.variables.emplace_back(View(var_data));
//                 }
//             }
//             else {
//                 // create variable data for real
//                 std::vector<double> var_data;
//                 // reserve memory
//                 var_data.reserve(actual_points);
//                 // loop expected points
//                 for (size_t r = 0; r < actual_points; ++r) {
//                     // append value
//                     var_data.push_back(all_floats[r * floats_per_point + idx]);
//                 }
//                 // add owning view
//                 result.variables.emplace_back(View(var_data));
//             }
//         }
//         // return result
//         return result;
//     }
//
//     Expression process_abscissa_scale(Expression abscissa, const AbscissaScale scale) {
//         // do nothing on linear
//         if (scale == AbscissaScale::LINEAR)
//             return abscissa;
//         // copy name, unit and vt
//         std::string name(abscissa.name());
//         std::string unit(abscissa.unit());
//         std::string variable_type(abscissa.variable_type());
//         // check scale type
//         if (scale == AbscissaScale::DECADE) {
//             // map expression
//             return abscissa.transform<double, double>(name, unit, variable_type, [](const double value)-> double { return std::log10(value); });
//         }
//         // map expression
//         return abscissa.transform<double, double>(name, unit, variable_type, [](const double value)-> double { return std::log2(value); });
//     }
//
//     struct ParsedBlock
//     {
//         // points count
//         size_t num_points = 0;
//         // complex status flag
//         bool is_complex = false;
//         // block variables list
//         std::vector<std::tuple<std::string, std::variant<View<double>, View<std::complex<double>>>, std::string, std::string>> variables;
//     };
//
//
//     // temporary variable struct for accumulator
//     struct TempVariable
//     {
//         // name field
//         std::string name;
//         // steps variant field
//         Steps steps;
//         // unit field
//         std::string unit;
//         // variable type field
//         std::optional<std::string> variable_type;
//     };
// } // namespace
//
//
// // parse xyce raw file
// std::shared_ptr<XyceOutputFile> xyce_raw_file_parser(const std::filesystem::path& filename) {
//     // check if file exists
//     if (!std::filesystem::exists(filename))
//         return nullptr;
//     // open the file
//     int fd = open(filename.c_str(), O_RDONLY);
//     if (fd < 0)
//         return nullptr;
//     // stat file size
//     struct stat file_info{};
//     if (fstat(fd, &file_info) == -1) {
//         // close fd
//         close(fd);
//         // exit
//         return nullptr;
//     }
//     // assign length
//     size_t length = file_info.st_size;
//     if (length == 0) {
//         // close fd
//         close(fd);
//         // exit
//         return nullptr;
//     }
//     // memory map the file
//     void* addr = mmap(nullptr, length, PROT_READ, MAP_SHARED, fd, 0);
//     if (addr == MAP_FAILED) {
//         // close fd
//         close(fd);
//         // exit
//         return nullptr;
//     }
//     // cast to character pointer (plain buffer)
//     const auto data = static_cast<char*>(addr);
//     // initialize blocks list
//     std::vector<ParsedBlock> blocks;
//     // initialize step keys
//     std::vector<std::string> step_keys;
//     // initialize step values
//     std::vector<std::vector<double>> step_values;
//     // initialize scan position
//     size_t pos = 0;
//     // loop until no more blocks
//     while (true) {
//         // scan next block header
//         auto block_result = scan_block_header(data, length, pos);
//         if (!block_result)
//             break;
//         // move variable definitions
//         auto variable_definitions = std::move(block_result->variable_definitions);
//         // assign data offset
//         size_t data_offset = block_result->data_offset;
//         // assign ascii status
//         bool is_ascii = block_result->is_ascii;
//         // initialize actual points count
//         size_t actual_points = block_result->num_points;
//         // initialize variables list
//         std::vector<std::tuple<std::string, std::variant<View<double>, View<std::complex<double>>>, std::string, std::string>> block_variables;
//         // allocate capacity
//         block_variables.reserve(variable_definitions.size());
//         // check ascii file
//         if (is_ascii) {
//             // parse ascii values
//             auto ascii_res = parse_ascii_variables(data, length, data_offset, variable_definitions, block_result->is_complex, block_result->num_points);
//             if (!ascii_res) {
//                 // break loop
//                 break;
//             }
//             // update actual points
//             actual_points = ascii_res->actual_points;
//             // loop variable definitions
//             for (size_t i = 0; i < variable_definitions.size(); ++i) {
//                 // get var def reference
//                 const auto& var_def = variable_definitions[i];
//                 // get variable name
//                 std::string name = std::get<1>(var_def);
//                 // extract variable type info
//                 auto [vtype, unit] = get_variable_type_info(std::get<2>(var_def));
//                 // append variable
//                 block_variables.emplace_back(name, std::move(ascii_res->variables[i]), unit, vtype);
//             }
//         }
//         else {
//             // number of variables
//             const size_t num_variables = block_result->variable_definitions.size();
//             // check we need to read complex numbers from file
//             if (block_result->is_complex) {
//                 // calculate the number of points if not available
//                 if (actual_points == 0) {
//                     // compute row count from size
//                     actual_points = (length - data_offset) / (num_variables * 16);
//                 }
//                 // cast base pointer
//                 auto* base_ptr = reinterpret_cast<std::complex<double>*>(data + data_offset);
//                 // loop variable definitions
//                 for (const auto& var_def : variable_definitions) {
//                     // parse variable index
//                     int idx = std::get<0>(var_def);
//                     // parse variable name
//                     std::string name = std::get<1>(var_def);
//                     // extract variable type info
//                     auto [vtype, unit] = get_variable_type_info(std::get<2>(var_def));
//                     // check this is the abscissa (always a real number)
//                     if (idx == 0) {
//                         // cast pointer to double for real abscissa
//                         auto ptr = reinterpret_cast<double*>(base_ptr);
//                         // create stride view
//                         View view(ptr, actual_points, 2 * num_variables);
//                         // append variable view
//                         block_variables.emplace_back(name, std::move(view), unit, vtype);
//                     }
//                     else {
//                         // cast pointer for complex variables
//                         auto ptr = base_ptr + idx;
//                         // create stride view
//                         View view(ptr, actual_points, num_variables);
//                         // append variable view
//                         block_variables.emplace_back(name, std::move(view), unit, vtype);
//                     }
//                 }
//             }
//             else {
//                 // calculate the number of points if not available
//                 if (actual_points == 0) {
//                     // compute row count from size
//                     actual_points = (length - data_offset) / (num_variables * 8);
//                 }
//                 // cast base pointer
//                 auto* base_ptr = reinterpret_cast<double*>(data + data_offset);
//                 // loop variable definitions
//                 for (const auto& var_def : variable_definitions) {
//                     // parse index
//                     int idx = std::get<0>(var_def);
//                     // parse name
//                     std::string name = std::get<1>(var_def);
//                     // extract variable type info
//                     auto [vtype, unit] = get_variable_type_info(std::get<2>(var_def));
//                     // cast pointer
//                     auto ptr = base_ptr + idx;
//                     // create stride view
//                     View view(ptr, actual_points, num_variables);
//                     // append variable view
//                     block_variables.emplace_back(name, std::move(view), unit, vtype);
//                 }
//             }
//         }
//         if (actual_points == 0 || block_variables.empty()) {
//             // break loop
//             break;
//         }
//         // initialize parsed block
//         ParsedBlock block;
//         // assign points
//         block.num_points = actual_points;
//         // assign complex flag
//         block.is_complex = block_result->is_complex;
//         // assign variables
//         block.variables = std::move(block_variables);
//         // append block
//         blocks.push_back(std::move(block));
//         if (is_ascii) {
//             // stop scanning after ascii
//             break;
//         }
//         // compute bytes per value
//         size_t bytes_per_value = block.is_complex  ? 16 : 8;
//         // compute next scan position
//         pos = data_offset + actual_points * variable_definitions.size() * bytes_per_value;
//         //
//         //
//         // if (blocks.back().headers.contains("Plotname")) {
//         //     // get plotname
//         //     std::string plotname = blocks.back().headers.at("Plotname");
//         //     // initialize regex
//         //     std::regex pair_re(R"(name\s*=\s*(\S+)\s+value\s*=\s*([\d.eE+-]+))");
//         //     // initialize name list
//         //     std::vector<std::string> param_names;
//         //     // initialize value list
//         //     std::vector<double> param_values;
//         //     // get regex iterators
//         //     auto begin = std::sregex_iterator(plotname.begin(), plotname.end(), pair_re);
//         //     // get end iterator
//         //     auto end = std::sregex_iterator();
//         //     for (auto i = begin; i != end; ++i) {
//         //         // get match
//         //         const std::smatch& match = *i;
//         //         // append parsed name
//         //         param_names.push_back(match[1].str());
//         //         // append parsed value
//         //         param_values.push_back(std::stod(match[2].str()));
//         //     }
//         //     if (!param_names.empty()) {
//         //         // assign step keys
//         //         step_keys = std::move(param_names);
//         //         // append step values
//         //         step_values.push_back(std::move(param_values));
//         //     }
//         // }
//         if (pos >= length) {
//             // stop loop
//             break;
//         }
//     }
//     // check we found blocks in file
//     if (blocks.empty()) {
//         // un-map memory
//         munmap(addr, length);
//         // return null
//         return nullptr;
//     }
//     // get first block reference
//     const auto& first_block = blocks[0];
//     // get step count
//     size_t step_count = blocks.size();
//     // initialize slice indices
//     std::vector<std::pair<size_t, size_t>> abscissa_indices;
//     // append first block slice
//     abscissa_indices.emplace_back(0, first_block.num_points);
//     // assign scale
//     AbscissaScale abscissa_scale = AbscissaScale::LINEAR;
//     // initialize value ranges list
//     std::vector<std::pair<double, double>> abscissa_value_ranges;
//     // check first block number of points in expressions
//     if (first_block.num_points > 0) {
//         // get abscissa variable
//         const auto& abscissa_expression = first_block.variables[0];
//         // get view variant
//         const auto& abscissa_view = std::get<1>(abscissa_expression);
//         // get first & last values
//         double first_val = std::get<View<double>>(abscissa_view)[0];
//         double last_val = std::get<View<double>>(abscissa_view)[first_block.num_points - 1];
//         // append first range
//         abscissa_value_ranges.emplace_back(first_val, last_val);
//     }
//     else {
//         // append empty range
//         abscissa_value_ranges.emplace_back(0.0, 0.0);
//     }
//     // initialize temporary variables list
//     std::vector<TempVariable> temp_variables;
//     for (const auto& var_tuple : first_block.variables) {
//         // get variable name
//         std::string name = std::get<0>(var_tuple);
//         // get variable unit
//         std::string unit = std::get<2>(var_tuple);
//         // get variable type name
//         std::string variable_type = std::get<3>(var_tuple);
//         // initialize temp var
//         TempVariable temp_var;
//         // assign fields
//         temp_var.name = std::move(name);
//         // assign unit
//         temp_var.unit = std::move(unit);
//         // assign type
//         temp_var.variable_type = std::move(vtype);
//         // get variable value view
//         const auto& var_val = std::get<1>(var_tuple);
//         if (std::holds_alternative<View<double>>(var_val)) {
//             // create steps list
//             RealStepData steps;
//             // append first step view
//             steps.push_back(std::get<View<double>>(var_val));
//             // assign steps variant
//             temp_var.steps = std::move(steps);
//         }
//         else {
//             // create steps list for complex
//             ComplexStepData steps;
//             // append first step view
//             steps.push_back(std::get<View<std::complex<double>>>(var_val));
//             // assign steps variant
//             temp_var.steps = std::move(steps);
//         }
//         // append temp variable
//         temp_variables.push_back(std::move(temp_var));
//     }
//     // initialize abscissa offset
//     size_t abscissa_index_offset = first_block.num_points;
//
//     for (size_t b = 1; b < blocks.size(); ++b) {
//         // get block reference
//         const auto& block = blocks[b];
//         // loop variables
//         for (size_t idx = 0; idx < block.variables.size(); ++idx) {
//             // get variable tuple reference
//             const auto& var_tuple = block.variables[idx];
//             // get name
//             std::string name = std::get<0>(var_tuple);
//             // get value view
//             const auto& var_val = std::get<1>(var_tuple);
//             if (idx >= temp_variables.size() || temp_variables[idx].name != name) {
//                 // un-map memory
//                 munmap(addr, length);
//                 // return null
//                 return nullptr;
//             }
//             // get temp var reference
//             auto& temp_var = temp_variables[idx];
//             if (std::holds_alternative<View<double>>(var_val)) {
//                 // append real view
//                 std::get<RealStepData>(temp_var.steps).push_back(std::get<View<double>>(var_val));
//             }
//             else {
//                 // append complex view
//                 std::get<ComplexStepData>(temp_var.steps).push_back(std::get<View<std::complex<double>>>(var_val));
//             }
//         }
//         if (block.num_points > 0) {
//             // get abscissa tuple reference
//             const auto& abscissa_var = block.variables[0];
//             // get view reference
//             const auto& view_var = std::get<1>(abscissa_var);
//             // get first value
//             double first_val = std::get<View<double>>(view_var)[0];
//             // get last value
//             double last_val = std::get<View<double>>(view_var)[block.num_points - 1];
//             // append range
//             abscissa_value_ranges.emplace_back(first_val, last_val);
//         }
//         else {
//             // append empty range
//             abscissa_value_ranges.emplace_back(0.0, 0.0);
//         }
//         // append index pair
//         abscissa_indices.emplace_back(abscissa_index_offset, abscissa_index_offset + block.num_points);
//         // advance offset
//         abscissa_index_offset += block.num_points;
//     }
//     // create step information
//     StepInformation step_information(std::move(step_keys), std::move(step_values), std::move(abscissa_value_ranges));
//     // initialize expressions list
//     std::vector<Expression> expressions;
//     // reserve memory
//     expressions.reserve(temp_variables.size());
//     // loop variables
//     for (auto& [name, steps, unit, variable_type] : temp_variables) {
//         // append expression
//         expressions.emplace_back(std::move(name), std::move(steps), std::move(unit), std::nullopt, std::move(variable_type));
//     }
//     // scale abscissa
//     Expression abscissa = process_abscissa_scale(expressions[0], abscissa_scale);
//     // update list
//     expressions[0] = abscissa;
//     // initialize optional slices
//     std::optional<std::vector<std::pair<size_t, size_t>>> step_slices = std::nullopt;
//     if (step_count > 1) {
//         // assign step slices
//         step_slices = std::move(abscissa_indices);
//     }
//     // create expression manager
//     ExpressionManager expression_manager(std::move(expressions), std::move(step_slices));
//     // initialize title
//     std::string title;
//     if (first_block.headers.contains("Title")) {
//         // assign title
//         title = first_block.headers.at("Title");
//     }
//     // return shared pointer
//     return std::make_shared<XyceOutputFile>(filename, std::move(title), first_block.is_complex, std::move(step_information), std::move(abscissa), abscissa_scale, std::move(expression_manager), addr, length);
// }
