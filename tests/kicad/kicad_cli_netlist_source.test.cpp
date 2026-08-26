#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <system_error>

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

namespace
{
    // create a fake kicad-cli shell script that writes a spice netlist to the --output path
    std::filesystem::path make_fake_kicad_cli(const std::string& name, const std::string& netlist, int exit_code) {
        // build a unique script path
        const auto path = std::filesystem::temp_directory_path() / name;
        // write the script body
        std::ofstream script(path, std::ios::out | std::ios::trunc);
        script << "#!/bin/sh\n";
        script << "out=\"\"\n";
        script << "prev=\"\"\n";
        script << "for arg in \"$@\"; do\n";
        script << "  if [ \"$prev\" = \"--output\" ]; then out=\"$arg\"; fi\n";
        script << "  prev=\"$arg\"\n";
        script << "done\n";
        if (exit_code == 0) {
            script << "printf '%s' '" << netlist << "' > \"$out\"\n";
        }
        script << "exit " << exit_code << "\n";
        script.close();
        // make the script executable
        std::error_code ec;
        std::filesystem::permissions(path, std::filesystem::perms::owner_exec, std::filesystem::perm_options::add, ec);
        // exit
        return path;
    }

    // create a project directory holding a root schematic
    std::filesystem::path make_project_dir(const std::string& name) {
        // temp project directory
        const auto project_dir = make_temp_dir(name);
        // create project and root schematic files
        write_file(project_dir / "demo.kicad_pro", "{}");
        write_file(project_dir / "demo.kicad_sch", "(kicad_sch)");
        // exit
        return project_dir;
    }
} // namespace

TEST(KicadCliNetlistSourceChecks, source_metadata_reflect_the_project) {
    // arrange
    const auto project_dir = make_project_dir("kicad_xyce_cli_meta");
    const auto cli = make_fake_kicad_cli("kicad_xyce_fake_cli_meta", "", 0);
    const KicadCliNetlistSource source(project_dir, cli.string());
    // act / assert
    EXPECT_EQ(source.title(), "demo.kicad_sch");
    EXPECT_TRUE(source.is_read_only());
    EXPECT_EQ(source.working_directory(), project_dir);
    // clean up
    std::error_code ec;
    std::filesystem::remove(cli, ec);
    std::filesystem::remove_all(project_dir, ec);
}

TEST(KicadCliNetlistSourceChecks, load_netlist_exports_through_kicad_cli) {
    // arrange
    const auto project_dir = make_project_dir("kicad_xyce_cli_load");
    const auto cli = make_fake_kicad_cli("kicad_xyce_fake_cli_load", "V1 1 0 5\nR1 1 0 1K\n.END\n", 0);
    KicadCliNetlistSource source(project_dir, cli.string());
    // act
    const auto [reloaded, content] = source.load_netlist();
    // assert — the exported netlist content was returned
    ASSERT_TRUE(reloaded);
    EXPECT_NE(content.find("R1 1 0 1K"), std::string::npos);
    EXPECT_NE(content.find(".END"), std::string::npos);
    // clean up
    std::error_code ec;
    std::filesystem::remove(cli, ec);
    std::filesystem::remove_all(project_dir, ec);
}

TEST(KicadCliNetlistSourceChecks, load_netlist_caches_until_the_schematic_changes) {
    // arrange
    const auto project_dir = make_project_dir("kicad_xyce_cli_cache");
    const auto cli = make_fake_kicad_cli("kicad_xyce_fake_cli_cache", "V1 1 0 5\n.END\n", 0);
    KicadCliNetlistSource source(project_dir, cli.string());
    const auto [first_reloaded, first_content] = source.load_netlist();
    ASSERT_TRUE(first_reloaded);
    // remove the fake cli so a re-export would fail
    std::error_code ec;
    std::filesystem::remove(cli, ec);
    // act — the schematic did not change since the last export
    const auto [second_reloaded, second_content] = source.load_netlist();
    // assert — the cached content was returned without re-exporting
    EXPECT_FALSE(second_reloaded);
    EXPECT_EQ(second_content, "V1 1 0 5\n.END\n");
    // clean up
    std::filesystem::remove_all(project_dir, ec);
}

TEST(KicadCliNetlistSourceChecks, load_netlist_throws_when_the_export_fails) {
    // arrange — a fake cli that exits with a failure code
    const auto project_dir = make_project_dir("kicad_xyce_cli_fail");
    const auto cli = make_fake_kicad_cli("kicad_xyce_fake_cli_fail", "", 1);
    KicadCliNetlistSource source(project_dir, cli.string());
    // act / assert
    ASSERT_THROW(static_cast<void>(source.load_netlist()), std::runtime_error);
    // clean up
    std::error_code ec;
    std::filesystem::remove(cli, ec);
    std::filesystem::remove_all(project_dir, ec);
}

TEST(KicadCliNetlistSourceChecks, save_netlist_is_a_no_op) {
    // arrange
    const auto project_dir = make_project_dir("kicad_xyce_cli_save");
    const auto cli = make_fake_kicad_cli("kicad_xyce_fake_cli_save", "", 0);
    KicadCliNetlistSource source(project_dir, cli.string());
    // act — must not throw or modify anything
    source.save_netlist("ignored");
    // assert
    SUCCEED();
    // clean up
    std::error_code ec;
    std::filesystem::remove(cli, ec);
    std::filesystem::remove_all(project_dir, ec);
}
