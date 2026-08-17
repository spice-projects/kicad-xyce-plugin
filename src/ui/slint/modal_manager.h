#pragma once

namespace slint
{
    class Window;
}

// platform-neutral modal dialog manager. The slint Dialog component owns a
// separate native window, so a visual scrim inside the main window can neither
// cover it nor reliably block its input. To emulate modality for the lifetime
// of a slint dialog, the underlying OS window of the main window is blocked for
// input (the same mechanism native modal dialogs use) and is re-enabled when the
// dialog closes. There is no nested event loop involved: the slint event loop
// keeps running, the OS simply stops routing input to the blocked window.
namespace modal_manager
{
    // block or unblock pointer/keyboard input to the OS window backing the
    // given slint window. `dialog_window` is the dialogue window shown over it;
    // on platforms where a native parent/dialog relationship exists, blocking
    // records the dialog so it stays on top of the blocked parent. Unsupported
    // platforms degrade to a no-op (input is then still blocked at the slint
    // level by the view gating its actions).
    void set_input_blocked(slint::Window& parent_window, slint::Window& dialog_window, bool blocked);
} // namespace modal_manager
