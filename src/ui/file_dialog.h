#pragma once

#include <filesystem>
#include <optional>

// platform-native open file dialog backed by nativefiledialog-extended (btzy/nativefiledialog-extended)
class FileDialog
{
public:
    // show a native dialog to select a Xyce input/output file; returns the
    // selected path or std::nullopt when the user cancels
    [[nodiscard]] static std::optional<std::filesystem::path> open_xyce_file();

    // show a native dialog to select the Xyce executable; returns the selected
    // path or std::nullopt when the user cancels
    [[nodiscard]] static std::optional<std::filesystem::path> open_xyce_executable();
};
