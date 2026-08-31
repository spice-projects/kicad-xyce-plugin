#include "file_dialog.h"

#include <array>
#include <filesystem>
#include <optional>

#include <nfd.hpp>
#include <spdlog/spdlog.h>

namespace
{
    // RAII guard that initializes NFD once per thread and shuts it down when
    // the guard goes out of scope; NFD requires init/quit on each thread that
    // uses it
    struct NFDGuard
    {
        NFDGuard() {
            // init the nativefiledialog library for this thread
            if (NFD::Init() != NFD_OKAY) {
                // initialization can fail when the platform backend is missing
                spdlog::error("NFD_Init failed: {}", NFD::GetError());
            }
        }

        ~NFDGuard() {
            // shutdown the nativefiledialog library for this thread
            NFD::Quit();
        }
    };

    // returns a reference to the process-wide NFD guard, ensuring NFD is
    // initialized on the first call and stays alive until process exit
    const NFDGuard& ensure_nfd() {
        // static storage guarantees a single init/quit per process
        static NFDGuard guard;
        return guard;
    }

} // anonymous namespace

std::optional<std::filesystem::path> FileDialog::open_xyce_file() {
    // ensure the native file dialog library is initialized
    ensure_nfd();

    // filter list limited to the two file types relevant to Xyce
    const std::array filters{
        nfdu8filteritem_t{"Xyce Files", "cir,raw"},
    };

    // open the native dialog with the filter applied
    nfdu8char_t* outPath = nullptr;
    nfdopendialogu8args_t args{};
    args.filterList = filters.data();
    args.filterCount = filters.size();
    const auto result = ::NFD_OpenDialogU8_With(&outPath, &args);
    // user canceled or an error occurred
    if (result != NFD_OKAY) {
        // log the error when the dialog failed programmatically
        if (result == NFD_ERROR) {
            // log information
            spdlog::error("open-xyce-file: NFD error: {}", NFD::GetError());
        }
        // no path to return
        return std::nullopt;
    }

    // convert the native path string to a filesystem path
    NFD::UniquePathU8 pathGuard(outPath);
    return std::filesystem::path(outPath);
}

std::optional<std::filesystem::path> FileDialog::open_xyce_executable() {
    // ensure the native file dialog library is initialized
    ensure_nfd();

    // open the native dialog with no filter so any executable is selectable
    nfdu8char_t* outPath = nullptr;
    const nfdopendialogu8args_t args{};
    const auto result = ::NFD_OpenDialogU8_With(&outPath, &args);
    // user canceled or an error occurred
    if (result != NFD_OKAY) {
        // log the error when the dialog failed programmatically
        if (result == NFD_ERROR) {
            // log information
            spdlog::error("open-xyce-executable: NFD error: {}", NFD::GetError());
        }
        // no path to return
        return std::nullopt;
    }

    // convert the native path string to a filesystem path
    NFD::UniquePathU8 pathGuard(outPath);
    return std::filesystem::path(outPath);
}
