#include <algorithm>
#include <cctype>
#include <string>

#include <main-window.h>
#include <slint.h>
#include <spdlog/spdlog.h>

#include "app.h"
#include "kicad/kicad_session.h"

App& App::instance() {
    static App app;
    return app;
}

App::~App() = default;

int App::run() {
    // build a session when running as a KiCad plugin
    auto session = KiCadSession::from_environment();
    // share the session with the app when present
    if (session)
        m_kicad_session = std::make_shared<KiCadSession>(std::move(*session));
    // create the main window instance from the slint component
    auto main_window = MainWindow::create();
    // register the window for application lifetime tracking
    register_window();
    // show the main window
    main_window->show();
    // run the slint event loop until the last window closes
    slint::run_event_loop();
    // the event loop exited, end the application
    return 0;
}

void App::initialize(int argc, char** argv) {
    // parse command line arguments
    for (int i = 1; i < argc; ++i) {
        // --log-level VALUE or -l VALUE
        if (i + 1 < argc && (std::string(argv[i]) == "--log-level" || std::string(argv[i]) == "-l")) {
            m_log_level = argv[++i];
        }
        // --log-level=VALUE
        else if (std::string(argv[i]).starts_with("--log-level=")) {
            m_log_level = std::string(argv[i]).substr(12);
        }
    }
    // normalize to lowercase (matches prior wxString::Lower() behaviour)
    std::transform(m_log_level.begin(), m_log_level.end(), m_log_level.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    // apply the configured log level
    setup_logger();
    // run platform-specific initialization (dock icon, etc.)
    platform_initialize();
}

void App::register_window() {
    // track each main window independently of transient top-level windows
    m_window_count++;
}

void App::unregister_window() {
    // close notifications only originate from registered windows
    if (m_window_count == 0)
        return;
    // remove the window from the application lifetime count
    m_window_count--;
    // if the last window has been closed, exit the application
    if (m_window_count == 0) {
        // defer to the event loop to avoid reentrancy during window destruction
        slint::invoke_from_event_loop([] {
            // another window may have been created before the deferred quit runs
            if (instance().m_window_count == 0)
                slint::quit_event_loop();
        });
    }
}

void App::setup_logger() {
    // default log level
    spdlog::set_level(spdlog::level::info);
    // debug
    if (m_log_level == "debug") {
        spdlog::set_level(spdlog::level::debug);
        return;
    }
    // warn
    if (m_log_level == "warn") {
        spdlog::set_level(spdlog::level::warn);
        return;
    }
    // error
    if (m_log_level == "error") {
        spdlog::set_level(spdlog::level::err);
        return;
    }
}
