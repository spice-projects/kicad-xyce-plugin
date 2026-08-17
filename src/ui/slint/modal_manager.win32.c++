#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>

#include <spdlog/spdlog.h>

#include <slint.h>

#include "modal_manager.h"

namespace
{
    // the HWND of the main window while its input is blocked; enabling is not
    // possible through the blocked window itself, so the handle is kept here
    HWND blocked_hwnd = nullptr;

    // the HWND of the dialog shown above the blocked parent; it is made an
    // owned window so it stays on top of the parent
    HWND dialog_hwnd = nullptr;
} // namespace

void modal_manager::set_input_blocked(slint::Window& parent_window, slint::Window& dialog_window, bool blocked) {
    // the native window handles backing the slint windows
    HWND parent_hwnd = parent_window.win32_hwnd();
    HWND child_hwnd = dialog_window.win32_hwnd();
    // no native window yet (first show may create the backing HWND lazily)
    if (parent_hwnd == nullptr)
        return;
    if (blocked) {
        // remember the windows so unblock can restore the same instances
        blocked_hwnd = parent_hwnd;
        dialog_hwnd = child_hwnd;
        // make the dialog an owned window of the parent so it stays on top
        // while the parent is disabled
        if (dialog_hwnd != nullptr)
            SetWindowLongPtr(dialog_hwnd, GWLP_HWNDPARENT, reinterpret_cast<LONG_PTR>(parent_hwnd));
        // stop the main window from receiving input; the dialog window keeps
        // the keyboard and mouse focus for as long as it is enabled
        EnableWindow(parent_hwnd, FALSE);
    }
    else {
        // restore input to the main window
        if (blocked_hwnd != nullptr && IsWindow(blocked_hwnd))
            EnableWindow(blocked_hwnd, TRUE);
        // break the owner relationship established at block time, so future
        // dialogs can be owned by the same main window again
        if (dialog_hwnd != nullptr && IsWindow(dialog_hwnd))
            SetWindowLongPtr(dialog_hwnd, GWLP_HWNDPARENT, 0);
        blocked_hwnd = nullptr;
        dialog_hwnd = nullptr;
    }
    spdlog::debug("modal_manager: main window input {} ({})", blocked ? "blocked" : "unblocked", static_cast<void*>(parent_hwnd));
}
