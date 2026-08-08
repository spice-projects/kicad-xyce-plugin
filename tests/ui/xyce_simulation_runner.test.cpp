#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/event.h>
#endif

#include "ui/events.h"
#include "ui/xyce_simulation_runner.h"

TEST(XyceSimulationRunnerChecks, create_temp_netlist_creates_file_with_content) {
    // arrange
    std::string content = "Test netlist content\n.OP\n.END\n";
    // act
    auto path = XyceSimulationRunner::create_temp_netlist(content);
    // assert
    ASSERT_FALSE(path.empty());
    ASSERT_TRUE(std::filesystem::exists(path));
    // read back and verify content
    std::ifstream file(path);
    ASSERT_TRUE(file.is_open());
    std::string read_content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    ASSERT_EQ(read_content, content);
    // cleanup
    file.close();
    std::filesystem::remove(path);
}

TEST(XyceSimulationRunnerChecks, create_temp_netlist_handles_empty_content) {
    // arrange
    std::string content;
    // act
    auto path = XyceSimulationRunner::create_temp_netlist(content);
    // assert
    ASSERT_FALSE(path.empty());
    ASSERT_TRUE(std::filesystem::exists(path));
    ASSERT_EQ(std::filesystem::file_size(path), 0);
    // cleanup
    std::filesystem::remove(path);
}

TEST(XyceSimulationRunnerChecks, create_temp_netlist_produces_unique_paths) {
    // arrange
    // act
    auto path1 = XyceSimulationRunner::create_temp_netlist("one");
    auto path2 = XyceSimulationRunner::create_temp_netlist("two");
    // assert
    ASSERT_NE(path1, path2);
    // cleanup
    std::filesystem::remove(path1);
    std::filesystem::remove(path2);
}

TEST(XyceSimulationRunnerChecks, constructor_initializes_not_running) {
    // arrange / act
    XyceSimulationRunner runner;
    // assert
    ASSERT_FALSE(runner.is_running());
    ASSERT_FALSE(runner.was_canceled());
    ASSERT_EQ(runner.exit_code(), -1);
    ASSERT_TRUE(runner.working_directory().empty());
    ASSERT_TRUE(runner.netlist_file_path().empty());
}

TEST(XyceSimulationRunnerChecks, cancel_before_start_is_safe) {
    // arrange / act
    XyceSimulationRunner runner;
    // should not crash or assert
    runner.cancel();
    // assert
    ASSERT_FALSE(runner.is_running());
    ASSERT_FALSE(runner.was_canceled());
}

TEST(XyceSimulationRunnerChecks, destructor_without_start_is_safe) {
    // arrange / act / assert (should not crash or leak)
    {
        XyceSimulationRunner runner;
        ASSERT_FALSE(runner.is_running());
    }
}

TEST(XyceSimulationRunnerChecks, create_temp_netlist_file_is_writable) {
    // arrange
    std::string content = "Write test content";
    auto path = XyceSimulationRunner::create_temp_netlist(content);
    // act
    std::ofstream file(path, std::ios::app);
    ASSERT_TRUE(file.is_open());
    file << " appended";
    file.close();
    // assert
    std::ifstream read_file(path);
    std::string result((std::istreambuf_iterator<char>(read_file)), std::istreambuf_iterator<char>());
    ASSERT_EQ(result, "Write test content appended");
    // cleanup
    std::filesystem::remove(path);
}

// simulation event tests
TEST(XyceSimulationEventChecks, stdout_event_carries_string_payload) {
    // arrange
    wxThreadEvent event(wxEVT_SIMULATION_STDOUT);
    std::string test_line = "V(out) = 5.0";
    // act
    event.SetPayload(test_line);
    // assert
    ASSERT_EQ(event.GetPayload<std::string>(), test_line);
}

TEST(XyceSimulationEventChecks, stderr_event_carries_string_payload) {
    // arrange
    wxThreadEvent event(wxEVT_SIMULATION_STDERR);
    std::string test_line = "Warning: no .PRINT statement";
    // act
    event.SetPayload(test_line);
    // assert
    ASSERT_EQ(event.GetPayload<std::string>(), test_line);
}

TEST(XyceSimulationEventChecks, finished_event_carries_exit_code_and_canceled) {
    // arrange
    wxThreadEvent event(wxEVT_SIMULATION_FINISHED);
    // act
    event.SetInt(42);
    event.SetPayload(bool(true));
    // assert
    ASSERT_EQ(event.GetInt(), 42);
    ASSERT_EQ(event.GetPayload<bool>(), true);
}

TEST(XyceSimulationEventChecks, finished_event_carries_zero_exit_code) {
    // arrange
    wxThreadEvent event(wxEVT_SIMULATION_FINISHED);
    // act
    event.SetInt(0);
    event.SetPayload(bool(false));
    // assert
    ASSERT_EQ(event.GetInt(), 0);
    ASSERT_EQ(event.GetPayload<bool>(), false);
}
