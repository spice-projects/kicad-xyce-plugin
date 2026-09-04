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
    // initialize GLFW
    if (glfwInit() != GLFW_TRUE) {
        // log information
        spdlog::error("Failed to initialize GLFW");
        // exit the application with an error code
        std::exit(1);
    }
    // explicitly disable OpenGL
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
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
    // // extract the schematic netlist before the first frame (KiCad plugin mode)
    // if (m_kicad_session != nullptr)
    //     main_presenter->on_extract_schematic_netlist();
    // // run the slint event loop until the last window closes
    // slint::run_event_loop();
    // // WORKAROUND (see slint-bug.md): slint 1.17.1 caches the native context menu item tree in
    // // WinitWindowAdapter::context_menu and never releases it, and that field is declared after the
    // // renderer, so destroying a window frees the skia renderer first and the cached menu item tree
    // // then calls free_graphics_resources() on it. The result is a use-after-free that aborts on exit
    // // as soon as the charts context menu has been opened once. Intentionally leak every window (the
    // // main window and any spawned through new_window()) so the window adapters are never destroyed;
    // // the process is exiting and the OS reclaims the memory. Remove once the slint bug is fixed.
    // // release the gpu context on every window before leaking it; the gpu
    // // context must be torn down before static destructors or skia's
    // // grmanagedresource trace asserts during teardown
    // for (auto& window : m_windows)
    //     window->view->release_gpu_resources();
    // // intentionally leak the windows so the slint workaround stays active
    // for (auto& window : m_windows)
    //     (void)window.release();

    // stay running until the last window closes
    while (!m_windows.empty()) {
        // wait for events, do not consume CPU
        glfwWaitEvents();
        // loop windows
        for (auto it = m_windows.begin(); it != m_windows.end();) {
            // window instance
            auto& window_instance = *it;
            // check we should close the window
            if (glfwWindowShouldClose(window_instance->window)) {
                // remove from vector, TODO: glfwDestroyWindow(window_instance->window) is not called, but the window is intentionally leaked to work around a slint bug (see slint-bug.md)
                it = m_windows.erase(it);
                // next
                continue;
            }
            // next
            ++it;
        }
    }
    // terminate GLFW
    glfwTerminate();
    // return success code
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

SlintMainWindowPresenter* App::create_window(std::unique_ptr<NetlistSource> netlist_source, std::shared_ptr<KiCadSession> session) {
    // plugin config
    auto config = PluginConfig::load();
    // the view needs only a placeholder netlist source; the presenter owns the real source
    auto instance = std::make_unique<WindowInstance>();
    // window, view and presenter
    instance->window = glfwCreateWindow(800, 600, "KiCad Xyce Plugin", nullptr, nullptr);
    // instance->view = std::make_unique<SlintMainWindowView>(std::make_unique<EditorNetlistSource>([]() -> std::string { return std::string{}; }, std::filesystem::path{}), config);
    // instance->presenter = std::make_unique<SlintMainWindowPresenter>(*instance->view, std::move(netlist_source), config, std::move(session));
    // // wire the event handler so the view forwards user interactions to the presenter
    // instance->view->set_event_handler(*instance->presenter);
    // keep the window alive while the event loop runs
    m_windows.push_back(std::move(instance));
    // // show the window; the caller seeds the content afterwards
    // m_windows.back()->view->show();
    // // hand back the presenter so the caller can seed the window
    // return m_windows.back()->presenter.get();

    return nullptr;
}
