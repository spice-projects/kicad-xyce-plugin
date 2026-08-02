#pragma once

#include <filesystem>
#include <memory>
#include <string>

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/event.h>
#include <wx/process.h>
#include <wx/timer.h>
#endif

#include <wx/process.h>

class XyceProcess : public wxProcess
{
public:
    void set_runner(const std::weak_ptr<class XyceSimulationRunner>& runner) { m_runner = runner; }

    [[nodiscard]] int exit_code() const { return m_exit_code; }

private:
    int m_exit_code = -1;
    std::weak_ptr<class XyceSimulationRunner> m_runner;

    void OnTerminate(int, int) override;
};

class XyceSimulationRunner : public wxEvtHandler, public std::enable_shared_from_this<XyceSimulationRunner>
{
public:
    XyceSimulationRunner();
    ~XyceSimulationRunner() override;

    void start(const std::string& program, const std::filesystem::path& netlist_path, const std::filesystem::path& working_directory);

    void cancel();

    [[nodiscard]] bool is_running() const { return m_running; }

    [[nodiscard]] bool was_canceled() const { return m_canceled; }

    [[nodiscard]] int exit_code() const { return m_exit_code; }

    [[nodiscard]] const std::filesystem::path& working_directory() const { return m_working_directory; }

    [[nodiscard]] const std::filesystem::path& netlist_file_path() const { return m_netlist_file_path; }

    [[nodiscard]] static std::filesystem::path create_temp_netlist(std::string_view text);

    void notify_process_ended(int status);

private:
    XyceProcess* m_process = nullptr;
    long m_pid = 0;
    wxTimer m_io_timer;
    wxTimer m_kill_timer;

    std::filesystem::path m_netlist_file_path;
    std::filesystem::path m_working_directory;
    std::filesystem::path m_temp_netlist_path;

    std::string m_stdout_buffer;
    std::string m_stderr_buffer;

    int m_exit_code = -1;
    bool m_running = false;
    bool m_canceled = false;

    void on_timer(wxTimerEvent&);

    void emit_buffered_lines();

    void on_kill_timeout(wxTimerEvent&);

    void cleanup_temp_netlist();
};
