#include <filesystem>
#include <fstream>
#include <system_error>

#include <gtest/gtest.h>

#include "config/plugin_config.h"

TEST(PluginConfigChecks, is_valid_returns_false_for_empty_path) {
    // arrange
    PluginConfig config("");
    // act
    bool valid = config.is_xyce_executable_valid();
    // assert
    ASSERT_FALSE(valid);
}

TEST(PluginConfigChecks, is_valid_returns_false_for_non_existent_path) {
    // arrange
    PluginConfig config("/nonexistent/path/to/Xyce");
    // act
    bool valid = config.is_xyce_executable_valid();
    // assert
    ASSERT_FALSE(valid);
}

TEST(PluginConfigChecks, is_valid_returns_true_for_self) {
    // arrange
    std::string self_path = testing::internal::GetArgvs()[0];
    PluginConfig config(self_path);
    // act
    bool valid = config.is_xyce_executable_valid();
    // assert
    ASSERT_TRUE(valid);
}

TEST(PluginConfigChecks, discover_returns_empty_when_not_on_path) {
    // arrange / act
    std::string found = PluginConfig::discover_xyce_executable();
    // assert
    if (found.empty()) {
        SUCCEED();
    }
    else {
        PluginConfig config(found);
        ASSERT_TRUE(config.is_xyce_executable_valid());
    }
}

TEST(PluginConfigChecks, default_config_validity) {
    // arrange / act
    PluginConfig config = PluginConfig::default_config();
    // assert
    if (!config.xyce_executable_path().empty()) {
        ASSERT_TRUE(config.is_xyce_executable_valid());
    }
}

TEST(PluginConfigChecks, round_trips_xyce_path) {
    // arrange
    std::string test_path = "/usr/bin/env";
    PluginConfig original(test_path);
    // act
    original.save();
    PluginConfig loaded = PluginConfig::load();
    // assert
    ASSERT_EQ(loaded.xyce_executable_path(), test_path);
    // cleanup: restore the path to empty to avoid test pollution
    PluginConfig empty("");
    empty.save();
}

TEST(PluginConfigChecks, is_valid_returns_false_for_directory) {
    // arrange
    PluginConfig config("/usr/local");
    // act
    bool valid = config.is_xyce_executable_valid();
    // assert
    ASSERT_FALSE(valid);
}

TEST(PluginConfigChecks, is_valid_returns_false_for_non_executable_file) {
#if defined(_WIN32)
    GTEST_SKIP() << "Windows does not have an executable permission bit; any regular file passes validity";
#else
    // arrange — a regular file without the executable bit
    const auto path = std::filesystem::temp_directory_path() / "kicad_xyce_not_executable.bin";
    {
        std::ofstream file(path);
        file << "data";
    }
    PluginConfig config(path.string());
    // act
    bool valid = config.is_xyce_executable_valid();
    // assert
    ASSERT_FALSE(valid);
    // cleanup
    std::error_code ec;
    std::filesystem::remove(path, ec);
#endif
}

TEST(PluginConfigChecks, is_valid_returns_true_for_executable_file) {
    // arrange — the running test binary is an executable file
    PluginConfig config(testing::internal::GetArgvs()[0]);
    // act
    bool valid = config.is_xyce_executable_valid();
    // assert
    ASSERT_TRUE(valid);
}

TEST(PluginConfigChecks, set_path_updates_the_configured_value) {
    // arrange
    PluginConfig config("");
    // act
#if defined(_WIN32)
    config.set_xyce_executable_path("C:\\Windows\\System32\\cmd.exe");
    // assert
    ASSERT_EQ(config.xyce_executable_path(), "C:\\Windows\\System32\\cmd.exe");
#else
    config.set_xyce_executable_path("/usr/bin/env");
    // assert
    ASSERT_EQ(config.xyce_executable_path(), "/usr/bin/env");
#endif
    ASSERT_TRUE(config.is_xyce_executable_valid());
}
