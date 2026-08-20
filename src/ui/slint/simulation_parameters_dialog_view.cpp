#include <algorithm>
#include <array>
#include <charconv>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include <slint.h>

#include <simulation_parameters_dialog.h>

#include "../../simulation_parameters/ac_simulation_parameters.h"
#include "../../simulation_parameters/dc_simulation_parameters.h"
#include "../../simulation_parameters/fft_parameters.h"
#include "../../simulation_parameters/four_parameters.h"
#include "../../simulation_parameters/hb_simulation_parameters.h"
#include "../../simulation_parameters/lin_simulation_parameters.h"
#include "../../simulation_parameters/measure_parameters.h"
#include "../../simulation_parameters/noise_simulation_parameters.h"
#include "../../simulation_parameters/op_simulation_parameters.h"
#include "../../simulation_parameters/print_parameters.h"
#include "../../simulation_parameters/simulation_config.h"
#include "../../simulation_parameters/transient_simulation_parameters.h"
#include "../../util.h"
#include "../main_window_view_def.h"
#include "simulation_parameters_dialog_view.h"

namespace simulation_parameters_dialog_view
{
    namespace
    {
        // analysis type string per sidebar tab index, matching the wx page order
        constexpr std::array<const char*, 7> ANALYSIS_TYPES = {"OP", "TRAN", "DC", "AC", "NOISE", "HB", "LIN"};

        // join a range of strings with a delimiter; util.h has no join() helper
        [[nodiscard]] std::string join(const std::vector<std::string>& parts, std::string_view delimiter) {
            std::string result;
            for (size_t i = 0; i < parts.size(); ++i) {
                if (i != 0)
                    result += delimiter;
                result += parts[i];
            }
            return result;
        }

        // map an analysis type to its sidebar tab index; defaults to the OP tab
        [[nodiscard]] int tab_index_for(const std::string& analysis_type) {
            for (size_t i = 0; i < ANALYSIS_TYPES.size(); ++i) {
                if (analysis_type == ANALYSIS_TYPES[i])
                    return static_cast<int>(i);
            }
            return 0;
        }

        // sidebar tab index of each analysis, matching the wx page order
        static constexpr int PAGE_OP = 0;
        static constexpr int PAGE_TRAN = 1;
        static constexpr int PAGE_DC = 2;
        static constexpr int PAGE_AC = 3;
        static constexpr int PAGE_NOISE = 4;
        static constexpr int PAGE_HB = 5;
        static constexpr int PAGE_LIN = 6;

        // print format combo model indexed by the .PRINT format combo position;
        // index 0 is the "(default)" entry which serializes to an empty string
        static constexpr std::array<const char*, 9> PRINT_FORMAT_VALUES = {"", "STD", "NOINDEX", "PROBE", "TECPLOT", "RAW", "CSV", "GNUPLOT", "SPLOT"};

        // AC/LIN/NOISE sweep mode values matching the wx combo order
        static constexpr std::array<const char*, 4> SWEEP_MODE_VALUES = {"LIN", "DEC", "OCT", "DATA"};

        // DC sweep mode values matching the wx DcParametersPanel combo order
        static constexpr std::array<const char*, 5> DC_SWEEP_MODE_VALUES = {"LIN", "DEC", "OCT", "LIST", "DATA"};

        // OP keyword values matching the wx TransientParametersPanel combo order
        static constexpr std::array<const char*, 3> OP_KEYWORD_VALUES = {"", "NOOP", "UIC"};

        // LIN panel choice models matching the wx LinParametersPanel combo order
        static constexpr std::array<const char*, 4> LIN_FORMAT_CHOICES = {"TOUCHSTONE2", "TOUCHSTONE1", "CITIFILE", "TSI"};
        static constexpr std::array<const char*, 5> LIN_LINTYPE_CHOICES = {"S", "Y", "Z", "G", "H"};
        static constexpr std::array<const char*, 3> LIN_DATAFORMAT_CHOICES = {"RI", "MA", "DB"};

        // .PRINT type combo models matching the wx PrintSectionPanel combo order;
        // index 0 is the analysis prefix fallback type
        static constexpr std::array<const char*, 2> TRAN_PRINT_TYPES = {"TRAN", "TRANADJOINT"};
        static constexpr std::array<const char*, 2> DC_PRINT_TYPES = {"DC", "HOMOTOPY"};
        static constexpr std::array<const char*, 1> NOISE_PRINT_TYPES = {"NOISE"};
        static constexpr std::array<const char*, 3> HB_PRINT_TYPES = {"HB", "HB_FD", "HB_TD"};
        static constexpr std::array<const char*, 1> LIN_PRINT_TYPES = {"AC"};
        static constexpr std::array<const char*, 2> AC_PRINT_TYPES = {"AC", "AC_IC"};
        // empty model for analyses without a print-type combo (index 0 is unused)
        static constexpr std::array<const char*, 0> NO_PRINT_TYPES = {};

        // HB TAHB combo values matching the wx HbParametersPanel combo order;
        // index 0 is the "(None)" entry
        static constexpr std::array<int, 7> TAHB_VALUES = {-1, 0, 1, 2, 5, 10, 20};

        // HB SELECTHARMS combo values matching the wx HbParametersPanel combo
        // order; index 0 is the "(None)" entry
        static constexpr std::array<const char*, 7> SELECTHARMS_VALUES = {"", "ALL", "1", "2", "3", "5", "10"};

        // wildcard tokens shared by the BJT and FET lead groups
        static const std::set<std::string> WILDCARD_TOKENS = {"V(*)", "I(*)", "P(*)", "W(*)", "IB(*)", "IC(*)", "IE(*)", "IS(*)", "ID(*)", "IG(*)"};

        // ordered BJT lead current wildcards, matching the wx PrintSectionPanel
        static const std::vector<std::string> BJT_WILDCARDS = {"IB(*)", "IC(*)", "IE(*)", "IS(*)"};
        // ordered FET lead current wildcards, matching the wx PrintSectionPanel
        static const std::vector<std::string> FET_WILDCARDS = {"IB(*)", "ID(*)", "IG(*)", "IS(*)"};

        using DialogHandle = slint::ComponentHandle<simulation_parameters_dialog::SimulationParametersDialog>;

        // resolve a model value to its combo index (case-insensitive); falls
        // back to the first entry when not found
        template <size_t N>
        [[nodiscard]] int choice_index_for(const std::array<const char*, N>& model, std::string_view value) {
            const std::string upper = to_upper(value);
            for (size_t i = 0; i < N; ++i) {
                if (to_upper(model[i]) == upper)
                    return static_cast<int>(i);
            }
            return 0;
        }

        // resolve a format string (eg. "STD") to its combo index; falls back to 0
        [[nodiscard]] int format_index_for_string(const std::string& format_str) { return choice_index_for(PRINT_FORMAT_VALUES, format_str); }

        // resolve a sweep mode string to its combo index; defaults to LIN
        [[nodiscard]] int sweep_mode_index_for(const std::string& sweep_mode) { return choice_index_for(SWEEP_MODE_VALUES, sweep_mode); }

        // resolve an OP keyword string to its combo index; defaults to "(None)"
        [[nodiscard]] int op_keyword_index_for(const std::string& keyword) { return choice_index_for(OP_KEYWORD_VALUES, keyword); }

        // true when any of the candidate tokens is present in the variable set
        [[nodiscard]] bool has_any_wildcard(const std::set<std::string>& var_set, std::initializer_list<const char*> candidates) {
            for (const char* c : candidates) {
                if (var_set.count(c))
                    return true;
            }
            return false;
        }

        // trim leading and trailing whitespace
        [[nodiscard]] std::string trim(std::string_view view) {
            const size_t begin = view.find_first_not_of(" \t\r\n");
            if (begin == std::string_view::npos)
                return "";
            const size_t end = view.find_last_not_of(" \t\r\n");
            return std::string(view.substr(begin, end - begin + 1));
        }

        // parse a leading integer, tolerating trailing junk like wx ToLong
        [[nodiscard]] std::optional<int> parse_int(std::string_view text) {
            const std::string value = trim(text);
            if (value.empty())
                return std::nullopt;
            int parsed = 0;
            const auto result = std::from_chars(value.data(), value.data() + value.size(), parsed);
            if (result.ec == std::errc{})
                return parsed;
            return std::nullopt;
        }

        // --- shared .PRINT section accessors ---
        // the print fields are duplicated across every analysis panel; these
        // bundles route the shared apply/build logic to the per-analysis
        // generated accessors

        struct PrintSetters
        {
            std::function<void(bool)> enabled;
            std::function<void(bool)> all_nodes;
            std::function<void(bool)> all_currents;
            std::function<void(bool)> power;
            std::function<void(bool)> bjt_leads;
            std::function<void(bool)> fet_leads;
            std::function<void(slint::SharedString)> specific_variables;
            std::function<void(int)> format_index;
            std::function<void(slint::SharedString)> output_file;
            std::function<void(int)> type_index;
        };

        struct PrintGetters
        {
            std::function<bool()> enabled;
            std::function<bool()> all_nodes;
            std::function<bool()> all_currents;
            std::function<bool()> power;
            std::function<bool()> bjt_leads;
            std::function<bool()> fet_leads;
            std::function<std::string()> specific_variables;
            std::function<int()> format_index;
            std::function<std::string()> output_file;
            std::function<int()> type_index;
        };

        // populate the dialog root's print fields from saved parameters
        template <size_t N>
        void apply_print_section(const std::optional<PrintParameters>& params, bool show_power, bool has_bjt, bool has_fet, bool has_type_combo, const std::array<const char*, N>& type_model, const PrintSetters& s) {
            if (!params) {
                s.enabled(false);
                return;
            }
            const auto& pp = *params;
            const std::set<std::string> var_set(pp.output_variables.begin(), pp.output_variables.end());
            s.enabled(true);
            s.all_nodes(var_set.count("V(*)"));
            s.all_currents(var_set.count("I(*)"));
            if (show_power)
                s.power(var_set.count("P(*)"));
            if (has_bjt)
                s.bjt_leads(has_any_wildcard(var_set, {"IC(*)", "IE(*)"}));
            if (has_fet)
                s.fet_leads(has_any_wildcard(var_set, {"ID(*)", "IG(*)"}));
            // non-wildcard tokens become the additional-variables text
            std::vector<std::string> specific;
            for (const auto& v : pp.output_variables) {
                if (WILDCARD_TOKENS.count(v) == 0)
                    specific.push_back(v);
            }
            s.specific_variables(slint::SharedString(join(specific, " ")));
            s.format_index(format_index_for_string(pp.print_format));
            s.output_file(slint::SharedString(pp.print_file));
            if (has_type_combo)
                s.type_index(choice_index_for(type_model, pp.print_type));
        }

        // read the dialog root's print fields into a PrintParameters model;
        // returns nullopt when the print section is disabled
        template <size_t N>
        [[nodiscard]] std::optional<PrintParameters> build_print_section(bool show_power, bool has_bjt, bool has_fet, bool has_type_combo, std::string_view default_type, const std::array<const char*, N>& type_model, const PrintGetters& g) {
            if (!g.enabled())
                return std::nullopt;
            std::vector<std::string> output_vars;
            if (g.all_nodes())
                output_vars.push_back("V(*)");
            if (g.all_currents())
                output_vars.push_back("I(*)");
            if (show_power && g.power())
                output_vars.push_back("P(*)");
            if (has_bjt && g.bjt_leads()) {
                for (const auto& w : BJT_WILDCARDS) {
                    if (std::find(output_vars.begin(), output_vars.end(), w) == output_vars.end())
                        output_vars.push_back(w);
                }
            }
            if (has_fet && g.fet_leads()) {
                for (const auto& w : FET_WILDCARDS) {
                    if (std::find(output_vars.begin(), output_vars.end(), w) == output_vars.end())
                        output_vars.push_back(w);
                }
            }
            // additional variables (tokenize trims whitespace and skips empties);
            // cast to string_view so the util overload wins over the parser's
            // global tokenize(const std::string&)
            for (const auto& tok : tokenize(std::string_view(g.specific_variables())))
                output_vars.emplace_back(std::string(tok));
            // format index -> format string (0 == default == empty)
            const int fmt_idx = g.format_index();
            const std::string print_format = (fmt_idx > 0 && fmt_idx < static_cast<int>(PRINT_FORMAT_VALUES.size())) ? PRINT_FORMAT_VALUES[static_cast<size_t>(fmt_idx)] : "";
            // the print type comes from the combo, or the analysis prefix when
            // the combo is hidden
            const std::string print_type = has_type_combo ? type_model[static_cast<size_t>(std::clamp(g.type_index(), 0, static_cast<int>(N) - 1))] : std::string(default_type);
            return PrintParameters(print_type, print_format, g.output_file(), std::move(output_vars), {});
        }

        // --- shared multi-line directive helpers ---

        // parse .MEASURE directives (one per line)
        [[nodiscard]] std::vector<MeasureEntry> parse_measure_lines(std::string_view text) {
            std::vector<MeasureEntry> measures;
            for (const auto& line : split_by(text, '\n')) {
                auto parsed = MeasureEntry::from_xyce_statement(std::string(line));
                if (parsed)
                    measures.push_back(std::move(*parsed));
            }
            return measures;
        }

        // format .MEASURE directives as one line each
        [[nodiscard]] std::string format_measure_lines(const std::vector<MeasureEntry>& measures) {
            std::vector<std::string> lines;
            for (const auto& m : measures)
                lines.push_back(m.to_xyce_statement());
            return join(lines, "\n");
        }

        // parse .FFT directives (one per line)
        [[nodiscard]] std::vector<FftParameters> parse_fft_lines(std::string_view text) {
            std::vector<FftParameters> ffts;
            for (const auto& line : split_by(text, '\n')) {
                auto parsed = FftParameters::from_xyce_statement(std::string(line));
                if (parsed)
                    ffts.push_back(std::move(*parsed));
            }
            return ffts;
        }

        // format .FFT directives as one line each
        [[nodiscard]] std::string format_fft_lines(const std::vector<FftParameters>& ffts) {
            std::vector<std::string> lines;
            for (const auto& f : ffts)
                lines.push_back(f.to_xyce_statement());
            return join(lines, "\n");
        }

        // parse .FOUR directives (one per line)
        [[nodiscard]] std::vector<FourParameters> parse_four_lines(std::string_view text) {
            std::vector<FourParameters> fours;
            for (const auto& line : split_by(text, '\n')) {
                auto parsed = FourParameters::from_xyce_statement(std::string(line));
                if (parsed)
                    fours.push_back(std::move(*parsed));
            }
            return fours;
        }

        // format .FOUR directives as one line each
        [[nodiscard]] std::string format_four_lines(const std::vector<FourParameters>& fours) {
            std::vector<std::string> lines;
            for (const auto& f : fours)
                lines.push_back(f.to_xyce_statement());
            return join(lines, "\n");
        }

        // parse schedule points from multi-line "time, max_step" text
        [[nodiscard]] std::vector<TransientSchedulePoint> parse_schedule_text(std::string_view text) {
            std::vector<TransientSchedulePoint> points;
            for (const auto& line_view : split_by(text, '\n')) {
                const std::string line = trim(line_view);
                if (line.empty())
                    continue;
                const auto comma_pos = line.find(',');
                if (comma_pos == std::string::npos)
                    continue;
                const std::string time_part = trim(line.substr(0, comma_pos));
                const std::string step_part = trim(line.substr(comma_pos + 1));
                if (!time_part.empty() && !step_part.empty())
                    points.emplace_back(time_part, step_part);
            }
            return points;
        }

        // format schedule points as "time, max_step" lines
        [[nodiscard]] std::string format_schedule_text(const std::vector<TransientSchedulePoint>& points) {
            std::vector<std::string> lines;
            for (const auto& point : points)
                lines.push_back(point.time_value + ", " + point.max_time_step_value);
            return join(lines, "\n");
        }

        // parse newline-separated key=value options into a map (keys uppercased)
        [[nodiscard]] std::map<std::string, std::string> parse_options_text(std::string_view text) {
            std::map<std::string, std::string> options;
            for (const auto& line_view : split_by(text, '\n')) {
                const std::string line = trim(line_view);
                if (line.empty())
                    continue;
                const auto eq_pos = line.find('=');
                if (eq_pos == std::string::npos)
                    continue;
                const std::string key = to_upper(trim(line.substr(0, eq_pos)));
                const std::string value = trim(line.substr(eq_pos + 1));
                if (!key.empty())
                    options[key] = value;
            }
            return options;
        }

        // format an options map as one "key=value" line per entry
        [[nodiscard]] std::string format_options_text(const std::map<std::string, std::string>& options) {
            std::vector<std::string> lines;
            for (const auto& [key, value] : options)
                lines.push_back(key + "=" + value);
            return join(lines, "\n");
        }

        // parse device noise operators from "type node source" lines
        [[nodiscard]] std::vector<DeviceNoiseOperator> parse_device_noise_text(std::string_view text) {
            std::vector<DeviceNoiseOperator> operators;
            for (const auto& line : split_by(text, '\n')) {
                const auto tokens = tokenize(line);
                if (tokens.size() >= 3)
                    operators.emplace_back(std::string(tokens[0]), std::string(tokens[1]), std::string(tokens[2]));
            }
            return operators;
        }

        // format device noise operators as "type node source" lines
        [[nodiscard]] std::string format_device_noise_text(const std::vector<DeviceNoiseOperator>& operators) {
            std::vector<std::string> lines;
            for (const auto& op : operators)
                lines.push_back(op.type + " " + op.node + " " + op.source);
            return join(lines, "\n");
        }

        // join a range of {node, voltage} entries into " V(node)=voltage" tokens
        template <typename Entry>
        [[nodiscard]] std::string join_entries(const std::vector<Entry>& entries) {
            std::string result;
            for (size_t i = 0; i < entries.size(); ++i) {
                if (i != 0)
                    result += ' ';
                result += "V(" + entries[i].node + ")=" + entries[i].voltage;
            }
            return result;
        }

        // parse space-separated V(node)=voltage tokens into NodesetEntry objects
        [[nodiscard]] std::vector<NodesetEntry> parse_nodeset_entries(std::string_view text) {
            std::vector<NodesetEntry> entries;
            for (const auto& tok : tokenize(text)) {
                const auto eq_pos = tok.find('=');
                if (eq_pos == std::string::npos)
                    continue;
                const std::string_view lhs = tok.substr(0, eq_pos);
                const std::string_view rhs = tok.substr(eq_pos + 1);
                if (lhs.starts_with("V(") && lhs.ends_with(")")) {
                    const std::string_view node = lhs.substr(2, lhs.size() - 3);
                    if (!node.empty() && !rhs.empty())
                        entries.emplace_back(std::string(node), std::string(rhs));
                }
            }
            return entries;
        }

        // parse IC text into IcEntry objects; accepts V(node)=val, node=val, and bare pairs
        [[nodiscard]] std::vector<IcEntry> parse_ic_entries(std::string_view text) {
            std::vector<IcEntry> entries;
            const auto tokens = tokenize(text);
            for (size_t i = 0; i < tokens.size();) {
                const auto& tok = tokens[i];
                const auto eq_pos = tok.find('=');
                if (eq_pos != std::string::npos) {
                    std::string_view lhs = tok.substr(0, eq_pos);
                    std::string_view rhs = tok.substr(eq_pos + 1);
                    std::string_view node;
                    if (lhs.starts_with("V(") && lhs.ends_with(")"))
                        node = lhs.substr(2, lhs.size() - 3);
                    else
                        node = lhs;
                    if (!node.empty() && !rhs.empty())
                        entries.emplace_back(std::string(node), std::string(rhs));
                    ++i;
                }
                else if (i + 1 < tokens.size()) {
                    entries.emplace_back(std::string(tokens[i]), std::string(tokens[i + 1]));
                    i += 2;
                }
                else {
                    ++i;
                }
            }
            return entries;
        }

        // push the saved operating point parameters into the dialog root's op-* fields
        void apply_op_parameters(const DialogHandle& dialog, const OpSimulationParameters& params) {
            // print section (no print-type combo; the type is always DC)
            apply_print_section(params.print_parameters, true, true, true, false, NO_PRINT_TYPES,
                                PrintSetters{
                                    .enabled = [&](bool v) { dialog->set_op_print_enabled(v); },
                                    .all_nodes = [&](bool v) { dialog->set_op_print_all_nodes(v); },
                                    .all_currents = [&](bool v) { dialog->set_op_print_all_currents(v); },
                                    .power = [&](bool v) { dialog->set_op_print_power(v); },
                                    .bjt_leads = [&](bool v) { dialog->set_op_print_bjt_leads(v); },
                                    .fet_leads = [&](bool v) { dialog->set_op_print_fet_leads(v); },
                                    .specific_variables = [&](slint::SharedString v) { dialog->set_op_print_specific_variables(v); },
                                    .format_index = [&](int v) { dialog->set_op_print_format_index(v); },
                                    .output_file = [&](slint::SharedString v) { dialog->set_op_print_output_file(v); },
                                    .type_index = [&](int) {},
                                });
            // save section
            dialog->set_op_save_enabled(params.save_enabled);
            dialog->set_op_save_type_index(to_upper(params.save_type) == "IC" ? 1 : 0);
            dialog->set_op_save_file(slint::SharedString(params.save_file));
            // convergence hints / initial conditions
            dialog->set_op_nodeset(slint::SharedString(join_entries(params.nodeset_entries)));
            dialog->set_op_initial_conditions(slint::SharedString(join_entries(params.ic_entries)));
        }

        // read the operating point parameters from the dialog root's op-* fields
        [[nodiscard]] OpSimulationParameters build_op_parameters(const DialogHandle& dialog) {
            // print parameters (the print type is always DC for an OP analysis)
            auto print_params = build_print_section(true, true, true, false, "DC", NO_PRINT_TYPES,
                                                    PrintGetters{
                                                        .enabled = [&] { return dialog->get_op_print_enabled(); },
                                                        .all_nodes = [&] { return dialog->get_op_print_all_nodes(); },
                                                        .all_currents = [&] { return dialog->get_op_print_all_currents(); },
                                                        .power = [&] { return dialog->get_op_print_power(); },
                                                        .bjt_leads = [&] { return dialog->get_op_print_bjt_leads(); },
                                                        .fet_leads = [&] { return dialog->get_op_print_fet_leads(); },
                                                        .specific_variables = [&] { return std::string(dialog->get_op_print_specific_variables()); },
                                                        .format_index = [&] { return dialog->get_op_print_format_index(); },
                                                        .output_file = [&] { return std::string(dialog->get_op_print_output_file()); },
                                                        .type_index = [&] { return 0; },
                                                    });
            // save section
            const bool save_enabled = dialog->get_op_save_enabled();
            const std::string save_type = dialog->get_op_save_type_index() == 1 ? "IC" : "NODESET";
            const std::string save_file = std::string(dialog->get_op_save_file());
            // nodeset / initial conditions
            const auto nodeset_entries = parse_nodeset_entries(std::string(dialog->get_op_nodeset()));
            const auto ic_entries = parse_ic_entries(std::string(dialog->get_op_initial_conditions()));
            return OpSimulationParameters(print_params.has_value(), false, false, {}, "", "", save_enabled, save_type, save_file, std::move(nodeset_entries), std::move(ic_entries), std::move(print_params));
        }

        // default operating point parameters, used to reset the panel to defaults
        [[nodiscard]] OpSimulationParameters default_op_parameters() { return OpSimulationParameters(false, false, false, {}, "", "", false, "", "", {}, {}, std::nullopt); }

        // push the saved AC analysis parameters into the dialog root's ac-* fields
        void apply_ac_parameters(const DialogHandle& dialog, const AcSimulationParameters& params) {
            // sweep configuration
            dialog->set_ac_sweep_mode_index(sweep_mode_index_for(params.sweep_mode));
            dialog->set_ac_points(slint::SharedString(params.points));
            dialog->set_ac_start(slint::SharedString(params.start));
            dialog->set_ac_end(slint::SharedString(params.end));
            dialog->set_ac_data_table(slint::SharedString(params.data_table_name));
            // measure directives (one per line)
            dialog->set_ac_measure(slint::SharedString(format_measure_lines(params.measure_parameters)));
            // print section (power is not available for an AC analysis)
            apply_print_section(params.print_parameters, false, true, true, true, AC_PRINT_TYPES,
                                PrintSetters{
                                    .enabled = [&](bool v) { dialog->set_ac_print_enabled(v); },
                                    .all_nodes = [&](bool v) { dialog->set_ac_print_all_nodes(v); },
                                    .all_currents = [&](bool v) { dialog->set_ac_print_all_currents(v); },
                                    .power = [&](bool) {},
                                    .bjt_leads = [&](bool v) { dialog->set_ac_print_bjt_leads(v); },
                                    .fet_leads = [&](bool v) { dialog->set_ac_print_fet_leads(v); },
                                    .specific_variables = [&](slint::SharedString v) { dialog->set_ac_print_specific_variables(v); },
                                    .format_index = [&](int v) { dialog->set_ac_print_format_index(v); },
                                    .output_file = [&](slint::SharedString v) { dialog->set_ac_print_output_file(v); },
                                    .type_index = [&](int v) { dialog->set_ac_print_type_index(v); },
                                });
        }

        // read the AC analysis parameters from the dialog root's ac-* fields
        [[nodiscard]] AcSimulationParameters build_ac_parameters(const DialogHandle& dialog) {
            const int sweep_mode_index = dialog->get_ac_sweep_mode_index();
            const std::string sweep_mode = SWEEP_MODE_VALUES[static_cast<size_t>(sweep_mode_index)];
            // the sweep range fields only apply to LIN/DEC/OCT sweeps
            const bool is_data_sweep = sweep_mode_index == 3;
            const std::string points = is_data_sweep ? "" : std::string(dialog->get_ac_points());
            const std::string start = is_data_sweep ? "" : std::string(dialog->get_ac_start());
            const std::string end = is_data_sweep ? "" : std::string(dialog->get_ac_end());
            // the data table name only applies to DATA sweeps
            const std::string data_table = is_data_sweep ? std::string(dialog->get_ac_data_table()) : "";
            // parse .MEASURE directives (one per line)
            auto measures = parse_measure_lines(std::string(dialog->get_ac_measure()));
            // print parameters (power is not available for an AC analysis)
            auto print_params = build_print_section(false, true, true, true, "AC", AC_PRINT_TYPES,
                                                    PrintGetters{
                                                        .enabled = [&] { return dialog->get_ac_print_enabled(); },
                                                        .all_nodes = [&] { return dialog->get_ac_print_all_nodes(); },
                                                        .all_currents = [&] { return dialog->get_ac_print_all_currents(); },
                                                        .power = [&] { return false; },
                                                        .bjt_leads = [&] { return dialog->get_ac_print_bjt_leads(); },
                                                        .fet_leads = [&] { return dialog->get_ac_print_fet_leads(); },
                                                        .specific_variables = [&] { return std::string(dialog->get_ac_print_specific_variables()); },
                                                        .format_index = [&] { return dialog->get_ac_print_format_index(); },
                                                        .output_file = [&] { return std::string(dialog->get_ac_print_output_file()); },
                                                        .type_index = [&] { return dialog->get_ac_print_type_index(); },
                                                    });
            return AcSimulationParameters(sweep_mode, points, start, end, data_table, std::move(print_params), std::move(measures), std::nullopt);
        }

        // default AC analysis parameters, used to reset the panel to defaults
        [[nodiscard]] AcSimulationParameters default_ac_parameters() { return AcSimulationParameters("LIN", "", "", "", "", std::nullopt, {}, std::nullopt); }

        // --- transient analysis panel ---

        // push the saved transient parameters into the dialog root's tran-* fields
        void apply_transient_parameters(const DialogHandle& dialog, const TransientSimulationParameters& params) {
            dialog->set_tran_initial_step(slint::SharedString(params.initial_step_value));
            dialog->set_tran_final_time(slint::SharedString(params.final_time_value));
            dialog->set_tran_start_time(slint::SharedString(params.start_time_value));
            dialog->set_tran_step_ceiling(slint::SharedString(params.step_ceiling_value));
            dialog->set_tran_op_keyword_index(op_keyword_index_for(params.op_keyword));
            dialog->set_tran_schedule(slint::SharedString(format_schedule_text(params.schedule_points)));
            dialog->set_tran_fft(slint::SharedString(format_fft_lines(params.fft_parameters)));
            dialog->set_tran_four(slint::SharedString(format_four_lines(params.four_parameters)));
            dialog->set_tran_measure(slint::SharedString(format_measure_lines(params.measure_parameters)));
            // print section
            apply_print_section(params.print_parameters, true, true, true, true, TRAN_PRINT_TYPES,
                                PrintSetters{
                                    .enabled = [&](bool v) { dialog->set_tran_print_enabled(v); },
                                    .all_nodes = [&](bool v) { dialog->set_tran_print_all_nodes(v); },
                                    .all_currents = [&](bool v) { dialog->set_tran_print_all_currents(v); },
                                    .power = [&](bool v) { dialog->set_tran_print_power(v); },
                                    .bjt_leads = [&](bool v) { dialog->set_tran_print_bjt_leads(v); },
                                    .fet_leads = [&](bool v) { dialog->set_tran_print_fet_leads(v); },
                                    .specific_variables = [&](slint::SharedString v) { dialog->set_tran_print_specific_variables(v); },
                                    .format_index = [&](int v) { dialog->set_tran_print_format_index(v); },
                                    .output_file = [&](slint::SharedString v) { dialog->set_tran_print_output_file(v); },
                                    .type_index = [&](int v) { dialog->set_tran_print_type_index(v); },
                                });
        }

        // read the transient parameters from the dialog root's tran-* fields
        [[nodiscard]] TransientSimulationParameters build_transient_parameters(const DialogHandle& dialog) {
            // resolve the OP keyword from the combo; index 0 is "(None)"
            std::string op_keyword;
            const int op_index = dialog->get_tran_op_keyword_index();
            if (op_index > 0 && op_index < static_cast<int>(OP_KEYWORD_VALUES.size()))
                op_keyword = OP_KEYWORD_VALUES[static_cast<size_t>(op_index)];
            // parse the schedule / .FFT / .FOUR / .MEASURE directives
            auto schedule_points = parse_schedule_text(std::string(dialog->get_tran_schedule()));
            auto fft_params = parse_fft_lines(std::string(dialog->get_tran_fft()));
            auto four_params = parse_four_lines(std::string(dialog->get_tran_four()));
            auto measure_params = parse_measure_lines(std::string(dialog->get_tran_measure()));
            // print parameters
            auto print_params = build_print_section(true, true, true, true, "TRAN", TRAN_PRINT_TYPES,
                                                    PrintGetters{
                                                        .enabled = [&] { return dialog->get_tran_print_enabled(); },
                                                        .all_nodes = [&] { return dialog->get_tran_print_all_nodes(); },
                                                        .all_currents = [&] { return dialog->get_tran_print_all_currents(); },
                                                        .power = [&] { return dialog->get_tran_print_power(); },
                                                        .bjt_leads = [&] { return dialog->get_tran_print_bjt_leads(); },
                                                        .fet_leads = [&] { return dialog->get_tran_print_fet_leads(); },
                                                        .specific_variables = [&] { return std::string(dialog->get_tran_print_specific_variables()); },
                                                        .format_index = [&] { return dialog->get_tran_print_format_index(); },
                                                        .output_file = [&] { return std::string(dialog->get_tran_print_output_file()); },
                                                        .type_index = [&] { return dialog->get_tran_print_type_index(); },
                                                    });
            return TransientSimulationParameters(std::string(dialog->get_tran_initial_step()), std::string(dialog->get_tran_final_time()), std::string(dialog->get_tran_start_time()), std::string(dialog->get_tran_step_ceiling()), std::move(op_keyword), std::move(schedule_points), std::move(print_params), std::move(fft_params), std::move(four_params), std::move(measure_params), std::nullopt);
        }

        // default transient analysis parameters, used to reset the panel to defaults
        [[nodiscard]] TransientSimulationParameters default_transient_parameters() { return TransientSimulationParameters("", "", "", "", "", {}, std::nullopt, {}, {}, {}, std::nullopt); }

        // --- DC analysis panel ---

        // push the saved DC parameters into the dialog root's dc-* fields
        void apply_dc_parameters(const DialogHandle& dialog, const DCSimulationParameters& params) {
            dialog->set_dc_sweep_mode_index(choice_index_for(DC_SWEEP_MODE_VALUES, params.sweep_mode));
            dialog->set_dc_primary_variable(slint::SharedString(params.primary_variable));
            dialog->set_dc_start(slint::SharedString(params.start));
            dialog->set_dc_stop(slint::SharedString(params.stop));
            dialog->set_dc_step(slint::SharedString(params.step));
            dialog->set_dc_points(slint::SharedString(params.points));
            dialog->set_dc_list_values(slint::SharedString(join(params.list_values, " ")));
            dialog->set_dc_data_table(slint::SharedString(params.data_table_name));
            dialog->set_dc_secondary_variable(slint::SharedString(params.secondary_variable));
            dialog->set_dc_secondary_start(slint::SharedString(params.secondary_start));
            dialog->set_dc_secondary_stop(slint::SharedString(params.secondary_stop));
            dialog->set_dc_secondary_step(slint::SharedString(params.secondary_step));
            dialog->set_dc_secondary_points(slint::SharedString(params.secondary_points));
            dialog->set_dc_measure(slint::SharedString(format_measure_lines(params.measure_parameters)));
            // print section
            apply_print_section(params.print_parameters, true, true, true, true, DC_PRINT_TYPES,
                                PrintSetters{
                                    .enabled = [&](bool v) { dialog->set_dc_print_enabled(v); },
                                    .all_nodes = [&](bool v) { dialog->set_dc_print_all_nodes(v); },
                                    .all_currents = [&](bool v) { dialog->set_dc_print_all_currents(v); },
                                    .power = [&](bool v) { dialog->set_dc_print_power(v); },
                                    .bjt_leads = [&](bool v) { dialog->set_dc_print_bjt_leads(v); },
                                    .fet_leads = [&](bool v) { dialog->set_dc_print_fet_leads(v); },
                                    .specific_variables = [&](slint::SharedString v) { dialog->set_dc_print_specific_variables(v); },
                                    .format_index = [&](int v) { dialog->set_dc_print_format_index(v); },
                                    .output_file = [&](slint::SharedString v) { dialog->set_dc_print_output_file(v); },
                                    .type_index = [&](int v) { dialog->set_dc_print_type_index(v); },
                                });
        }

        // read the DC parameters from the dialog root's dc-* fields
        [[nodiscard]] DCSimulationParameters build_dc_parameters(const DialogHandle& dialog) {
            const int sweep_mode_index = std::clamp(dialog->get_dc_sweep_mode_index(), 0, static_cast<int>(DC_SWEEP_MODE_VALUES.size()) - 1);
            const std::string sweep_mode = DC_SWEEP_MODE_VALUES[static_cast<size_t>(sweep_mode_index)];
            const std::string primary_variable = std::string(dialog->get_dc_primary_variable());
            const std::string start = std::string(dialog->get_dc_start());
            const std::string stop = std::string(dialog->get_dc_stop());
            // the primary step / points / list / data-table fields depend on the mode
            std::string step;
            std::string points;
            std::vector<std::string> list_values;
            std::string data_table_name;
            if (sweep_mode == "LIN") {
                step = std::string(dialog->get_dc_step());
                if (step.empty())
                    step = std::string(dialog->get_dc_points());
            }
            else if (sweep_mode == "DEC" || sweep_mode == "OCT") {
                points = std::string(dialog->get_dc_points());
                if (points.empty())
                    points = std::string(dialog->get_dc_step());
            }
            else if (sweep_mode == "LIST") {
                for (const auto& tok : tokenize(dialog->get_dc_list_values()))
                    list_values.emplace_back(std::string(tok));
            }
            else if (sweep_mode == "DATA") {
                data_table_name = std::string(dialog->get_dc_data_table());
            }
            const std::string secondary_variable = std::string(dialog->get_dc_secondary_variable());
            const std::string secondary_start = std::string(dialog->get_dc_secondary_start());
            const std::string secondary_stop = std::string(dialog->get_dc_secondary_stop());
            std::string secondary_step;
            std::string secondary_points;
            if (sweep_mode == "LIN") {
                secondary_step = std::string(dialog->get_dc_secondary_step());
                if (secondary_step.empty())
                    secondary_step = std::string(dialog->get_dc_secondary_points());
            }
            else if (sweep_mode == "DEC" || sweep_mode == "OCT") {
                secondary_points = std::string(dialog->get_dc_secondary_points());
                if (secondary_points.empty())
                    secondary_points = std::string(dialog->get_dc_secondary_step());
            }
            // parse .MEASURE directives (one per line)
            auto measure_params = parse_measure_lines(std::string(dialog->get_dc_measure()));
            // print parameters
            auto print_params = build_print_section(true, true, true, true, "DC", DC_PRINT_TYPES,
                                                    PrintGetters{
                                                        .enabled = [&] { return dialog->get_dc_print_enabled(); },
                                                        .all_nodes = [&] { return dialog->get_dc_print_all_nodes(); },
                                                        .all_currents = [&] { return dialog->get_dc_print_all_currents(); },
                                                        .power = [&] { return dialog->get_dc_print_power(); },
                                                        .bjt_leads = [&] { return dialog->get_dc_print_bjt_leads(); },
                                                        .fet_leads = [&] { return dialog->get_dc_print_fet_leads(); },
                                                        .specific_variables = [&] { return std::string(dialog->get_dc_print_specific_variables()); },
                                                        .format_index = [&] { return dialog->get_dc_print_format_index(); },
                                                        .output_file = [&] { return std::string(dialog->get_dc_print_output_file()); },
                                                        .type_index = [&] { return dialog->get_dc_print_type_index(); },
                                                    });
            return DCSimulationParameters(std::move(sweep_mode), std::move(primary_variable), std::move(start), std::move(stop), std::move(step), std::move(points), std::move(list_values), std::move(data_table_name), std::move(secondary_variable), std::move(secondary_start), std::move(secondary_stop), std::move(secondary_step), std::move(secondary_points), std::move(print_params), std::move(measure_params), std::nullopt);
        }

        // default DC analysis parameters, used to reset the panel to defaults
        [[nodiscard]] DCSimulationParameters default_dc_parameters() { return DCSimulationParameters("", "", "", "", "", "", {}, "", "", "", "", "", "", std::nullopt, {}, std::nullopt); }

        // --- noise analysis panel ---

        // push the saved noise parameters into the dialog root's noise-* fields
        void apply_noise_parameters(const DialogHandle& dialog, const NoiseSimulationParameters& params) {
            dialog->set_noise_output_node(slint::SharedString(params.output_node));
            dialog->set_noise_ref_node(slint::SharedString(params.ref_node));
            dialog->set_noise_source_name(slint::SharedString(params.source_name));
            dialog->set_noise_start_freq(slint::SharedString(params.start_freq_value));
            dialog->set_noise_end_freq(slint::SharedString(params.end_freq_value));
            dialog->set_noise_num_points(slint::SharedString(params.num_points_value));
            dialog->set_noise_sweep_type_index(sweep_mode_index_for(params.sweep_type));
            dialog->set_noise_data_table(slint::SharedString(params.data_table_name));
            dialog->set_noise_device_noise(slint::SharedString(format_device_noise_text(params.device_noise_operators)));
            // print section (no power for a noise analysis)
            apply_print_section(params.print_parameters, false, true, true, true, NOISE_PRINT_TYPES,
                                PrintSetters{
                                    .enabled = [&](bool v) { dialog->set_noise_print_enabled(v); },
                                    .all_nodes = [&](bool v) { dialog->set_noise_print_all_nodes(v); },
                                    .all_currents = [&](bool v) { dialog->set_noise_print_all_currents(v); },
                                    .power = [&](bool) {},
                                    .bjt_leads = [&](bool v) { dialog->set_noise_print_bjt_leads(v); },
                                    .fet_leads = [&](bool v) { dialog->set_noise_print_fet_leads(v); },
                                    .specific_variables = [&](slint::SharedString v) { dialog->set_noise_print_specific_variables(v); },
                                    .format_index = [&](int v) { dialog->set_noise_print_format_index(v); },
                                    .output_file = [&](slint::SharedString v) { dialog->set_noise_print_output_file(v); },
                                    .type_index = [&](int v) { dialog->set_noise_print_type_index(v); },
                                });
        }

        // read the noise parameters from the dialog root's noise-* fields
        [[nodiscard]] NoiseSimulationParameters build_noise_parameters(const DialogHandle& dialog) {
            const int sweep_type_index = std::clamp(dialog->get_noise_sweep_type_index(), 0, static_cast<int>(SWEEP_MODE_VALUES.size()) - 1);
            const std::string sweep_type = SWEEP_MODE_VALUES[static_cast<size_t>(sweep_type_index)];
            // the frequency range fields only apply to LIN/DEC/OCT sweeps
            const bool is_data_sweep = sweep_type_index == 3;
            const std::string start_freq = is_data_sweep ? "" : std::string(dialog->get_noise_start_freq());
            const std::string end_freq = is_data_sweep ? "" : std::string(dialog->get_noise_end_freq());
            const std::string num_points = is_data_sweep ? "" : std::string(dialog->get_noise_num_points());
            const std::string data_table = is_data_sweep ? std::string(dialog->get_noise_data_table()) : "";
            // parse device noise operators (type node source per line)
            auto device_noise = parse_device_noise_text(std::string(dialog->get_noise_device_noise()));
            // print parameters (no power for a noise analysis)
            auto print_params = build_print_section(false, true, true, true, "NOISE", NOISE_PRINT_TYPES,
                                                    PrintGetters{
                                                        .enabled = [&] { return dialog->get_noise_print_enabled(); },
                                                        .all_nodes = [&] { return dialog->get_noise_print_all_nodes(); },
                                                        .all_currents = [&] { return dialog->get_noise_print_all_currents(); },
                                                        .power = [&] { return false; },
                                                        .bjt_leads = [&] { return dialog->get_noise_print_bjt_leads(); },
                                                        .fet_leads = [&] { return dialog->get_noise_print_fet_leads(); },
                                                        .specific_variables = [&] { return std::string(dialog->get_noise_print_specific_variables()); },
                                                        .format_index = [&] { return dialog->get_noise_print_format_index(); },
                                                        .output_file = [&] { return std::string(dialog->get_noise_print_output_file()); },
                                                        .type_index = [&] { return dialog->get_noise_print_type_index(); },
                                                    });
            return NoiseSimulationParameters(std::string(dialog->get_noise_output_node()), std::string(dialog->get_noise_ref_node()), std::string(dialog->get_noise_source_name()), std::move(start_freq), std::move(end_freq), std::move(num_points), std::move(sweep_type), std::move(device_noise), std::move(data_table), std::move(print_params));
        }

        // default noise analysis parameters, used to reset the panel to defaults
        [[nodiscard]] NoiseSimulationParameters default_noise_parameters() { return NoiseSimulationParameters("", "", "", "", "", "", "", {}, "", std::nullopt); }

        // --- harmonic balance panel ---

        // format harmonics as a comma-separated string
        [[nodiscard]] std::string format_harmonics(const std::vector<int>& harmonics) {
            std::vector<std::string> parts;
            for (int h : harmonics)
                parts.push_back(std::to_string(h));
            return join(parts, ",");
        }

        // resolve a TAHB value to its combo index; "(None)" when unset or unknown
        [[nodiscard]] int tahb_index_for(const std::optional<int>& tahb) {
            for (size_t i = 1; i < TAHB_VALUES.size(); ++i) {
                if (tahb.has_value() && TAHB_VALUES[i] == *tahb)
                    return static_cast<int>(i);
            }
            return 0;
        }

        // resolve a SELECTHARMS value to its combo index; "(None)" when unset
        [[nodiscard]] int selectharms_index_for(const std::optional<std::string>& selectharms) { return selectharms.has_value() ? choice_index_for(SELECTHARMS_VALUES, *selectharms) : 0; }

        // push the saved HB parameters into the dialog root's hb-* fields
        void apply_hb_parameters(const DialogHandle& dialog, const HbSimulationParameters& params) {
            dialog->set_hb_frequencies(slint::SharedString(join(params.frequencies, " ")));
            dialog->set_hb_harmonics(slint::SharedString(format_harmonics(params.harmonics)));
            dialog->set_hb_tahb_index(tahb_index_for(params.tahb));
            dialog->set_hb_selectharms_index(selectharms_index_for(params.selectharms));
            dialog->set_hb_startup_periods(slint::SharedString(params.startup_periods.has_value() ? std::to_string(*params.startup_periods) : ""));
            dialog->set_hb_nonlin_options(slint::SharedString(format_options_text(params.nonlin_options)));
            dialog->set_hb_linsol_options(slint::SharedString(format_options_text(params.linsol_options)));
            // print section (no power, no BJT/FET leads for HB)
            apply_print_section(params.print_parameters, false, false, false, true, HB_PRINT_TYPES,
                                PrintSetters{
                                    .enabled = [&](bool v) { dialog->set_hb_print_enabled(v); },
                                    .all_nodes = [&](bool v) { dialog->set_hb_print_all_nodes(v); },
                                    .all_currents = [&](bool v) { dialog->set_hb_print_all_currents(v); },
                                    .power = [&](bool) {},
                                    .bjt_leads = [&](bool) {},
                                    .fet_leads = [&](bool) {},
                                    .specific_variables = [&](slint::SharedString v) { dialog->set_hb_print_specific_variables(v); },
                                    .format_index = [&](int v) { dialog->set_hb_print_format_index(v); },
                                    .output_file = [&](slint::SharedString v) { dialog->set_hb_print_output_file(v); },
                                    .type_index = [&](int v) { dialog->set_hb_print_type_index(v); },
                                });
        }

        // read the HB parameters from the dialog root's hb-* fields
        [[nodiscard]] HbSimulationParameters build_hb_parameters(const DialogHandle& dialog) {
            // parse frequencies as space-separated values
            std::vector<std::string> frequencies;
            for (const auto& tok : tokenize(dialog->get_hb_frequencies()))
                frequencies.emplace_back(std::string(tok));
            // parse harmonics as comma-separated integers
            std::vector<int> harmonics;
            for (const auto& part : split_by(dialog->get_hb_harmonics(), ',')) {
                if (const auto value = parse_int(part))
                    harmonics.push_back(*value);
            }
            // TAHB / SELECTHARMS combo values; index 0 is "(None)"
            std::optional<int> tahb;
            const int tahb_index = std::clamp(dialog->get_hb_tahb_index(), 0, static_cast<int>(TAHB_VALUES.size()) - 1);
            if (tahb_index > 0)
                tahb = TAHB_VALUES[static_cast<size_t>(tahb_index)];
            std::optional<std::string> selectharms;
            const int selectharms_index = std::clamp(dialog->get_hb_selectharms_index(), 0, static_cast<int>(SELECTHARMS_VALUES.size()) - 1);
            if (selectharms_index > 0)
                selectharms = SELECTHARMS_VALUES[static_cast<size_t>(selectharms_index)];
            // optional startup periods
            std::optional<int> startup_periods;
            if (const auto value = parse_int(dialog->get_hb_startup_periods()))
                startup_periods = value;
            // print parameters (no power, no BJT/FET leads for HB)
            auto print_params = build_print_section(false, false, false, true, "HB", HB_PRINT_TYPES,
                                                    PrintGetters{
                                                        .enabled = [&] { return dialog->get_hb_print_enabled(); },
                                                        .all_nodes = [&] { return dialog->get_hb_print_all_nodes(); },
                                                        .all_currents = [&] { return dialog->get_hb_print_all_currents(); },
                                                        .power = [&] { return false; },
                                                        .bjt_leads = [&] { return false; },
                                                        .fet_leads = [&] { return false; },
                                                        .specific_variables = [&] { return std::string(dialog->get_hb_print_specific_variables()); },
                                                        .format_index = [&] { return dialog->get_hb_print_format_index(); },
                                                        .output_file = [&] { return std::string(dialog->get_hb_print_output_file()); },
                                                        .type_index = [&] { return dialog->get_hb_print_type_index(); },
                                                    });
            // solver options (one key=value per line)
            auto nonlin_options = parse_options_text(std::string(dialog->get_hb_nonlin_options()));
            auto linsol_options = parse_options_text(std::string(dialog->get_hb_linsol_options()));
            return HbSimulationParameters(std::move(frequencies), std::move(harmonics), tahb, std::move(selectharms), startup_periods, std::move(print_params), std::move(nonlin_options), std::move(linsol_options));
        }

        // default harmonic balance parameters, used to reset the panel to defaults
        [[nodiscard]] HbSimulationParameters default_hb_parameters() { return HbSimulationParameters({}, {}, std::nullopt, std::nullopt, std::nullopt, std::nullopt, {}, {}); }

        // --- linear analysis panel ---

        // push the saved LIN parameters into the dialog root's lin-* fields
        void apply_lin_parameters(const DialogHandle& dialog, const LinSimulationParameters& params) {
            dialog->set_lin_sparcalc(params.sparcalc);
            dialog->set_lin_format_index(choice_index_for(LIN_FORMAT_CHOICES, params.format));
            dialog->set_lin_lintype_index(choice_index_for(LIN_LINTYPE_CHOICES, params.lintype));
            dialog->set_lin_dataformat_index(choice_index_for(LIN_DATAFORMAT_CHOICES, params.dataformat));
            dialog->set_lin_file(slint::SharedString(params.file));
            dialog->set_lin_output_width(slint::SharedString(params.width));
            dialog->set_lin_precision(slint::SharedString(params.precision));
            dialog->set_lin_sweep_mode_index(sweep_mode_index_for(params.sweep_mode));
            dialog->set_lin_points(slint::SharedString(params.points));
            dialog->set_lin_start(slint::SharedString(params.start));
            dialog->set_lin_end(slint::SharedString(params.end));
            dialog->set_lin_data_table(slint::SharedString(params.data_table_name));
            // print section (single AC print type)
            apply_print_section(params.print_parameters, true, true, true, true, LIN_PRINT_TYPES,
                                PrintSetters{
                                    .enabled = [&](bool v) { dialog->set_lin_print_enabled(v); },
                                    .all_nodes = [&](bool v) { dialog->set_lin_print_all_nodes(v); },
                                    .all_currents = [&](bool v) { dialog->set_lin_print_all_currents(v); },
                                    .power = [&](bool v) { dialog->set_lin_print_power(v); },
                                    .bjt_leads = [&](bool v) { dialog->set_lin_print_bjt_leads(v); },
                                    .fet_leads = [&](bool v) { dialog->set_lin_print_fet_leads(v); },
                                    .specific_variables = [&](slint::SharedString v) { dialog->set_lin_print_specific_variables(v); },
                                    .format_index = [&](int v) { dialog->set_lin_print_format_index(v); },
                                    .output_file = [&](slint::SharedString v) { dialog->set_lin_print_output_file(v); },
                                    .type_index = [&](int v) { dialog->set_lin_print_type_index(v); },
                                });
        }

        // read the LIN parameters from the dialog root's lin-* fields
        [[nodiscard]] LinSimulationParameters build_lin_parameters(const DialogHandle& dialog) {
            const int format_index = std::clamp(dialog->get_lin_format_index(), 0, static_cast<int>(LIN_FORMAT_CHOICES.size()) - 1);
            const int lintype_index = std::clamp(dialog->get_lin_lintype_index(), 0, static_cast<int>(LIN_LINTYPE_CHOICES.size()) - 1);
            const int dataformat_index = std::clamp(dialog->get_lin_dataformat_index(), 0, static_cast<int>(LIN_DATAFORMAT_CHOICES.size()) - 1);
            const int sweep_mode_index = std::clamp(dialog->get_lin_sweep_mode_index(), 0, static_cast<int>(SWEEP_MODE_VALUES.size()) - 1);
            const std::string sweep_mode = SWEEP_MODE_VALUES[static_cast<size_t>(sweep_mode_index)];
            // the sweep range fields only apply to LIN/DEC/OCT sweeps
            const bool is_data_sweep = sweep_mode_index == 3;
            const std::string points = is_data_sweep ? "" : std::string(dialog->get_lin_points());
            const std::string start = is_data_sweep ? "" : std::string(dialog->get_lin_start());
            const std::string end = is_data_sweep ? "" : std::string(dialog->get_lin_end());
            const std::string data_table = is_data_sweep ? std::string(dialog->get_lin_data_table()) : "";
            // print parameters (single AC print type)
            auto print_params = build_print_section(true, true, true, true, "AC", LIN_PRINT_TYPES,
                                                    PrintGetters{
                                                        .enabled = [&] { return dialog->get_lin_print_enabled(); },
                                                        .all_nodes = [&] { return dialog->get_lin_print_all_nodes(); },
                                                        .all_currents = [&] { return dialog->get_lin_print_all_currents(); },
                                                        .power = [&] { return dialog->get_lin_print_power(); },
                                                        .bjt_leads = [&] { return dialog->get_lin_print_bjt_leads(); },
                                                        .fet_leads = [&] { return dialog->get_lin_print_fet_leads(); },
                                                        .specific_variables = [&] { return std::string(dialog->get_lin_print_specific_variables()); },
                                                        .format_index = [&] { return dialog->get_lin_print_format_index(); },
                                                        .output_file = [&] { return std::string(dialog->get_lin_print_output_file()); },
                                                        .type_index = [&] { return dialog->get_lin_print_type_index(); },
                                                    });
            return LinSimulationParameters(dialog->get_lin_sparcalc(), LIN_FORMAT_CHOICES[static_cast<size_t>(format_index)], LIN_LINTYPE_CHOICES[static_cast<size_t>(lintype_index)], LIN_DATAFORMAT_CHOICES[static_cast<size_t>(dataformat_index)], std::string(dialog->get_lin_file()), std::string(dialog->get_lin_output_width()), std::string(dialog->get_lin_precision()), std::move(sweep_mode), std::move(points), std::move(start), std::move(end), std::move(data_table), std::move(print_params));
        }

        // default linear analysis parameters, used to reset the panel to defaults
        [[nodiscard]] LinSimulationParameters default_lin_parameters() { return LinSimulationParameters(false, "", "", "", "", "", "", "", "", "", "", "", std::nullopt); }
    } // namespace

    struct SimulationParametersDialogView::Impl
    {
        // the slint dialog window, created lazily on the first use
        slint::ComponentHandle<simulation_parameters_dialog::SimulationParametersDialog> dialog;

        // presenter notified with the accepted configuration
        MainWindowViewDefEvents* handler = nullptr;

        // notified on both accept and cancel, after the dialog window is hidden;
        // the caller releases the modal state from here
        std::function<void()> on_closed;

        // configuration seeded on show; all analysis panels are migrated to Slint
        SimulationConfig m_config;

        Impl() :
            dialog(simulation_parameters_dialog::SimulationParametersDialog::create()), m_config(SimulationConfig::from_xyce_directives({})) {
            // wire callbacks
            dialog->on_accepted([this] { accept(); });
            dialog->on_dismissed([this] { dismiss(); });
        }

        void accept() {
            // read the tab selected by the user
            const int selected_tab = dialog->get_selected_tab();
            // update the analysis type from the selected tab
            m_config.analysis_type = ANALYSIS_TYPES[static_cast<size_t>(selected_tab)];
            // read the migrated operating point panel back into the analysis variant
            if (selected_tab == PAGE_OP) {
                m_config.analysis = build_op_parameters(dialog);
                m_config.replace_ground = dialog->get_op_replace_ground();
            }
            // read the migrated AC analysis panel back into the analysis variant
            else if (selected_tab == PAGE_AC) {
                m_config.analysis = build_ac_parameters(dialog);
                m_config.replace_ground = dialog->get_ac_replace_ground();
            }
            // read the migrated transient analysis panel back into the analysis variant
            else if (selected_tab == PAGE_TRAN) {
                m_config.analysis = build_transient_parameters(dialog);
                m_config.replace_ground = dialog->get_tran_replace_ground();
            }
            // read the migrated DC analysis panel back into the analysis variant;
            // mirror the wx dialog by rejecting an invalid DC sweep without closing
            else if (selected_tab == PAGE_DC) {
                auto dc = build_dc_parameters(dialog);
                if (const auto error = dc.validate()) {
                    dialog->set_error_message(slint::SharedString(*error));
                    dialog->set_show_error(true);
                    return;
                }
                m_config.analysis = std::move(dc);
                m_config.replace_ground = dialog->get_dc_replace_ground();
            }
            // read the migrated noise analysis panel back into the analysis variant
            else if (selected_tab == PAGE_NOISE) {
                m_config.analysis = build_noise_parameters(dialog);
                m_config.replace_ground = dialog->get_noise_replace_ground();
            }
            // read the migrated harmonic balance panel back into the analysis variant
            else if (selected_tab == PAGE_HB) {
                m_config.analysis = build_hb_parameters(dialog);
                m_config.replace_ground = dialog->get_hb_replace_ground();
            }
            // read the migrated linear analysis panel back into the analysis variant
            else if (selected_tab == PAGE_LIN) {
                m_config.analysis = build_lin_parameters(dialog);
                m_config.replace_ground = dialog->get_lin_replace_ground();
            }
            // hide the dialog before delivering the result
            dialog->hide();
            // release the modal state held by the caller
            if (on_closed)
                on_closed();
            // deliver the updated configuration to the presenter
            if (handler != nullptr)
                handler->on_simulation_parameters_dialog_result(m_config);
        }

        void dismiss() {
            // hide the dialog and drop the pending state
            dialog->hide();
            // release the modal state held by the caller
            if (on_closed)
                on_closed();
        }
    };

    SimulationParametersDialogView::SimulationParametersDialogView() :
        m_impl(std::make_unique<Impl>()) {}

    SimulationParametersDialogView::~SimulationParametersDialogView() = default;

    slint::Window& SimulationParametersDialogView::window() {
        // expose the dialog window (dialog must be shown first)
        return m_impl->dialog->window();
    }

    void SimulationParametersDialogView::show(const SimulationConfig& current, MainWindowViewDefEvents& handler, const std::function<void()>& on_closed) {
        // remember the result destination for the dialog lifetime
        m_impl->handler = &handler;
        // remember the close notification for this show
        m_impl->on_closed = on_closed;
        // seed the dialog with the current configuration
        m_impl->m_config = current;
        // select the tab matching the analysis type
        m_impl->dialog->set_selected_tab(tab_index_for(current.analysis_type));
        // sync the operating point panel to the seeded config, or reset it to
        // defaults when a different analysis is currently active
        if (const auto* op = std::get_if<OpSimulationParameters>(&current.analysis))
            apply_op_parameters(m_impl->dialog, *op);
        else
            apply_op_parameters(m_impl->dialog, default_op_parameters());
        // mirror the replace-ground toggle onto the OP panel state
        m_impl->dialog->set_op_replace_ground(current.replace_ground);
        // sync the AC analysis panel to the seeded config, or reset it to
        // defaults when a different analysis is currently active
        if (const auto* ac = std::get_if<AcSimulationParameters>(&current.analysis))
            apply_ac_parameters(m_impl->dialog, *ac);
        else
            apply_ac_parameters(m_impl->dialog, default_ac_parameters());
        // mirror the replace-ground toggle onto the AC panel state
        m_impl->dialog->set_ac_replace_ground(current.replace_ground);
        // sync the transient analysis panel to the seeded config, or reset it to
        // defaults when a different analysis is currently active
        if (const auto* tran = std::get_if<TransientSimulationParameters>(&current.analysis))
            apply_transient_parameters(m_impl->dialog, *tran);
        else
            apply_transient_parameters(m_impl->dialog, default_transient_parameters());
        // mirror the replace-ground toggle onto the TRAN panel state
        m_impl->dialog->set_tran_replace_ground(current.replace_ground);
        // sync the DC analysis panel to the seeded config, or reset it to
        // defaults when a different analysis is currently active
        if (const auto* dc = std::get_if<DCSimulationParameters>(&current.analysis))
            apply_dc_parameters(m_impl->dialog, *dc);
        else
            apply_dc_parameters(m_impl->dialog, default_dc_parameters());
        // mirror the replace-ground toggle onto the DC panel state
        m_impl->dialog->set_dc_replace_ground(current.replace_ground);
        // sync the noise analysis panel to the seeded config, or reset it to
        // defaults when a different analysis is currently active
        if (const auto* noise = std::get_if<NoiseSimulationParameters>(&current.analysis))
            apply_noise_parameters(m_impl->dialog, *noise);
        else
            apply_noise_parameters(m_impl->dialog, default_noise_parameters());
        // mirror the replace-ground toggle onto the noise panel state
        m_impl->dialog->set_noise_replace_ground(current.replace_ground);
        // sync the harmonic balance panel to the seeded config, or reset it to
        // defaults when a different analysis is currently active
        if (const auto* hb = std::get_if<HbSimulationParameters>(&current.analysis))
            apply_hb_parameters(m_impl->dialog, *hb);
        else
            apply_hb_parameters(m_impl->dialog, default_hb_parameters());
        // mirror the replace-ground toggle onto the HB panel state
        m_impl->dialog->set_hb_replace_ground(current.replace_ground);
        // sync the linear analysis panel to the seeded config, or reset it to
        // defaults when a different analysis is currently active
        if (const auto* lin = std::get_if<LinSimulationParameters>(&current.analysis))
            apply_lin_parameters(m_impl->dialog, *lin);
        else
            apply_lin_parameters(m_impl->dialog, default_lin_parameters());
        // mirror the replace-ground toggle onto the LIN panel state
        m_impl->dialog->set_lin_replace_ground(current.replace_ground);
        // clear any previous validation feedback
        m_impl->dialog->set_show_error(false);
        // show the dialog window
        m_impl->dialog->show();
    }
} // namespace simulation_parameters_dialog_view
