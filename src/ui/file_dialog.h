#pragma once

#include <filesystem>
#include <optional>

// platform-native open file dialog used by the slint ui; implemented per
// platform in file_dialog.osx.mm / file_dialog.win32.c++ / file_dialog.linux.c++
class FileDialog
{
public:
    // show a native dialog to select a Xyce input/output file; returns the
    // selected path or std::nullopt when the user cancels
    [[nodiscard]] static std::optional<std::filesystem::path> open_xyce_file();
};
