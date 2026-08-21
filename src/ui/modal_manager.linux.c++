#include <spdlog/spdlog.h>

#include <slint.h>

#include "modal_manager.h"

void modal_manager::set_input_blocked(slint::Window&, slint::Window&, bool) {
    // there is no native way to stop input to a single window on x11/wayland
    // through the slint api; modality is enforced at the slint level by the
    // view gating its actions while a dialog is open
    spdlog::debug("modal_manager: input blocking not supported on this platform");
}
