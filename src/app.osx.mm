#import <AppKit/AppKit.h>

#include <spdlog/spdlog.h>

#include "app.h"
#include "kicad/kicad_session.h"
#include "ui/icon_data.h"
#include "ui/main_window.h"

bool App::OnInit() {
    // initialize base
    if (!wxApp::OnInit())
        return false;
    // keep the event loop alive until every application frame is closed
    SetExitOnFrameDelete(false);
    // log information
    spdlog::info("Starting KiCad Xyce Plugin on macOS");
    // initialize image handlers
    wxInitAllImageHandlers();
    // use system appearance for the app
    SetAppearance(Appearance::System);
    // build a session when running as a KiCad plugin
    auto session = KiCadSession::from_environment();
    // share the session with the main window when present
    std::shared_ptr<KiCadSession> shared_session;
    if (session)
        shared_session = std::make_shared<KiCadSession>(std::move(*session));
    // create main window instance
    const auto frame = new MainWindow("KiCad Xyce Plugin", std::move(shared_session));
    // show main window
    frame->Show(true);
    // set application dock icon from embedded png bytes
    NSData *png_data = [NSData dataWithBytes:window_icon_512x512_png length:window_icon_512x512_png_len];
    // create nsimage from png data
    NSImage *image = [[NSImage alloc] initWithData:png_data];
    // check if image was loaded
    if (image) {
        // set the application icon in the dock
        [NSApp setApplicationIconImage:image];
    }
    // return true to continue running the app
    return true;
}
