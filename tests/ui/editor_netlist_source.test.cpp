#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include <gtest/gtest.h>

#include "ui/editor_netlist_source.h"

namespace
{
    // write the given text to a file path for setup
    void write_file(const std::filesystem::path& path, const std::string& text) {
        // open the file in write mode
        std::ofstream out(path, std::ios::out | std::ios::trunc);
        // write the text
        out << text;
    }

    // read the full content of a file path
    std::string read_file(const std::filesystem::path& path) {
        // open the file for reading
        std::ifstream in(path);
        // read all content into a string
        return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
    }
} // namespace

TEST(EditorNetlistSourceChecks, first_load_reads_content_from_file) {
    // arrange
    const auto path = std::filesystem::temp_directory_path() / "editor_netlist_source_first.cir";
    write_file(path, "file content");
    std::string live_text = "live text";
    EditorNetlistSource source([&]() { return live_text; }, path);
    // act
    const auto [reloaded, content] = source.load_netlist();
    // assert
    EXPECT_TRUE(reloaded);
    EXPECT_EQ(content, "file content");
    // cleanup
    std::filesystem::remove(path);
}

TEST(EditorNetlistSourceChecks, title_and_working_directory_reflect_file) {
    // arrange
    const auto dir = std::filesystem::temp_directory_path();
    const auto path = dir / "editor_netlist_source_title.cir";
    write_file(path, "file content");
    std::string live_text;
    EditorNetlistSource source([&]() { return live_text; }, path);
    // act
    const auto title = source.title();
    const auto working_directory = source.working_directory();
    // assert
    EXPECT_EQ(title, "editor_netlist_source_title.cir");
    EXPECT_EQ(working_directory, path.parent_path());
    // cleanup
    std::filesystem::remove(path);
}

TEST(EditorNetlistSourceChecks, editor_source_is_never_read_only) {
    // arrange
    const auto path = std::filesystem::temp_directory_path() / "editor_netlist_source_ro.cir";
    write_file(path, "file content");
    std::string live_text;
    EditorNetlistSource source([&]() { return live_text; }, path);
    // assert
    EXPECT_FALSE(source.is_read_only());
    // cleanup
    std::filesystem::remove(path);
}

TEST(EditorNetlistSourceChecks, subsequent_loads_read_live_callback_text) {
    // arrange
    const auto path = std::filesystem::temp_directory_path() / "editor_netlist_source_subsequent.cir";
    write_file(path, "file content");
    std::string live_text = "edited text";
    EditorNetlistSource source([&]() { return live_text; }, path);
    (void)source.load_netlist();
    // act
    const auto [reloaded, content] = source.load_netlist();
    // assert
    EXPECT_FALSE(reloaded);
    EXPECT_EQ(content, "edited text");
    // cleanup
    std::filesystem::remove(path);
}

TEST(EditorNetlistSourceChecks, save_uses_live_callback_text_when_no_content_given) {
    // arrange
    const auto path = std::filesystem::temp_directory_path() / "editor_netlist_source_save.cir";
    write_file(path, "file content");
    std::string live_text = "live text";
    EditorNetlistSource source([&]() { return live_text; }, path);
    // act
    source.save_netlist();
    // assert
    EXPECT_EQ(read_file(path), "live text");
    // cleanup
    std::filesystem::remove(path);
}

TEST(EditorNetlistSourceChecks, save_uses_explicit_content_when_provided) {
    // arrange
    const auto path = std::filesystem::temp_directory_path() / "editor_netlist_source_explicit.cir";
    write_file(path, "file content");
    std::string live_text = "live text";
    EditorNetlistSource source([&]() { return live_text; }, path);
    // act
    source.save_netlist("explicit content");
    // assert
    EXPECT_EQ(read_file(path), "explicit content");
    // cleanup
    std::filesystem::remove(path);
}
