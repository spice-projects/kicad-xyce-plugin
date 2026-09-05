#include "simulation_history_store.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <system_error>

#include <spdlog/spdlog.h>

namespace
{
    // two-digit zero-padded number
    std::string pad2(int value)
    {
        if (value < 10)
            return "0" + std::to_string(value);
        return std::to_string(value);
    }

    // check the character at position is a digit
    bool is_digit_at(const std::string& name, size_t position)
    {
        return position < name.size() && name[position] >= '0' && name[position] <= '9';
    }
} // namespace

std::string history_file_type(const std::filesystem::path& file)
{
    // extension of the file
    auto extension = file.extension().string();
    // lowercase the extension for the comparison
    std::ranges::transform(extension, extension.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    // FFT calculation files carry a numeric index: .fft0, .fft1, ...
    if (extension.rfind(".fft", 0) == 0)
        return "fft";
    if (extension == ".raw")
        return "raw";
    if (extension == ".out")
        return "out";
    if (extension == ".log")
        return "log";
    // other files fall back to the bare extension
    if (!extension.empty() && extension.front() == '.')
        return extension.substr(1);
    return extension;
}

std::string simulation_history_timestamp()
{
    // wall clock time, UTC so folder names sort chronologically
    const auto now = std::chrono::system_clock::now();
    const auto days = std::chrono::floor<std::chrono::days>(now);
    const std::chrono::year_month_day ymd{days};
    const std::chrono::hh_mm_ss tod{std::chrono::floor<std::chrono::seconds>(now - days)};
    // build the folder name: YYYY-MM-DD_HH-MM-SS
    std::string name;
    name.reserve(19);
    name += std::to_string(static_cast<int>(ymd.year()));
    name += "-" + pad2(static_cast<unsigned>(ymd.month()));
    name += "-" + pad2(static_cast<unsigned>(ymd.day()));
    name += "_" + pad2(static_cast<int>(tod.hours().count()));
    name += "-" + pad2(static_cast<int>(tod.minutes().count()));
    name += "-" + pad2(static_cast<int>(tod.seconds().count()));
    return name;
}

std::filesystem::path simulation_history_directory(const std::filesystem::path& netlist_directory)
{
    return netlist_directory / ".kicad-xyce-history";
}

bool is_simulation_history_run_name(const std::string& name)
{
    // pattern: YYYY-MM-DD_HH-MM-SS (19 chars) optionally followed by _N
    static constexpr size_t base_length = 19;
    if (name.size() < base_length)
        return false;
    // digits at the numeric positions, separators at the others
    static constexpr std::string_view pattern = "dddd-dd-dd_dd-dd-dd";
    for (size_t i = 0; i < base_length; ++i) {
        if (pattern[i] == 'd') {
            if (!is_digit_at(name, i))
                return false;
        }
        else if (name[i] != pattern[i]) {
            return false;
        }
    }
    // optional collision suffix: _N
    if (name.size() == base_length)
        return true;
    return name[base_length] == '_' && std::ranges::all_of(name.substr(base_length + 1), [](unsigned char c) { return c >= '0' && c <= '9'; });
}

std::optional<RecordedRun> record_simulation_run(const std::filesystem::path& history_directory, const std::vector<std::filesystem::path>& files, int max_runs)
{
    // ensure the history directory exists
    std::error_code error;
    std::filesystem::create_directories(history_directory, error);
    if (!std::filesystem::is_directory(history_directory)) {
        spdlog::warn("Simulation history directory not accessible: {}", history_directory.string());
        return std::nullopt;
    }
    // build a unique run folder name: the timestamp plus a numeric suffix on collision
    const auto base_name = simulation_history_timestamp();
    auto run_name = base_name;
    for (int suffix = 1; std::filesystem::exists(history_directory / run_name); ++suffix)
        run_name = base_name + "_" + std::to_string(suffix);
    // create the run folder; a failure here means no write access
    const auto run_directory = history_directory / run_name;
    std::filesystem::create_directory(run_directory, error);
    if (error) {
        spdlog::warn("Failed to create simulation history run folder {}: {}", run_directory.string(), error.message());
        return std::nullopt;
    }
    // copy the output files into the run folder, collecting failures per file
    RecordedRun recorded;
    recorded.timestamp = run_name;
    for (const auto& file : files) {
        try {
            std::filesystem::copy_file(file, run_directory / file.filename(), std::filesystem::copy_options::overwrite_existing);
        }
        catch (const std::filesystem::filesystem_error& copy_error) {
            spdlog::warn("Failed to copy {} into simulation history: {}", file.string(), copy_error.what());
            recorded.failed_files.push_back(file);
        }
    }
    // prune old runs beyond the configured maximum
    prune_simulation_history(history_directory, max_runs);
    return recorded;
}

void prune_simulation_history(const std::filesystem::path& history_directory, int max_runs)
{
    // always keep at least the newest run
    const size_t keep = max_runs < 1 ? 1 : static_cast<size_t>(max_runs);
    // collect the run folders
    std::vector<std::filesystem::path> runs;
    std::error_code error;
    for (const auto& entry : std::filesystem::directory_iterator(history_directory, error)) {
        if (entry.is_directory() && is_simulation_history_run_name(entry.path().filename().string()))
            runs.push_back(entry.path());
    }
    // sort the run folders oldest first
    std::ranges::sort(runs);
    // remove the oldest runs beyond the maximum
    for (size_t i = 0; i + keep < runs.size(); ++i) {
        std::filesystem::remove_all(runs[i], error);
        if (error)
            spdlog::warn("Failed to prune simulation history run {}: {}", runs[i].string(), error.message());
    }
}

std::vector<HistoryRun> scan_simulation_history(const std::filesystem::path& history_directory)
{
    // no history directory means no runs
    if (!std::filesystem::is_directory(history_directory))
        return {};
    // collect the run folders
    std::vector<std::filesystem::path> run_directories;
    std::error_code error;
    for (const auto& entry : std::filesystem::directory_iterator(history_directory, error)) {
        if (entry.is_directory() && is_simulation_history_run_name(entry.path().filename().string()))
            run_directories.push_back(entry.path());
    }
    // newest first
    std::ranges::sort(run_directories, std::ranges::greater{});
    // gather the files of each run
    std::vector<HistoryRun> runs;
    runs.reserve(run_directories.size());
    for (const auto& run_directory : run_directories) {
        HistoryRun run;
        run.timestamp = run_directory.filename().string();
        for (const auto& entry : std::filesystem::directory_iterator(run_directory, error)) {
            if (entry.is_regular_file())
                run.files.push_back(entry.path());
        }
        // sort the files by name
        std::ranges::sort(run.files);
        runs.push_back(std::move(run));
    }
    return runs;
}
