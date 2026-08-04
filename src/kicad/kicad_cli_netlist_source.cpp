#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/filename.h>
#include <wx/process.h>
#include <wx/utils.h>
#endif

#include <spdlog/spdlog.h>

#include "kicad_cli_netlist_source.h"

namespace
{
    // prefix used by wxFileName::CreateTempFileName for exported netlists
    constexpr const char* TEMP_FILE_PREFIX = "kicad_xyce_";

    // read the full content of an open stream
    std::string read_stream(wxInputStream* stream) {
        // empty string when no stream is available
        if (!stream)
            return {};
        // reserve a reasonable buffer for the content
        std::string content;
        // read available chunks
        while (stream->CanRead()) {
            // read a chunk of data
            char chunk[4096];
            size_t bytes_read = stream->Read(chunk, sizeof(chunk)).LastRead();
            // stop at the end of the stream
            if (bytes_read == 0)
                break;
            // append the chunk
            content.append(chunk, bytes_read);
        }
        // exit
        return content;
    }

    // run kicad-cli and return the process exit code, capturing stdout and stderr
    int run_kicad_cli(const std::filesystem::path& command_cwd, const std::vector<std::string>& args, std::string& stdout_text, std::string& stderr_text) {
        // build the command line with quoted arguments
        wxString command_line;
        // iterate over the arguments
        for (const auto& arg : args) {
            // quote each argument to preserve spaces in paths
            command_line += "\"" + wxString(arg) + "\" ";
        }
        // trim the trailing space
        command_line.Trim();
        // process object that captures the redirected output
        wxProcess process;
        process.Redirect();
        // working directory for the export
        wxExecuteEnv env;
        env.cwd = command_cwd.string();
        // execute the command synchronously
        long exit_code = wxExecute(command_line, wxEXEC_SYNC, &process, &env);
        // read the captured stdout and stderr
        stdout_text = read_stream(process.GetInputStream());
        stderr_text = read_stream(process.GetErrorStream());
        // exit
        return static_cast<int>(exit_code);
    }
} // namespace

KicadCliNetlistSource::KicadCliNetlistSource(std::filesystem::path project_dir, std::string kicad_cli_path) :
    m_project_dir(std::move(project_dir)), m_kicad_cli_path(std::move(kicad_cli_path)) {
    // resolve the schematic path for this project up front
    m_schematic_path = resolve_schematic_path(m_project_dir);
}

[[nodiscard]] std::string KicadCliNetlistSource::title() const { return m_schematic_path.filename().string(); }

[[nodiscard]] bool KicadCliNetlistSource::is_read_only() const { return true; }

[[nodiscard]] std::filesystem::path KicadCliNetlistSource::working_directory() const { return m_project_dir; }

std::tuple<bool, std::string> KicadCliNetlistSource::load_netlist() {
    // check if the schematic changed since the last export
    const auto last_modified = std::filesystem::last_write_time(m_schematic_path);
    // re-export when never exported or when the schematic changed
    const bool needs_export = !m_has_exported || last_modified != m_last_export_time;
    // return the cached content when the schematic is unchanged
    if (!needs_export) {
        // exit with no re-export performed
        return {false, m_cached_netlist};
    }
    // log information
    spdlog::info("Exporting schematic netlist from {}", m_schematic_path.string());
    // create a temporary output file for the exported netlist
    const wxString temp_path = wxFileName::CreateTempFileName(TEMP_FILE_PREFIX);
    // fail when no temporary file could be created
    if (temp_path.IsEmpty())
        throw std::runtime_error("failed to create a temporary netlist output file");
    // resolve the temporary output path
    const std::filesystem::path output_path = temp_path.ToStdString();
    // run the export through the kicad-cli binary
    std::string stdout_text;
    std::string stderr_text;
    // build the export command arguments
    const std::vector<std::string> args = {m_kicad_cli_path, "sch", "export", "netlist", "--format", "spice", "--output", output_path.string(), m_schematic_path.string()};
    // execute the export
    const int exit_code = run_kicad_cli(m_project_dir, args, stdout_text, stderr_text);
    // fail when the export did not complete successfully
    if (exit_code != 0) {
        // log the failure with captured output
        spdlog::error("kicad-cli netlist export failed ({}): {}", exit_code, stderr_text.empty() ? stdout_text : stderr_text);
        // remove the temporary file
        std::error_code ec;
        std::filesystem::remove(output_path, ec);
        // throw with the process output diagnostics
        throw std::runtime_error("failed to export schematic netlist with kicad-cli: " + (stderr_text.empty() ? stdout_text : stderr_text));
    }
    // read the exported netlist content
    std::ifstream file(output_path, std::ios::in);
    // fail when the exported file could not be opened
    if (!file.is_open()) {
        // throw with a descriptive error
        throw std::runtime_error("failed to open the exported netlist file");
    }
    // read the exported content
    std::string netlist;
    netlist.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    // close the file
    file.close();
    // remove the temporary file
    std::error_code ec;
    std::filesystem::remove(output_path, ec);
    // cache the exported content
    m_cached_netlist = netlist;
    // store the export time for change detection
    m_last_export_time = last_modified;
    m_has_exported = true;
    // indicate a fresh export was performed
    return {true, std::move(netlist)};
}

void KicadCliNetlistSource::save_netlist(const std::string&) {}

std::filesystem::path KicadCliNetlistSource::resolve_schematic_path(const std::filesystem::path& project_dir) {
    // error code for project file lookup
    std::error_code error_code;
    // find the .kicad_pro file to derive the true project name
    for (const auto& entry : std::filesystem::directory_iterator(project_dir, error_code)) {
        // skip non-project files
        if (entry.path().extension() != ".kicad_pro")
            continue;
        // schematic file with the same stem is the root schematic
        const auto root_schematic = project_dir / (entry.path().stem().string() + ".kicad_sch");
        // use the root schematic when it exists
        if (std::filesystem::exists(root_schematic))
            return root_schematic;
    }
    // iterate over the project directory
    for (const auto& entry : std::filesystem::directory_iterator(project_dir, error_code)) {
        // skip directories
        if (entry.is_directory(error_code))
            continue;
        // only consider schematic files
        if (entry.path().extension() != ".kicad_sch")
            continue;
        // use the first schematic found as a fallback when no root schematic exists
        return entry.path();
    }
    // no schematic found
    throw std::runtime_error("no schematic file (.kicad_sch) found inside " + project_dir.string());
}
