#include <filesystem>
#include <optional>

#include <spdlog/spdlog.h>

#include "file_dialog.h"

std::optional<std::filesystem::path> FileDialog::open_xyce_file() {
    // native linux dialog not implemented yet
    spdlog::warn("open-xyce-file: linux file dialog not implemented");
    // no file selected
    return std::nullopt;
}

std::optional<std::filesystem::path> FileDialog::open_xyce_executable() {
    // native linux dialog not implemented yet
    spdlog::warn("open-xyce-executable: linux file dialog not implemented");
    // no file selected
    return std::nullopt;
}
