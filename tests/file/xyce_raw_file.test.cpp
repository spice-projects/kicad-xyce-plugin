// #include <complex>
// #include <gtest/gtest.h>
// #include <fstream>
// #include <filesystem>
// #include <sstream>
// #include <string>
// #include <vector>
// #include <optional>
// #include <chrono>

// #include "../../src/expression/expression.h"
// #include "../../src/expression/expression_manager.h"
// #include "../../src/file/xyce_raw_file.h"

// // temp file manager helper class
// class TempFileRAII {
// public:
//     // constructor
//     explicit TempFileRAII(const std::string& content) {
//         static int counter = 0;
//         // build unique path
//         m_path = std::filesystem::temp_directory_path() / ("test_xyce_gtest_" + std::to_string(counter++) + "_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + ".raw");
//         // open file stream
//         std::ofstream out(m_path, std::ios::binary);
//         // write content
//         out.write(content.data(), static_cast<std::streamsize>(content.size()));
//         // close file
//         out.close();
//     }

//     // destructor
//     ~TempFileRAII() {
//         if (std::filesystem::exists(m_path)) {
//             // delete temporary file
//             std::filesystem::remove(m_path);
//         }
//     }

//     // path getter
//     [[nodiscard]] const std::filesystem::path& path() const {
//         // return path
//         return m_path;
//     }

// private:
//     // path field
//     std::filesystem::path m_path;
// };

// // variable definition helper structure
// struct TestVarDef {
//     // index field
//     int index;
//     // name field
//     std::string name;
//     // type field
//     std::string type;
// };

// // helper to generate raw bytes
// std::string make_raw_bytes(
//     const std::string& title = "Test Circuit",
//     const std::string& plotname = "Transient Analysis",
//     const std::string& flags = "real",
//     const std::vector<TestVarDef>& variable_defs = {{0, "time", "time"}, {1, "V(1)", "voltage"}},
//     const std::vector<std::vector<double>>& data_matrix = {{0.0, 0.0}, {1e-9, 1.0}},
//     bool is_ascii = false,
//     std::optional<size_t> num_points_override = std::nullopt)
// {
//     // get variables count
//     size_t num_variables = variable_defs.size();
//     // get points count
//     size_t num_points = num_points_override.value_or(data_matrix.size());
//     // initialize stream
//     std::ostringstream ss;
//     // write title
//     ss << "Title: " << title << "\n";
//     // write plotname
//     ss << "Plotname: " << plotname << "\n";
//     // write flags
//     ss << "Flags: " << flags << "\n";
//     // write variables count
//     ss << "No. Variables: " << num_variables << "\n";
//     // write points count
//     ss << "No. Points: " << num_points << "\n";
//     // write variables header
//     ss << "Variables:\n";
//     // loop variables
//     for (const auto&[index, name, type] : variable_defs) {
//         // write variable line
//         ss << "\t" << index << "\t" << name << "\t" << type << "\n";
//     }
//     if (is_ascii) {
//         // write values header
//         ss << "Values:\n";
//         for (size_t r = 0; r < data_matrix.size(); ++r) {
//             // write row index
//             ss << " " << r << "  ";
//             const auto& row = data_matrix[r];
//             for (size_t col = 0; col < row.size(); ++col) {
//                 if (col > 0) {
//                     // write spacing
//                     ss << "  ";
//                 }
//                 // write value
//                 ss << row[col];
//             }
//             // write newline
//             ss << "\n";
//         }
//         // return string
//         return ss.str();
//     }
//     // write binary header
//     ss << "Binary:\n";
//     std::string header = ss.str();
//     std::string payload;
//     for (const auto& row : data_matrix) {
//         for (double val : row) {
//             payload.append(reinterpret_cast<const char*>(&val), sizeof(double));
//         }
//     }
//     return header + payload;
// }

// // helper to generate multi block raw bytes
// std::string make_multi_block_raw_bytes(
//     const std::string& title = "Stepped Circuit",
//     const std::vector<TestVarDef>& variable_defs = {{0, "sweep", "voltage"}, {1, "V(2)", "voltage"}},
//     const std::vector<std::vector<std::vector<double>>>& step_matrices = {
//         {{0.0, 0.5}, {1.0, 1.0}, {2.0, 1.5}, {3.0, 2.0}},
//         {{0.0, 1.0}, {1.0, 1.5}, {2.0, 2.0}, {3.0, 2.5}},
//         {{0.0, 1.5}, {1.0, 2.0}, {2.0, 2.5}, {3.0, 3.0}}
//     },
//     const std::string& param_name = "R1",
//     const std::vector<double>& param_values = {1000.0, 2000.0, 3000.0})
// {
//     size_t num_steps = step_matrices.size();
//     size_t num_variables = variable_defs.size();
//     std::string result;
//     for (size_t step_index = 0; step_index < num_steps; ++step_index) {
//         const auto& data_matrix = step_matrices[step_index];
//         double param_value = param_values[step_index];
//         size_t num_points = data_matrix.size();
//         std::ostringstream ss;
//         ss << "Title: " << title << "\n";
//         ss << "Plotname: Step Analysis: Step " << (step_index + 1) << " of " << num_steps << " params:  name = " << param_name << " value = " << param_value << "  DC transfer characteristic\n";
//         ss << "Flags: real\n";
//         ss << "No. Variables: " << num_variables << "\n";
//         ss << "No. Points: " << num_points << "\n";
//         ss << "Variables:\n";
//         for (const auto& var : variable_defs) {
//             ss << "\t" << var.index << "\t" << var.name << "\t" << var.type << "\n";
//         }
//         ss << "Binary:\n";
//         std::string header = ss.str();
//         std::string payload;
//         for (const auto& row : data_matrix) {
//             for (double val : row) {
//                 payload.append(reinterpret_cast<const char*>(&val), sizeof(double));
//             }
//         }
//         result += header + payload;
//     }
//     return result;
// }

// Expression<double>* evaluate_real(ExpressionManager& manager, const std::string& expression_name) {
//     // evaluate expression in manager
//     AnyExpression* expression = manager.evaluate(expression_name);
//     // cast to real expression
//     return expression ? std::get_if<Expression<double>>(expression) : nullptr;
// }

// Expression<std::complex<double>>* evaluate_complex(ExpressionManager& manager, const std::string& expression_name) {
//     // evaluate expression in manager
//     AnyExpression* expression = manager.evaluate(expression_name);
//     // cast to complex expression
//     return expression ? std::get_if<Expression<std::complex<double>>>(expression) : nullptr;
// }

// TEST(XyceRawFileTest, load_returns_nullptr_when_file_not_found) {
//     // arrange
//     const std::filesystem::path path = "/tmp/nonexistent_xyce_raw_file_abc123.raw";
//     // act
//     const auto result = xyce_raw_file_parser(path);
//     // assert
//     ASSERT_EQ(result, nullptr);
// }

// TEST(XyceRawFileTest, load_returns_nullptr_when_file_is_empty) {
//     // arrange
//     const TempFileRAII temp_file("");
//     // act
//     const auto result = xyce_raw_file_parser(temp_file.path());
//     // assert
//     ASSERT_EQ(result, nullptr);
// }

// TEST(XyceRawFileTest, load_real_binary_title) {
//     // arrange
//     const std::string content = make_raw_bytes("RC Circuit");
//     const TempFileRAII temp_file(content);
//     // act
//     const auto raw = xyce_raw_file_parser(temp_file.path());
//     // assert
//     ASSERT_NE(raw, nullptr);
//     ASSERT_EQ(raw->title(), "RC Circuit");
// }

// TEST(XyceRawFileTest, load_real_binary_filename) {
//     // arrange
//     const std::string content = make_raw_bytes();
//     const TempFileRAII temp_file(content);
//     // act
//     const auto raw = xyce_raw_file_parser(temp_file.path());
//     // assert
//     ASSERT_NE(raw, nullptr);
//     ASSERT_EQ(raw->filename(), temp_file.path());
// }

// TEST(XyceRawFileTest, load_real_binary_complex_flag_false) {
//     // arrange
//     // act
//     const std::string content = make_raw_bytes("RC Circuit", "Transient Analysis", "real");
//     const TempFileRAII temp_file(content);
//     const auto raw = xyce_raw_file_parser(temp_file.path());
//     // assert
//     ASSERT_NE(raw, nullptr);
//     ASSERT_FALSE(raw->is_complex());
// }

// TEST(XyceRawFileTest, load_complex_binary_complex_flag_true) {
//     // arrange
//     const std::vector<TestVarDef> variable_definitions = {{0, "frequency", "frequency"}, {1, "V(out)", "voltage"}};
//     const std::vector<std::vector<double>> data_matrix = {{1e3, 0.0, 0.5, 0.5}, {1e4, 0.0, 0.7, 0.3}};
//     const std::string content = make_raw_bytes("AC Sweep Test", "AC Analysis", "complex", variable_definitions, data_matrix);
//     const TempFileRAII temp_file(content);
//     // act
//     const auto raw = xyce_raw_file_parser(temp_file.path());
//     // assert
//     ASSERT_NE(raw, nullptr);
//     ASSERT_TRUE(raw->is_complex());
// }

// TEST(XyceRawFileTest, load_complex_binary_mixed_case_flag_true) {
//     // arrange
//     const std::vector<TestVarDef> variable_definitions = {{0, "frequency", "frequency"}, {1, "V(out)", "voltage"}};
//     const std::vector<std::vector<double>> data_matrix = {{1e3, 0.0, 0.5, 0.5}, {1e4, 0.0, 0.7, 0.3}};
//     const std::string content = make_raw_bytes("AC Sweep Test", "AC Analysis", "CoMpLeX", variable_definitions, data_matrix);
//     const TempFileRAII temp_file(content);
//     // act
//     const auto raw = xyce_raw_file_parser(temp_file.path());
//     // assert
//     ASSERT_NE(raw, nullptr);
//     ASSERT_TRUE(raw->is_complex());
// }

// TEST(XyceRawFileTest, load_real_binary_abscissa_values) {
//     // arrange
//     const std::vector<std::vector<double>> data_matrix = {{0.0, 1.0}, {1e-9, 1.1}, {2e-9, 1.2}};
//     const std::string content = make_raw_bytes("Test Circuit", "Transient Analysis", "real", {{0, "time", "time"}, {1, "V(1)", "voltage"}}, data_matrix);
//     const TempFileRAII temp_file(content);
//     // act
//     const auto raw = xyce_raw_file_parser(temp_file.path());
//     // assert
//     ASSERT_NE(raw, nullptr);
//     const auto& abscissa_data = raw->abscissa().step_data(0);
//     ASSERT_EQ(abscissa_data.size(), 3);
//     ASSERT_DOUBLE_EQ(abscissa_data[0], 0.0);
//     ASSERT_DOUBLE_EQ(abscissa_data[1], 1e-9);
//     ASSERT_DOUBLE_EQ(abscissa_data[2], 2e-9);
// }

// TEST(XyceRawFileTest, load_real_binary_abscissa_scale_is_linear) {
//     // arrange
//     const std::string content = make_raw_bytes();
//     const TempFileRAII temp_file(content);
//     // act
//     const auto raw = xyce_raw_file_parser(temp_file.path());
//     // assert
//     ASSERT_NE(raw, nullptr);
//     ASSERT_EQ(raw->abscissa_scale(), AbscissaScale::LINEAR);
// }

// TEST(XyceRawFileTest, load_real_binary_single_step) {
//     // arrange
//     const std::vector<std::vector<double>> data_matrix = {{0.0, 1.0}, {1e-9, 1.1}, {2e-9, 1.2}};
//     const std::string content = make_raw_bytes("RC Circuit", "Transient Analysis", "real", {{0, "time", "time"}, {1, "V(1)", "voltage"}}, data_matrix);
//     const TempFileRAII temp_file(content);
//     // act
//     const auto raw = xyce_raw_file_parser(temp_file.path());
//     // assert
//     ASSERT_NE(raw, nullptr);
//     ASSERT_EQ(raw->step_information().length(), 1);
// }

// TEST(XyceRawFileTest, load_chart_type_transient) {
//     // arrange
//     const std::string content = make_raw_bytes();
//     const TempFileRAII temp_file(content);
//     // act
//     const auto raw = xyce_raw_file_parser(temp_file.path());
//     // assert
//     ASSERT_NE(raw, nullptr);
//     ASSERT_EQ(raw->abscissa().variable_type(), "time");
// }

// TEST(XyceRawFileTest, load_chart_type_ac) {
//     // arrange
//     const std::vector<TestVarDef> variable_definitions = {{0, "frequency", "frequency"}, {1, "V(out)", "voltage"}};
//     const std::vector<std::vector<double>> data_matrix = {{1e3, 0.0, 0.5, 0.5}, {1e4, 0.0, 0.7, 0.3}};
//     const std::string content = make_raw_bytes("AC Sweep Test", "AC Analysis", "complex", variable_definitions, data_matrix);
//     const TempFileRAII temp_file(content);
//     // act
//     const auto raw = xyce_raw_file_parser(temp_file.path());
//     // assert
//     ASSERT_NE(raw, nullptr);
//     ASSERT_EQ(raw->abscissa().variable_type(), "frequency");
// }

// TEST(XyceRawFileTest, load_chart_type_dc) {
//     // arrange
//     const std::vector<TestVarDef> variable_definitions = {{0, "v(v-sweep)", "voltage"}, {1, "V(out)", "voltage"}};
//     const std::vector<std::vector<double>> data_matrix = {{0.0, 0.0}, {1.0, 0.5}, {2.0, 1.0}};
//     const std::string content = make_raw_bytes("DC Sweep Test", "DC transfer characteristic", "real", variable_definitions, data_matrix);
//     const TempFileRAII temp_file(content);
//     // act
//     const auto raw = xyce_raw_file_parser(temp_file.path());
//     // assert
//     ASSERT_NE(raw, nullptr);
//     ASSERT_EQ(raw->abscissa().variable_type(), "voltage");
// }

// TEST(XyceRawFileTest, load_binary_with_trailing_content_ignored) {
//     // arrange
//     std::string content = make_raw_bytes();
//     content += "\nSome extra CSV junk\n1,2,3\n4,5,6\n";
//     const TempFileRAII temp_file(content);
//     // act
//     const auto raw = xyce_raw_file_parser(temp_file.path());
//     // assert
//     ASSERT_NE(raw, nullptr);
//     ASSERT_EQ(raw->abscissa().step_data(0).size(), 2);
// }

// TEST(XyceRawFileTest, load_ascii_values_section) {
//     // arrange
//     const std::vector<std::vector<double>> data_matrix = {{0.0, 1.0}, {1e-9, 1.1}, {2e-9, 1.2}};
//     const std::string content = make_raw_bytes("RC Circuit", "Transient Analysis", "real", {{0, "time", "time"}, {1, "V(1)", "voltage"}}, data_matrix, true);
//     const TempFileRAII temp_file(content);
//     // act
//     const auto raw = xyce_raw_file_parser(temp_file.path());
//     // assert
//     ASSERT_NE(raw, nullptr);
//     const auto& abscissa_data = raw->abscissa().step_data(0);
//     ASSERT_EQ(abscissa_data.size(), 3);
//     ASSERT_DOUBLE_EQ(abscissa_data[0], 0.0);
//     ASSERT_DOUBLE_EQ(abscissa_data[1], 1e-9);
//     ASSERT_DOUBLE_EQ(abscissa_data[2], 2e-9);
// }

// TEST(XyceRawFileTest, load_ascii_values_variable_data_correct) {
//     // arrange
//     const std::vector<std::vector<double>> data_matrix = {{0.0, 1.0}, {1e-9, 1.5}, {2e-9, 2.0}};
//     const std::string content = make_raw_bytes("RC Circuit", "Transient Analysis", "real", {{0, "time", "time"}, {1, "V(out)", "voltage"}}, data_matrix, true);
//     const TempFileRAII temp_file(content);
//     // act
//     const auto raw = xyce_raw_file_parser(temp_file.path());
//     // assert
//     ASSERT_NE(raw, nullptr);
//     auto* v_out = evaluate_real(raw->expression_manager(), "V(out)");
//     ASSERT_NE(v_out, nullptr);
//     ASSERT_DOUBLE_EQ(v_out->step_data(0)[0], 1.0);
//     ASSERT_DOUBLE_EQ(v_out->step_data(0)[1], 1.5);
//     ASSERT_DOUBLE_EQ(v_out->step_data(0)[2], 2.0);
// }

// TEST(XyceRawFileTest, load_returns_nullptr_when_data_section_missing) {
//     // arrange
//     const std::string content = "Title: Test\nDate: Mon Jan 1 00:00:00 2024\nPlotname: Transient Analysis\nFlags: real\nNo. Variables: 2\nNo. Points: 1\nVariables:\n\t0\ttime\ttime\n\t1\tV(1)\tvoltage\n";
//     const TempFileRAII temp_file(content);
//     // act
//     const auto result = xyce_raw_file_parser(temp_file.path());
//     // assert
//     ASSERT_EQ(result, nullptr);
// }

// TEST(XyceRawFileTest, load_skips_malformed_variable_lines) {
//     // arrange
//     const std::string header = "Title: Test\nDate: Mon Jan 1 00:00:00 2024\nPlotname: Transient Analysis\nFlags: real\nNo. Variables: 2\nNo. Points: 1\nVariables:\n\t0\ttime\ttime\n\tBAD LINE\n\t1\tV(1)\tvoltage\nBinary:\n";
//     const std::vector<double> row = {0.0, 1.0};
//     std::string content = header;
//     for (double val : row) {
//         content.append(reinterpret_cast<const char*>(&val), sizeof(double));
//     }
//     const TempFileRAII temp_file(content);
//     // act
//     const auto result = xyce_raw_file_parser(temp_file.path());
//     // assert
//     ASSERT_NE(result, nullptr);
//     ASSERT_EQ(result->expression_manager().expressions().size(), 2);
// }

// TEST(XyceRawFileTest, load_returns_nullptr_when_variable_count_header_mismatch) {
//     // arrange
//     const std::string content = "Title: Test\nDate: Mon Jan 1 00:00:00 2024\nPlotname: Transient Analysis\nFlags: real\nNo. Variables: 3\nNo. Points: 1\nVariables:\n\t0\ttime\ttime\n\t1\tV(1)\tvoltage\nBinary:\n";
//     const TempFileRAII temp_file(content);
//     // act
//     const auto result = xyce_raw_file_parser(temp_file.path());
//     // assert
//     ASSERT_EQ(result, nullptr);
// }

// TEST(XyceRawFileTest, load_returns_nullptr_when_binary_payload_is_truncated) {
//     // arrange
//     const std::string header = "Title: Test\nPlotname: Transient Analysis\nFlags: real\nNo. Variables: 2\nNo. Points: 2\nVariables:\n\t0\ttime\ttime\n\t1\tV(1)\tvoltage\nBinary:\n";
//     const std::vector<double> one_row = {0.0, 1.0};
//     std::string content = header;
//     for (double val : one_row) {
//         // append payload value
//         content.append(reinterpret_cast<const char*>(&val), sizeof(double));
//     }
//     const TempFileRAII temp_file(content);
//     // act
//     const auto result = xyce_raw_file_parser(temp_file.path());
//     // assert
//     ASSERT_EQ(result, nullptr);
// }

// TEST(XyceRawFileTest, load_returns_nullptr_when_ascii_parse_produces_no_data) {
//     // arrange
//     const std::string content = "Title: Test\nDate: Mon Jan 1 00:00:00 2024\nPlotname: Transient Analysis\nFlags: real\nNo. Variables: 2\nNo. Points: 0\nVariables:\n\t0\ttime\ttime\n\t1\tV(1)\tvoltage\nValues:\nNO_NUMERIC_DATA\n";
//     const TempFileRAII temp_file(content);
//     // act
//     const auto result = xyce_raw_file_parser(temp_file.path());
//     // assert
//     ASSERT_EQ(result, nullptr);
// }

// TEST(XyceRawFileTest, load_returns_nullptr_when_file_has_no_trailing_newline) {
//     // arrange
//     const std::string content = "Title: Test\nDate: Mon Jan 1 00:00:00 2024";
//     const TempFileRAII temp_file(content);
//     // act
//     const auto result = xyce_raw_file_parser(temp_file.path());
//     // assert
//     ASSERT_EQ(result, nullptr);
// }

// TEST(XyceRawFileTest, load_expression_manager_contains_all_variables) {
//     // arrange
//     const std::vector<TestVarDef> variable_definitions = {{0, "time", "time"}, {1, "V(1)", "voltage"}, {2, "I(R1)", "current"}};
//     const std::vector<std::vector<double>> data_matrix = {{0.0, 1.0, 2.0}, {1e-9, 1.1, 2.1}};
//     const std::string content = make_raw_bytes("RC Circuit", "Transient Analysis", "real", variable_definitions, data_matrix);
//     const TempFileRAII temp_file(content);
//     // act
//     const auto raw = xyce_raw_file_parser(temp_file.path());
//     // assert
//     ASSERT_NE(raw, nullptr);
//     ASSERT_NE(raw->expression_manager().evaluate("V(1)"), nullptr);
//     ASSERT_NE(raw->expression_manager().evaluate("I(R1)"), nullptr);
//     ASSERT_NE(raw->expression_manager().evaluate("time"), nullptr);
// }

// TEST(XyceRawFileTest, load_unknown_variable_type_still_loaded) {
//     // arrange
//     const std::vector<TestVarDef> variable_definitions = {{0, "time", "time"}, {1, "CUSTOM_SIG", "custom_type"}};
//     const std::vector<std::vector<double>> data_matrix = {{0.0, 5.0}, {1.0, 6.0}};
//     const std::string content = make_raw_bytes("Circuit", "Transient", "real", variable_definitions, data_matrix);
//     const TempFileRAII temp_file(content);
//     // act
//     const auto raw = xyce_raw_file_parser(temp_file.path());
//     // assert
//     ASSERT_NE(raw, nullptr);
//     ASSERT_NE(raw->expression_manager().evaluate("CUSTOM_SIG"), nullptr);
// }

// TEST(XyceRawFileTest, load_step_information_abscissa_range) {
//     // arrange
//     const std::vector<std::vector<double>> data_matrix = {{0.0, 1.0}, {1e-9, 1.1}, {2e-9, 1.2}};
//     const std::string content = make_raw_bytes("RC Circuit", "Transient Analysis", "real", {{0, "time", "time"}, {1, "V(1)", "voltage"}}, data_matrix);
//     const TempFileRAII temp_file(content);
//     // act
//     const auto raw = xyce_raw_file_parser(temp_file.path());
//     // assert
//     ASSERT_NE(raw, nullptr);
//     ASSERT_DOUBLE_EQ(raw->step_information().step_abscissa_left_value(0), 0.0);
//     ASSERT_DOUBLE_EQ(raw->step_information().step_abscissa_right_value(0), 2e-9);
// }

// TEST(XyceRawFileTest, load_utf8_encoded_header) {
//     // arrange
//     const std::string content = make_raw_bytes("RC Schéma");
//     const TempFileRAII temp_file(content);
//     // act
//     const auto raw = xyce_raw_file_parser(temp_file.path());
//     // assert
//     ASSERT_NE(raw, nullptr);
//     ASSERT_EQ(raw->title(), "RC Schéma");
// }

// TEST(XyceRawFileTest, load_complex_ac_abscissa_is_frequency) {
//     // arrange
//     const std::vector<TestVarDef> variable_definitions = {{0, "frequency", "frequency"}, {1, "V(out)", "voltage"}};
//     const std::vector<std::vector<double>> data_matrix = {{1e3, 0.0, 1.0, 0.0}, {1e4, 0.0, 0.7, 0.7}, {1e5, 0.0, 0.0, 1.0}};
//     const std::string content = make_raw_bytes("AC Sweep Test", "AC Analysis", "complex", variable_definitions, data_matrix);
//     const TempFileRAII temp_file(content);
//     // act
//     const auto raw = xyce_raw_file_parser(temp_file.path());
//     // assert
//     ASSERT_NE(raw, nullptr);
//     const auto& abscissa_data = raw->abscissa().step_data(0);
//     ASSERT_EQ(abscissa_data.size(), 3);
//     ASSERT_DOUBLE_EQ(abscissa_data[0], 1e3);
//     ASSERT_DOUBLE_EQ(abscissa_data[1], 1e4);
//     ASSERT_DOUBLE_EQ(abscissa_data[2], 1e5);
// }

// TEST(XyceRawFileTest, load_complex_ac_signal_is_complex) {
//     // arrange
//     const std::vector<TestVarDef> variable_definitions = {{0, "frequency", "frequency"}, {1, "V(out)", "voltage"}};
//     const std::vector<std::vector<double>> data_matrix = {{1e3, 0.0, 0.5, 0.5}, {1e4, 0.0, 0.7, 0.3}};
//     const std::string content = make_raw_bytes("AC Sweep Test", "AC Analysis", "complex", variable_definitions, data_matrix);
//     const TempFileRAII temp_file(content);
//     // act
//     const auto raw = xyce_raw_file_parser(temp_file.path());
//     // assert
//     ASSERT_NE(raw, nullptr);
//     auto* v_out = evaluate_complex(raw->expression_manager(), "V(out)");
//     ASSERT_NE(v_out, nullptr);
//     ASSERT_DOUBLE_EQ(v_out->step_data(0)[0].real(), 0.5);
//     ASSERT_DOUBLE_EQ(v_out->step_data(0)[0].imag(), 0.5);
// }

// TEST(XyceRawFileTest, load_ascii_complex_ac) {
//     // arrange
//     // act
//     const std::vector<TestVarDef> variable_definitions = {{0, "frequency", "frequency"}, {1, "V(out)", "voltage"}};
//     const std::vector<std::vector<double>> data_matrix = {{1e3, 0.0, 0.5, 0.5}, {1e4, 0.0, 0.7, 0.3}};
//     const std::string content = make_raw_bytes("AC Sweep Test", "AC Analysis", "complex", variable_definitions, data_matrix, true);
//     const TempFileRAII temp_file(content);
//     const auto raw = xyce_raw_file_parser(temp_file.path());
//     // assert
//     ASSERT_NE(raw, nullptr);
//     ASSERT_TRUE(raw->is_complex());
//     ASSERT_EQ(raw->abscissa().variable_type(), "frequency");
//     ASSERT_EQ(raw->abscissa().step_data(0).size(), 2);
// }

// TEST(XyceRawFileTest, load_ascii_no_points_zero_reads_all) {
//     // arrange
//     // act
//     const std::vector<std::vector<double>> data_matrix = {{0.0, 1.0}, {1e-9, 1.1}, {2e-9, 1.2}};
//     const std::string content = make_raw_bytes("RC Circuit", "Transient Analysis", "real", {{0, "time", "time"}, {1, "V(1)", "voltage"}}, data_matrix, true, 0);
//     const TempFileRAII temp_file(content);
//     const auto raw = xyce_raw_file_parser(temp_file.path());
//     // assert
//     ASSERT_NE(raw, nullptr);
//     ASSERT_EQ(raw->abscissa().step_data(0).size(), 3);
// }

// TEST(XyceRawFileTest, multi_block_step_count) {
//     // arrange
//     const std::string content = make_multi_block_raw_bytes("Stepped Circuit", {{0, "sweep", "voltage"}, {1, "V(2)", "voltage"}}, {
//         {{0.0, 1.0}, {1.0, 2.0}},
//         {{0.0, 1.5}, {1.0, 2.5}}
//     }, "R1", {1000.0, 2000.0});
//     const TempFileRAII temp_file(content);
//     // act
//     const auto raw = xyce_raw_file_parser(temp_file.path());
//     // assert
//     ASSERT_NE(raw, nullptr);
//     ASSERT_EQ(raw->step_information().length(), 2);
// }

// TEST(XyceRawFileTest, multi_block_three_steps) {
//     // arrange
//     const std::string content = make_multi_block_raw_bytes();
//     const TempFileRAII temp_file(content);
//     // act
//     const auto raw = xyce_raw_file_parser(temp_file.path());
//     // assert
//     ASSERT_NE(raw, nullptr);
//     ASSERT_EQ(raw->step_information().length(), 3);
// }

// TEST(XyceRawFileTest, multi_block_step_parameter_keys) {
//     // arrange
//     const std::string content = make_multi_block_raw_bytes("Stepped Circuit", {{0, "sweep", "voltage"}, {1, "V(2)", "voltage"}}, {
//         {{0.0, 1.0}, {1.0, 2.0}},
//         {{0.0, 1.5}, {1.0, 2.5}}
//     }, "R1_VAL", {1000.0, 2000.0});
//     const TempFileRAII temp_file(content);
//     // act
//     const auto raw = xyce_raw_file_parser(temp_file.path());
//     // assert
//     ASSERT_NE(raw, nullptr);
//     ASSERT_EQ(raw->step_information().keys().size(), 1);
//     ASSERT_EQ(raw->step_information().keys()[0], "R1_VAL");
// }

// TEST(XyceRawFileTest, multi_block_step_parameter_values) {
//     // arrange
//     const std::string content = make_multi_block_raw_bytes("Stepped Circuit", {{0, "sweep", "voltage"}, {1, "V(2)", "voltage"}}, {
//         {{0.0, 1.0}, {1.0, 2.0}},
//         {{0.0, 1.5}, {1.0, 2.5}}
//     }, "R1", {1000.0, 2000.0});
//     const TempFileRAII temp_file(content);
//     // act
//     const auto raw = xyce_raw_file_parser(temp_file.path());
//     // assert
//     ASSERT_NE(raw, nullptr);
//     ASSERT_EQ(raw->step_information().values().size(), 2);
//     ASSERT_DOUBLE_EQ(raw->step_information().values()[0][0], 1000.0);
//     ASSERT_DOUBLE_EQ(raw->step_information().values()[1][0], 2000.0);
// }

// TEST(XyceRawFileTest, multi_block_expression_step_count) {
//     // arrange
//     // act
//     const std::vector<std::vector<double>> matrix = {{0.0, 1.0}, {1.0, 2.0}, {2.0, 3.0}};
//     const std::string content = make_multi_block_raw_bytes("Stepped Circuit", {{0, "sweep", "voltage"}, {1, "V(2)", "voltage"}}, {matrix, matrix}, "R1", {1000.0, 2000.0});
//     const TempFileRAII temp_file(content);
//     const auto raw = xyce_raw_file_parser(temp_file.path());
//     // assert
//     ASSERT_NE(raw, nullptr);
//     ASSERT_EQ(raw->abscissa().step_count(), 2);
// }

// TEST(XyceRawFileTest, multi_block_step_data_correct_values) {
//     // arrange
//     // act
//     const std::vector<std::vector<double>> m0 = {{0.0, 1.0}, {1.0, 2.0}, {2.0, 3.0}};
//     const std::vector<std::vector<double>> m1 = {{0.0, 4.0}, {1.0, 5.0}, {2.0, 6.0}};
//     const std::string content = make_multi_block_raw_bytes("Stepped Circuit", {{0, "sweep", "voltage"}, {1, "V(2)", "voltage"}}, {m0, m1}, "R1", {1000.0, 2000.0});
//     const TempFileRAII temp_file(content);
//     const auto raw = xyce_raw_file_parser(temp_file.path());
//     // assert
//     ASSERT_NE(raw, nullptr);
//     auto* v2_expr = evaluate_real(raw->expression_manager(), "V(2)");
//     ASSERT_NE(v2_expr, nullptr);
//     ASSERT_DOUBLE_EQ(v2_expr->step_data(0)[0], 1.0);
//     ASSERT_DOUBLE_EQ(v2_expr->step_data(0)[1], 2.0);
//     ASSERT_DOUBLE_EQ(v2_expr->step_data(0)[2], 3.0);
//     ASSERT_DOUBLE_EQ(v2_expr->step_data(1)[0], 4.0);
//     ASSERT_DOUBLE_EQ(v2_expr->step_data(1)[1], 5.0);
//     ASSERT_DOUBLE_EQ(v2_expr->step_data(1)[2], 6.0);
// }

// TEST(XyceRawFileTest, multi_block_abscissa_step_data_zero_copy) {
//     // arrange
//     // act
//     const std::vector<std::vector<double>> m0 = {{0.0, 1.0}, {1.0, 2.0}};
//     const std::vector<std::vector<double>> m1 = {{0.0, 3.0}, {1.0, 4.0}};
//     const std::string content = make_multi_block_raw_bytes("Stepped Circuit", {{0, "sweep", "voltage"}, {1, "V(2)", "voltage"}}, {m0, m1}, "R1", {1000.0, 2000.0});
//     const TempFileRAII temp_file(content);
//     const auto raw = xyce_raw_file_parser(temp_file.path());
//     // assert
//     ASSERT_NE(raw, nullptr);
//     ASSERT_EQ(raw->abscissa().step_count(), 2);
//     ASSERT_EQ(raw->abscissa().step_data(0).size(), 2);
//     ASSERT_EQ(raw->abscissa().step_data(1).size(), 2);
// }

// TEST(XyceRawFileTest, multi_block_data_property_concatenates) {
//     // arrange
//     // act
//     const std::vector<std::vector<double>> m0 = {{0.0, 1.0}, {1.0, 2.0}};
//     const std::vector<std::vector<double>> m1 = {{0.0, 3.0}, {1.0, 4.0}};
//     const std::string content = make_multi_block_raw_bytes("Stepped Circuit", {{0, "sweep", "voltage"}, {1, "V(2)", "voltage"}}, {m0, m1}, "R1", {1000.0, 2000.0});
//     const TempFileRAII temp_file(content);
//     const auto raw = xyce_raw_file_parser(temp_file.path());
//     // assert
//     ASSERT_NE(raw, nullptr);
//     auto* v2_expr = evaluate_real(raw->expression_manager(), "V(2)");
//     ASSERT_NE(v2_expr, nullptr);
//     ASSERT_EQ(v2_expr->data().size(), 4);
//     ASSERT_DOUBLE_EQ(v2_expr->data()[0], 1.0);
//     ASSERT_DOUBLE_EQ(v2_expr->data()[1], 2.0);
//     ASSERT_DOUBLE_EQ(v2_expr->data()[2], 3.0);
//     ASSERT_DOUBLE_EQ(v2_expr->data()[3], 4.0);
// }

// TEST(XyceRawFileTest, multi_block_title_from_first_block) {
//     // arrange
//     // act
//     const std::string content = make_multi_block_raw_bytes("DC Stepped Test");
//     const TempFileRAII temp_file(content);
//     const auto raw = xyce_raw_file_parser(temp_file.path());
//     // assert
//     ASSERT_NE(raw, nullptr);
//     ASSERT_EQ(raw->title(), "DC Stepped Test");
// }

// TEST(XyceRawFileTest, multi_block_abscissa_value_ranges) {
//     // arrange
//     // act
//     const std::vector<std::vector<double>> m = {{0.0, 1.0}, {1.0, 2.0}, {2.0, 3.0}};
//     const std::string content = make_multi_block_raw_bytes("Stepped Circuit", {{0, "sweep", "voltage"}, {1, "V(2)", "voltage"}}, {m, m}, "R1", {1000.0, 2000.0});
//     const TempFileRAII temp_file(content);
//     const auto raw = xyce_raw_file_parser(temp_file.path());
//     // assert
//     ASSERT_NE(raw, nullptr);
//     ASSERT_DOUBLE_EQ(raw->step_information().step_abscissa_left_value(0), 0.0);
//     ASSERT_DOUBLE_EQ(raw->step_information().step_abscissa_right_value(0), 2.0);
//     ASSERT_DOUBLE_EQ(raw->step_information().step_abscissa_left_value(1), 0.0);
//     ASSERT_DOUBLE_EQ(raw->step_information().step_abscissa_right_value(1), 2.0);
// }

// TEST(XyceRawFileTest, ascii_values_with_blank_lines) {
//     // arrange
//     // act
//     const std::string content = "Title: Test Circuit\nPlotname: Transient Analysis\nFlags: real\nNo. Variables: 2\nNo. Points: 2\nVariables:\n\t0\ttime\ttime\n\t1\tV(1)\tvoltage\nValues:\n\n 0  0.0  1.0\n\n   \n 1  1.0  2.0\n";
//     const TempFileRAII temp_file(content);
//     const auto raw = xyce_raw_file_parser(temp_file.path());
//     // assert
//     ASSERT_NE(raw, nullptr);
//     ASSERT_EQ(raw->abscissa().step_data(0).size(), 2);
// }

// TEST(XyceRawFileTest, ascii_values_unexpected_index) {
//     // arrange
//     // act
//     const std::string content = "Title: Test Circuit\nPlotname: Transient Analysis\nFlags: real\nNo. Variables: 2\nNo. Points: 2\nVariables:\n\t0\ttime\ttime\n\t1\tV(1)\tvoltage\nValues:\n 0  0.0  1.0\n 2  1.0  2.0\n";
//     const TempFileRAII temp_file(content);
//     const auto raw = xyce_raw_file_parser(temp_file.path());
//     // assert
//     ASSERT_EQ(raw, nullptr);
// }

// TEST(XyceRawFileTest, ascii_values_invalid_token_count) {
//     // arrange
//     // act
//     const std::string content = "Title: Test Circuit\nPlotname: Transient Analysis\nFlags: real\nNo. Variables: 2\nNo. Points: 2\nVariables:\n\t0\ttime\ttime\n\t1\tV(1)\tvoltage\nValues:\n 0  0.0  1.0\n 1  1.0\n";
//     const TempFileRAII temp_file(content);
//     const auto raw = xyce_raw_file_parser(temp_file.path());
//     // assert
//     ASSERT_EQ(raw, nullptr);
// }

// TEST(XyceRawFileTest, ascii_values_parsing_exception) {
//     // arrange
//     // act
//     const std::string content = "Title: Test Circuit\nPlotname: Transient Analysis\nFlags: real\nNo. Variables: 2\nNo. Points: 2\nVariables:\n\t0\ttime\ttime\n\t1\tV(1)\tvoltage\nValues:\n 0  0.0  1.0\n 1  abc  2.0\n";
//     const TempFileRAII temp_file(content);
//     const auto raw = xyce_raw_file_parser(temp_file.path());
//     // assert
//     ASSERT_EQ(raw, nullptr);
// }

// TEST(XyceRawFileTest, ascii_values_point_count_mismatch) {
//     // arrange
//     // act
//     const std::string content = "Title: Test Circuit\nPlotname: Transient Analysis\nFlags: real\nNo. Variables: 2\nNo. Points: 3\nVariables:\n\t0\ttime\ttime\n\t1\tV(1)\tvoltage\nValues:\n 0  0.0  1.0\n 1  1.0  2.0\n";
//     const TempFileRAII temp_file(content);
//     const auto raw = xyce_raw_file_parser(temp_file.path());
//     // assert
//     ASSERT_EQ(raw, nullptr);
// }

// TEST(XyceRawFileTest, multi_block_variables_mismatch) {
//     // arrange
//     // act
//     const std::string content0 = make_multi_block_raw_bytes("Stepped Circuit", {{0, "sweep", "voltage"}, {1, "V(2)", "voltage"}}, { {{0.0, 1.0}, {1.0, 2.0}}    }, "R1", {1000.0});
//     const std::string content1 = make_multi_block_raw_bytes("Stepped Circuit", {{0, "sweep", "voltage"}, {1, "V(3)", "voltage"}}, {        {{0.0, 1.5}, {1.0, 2.5}}    }, "R1", {2000.0});
//     const size_t marker = content1.find("Title: Stepped Circuit");
//     const std::string content = content0 + content1.substr(marker);
//     const TempFileRAII temp_file(content);
//     const auto raw = xyce_raw_file_parser(temp_file.path());
//     // assert
//     ASSERT_EQ(raw, nullptr);
// }
