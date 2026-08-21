#include <chrono>

#include <spdlog/spdlog.h>

#import <AppKit/AppKit.h>
#import <objc/runtime.h>

#include <slint.h>

#include "modal_manager.h"

// A modal dialog must keep receiving keyboard input while the rest of the app is
// blocked. winit routes keyboard events to the window whose native window is the
// AppKit key window, and winit's NSWindow subclass always answers
// `canBecomeKeyWindow`/`canBecomeMainWindow` with true. So instead of a sheet
// (which pins the dialog and makes it unmovable), the dialog is made an
// independent, key-capable child window of the main window (so it stays movable
// and accepts text), while the *main* window is temporarily made unable to become
// key/main (so it cannot steal focus) and is covered by an invisible view that
// swallows every pointer event (so clicks on it do nothing at all).

// invisible pointer-event swallowing view placed over the main window's content
@interface SwallowView : NSView
@end

@implementation SwallowView

- (NSView*)hitTest:(NSPoint)point {
    return self;
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (void)mouseDown:(NSEvent*)event {
}

- (void)mouseDragged:(NSEvent*)event {
}

- (void)mouseUp:(NSEvent*)event {
}

- (void)otherMouseDown:(NSEvent*)event {
}

- (void)otherMouseDragged:(NSEvent*)event {
}

- (void)otherMouseUp:(NSEvent*)event {
}

- (void)rightMouseDown:(NSEvent*)event {
}

- (void)rightMouseDragged:(NSEvent*)event {
}

- (void)rightMouseUp:(NSEvent*)event {
}

- (void)scrollWheel:(NSEvent*)event {
}

- (void)mouseMoved:(NSEvent*)event {
}
@end

namespace
{
    // parent and dialog slint windows captured while a dialog is open; they are
    // used to resolve the native windows on each polling tick while the native
    // windows are still being created lazily by winit
    slint::Window* pending_parent = nullptr;
    slint::Window* pending_dialog = nullptr;

    // resolved native windows once both exist
    NSWindow* parent_window = nil;
    NSWindow* dialog_window = nil;

    // transparent view that swallows pointer events, covering the main window's
    // content while a dialog is open
    NSView* swallow_view = nil;

    // true while the parent is blocked and the dialog is shown over it
    bool modal_active = false;

    // swizzled `canBecomeKeyWindow` / `canBecomeMainWindow` implementations
    IMP original_can_become_key = nullptr;
    IMP original_can_become_main = nullptr;

    // retry timer that waits for both native windows to materialize; the dialog
    // window is created by winit on the first frame after show(), which may be
    // later than the point where the modal block is entered
    slint::Timer attach_timer;

    // while the dialog is open the parent can never become key/main, so it cannot
    // steal keyboard focus: the dialog is the only logical window for input.
    BOOL Runtime_CanBecomeKeyWindow(id self, SEL _cmd) {
        // check that the swizzled method is called on the parent window, not the dialog
        if (modal_active && self == (id)parent_window)
            return NO;
        return ((BOOL (*)(id, SEL))original_can_become_key)(self, _cmd);
    }

    BOOL Runtime_CanBecomeMainWindow(id self, SEL _cmd) {
        if (modal_active && self == (id)parent_window)
            return NO;
        return ((BOOL (*)(id, SEL))original_can_become_main)(self, _cmd);
    }

    // temporarily prevent the main window from becoming key/main so clicking it
    // cannot steal keyboard focus from the dialog. Swizzling the winit window
    // class is scoped by an instance check so the dialog windows (also winit
    // windows) keep answering true.
    void install_parent_block() {
        Class parent_class = [parent_window class];
        Method key_method = class_getInstanceMethod(parent_class, @selector(canBecomeKeyWindow));
        Method main_method = class_getInstanceMethod(parent_class, @selector(canBecomeMainWindow));
        if (key_method != NULL && main_method != NULL) {
            original_can_become_key = method_getImplementation(key_method);
            original_can_become_main = method_getImplementation(main_method);
            method_setImplementation(key_method, (IMP)Runtime_CanBecomeKeyWindow);
            method_setImplementation(main_method, (IMP)Runtime_CanBecomeMainWindow);
        }
    }

    void uninstall_parent_block() {
        Class parent_class = [parent_window class];
        Method key_method = class_getInstanceMethod(parent_class, @selector(canBecomeKeyWindow));
        Method main_method = class_getInstanceMethod(parent_class, @selector(canBecomeMainWindow));
        if (key_method != NULL && original_can_become_key != nullptr)
            method_setImplementation(key_method, original_can_become_key);
        if (main_method != NULL && original_can_become_main != nullptr)
            method_setImplementation(main_method, original_can_become_main);
        original_can_become_key = nullptr;
        original_can_become_main = nullptr;
    }

    // cover the main window's content with the pointer-swallowing view
    void install_swallow_view() {
        NSView* content = [parent_window contentView];
        SwallowView* view = [[SwallowView alloc] initWithFrame:[content bounds]];
        [view setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
        [content addSubview:view];
        swallow_view = view;
    }

    void remove_swallow_view() {
        [swallow_view removeFromSuperview];
        swallow_view = nil;
    }

    // once both native windows exist: make the dialog an independent child window
    // above the parent so it stays movable, make it key so text input works, and
    // block the parent window input
    void activate_modal() {
        NSWindow* parent_ns = (__bridge NSWindow*)[(NSView*)pending_parent->appkit_view() window];
        NSWindow* dialog_ns = (__bridge NSWindow*)[(NSView*)pending_dialog->appkit_view() window];
        // the dialog window is created lazily on the first frame after show()
        if (parent_ns == nil || dialog_ns == nil)
            return;
        // both windows are ready, stop polling
        attach_timer.stop();
        // remember the resolved windows
        parent_window = parent_ns;
        dialog_window = dialog_ns;
        // keep the dialog above the main window, yet independently movable and
        // key-capable (it is its own window, not a sheet)
        [parent_ns addChildWindow:dialog_ns ordered:NSWindowAbove];
        // make the parent unable to become key/main so it can't steal focus
        install_parent_block();
        // the dialog is the operational window and receives keyboard input
        [dialog_ns makeKeyWindow];
        // swallow all clicks on the main window
        [dialog_ns orderFront:nil];
        install_swallow_view();
        modal_active = true;
        spdlog::debug("modal_manager: dialog shown as blocked child of main window");
    }

    void deactivate_modal() {
        attach_timer.stop();
        if (modal_active) {
            remove_swallow_view();
            uninstall_parent_block();
            if (parent_window != nil && dialog_window != nil)
                [parent_window removeChildWindow:dialog_window];
            [dialog_window resignKeyWindow];
            [dialog_window orderOut:nil];
            modal_active = false;
        }
        parent_window = nil;
        dialog_window = nil;
        pending_parent = nullptr;
        pending_dialog = nullptr;
    }
} // namespace

void modal_manager::set_input_blocked(slint::Window& parent, slint::Window& dialog, bool blocked) {
    if (blocked) {
        // remember the slint windows for the polling retries
        pending_parent = &parent;
        pending_dialog = &dialog;
        // try to activate the modal block right away
        activate_modal();
        // poll at 16ms intervals while the dialog window is not ready yet
        if (!modal_active) {
            attach_timer.start(slint::TimerMode::Repeated, std::chrono::milliseconds(16), [] { activate_modal(); });
        }
    }
    else {
        deactivate_modal();
    }
}
