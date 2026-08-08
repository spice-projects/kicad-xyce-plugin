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
