#include <cstdlib>
#include <filesystem>
#include <string>

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/config.h>
#include <wx/dir.h>
#include <wx/filefn.h>
#include <wx/log.h>
#include <wx/utils.h>
#endif

#include <spdlog/spdlog.h>

#include "plugin_config.h"

namespace
{
    // config key under which the Xyce executable path is stored
    constexpr const char* CONFIG_PATH = "/PluginConfig/XyceExecutablePath";

    // iterate PATH entries looking for a file named "Xyce" (or "Xyce.exe" on Windows)
    std::string search_path_for_xyce() {
#ifdef __WXMSW__
        // PATH environment variable value
        const char* path_env = std::getenv("PATH");
        if (!path_env)
            return {};
        // convert to std::string for easier manipulation, and use Windows path separator
        std::string path_str(path_env);
        std::string separator = ";";
#else
        // PATH environment variable value
        const char* path_env = std::getenv("PATH");
        if (!path_env)
            return {};
        // convert to std::string for easier manipulation, and use POSIX path separator
        std::string path_str(path_env);
        std::string separator = ":";
#endif
        // start index
        size_t start = 0;
        // iterate over each directory in PATH
        while (start < path_str.size()) {
            // find the next separator
            size_t end = path_str.find(separator, start);
            // extract the directory path
            std::string dir_path = path_str.substr(start, end - start);
            if (!dir_path.empty()) {
                // eror code
                std::error_code ec;
                // iterate over the directory entries
                for (auto& entry : std::filesystem::directory_iterator(dir_path, ec)) {
                    // file name (without directory path)
                    std::string filename = entry.path().filename().string();
#ifdef __WXMSW__
                    // check for "Xyce" filename (append ".exe" on Windows)
                    if (filename == "Xyce.exe" || filename == "Xyce.EXE") {
#else
                    // check for "Xyce" filename
                    if (filename == "Xyce") {
#endif
                        // check if the file is executable
                        if (wxIsExecutable(entry.path().string())) {
                            // exit
                            return entry.path().string();
                        }
                    }
                }
            }
            // if no separator was found, we are done
            if (end == std::string::npos)
                break;
            // move to the next directory in PATH
            start = end + separator.size();
        }
        return {};
    }
} // namespace

PluginConfig::PluginConfig(std::string xyce_executable_path) :
    m_xyce_executable_path(std::move(xyce_executable_path)) {}

PluginConfig PluginConfig::load() {
    // open application-level wxConfig store
    wxConfig config("kicad-xyce-plugin", "Spice Projects");
    // read the stored Xyce executable path from wxConfig
    wxString stored_path;
    bool found = config.Read(CONFIG_PATH, &stored_path);
    // return stored path when available, otherwise fall back to discovery
    if (found && !stored_path.IsEmpty()) {
        // log information
        spdlog::debug("Loaded Xyce executable path from config: {}", stored_path.ToStdString());
        // return the stored path
        return PluginConfig(stored_path.ToStdString());
    }
    // discover the Xyce executable path when not found in wxConfig
    auto discovered = discover_xyce_executable();
    if (!discovered.empty()) {
        // log information
        spdlog::debug("Discovered Xyce executable path: {}", discovered);
        // return the discovered path
        return PluginConfig(std::move(discovered));
    }
    // log information
    spdlog::info("No Xyce executable discovered");
    // exit
    return PluginConfig();
}

PluginConfig PluginConfig::default_config() { return PluginConfig(discover_xyce_executable()); }

void PluginConfig::save() const {
    // persist the current executable path to wxConfig
    wxConfig config("kicad-xyce-plugin", "Spice Projects");
    // write the path to the config
    if (config.Write(CONFIG_PATH, wxString(m_xyce_executable_path))) {
        // log information
        spdlog::debug("Saved Xyce executable path to config: {}", m_xyce_executable_path);
        // flush the config to ensure it is written to disk
        config.Flush();
        // exit
        return;
    }
    // log information
    spdlog::warn("Failed to save Xyce executable path to config");
}

bool PluginConfig::is_xyce_executable_valid() const {
    // reject empty paths before filesystem checks
    if (m_xyce_executable_path.empty())
        return false;
    // check if the path exists and is a regular file
    std::error_code ec;
    auto fs_status = std::filesystem::status(m_xyce_executable_path, ec);
    // reject paths that do not reference an existing regular file
    if (ec || !std::filesystem::is_regular_file(fs_status))
        return false;
    // require executable permission
    return wxIsExecutable(m_xyce_executable_path);
}

std::string PluginConfig::discover_xyce_executable() {
    // check PATH first (covers source builds, Spack, and custom installs)
    std::string found = search_path_for_xyce();
    if (!found.empty()) {
        // log information
        spdlog::debug("Found Xyce on PATH: {}", found);
        // exit
        return found;
    }
    // fall back to well-known binary-installer directories
#ifdef __WXMSW__
    // check "C:\Program Files" for XyceNF_* directories (binary installer)
    wxString base = "C:\\Program Files";
    wxDir dir(base);
    // check if the directory was opened successfully
    if (dir.IsOpened()) {
        // iterate over subdirectories matching the pattern "XyceNF_*"
        wxString subdir;
        bool cont = dir.GetFirst(&subdir, "XyceNF_*", wxDIR_DIRS);
        // iterate all matching directories keeping the last (highest-versioned) one
        wxString best;
        while (cont) {
            best = subdir;
            cont = dir.GetNext(&subdir);
        }
        // check if we found a matching directory
        if (!best.IsEmpty()) {
            // construct the candidate path to the Xyce executable
            wxString candidate = base + "\\" + best + "\\bin\\Xyce.exe";
            if (wxFileExists(candidate)) {
                // log information
                spdlog::debug("Found Xyce in install directory: {}", candidate.ToStdString());
                // exit
                return candidate.ToStdString();
            }
        }
    }
#else
    // check /usr/local for XyceNF_* directories (binary installer)
    wxDir dir("/usr/local");
    if (dir.IsOpened()) {
        // iterate over subdirectories matching the pattern "XyceNF_*"
        wxString subdir;
        bool cont = dir.GetFirst(&subdir, "XyceNF_*", wxDIR_DIRS);
        // iterate all matching directories keeping the last (highest-versioned) one
        wxString best;
        while (cont) {
            best = subdir;
            cont = dir.GetNext(&subdir);
        }
        // check if we found a matching directory
        if (!best.IsEmpty()) {
            // construct the candidate path to the Xyce executable
            wxString candidate = "/usr/local/" + best + "/bin/Xyce";
            // check if the candidate path exists and is a file
            if (wxFileExists(candidate)) {
                // log information
                spdlog::debug("Found Xyce in install directory: {}", candidate.ToStdString());
                // exit
                return candidate.ToStdString();
            }
        }
    }
#endif
    // log information
    spdlog::warn("Xyce executable not found via discovery");
    // exit with empty string
    return {};
}
