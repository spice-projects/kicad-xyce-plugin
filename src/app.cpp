#include <cctype>
#include <memory>
#include <string>

#include <slint.h>
#include <spdlog/spdlog.h>

#include "app.h"
#include "util.h"

#include "kicad/kicad_session.h"
#include "ui/editor_netlist_source.h"
#include "ui/slint/main_window_presenter.h"
#include "ui/slint/main_window_view.h"

App& App::instance() {
    static App app;
    return app;
}

App::~App() = default;

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
    // normalize to lowercase
    m_log_level = to_lower(m_log_level);
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

int App::run() {
    // build a session when running as a KiCad plugin
    auto session = KiCadSession::from_environment();
    // share the session with the app when present
    if (session)
        m_kicad_session = std::make_shared<KiCadSession>(std::move(*session));
    // create the view and presenter separately; the parent owns both and wires them together so neither knows about the other at compile time
    auto view = std::make_unique<SlintMainWindowView2>(std::make_unique<EditorNetlistSource>([]() -> std::string { return std::string{}; }, std::filesystem::path{}), PluginConfig::load());
    // the presenter netlist source: the schematic-backed source when running as a
    // KiCad plugin (taken from the session), or an editable editor source standalone
    std::unique_ptr<NetlistSource> netlist_source;
    if (m_kicad_session != nullptr)
        netlist_source = m_kicad_session->take_netlist_source();
    else
        netlist_source = std::make_unique<EditorNetlistSource>([]() -> std::string { return std::string{}; }, std::filesystem::path{});
    auto presenter = std::make_unique<SlintMainWindowPresenter2>(*view, std::move(netlist_source), PluginConfig::load(), m_kicad_session);
    // wire the event handler so the view forwards user interactions to the presenter
    view->set_event_handler(*presenter);
    // extract the schematic netlist before showing the window (KiCad plugin mode)
    if (m_kicad_session != nullptr)
        presenter->on_extract_schematic_netlist();
    // show the main window
    view->show();
    // run the slint event loop until the last window closes
    slint::run_event_loop();
    // WORKAROUND (see slint-bug.md): slint 1.17.1 caches the native context menu item tree in
    // WinitWindowAdapter::context_menu and never releases it, and that field is declared after the
    // renderer, so destroying the window frees the skia renderer first and the cached menu item tree
    // then calls free_graphics_resources() on it. The result is a use-after-free that aborts on exit
    // as soon as the charts context menu has been opened once. Intentionally leak the view (and the
    // presenter it is wired to) so the window adapter is never destroyed; the process is exiting and
    // the OS reclaims the memory. Remove once the slint bug is fixed.
    (void)view.release();
    (void)presenter.release();
    // the event loop exited, end the application
    return 0;
}
