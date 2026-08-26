#include <cctype>
#include <memory>
#include <string>

#include <slint.h>
#include <spdlog/spdlog.h>

#include "../core/util.h"
#include "app.h"

#include "../kicad/kicad_session.h"
#include "../netlist/editor_netlist_source.h"
#include "../ui/main_window_presenter.h"
#include "../ui/main_window_view.h"

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
    // the main window's netlist source: the schematic-backed source when running as a KiCad plugin (taken from the session), or an editable editor source standalone
    std::unique_ptr<NetlistSource> netlist_source = m_kicad_session != nullptr ? m_kicad_session->take_netlist_source() : std::make_unique<EditorNetlistSource>([]() -> std::string { return std::string{}; }, std::filesystem::path{});
    // the main window goes through the same creation path as any spawned window
    auto* main_presenter = create_window(std::move(netlist_source), m_kicad_session);
    // extract the schematic netlist before the first frame (KiCad plugin mode)
    if (m_kicad_session != nullptr)
        main_presenter->on_extract_schematic_netlist();
    // run the slint event loop until the last window closes
    slint::run_event_loop();
    // WORKAROUND (see slint-bug.md): slint 1.17.1 caches the native context menu item tree in
    // WinitWindowAdapter::context_menu and never releases it, and that field is declared after the
    // renderer, so destroying a window frees the skia renderer first and the cached menu item tree
    // then calls free_graphics_resources() on it. The result is a use-after-free that aborts on exit
    // as soon as the charts context menu has been opened once. Intentionally leak every window (the
    // main window and any spawned through new_window()) so the window adapters are never destroyed;
    // the process is exiting and the OS reclaims the memory. Remove once the slint bug is fixed.
    for (auto& window : m_windows)
        (void)window.release();
    // the event loop exited, end the application
    return 0;
}

void App::new_window(std::shared_ptr<XyceOutputFile> raw_file) {
    // spawned windows are standalone (no kicad session); the window is wired
    // and shown by create_window
    auto* presenter = create_window(std::make_unique<EditorNetlistSource>([]() -> std::string { return std::string{}; }, std::filesystem::path{}), nullptr);
    // seed the new window with the raw file, switching to the charts view; the
    // window was shown by create_window, so the native content view exists when
    // the charts renderer attaches
    presenter->load_raw_file(std::move(raw_file));
}

SlintMainWindowPresenter2* App::create_window(std::unique_ptr<NetlistSource> netlist_source, std::shared_ptr<KiCadSession> session) {
    // the view needs only a placeholder netlist source; the presenter owns the real source
    auto instance = std::make_unique<WindowInstance>();
    instance->view = std::make_unique<SlintMainWindowView2>(std::make_unique<EditorNetlistSource>([]() -> std::string { return std::string{}; }, std::filesystem::path{}), PluginConfig::load());
    instance->presenter = std::make_unique<SlintMainWindowPresenter2>(*instance->view, std::move(netlist_source), PluginConfig::load(), std::move(session));
    // wire the event handler so the view forwards user interactions to the presenter
    instance->view->set_event_handler(*instance->presenter);
    // keep the window alive while the event loop runs
    m_windows.push_back(std::move(instance));
    // show the window; the caller seeds the content afterwards
    m_windows.back()->view->show();
    // hand back the presenter so the caller can seed the window
    return m_windows.back()->presenter.get();
}
