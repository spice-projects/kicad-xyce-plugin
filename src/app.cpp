#include <spdlog/spdlog.h>
#include <wx/cmdline.h>

#include "app.h"

static const wxCmdLineEntryDesc CMD_LINE_DESCRIPTION[] = {{wxCMD_LINE_OPTION, "l", "log-level", "Set the logging level (e.g., debug, info, warn, error)", wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL}, {wxCMD_LINE_NONE, nullptr, nullptr, nullptr, wxCMD_LINE_VAL_NONE, 0x0}};

void App::OnInitCmdLine(wxCmdLineParser& parser) {
    // keep default command line options
    wxApp::OnInitCmdLine(parser);
    // add custom command line options
    parser.SetDesc(CMD_LINE_DESCRIPTION);
}

bool App::OnCmdLineParsed(wxCmdLineParser& parser) {
    // call base class implementation, exit application if it returns false
    if (!wxApp::OnCmdLineParsed(parser))
        return false;
    // default log level
    spdlog::set_level(spdlog::level::info);
    // --log-level
    wxString log_level;
    if (parser.Found("log-level", &log_level))
        setup_logger(log_level.Lower());
    // run the application
    return true;
}

void App::register_frame() {
    // track each application frame independently of transient top-level windows
    m_frame_count++;
}

void App::unregister_frame() {
    // close notifications only originate from registered application frames
    if (m_frame_count == 0)
        return;
    // remove the frame from the application lifetime count
    m_frame_count--;
    // if the last frame has been closed, exit the application
    if (m_frame_count == 0) {
        // exit after the final frame's close event schedules its destruction
        CallAfter([this]() {
            // another frame may have been created before the deferred exit runs
            if (m_frame_count == 0)
                ExitMainLoop();
        });
    }
}

void App::setup_logger(const wxString& log_level) {
    // set log level
    if (log_level == "debug") {
        // debug
        spdlog::set_level(spdlog::level::debug);
        // exit
        return;
    }
    if (log_level == "warn") {
        // info
        spdlog::set_level(spdlog::level::warn);
        // exit
        return;
    }
    if (log_level == "error") {
        // info
        spdlog::set_level(spdlog::level::err);
        // exit
        return;
    }
}
