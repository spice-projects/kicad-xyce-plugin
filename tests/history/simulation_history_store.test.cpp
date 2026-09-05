#include <chrono>
#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

#include "history/simulation_history_store.h"

TEST(SimulationHistoryStoreChecks, file_type_maps_extensions) {
    // arrange
    // act / assert
    ASSERT_EQ(history_file_type("run.cir.raw"), "raw");
    ASSERT_EQ(history_file_type("run.fft0"), "fft");
    ASSERT_EQ(history_file_type("run.fft12"), "fft");
    ASSERT_EQ(history_file_type("run.out"), "out");
    ASSERT_EQ(history_file_type("run.log"), "log");
    ASSERT_EQ(history_file_type("run.cir"), "cir");
    ASSERT_EQ(history_file_type("noext"), "");
}

TEST(SimulationHistoryStoreChecks, timestamp_matches_run_folder_pattern) {
    // arrange
    // act
    const std::string stamp = simulation_history_timestamp();
    // assert
    ASSERT_EQ(stamp.size(), 19);
    ASSERT_TRUE(is_simulation_history_run_name(stamp));
}

TEST(SimulationHistoryStoreChecks, run_name_rejects_non_run_names) {
    // arrange
    // act / assert
    ASSERT_FALSE(is_simulation_history_run_name(""));
    ASSERT_FALSE(is_simulation_history_run_name("short"));
    ASSERT_FALSE(is_simulation_history_run_name("not-a-timestamp-fold"));
    ASSERT_FALSE(is_simulation_history_run_name("2026-9-04_12-00-00"));
    ASSERT_FALSE(is_simulation_history_run_name("2026-09-04 12-00-00"));
    ASSERT_FALSE(is_simulation_history_run_name("2026-09-04_12-00-00_x"));
    // valid variants
    ASSERT_TRUE(is_simulation_history_run_name("2026-09-04_12-00-00"));
    ASSERT_TRUE(is_simulation_history_run_name("2026-09-04_12-00-00_7"));
}

TEST(SimulationHistoryStoreChecks, history_directory_is_hidden_subfolder) {
    // arrange
    // act
    const auto dir = simulation_history_directory("/tmp/project");
    // assert
    ASSERT_EQ(dir.string(), "/tmp/project/.kicad-xyce-history");
}

TEST(SimulationHistoryStoreChecks, record_run_copies_files_and_scan_returns_them) {
    // arrange a unique scratch directory with two fake output files
    const auto scratch = std::filesystem::temp_directory_path() / ("kicad-xyce-history-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + "-copy");
    std::filesystem::remove_all(scratch);
    std::filesystem::create_directories(scratch);
    const auto raw_file = scratch / "netlist.cir.raw";
    const auto log_file = scratch / "netlist.cir.out";
    { std::ofstream(raw_file) << "raw data"; }
    { std::ofstream(log_file) << "log line"; }
    const auto history_dir = scratch / ".kicad-xyce-history";
    // act
    const auto recorded = record_simulation_run(history_dir, {raw_file, log_file}, 20);
    const auto runs = scan_simulation_history(history_dir);
    // assert
    ASSERT_TRUE(recorded.has_value());
    ASSERT_TRUE(recorded->failed_files.empty());
    ASSERT_EQ(runs.size(), 1);
    ASSERT_EQ(runs[0].timestamp, recorded->timestamp);
    ASSERT_EQ(runs[0].files.size(), 2);
    ASSERT_TRUE(std::filesystem::exists(history_dir / recorded->timestamp / "netlist.cir.raw"));
    ASSERT_TRUE(std::filesystem::exists(history_dir / recorded->timestamp / "netlist.cir.out"));
    // cleanup
    std::filesystem::remove_all(scratch);
}

TEST(SimulationHistoryStoreChecks, record_run_reports_missing_files) {
    // arrange a unique scratch directory with one real file and one missing
    const auto scratch = std::filesystem::temp_directory_path() / ("kicad-xyce-history-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + "-missing");
    std::filesystem::remove_all(scratch);
    std::filesystem::create_directories(scratch);
    const auto raw_file = scratch / "netlist.cir.raw";
    { std::ofstream(raw_file) << "raw data"; }
    // act
    const auto recorded = record_simulation_run(scratch / ".kicad-xyce-history", {raw_file, scratch / "gone.raw"}, 20);
    // assert
    ASSERT_TRUE(recorded.has_value());
    ASSERT_EQ(recorded->failed_files.size(), 1);
    ASSERT_EQ(recorded->failed_files[0].filename(), "gone.raw");
    // cleanup
    std::filesystem::remove_all(scratch);
}

TEST(SimulationHistoryStoreChecks, record_run_fails_without_write_access) {
    // arrange a file used in place of the history directory so folder creation fails
    const auto scratch = std::filesystem::temp_directory_path() / ("kicad-xyce-history-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + "-nowrite");
    std::filesystem::remove_all(scratch);
    std::filesystem::create_directories(scratch);
    const auto blocker = scratch / "blocker";
    { std::ofstream(blocker) << "not a directory"; }
    // act
    const auto recorded = record_simulation_run(blocker / ".kicad-xyce-history", {}, 20);
    // assert
    ASSERT_FALSE(recorded.has_value());
    // cleanup
    std::filesystem::remove_all(scratch);
}

TEST(SimulationHistoryStoreChecks, record_run_prunes_oldest_runs_beyond_max) {
    // arrange a unique scratch directory
    const auto scratch = std::filesystem::temp_directory_path() / ("kicad-xyce-history-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + "-prune");
    std::filesystem::remove_all(scratch);
    std::filesystem::create_directories(scratch);
    const auto raw_file = scratch / "netlist.cir.raw";
    { std::ofstream(raw_file) << "raw data"; }
    const auto history_dir = scratch / ".kicad-xyce-history";
    // act: record three runs within the same second, keeping only two
    const auto first = record_simulation_run(history_dir, {raw_file}, 2);
    const auto second = record_simulation_run(history_dir, {raw_file}, 2);
    const auto third = record_simulation_run(history_dir, {raw_file}, 2);
    const auto runs = scan_simulation_history(history_dir);
    // assert
    ASSERT_TRUE(first.has_value());
    ASSERT_TRUE(second.has_value());
    ASSERT_TRUE(third.has_value());
    ASSERT_NE(first->timestamp, second->timestamp);
    ASSERT_NE(second->timestamp, third->timestamp);
    ASSERT_EQ(runs.size(), 2);
    // the oldest run (the first one) was pruned, the newest two remain
    ASSERT_FALSE(std::filesystem::exists(history_dir / first->timestamp));
    ASSERT_TRUE(std::filesystem::exists(history_dir / second->timestamp));
    ASSERT_TRUE(std::filesystem::exists(history_dir / third->timestamp));
    // cleanup
    std::filesystem::remove_all(scratch);
}

TEST(SimulationHistoryStoreChecks, scan_returns_newest_first_and_ignores_foreign_folders) {
    // arrange a unique scratch history directory with two runs and one foreign folder
    const auto scratch = std::filesystem::temp_directory_path() / ("kicad-xyce-history-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + "-scan");
    std::filesystem::remove_all(scratch);
    std::filesystem::create_directories(scratch / ".kicad-xyce-history" / "2026-01-02_03-04-05");
    std::filesystem::create_directories(scratch / ".kicad-xyce-history" / "2026-05-06_07-08-09");
    std::filesystem::create_directories(scratch / ".kicad-xyce-history" / "not-a-run");
    const auto older = scratch / ".kicad-xyce-history" / "2026-01-02_03-04-05" / "a.raw";
    const auto newer = scratch / ".kicad-xyce-history" / "2026-05-06_07-08-09" / "b.raw";
    { std::ofstream(older) << "old"; }
    { std::ofstream(newer) << "new"; }
    // act
    const auto runs = scan_simulation_history(scratch / ".kicad-xyce-history");
    // assert
    ASSERT_EQ(runs.size(), 2);
    ASSERT_EQ(runs[0].timestamp, "2026-05-06_07-08-09");
    ASSERT_EQ(runs[1].timestamp, "2026-01-02_03-04-05");
    ASSERT_EQ(runs[0].files.size(), 1);
    ASSERT_EQ(runs[0].files[0].filename(), "b.raw");
    // cleanup
    std::filesystem::remove_all(scratch);
}

TEST(SimulationHistoryStoreChecks, scan_returns_empty_for_missing_directory) {
    // arrange
    const auto missing = std::filesystem::temp_directory_path() / "kicad-xyce-history-test-does-not-exist";
    std::filesystem::remove_all(missing);
    // act
    const auto runs = scan_simulation_history(missing);
    // assert
    ASSERT_TRUE(runs.empty());
}
