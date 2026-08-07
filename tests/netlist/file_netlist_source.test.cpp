#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include <gtest/gtest.h>

#include "netlist/file_netlist_source.h"

TEST(FileNetlistSourceChecks, title_returns_filename) {
    // arrange
    const auto path = std::filesystem::temp_directory_path() / "file_netlist_source_title.cir";
    FileNetlistSource source(path);
    // assert
    EXPECT_EQ(source.title(), "file_netlist_source_title.cir");
}

TEST(FileNetlistSourceChecks, is_read_only_returns_false) {
    // arrange
    const auto path = std::filesystem::temp_directory_path() / "file_netlist_source_ro.cir";
    FileNetlistSource source(path);
    // assert
    EXPECT_FALSE(source.is_read_only());
}

TEST(FileNetlistSourceChecks, working_directory_returns_parent_path) {
    // arrange
    const auto path = std::filesystem::temp_directory_path() / "file_netlist_source_dir.cir";
    FileNetlistSource source(path);
    // assert
    EXPECT_EQ(source.working_directory(), path.parent_path());
}

TEST(FileNetlistSourceChecks, load_netlist_reads_file_content) {
    // arrange
    const auto path = std::filesystem::temp_directory_path() / "file_netlist_source_load.cir";
    {
        std::ofstream out(path, std::ios::out | std::ios::trunc);
        out << "R1 1 0 100\n.END\n";
    }
    FileNetlistSource source(path);
    // act
    const auto [reloaded, content] = source.load_netlist();
    // assert
    EXPECT_TRUE(reloaded);
    EXPECT_EQ(content, "R1 1 0 100\n.END\n");
    // cleanup
    std::filesystem::remove(path);
}

TEST(FileNetlistSourceChecks, load_netlist_returns_false_when_file_missing) {
    // arrange
    const auto path = std::filesystem::temp_directory_path() / "file_netlist_source_missing.cir";
    std::filesystem::remove(path);
    FileNetlistSource source(path);
    // act
    const auto [reloaded, content] = source.load_netlist();
    // assert
    EXPECT_FALSE(reloaded);
    EXPECT_TRUE(content.empty());
}

TEST(FileNetlistSourceChecks, save_netlist_writes_content_to_file) {
    // arrange
    const auto path = std::filesystem::temp_directory_path() / "file_netlist_source_save.cir";
    {
        std::ofstream out(path, std::ios::out | std::ios::trunc);
        out << "old";
    }
    FileNetlistSource source(path);
    // act
    source.save_netlist("R1 1 0 100\n.END\n");
    // assert
    std::ifstream in(path);
    const std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    EXPECT_EQ(content, "R1 1 0 100\n.END\n");
    // cleanup
    std::filesystem::remove(path);
}

TEST(FileNetlistSourceChecks, save_netlist_without_content_truncates_file) {
    // arrange
    const auto path = std::filesystem::temp_directory_path() / "file_netlist_source_truncate.cir";
    {
        std::ofstream out(path, std::ios::out | std::ios::trunc);
        out << "old content";
    }
    FileNetlistSource source(path);
    // act
    source.save_netlist();
    // assert
    EXPECT_EQ(std::filesystem::file_size(path), 0);
    // cleanup
    std::filesystem::remove(path);
}
