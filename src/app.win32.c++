#include <spdlog/spdlog.h>

#include "app.h"
#include "kicad/kicad_session.h"
#include "ui/main_window.h"

bool App::OnInit() {
    // initialize base
    if (!wxApp::OnInit())
        return false;
    // keep the event loop alive until every application frame is closed
    SetExitOnFrameDelete(false);
    // log information
    spdlog::info("Starting KiCad Xyce Plugin on Windows");
    // initialize image handlers
    wxInitAllImageHandlers();
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
    // return true to continue running the app
    return true;
}
