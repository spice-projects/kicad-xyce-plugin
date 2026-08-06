#include <filesystem>
#include <fstream>
#include <stdexcept>

#include <gtest/gtest.h>

#include "kicad/kicad_cli_netlist_source.h"

namespace
{
    // write a file with the given content
    void write_file(const std::filesystem::path& path, const std::string& content) {
        // open stream in write mode
        std::ofstream file(path, std::ios::out | std::ios::trunc);
        // write content into file
        file << content;
    }

    // create a temporary directory and return its path
    std::filesystem::path make_temp_dir(const std::string& name) {
        // build a unique directory path
        const auto path = std::filesystem::temp_directory_path() / name;
        // create the directory tree
        [[maybe_unused]] const bool created = std::filesystem::create_directories(path);
        // exit
        return path;
    }
} // namespace

TEST(KicadCliNetlistSourceChecks, resolve_uses_project_stem_schematic) {
    // temp project directory
    const auto project_dir = make_temp_dir("kicad_xyce_proj_stem");
    // create project and root schematic files
    write_file(project_dir / "demo.kicad_pro", "{}");
    write_file(project_dir / "demo.kicad_sch", "(kicad_sch)");
    // act
    const auto schematic = KicadCliNetlistSource::resolve_schematic_path(project_dir);
    // assert
    ASSERT_EQ(schematic.filename().string(), "demo.kicad_sch");
    // clean up
    [[maybe_unused]] const auto removed = std::filesystem::remove_all(project_dir);
}

TEST(KicadCliNetlistSourceChecks, resolve_falls_back_to_first_schematic) {
    // temp project directory
    const auto project_dir = make_temp_dir("kicad_xyce_proj_fallback");
    // create only a schematic file
    write_file(project_dir / "sheet.kicad_sch", "(kicad_sch)");
    // act
    const auto schematic = KicadCliNetlistSource::resolve_schematic_path(project_dir);
    // assert
    ASSERT_EQ(schematic.filename().string(), "sheet.kicad_sch");
    // clean up
    [[maybe_unused]] const auto removed = std::filesystem::remove_all(project_dir);
}

TEST(KicadCliNetlistSourceChecks, resolve_throws_without_schematic) {
    // temp project directory
    const auto project_dir = make_temp_dir("kicad_xyce_proj_empty");
    // assert
    ASSERT_THROW(static_cast<void>(KicadCliNetlistSource::resolve_schematic_path(project_dir)), std::runtime_error);
    // clean up
    [[maybe_unused]] const auto removed = std::filesystem::remove_all(project_dir);
}
