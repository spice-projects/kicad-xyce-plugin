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
        const char* path_env = std::getenv("PATH");
        if (!path_env)
            return {};
        std::string path_str(path_env);
        std::string separator = ";";
#else
        const char* path_env = std::getenv("PATH");
        if (!path_env)
            return {};
        std::string path_str(path_env);
        std::string separator = ":";
#endif
        size_t start = 0;
        while (start < path_str.size()) {
            size_t end = path_str.find(separator, start);
            std::string dir_path = path_str.substr(start, end - start);
            if (!dir_path.empty()) {
                std::error_code ec;
                for (auto& entry : std::filesystem::directory_iterator(dir_path, ec)) {
                    std::string filename = entry.path().filename().string();
                    // check for "Xyce" filename (append ".exe" on Windows)
#ifdef __WXMSW__
                    if (filename == "Xyce.exe" || filename == "Xyce.EXE") {
#else
                    if (filename == "Xyce") {
#endif
                        if (wxIsExecutable(entry.path().string())) {
                            return entry.path().string();
                        }
                    }
                }
            }
            if (end == std::string::npos)
                break;
            start = end + separator.size();
        }
        return {};
    }
} // namespace

PluginConfig::PluginConfig(std::string xyce_executable_path) :
    m_xyce_executable_path(std::move(xyce_executable_path)) {
}

PluginConfig PluginConfig::load() {
    // open application-level wxConfig store
    wxConfig config("kicad-xyce-plugin", "GitHub Spice Projects");
    wxString stored_path;
    bool found = config.Read(CONFIG_PATH, &stored_path);
    // return stored path when available, otherwise fall back to discovery
    if (found && !stored_path.IsEmpty()) {
        spdlog::info("Loaded Xyce executable path from config: {}", stored_path.ToStdString());
        return PluginConfig(stored_path.ToStdString());
    }
    auto discovered = discover_xyce_executable();
    if (!discovered.empty()) {
        spdlog::info("Discovered Xyce executable path: {}", discovered);
    }
    else {
        spdlog::info("No Xyce executable discovered");
    }
    return PluginConfig(std::move(discovered));
}

PluginConfig PluginConfig::default_config() {
    return PluginConfig(discover_xyce_executable());
}

void PluginConfig::save() const {
    // persist the current executable path to wxConfig
    wxConfig config("kicad-xyce-plugin", "GitHub Spice Projects");
    bool written = config.Write(CONFIG_PATH, wxString(m_xyce_executable_path));
    if (written) {
        spdlog::info("Saved Xyce executable path: {}", m_xyce_executable_path);
    }
    else {
        spdlog::warn("Failed to save Xyce executable path to config");
    }
    config.Flush();
}

bool PluginConfig::is_xyce_executable_valid() const {
    // reject empty paths before filesystem checks
    if (m_xyce_executable_path.empty())
        return false;
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
        spdlog::info("Found Xyce on PATH: {}", found);
        return found;
    }
    // fall back to well-known binary-installer directories
#ifdef __WXMSW__
    wxString base = "C:\\Program Files";
    wxDir dir(base);
    if (dir.IsOpened()) {
        wxString subdir;
        bool cont = dir.GetFirst(&subdir, "XyceNF_*", wxDIR_DIRS);
        // iterate all matching directories keeping the last (highest-versioned) one
        wxString best;
        while (cont) {
            best = subdir;
            cont = dir.GetNext(&subdir);
        }
        if (!best.IsEmpty()) {
            wxString candidate = base + "\\" + best + "\\bin\\Xyce.exe";
            if (wxFileExists(candidate)) {
                spdlog::info("Found Xyce in install directory: {}", candidate.ToStdString());
                return candidate.ToStdString();
            }
        }
    }
#else
    wxDir dir("/usr/local");
    if (dir.IsOpened()) {
        wxString subdir;
        bool cont = dir.GetFirst(&subdir, "XyceNF_*", wxDIR_DIRS);
        // iterate all matching directories keeping the last (highest-versioned) one
        wxString best;
        while (cont) {
            best = subdir;
            cont = dir.GetNext(&subdir);
        }
        if (!best.IsEmpty()) {
            wxString candidate = "/usr/local/" + best + "/bin/Xyce";
            if (wxFileExists(candidate)) {
                spdlog::info("Found Xyce in install directory: {}", candidate.ToStdString());
                return candidate.ToStdString();
            }
        }
    }
#endif
    spdlog::warn("Xyce executable not found via discovery");
    return {};
}