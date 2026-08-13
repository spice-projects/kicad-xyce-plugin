#include <algorithm>
#include <cctype>
#include <memory>
#include <string>

#include <slint.h>
#include <spdlog/spdlog.h>

#include "app.h"
#include "kicad/kicad_session.h"
#include "ui/editor_netlist_source.h"
#include "ui/slint/main_window_view.h"

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
    // create application main view and presenter, passing in the netlist source and plugin configuration
    auto view = std::make_unique<SlintMainWindowView>(std::make_unique<EditorNetlistSource>([]() -> std::string { return std::string{}; }, std::filesystem::path{}), PluginConfig::load());
    // show the main window
    view->show();
    // run the slint event loop until the last window closes
    slint::run_event_loop();
    // the event loop exited, end the application
    return 0;
}

void App::initialize(int argc, char** argv) {
    // parse command line arguments
    for (int i = 1; i < argc; ++i) {
        // --log-level VALUE or -l VALUE
        if (i + 1 < argc && (std::string(argv[i]) == "--log-level" || std::string(argv[i]) == "-l"))
            m_log_level = argv[++i];
        // --log-level=VALUE
        else if (std::string(argv[i]).starts_with("--log-level="))
            m_log_level = std::string(argv[i]).substr(12);
    }
    // normalize to lowercase (matches prior wxString::Lower() behaviour)
    std::transform(m_log_level.begin(), m_log_level.end(), m_log_level.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    // apply the configured log level
    setup_logger();
    // run platform-specific initialization (dock icon, etc.)
    platform_initialize();
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
