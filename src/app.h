#pragma once

#include <cstddef>
#include <memory>
#include <string>

class KiCadSession;

class App
{
public:
    // access the singleton application instance
    static App& instance();

    // parse command line arguments, configure logging, and run platform setup
    void initialize(int argc, char** argv);

    // create the main window and run the application event loop; returns when the loop exits
    int run();

    // register a window for application lifetime tracking
    void register_window();

    // unregister a window; quits the event loop when the last window closes
    void unregister_window();

    // release the shared kicad session
    ~App();

    // the parsed logging level (e.g. "debug", "info", "warn", "error")
    [[nodiscard]] const std::string& log_level() const { return m_log_level; }

    // number of currently registered windows
    [[nodiscard]] size_t window_count() const { return m_window_count; }

    // the shared kicad session for plugin-mode communication, null when standalone
    [[nodiscard]] const std::shared_ptr<KiCadSession>& kicad_session() const { return m_kicad_session; }

private:
    App() = default;

    // apply the configured log level to spdlog
    void setup_logger();

    std::shared_ptr<KiCadSession> m_kicad_session;
    size_t m_window_count = 0;
    std::string m_log_level = "info";
};

// platform-specific initialization (e.g. dock icon), implemented per-platform
void platform_initialize();
