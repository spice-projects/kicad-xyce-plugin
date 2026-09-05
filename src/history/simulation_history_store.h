#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

// one recorded simulation run: the timestamp folder name and the copied files
struct HistoryRun
{
    std::string timestamp;
    std::vector<std::filesystem::path> files;
};

// result of recording a run: the created timestamp folder and the files that
// failed to copy (empty on full success)
struct RecordedRun
{
    std::string timestamp;
    std::vector<std::filesystem::path> failed_files;
};

// file type label for the history tree, derived from the file extension
[[nodiscard]] std::string history_file_type(const std::filesystem::path& file);

// timestamp string (UTC) used for run folder names: YYYY-MM-DD_HH-MM-SS
[[nodiscard]] std::string simulation_history_timestamp();

// history directory for a netlist directory: <netlist_directory>/.kicad-xyce-history
[[nodiscard]] std::filesystem::path simulation_history_directory(const std::filesystem::path& netlist_directory);

// check whether a directory name is a run folder: the timestamp pattern
// optionally followed by a numeric collision suffix
[[nodiscard]] bool is_simulation_history_run_name(const std::string& name);

// copy the given output files into a new timestamped run folder under the
// history directory, then prune old runs beyond max_runs; returns nullopt when
// the run folder could not be created (missing write access, ...)
[[nodiscard]] std::optional<RecordedRun> record_simulation_run(const std::filesystem::path& history_directory, const std::vector<std::filesystem::path>& files, int max_runs);

// remove the oldest run folders beyond max_runs
void prune_simulation_history(const std::filesystem::path& history_directory, int max_runs);

// scan the history directory for recorded runs, newest first
[[nodiscard]] std::vector<HistoryRun> scan_simulation_history(const std::filesystem::path& history_directory);
