#include <filesystem>
#include <fstream>
#include <string>

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/arrstr.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/process.h>
#include <wx/timer.h>
#endif

#include <spdlog/spdlog.h>

#include "events.h"
#include "xyce_simulation_runner.h"

namespace
{
    // poll every 50 ms for new stdout/stderr data
    constexpr int IO_POLL_MS = 50;

    // fallback kill delay after SIGTERM (2 s)
    constexpr int KILL_TIMEOUT_MS = 2000;

    // prefix used by wxFileName::CreateTempFileName
    constexpr const char* TEMP_FILE_PREFIX = "xyce_";

    // read all available data from an input stream into a buffer, emit
    // complete lines to the given event type, and keep the partial trailing
    // line in the buffer for the next poll cycle
    void drain_stream(wxInputStream* stream, std::string& buffer, wxEventType event_type, wxEvtHandler* handler) {
        if (!stream || !stream->CanRead())
            return;
        // read available bytes
        while (stream->CanRead()) {
            char chunk[4096];
            size_t bytes_read = stream->Read(chunk, sizeof(chunk)).LastRead();
            if (bytes_read == 0)
                break;
            buffer.append(chunk, bytes_read);
        }
        // split on newlines and emit complete lines
        size_t pos = 0;
        while (true) {
            size_t newline = buffer.find('\n', pos);
            if (newline == std::string::npos)
                break;
            std::string line = buffer.substr(pos, newline - pos);
            // strip trailing carriage return
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            wxThreadEvent event(event_type);
            event.SetPayload(line);
            handler->ProcessEvent(event);
            pos = newline + 1;
        }
        // keep the remaining partial line
        if (pos > 0)
            buffer.erase(0, pos);
    }

    struct CwdGuard
    {
        std::filesystem::path prev;
        bool restored = false;

        explicit CwdGuard(const std::filesystem::path& new_cwd) :
            prev(std::filesystem::current_path()) {
            std::error_code ec;
            std::filesystem::current_path(new_cwd, ec);
        }

        ~CwdGuard() {
            restore();
        }

        void restore() {
            if (restored)
                return;
            restored = true;
            std::error_code ec;
            std::filesystem::current_path(prev, ec);
        }

        CwdGuard(const CwdGuard&) = delete;

        CwdGuard& operator=(const CwdGuard&) = delete;
    };
} // namespace

void XyceProcess::OnTerminate(int /*pid*/, int status) {
    m_exit_code = status;
    if (m_runner) {
        // wxWidgets will delete this process object after OnTerminate
        // returns, so defer the callback via CallAfter on the runner
        auto* runner = m_runner;
        runner->CallAfter([runner, status]() {
            runner->notify_process_ended(status);
        });
    }
}

XyceSimulationRunner::XyceSimulationRunner() {
    // bind the I/O polling timer to this evt handler
    m_io_timer.SetOwner(this, wxID_ANY);
    Bind(wxEVT_TIMER, &XyceSimulationRunner::on_timer, this, m_io_timer.GetId());
    // bind the kill-fallback timer
    m_kill_timer.SetOwner(this, wxID_ANY);
    Bind(wxEVT_TIMER, &XyceSimulationRunner::on_kill_timeout, this, m_kill_timer.GetId());
}

XyceSimulationRunner::~XyceSimulationRunner() {
    // ensure timers are stopped
    m_io_timer.Stop();
    m_kill_timer.Stop();
    // clean up temp netlist
    cleanup_temp_netlist();
}

void XyceSimulationRunner::start(const std::string& program, const std::filesystem::path& netlist_path, const std::filesystem::path& working_directory) {
    // reject duplicate start
    if (m_running) {
        spdlog::warn("XyceSimulationRunner::start called while already running");
        return;
    }
    // store paths
    m_netlist_file_path = netlist_path;
    m_working_directory = working_directory;
    m_temp_netlist_path = netlist_path;
    m_exit_code = -1;
    m_canceled = false;
    m_stdout_buffer.clear();
    m_stderr_buffer.clear();

    // create process with redirected I/O
    auto* process = new XyceProcess();
    process->Redirect();
    process->set_runner(this);

    // prepare command-line arguments as a null-terminated argv array
    std::string prog_path = program;
    std::string net_path = netlist_path.string();
    const char* argv[3] = {prog_path.c_str(), net_path.c_str(), nullptr};

    // set working directory so Xyce writes output files relative to it
    CwdGuard cwd_guard(working_directory);

    long pid = wxExecute(argv, wxEXEC_ASYNC, process);
    if (pid <= 0) {
        spdlog::error("XyceSimulationRunner::start failed to launch: {} {}", program, netlist_path.string());
        delete process;
        return;
    }

    m_process = process;
    m_pid = pid;
    m_running = true;

    // emit started event
    wxThreadEvent started_event(wxEVT_SIMULATION_STARTED);
    ProcessEvent(started_event);

    // start I/O polling timer
    m_io_timer.Start(IO_POLL_MS);

    spdlog::info("Xyce simulation started (pid {})", static_cast<int>(pid));
}

void XyceSimulationRunner::cancel() {
    if (!m_running || m_canceled)
        return;

    m_canceled = true;
    spdlog::info("Canceling simulation (pid {})", static_cast<int>(m_pid));

    // send SIGTERM for graceful shutdown
    wxProcess::Kill(m_pid, wxSIGTERM, wxKILL_CHILDREN);

    // schedule SIGKILL fallback after 2 s if still running
    m_kill_timer.StartOnce(KILL_TIMEOUT_MS);
}

void XyceSimulationRunner::notify_process_ended(int status) {
    m_exit_code = status;
    m_running = false;
    m_process = nullptr;

    // stop I/O polling
    m_io_timer.Stop();
    // cancel any pending kill timer
    m_kill_timer.Stop();

    // flush any remaining buffered lines
    emit_buffered_lines();

    // clean up temp netlist
    cleanup_temp_netlist();

    // emit finished event
    wxThreadEvent finished_event(wxEVT_SIMULATION_FINISHED);
    finished_event.SetInt(m_exit_code);
    finished_event.SetPayload(m_canceled);
    ProcessEvent(finished_event);

    spdlog::info("Xyce simulation finished (exit code {}, canceled {})", m_exit_code, m_canceled);
}

void XyceSimulationRunner::on_timer(wxTimerEvent&) {
    if (!m_process || !m_running)
        return;

    // drain stdout
    drain_stream(m_process->GetInputStream(), m_stdout_buffer, wxEVT_SIMULATION_STDOUT, this);
    // drain stderr
    drain_stream(m_process->GetErrorStream(), m_stderr_buffer, wxEVT_SIMULATION_STDERR, this);
}

void XyceSimulationRunner::on_kill_timeout(wxTimerEvent&) {
    if (!m_running)
        return;

    spdlog::warn("SIGKILL fallback for pid {}", static_cast<int>(m_pid));
    wxProcess::Kill(m_pid, wxSIGKILL, wxKILL_CHILDREN);
}

void XyceSimulationRunner::emit_buffered_lines() {
    // flush remaining stdout content as a partial line
    if (!m_stdout_buffer.empty()) {
        wxThreadEvent event(wxEVT_SIMULATION_STDOUT);
        event.SetPayload(m_stdout_buffer);
        ProcessEvent(event);
        m_stdout_buffer.clear();
    }
    // flush remaining stderr content as a partial line
    if (!m_stderr_buffer.empty()) {
        wxThreadEvent event(wxEVT_SIMULATION_STDERR);
        event.SetPayload(m_stderr_buffer);
        ProcessEvent(event);
        m_stderr_buffer.clear();
    }
}

void XyceSimulationRunner::cleanup_temp_netlist() {
    if (m_temp_netlist_path.empty())
        return;
    std::error_code ec;
    std::filesystem::remove(m_temp_netlist_path, ec);
    if (ec) {
        spdlog::warn("Failed to remove temp netlist: {}", m_temp_netlist_path.string());
    }
    m_temp_netlist_path.clear();
}

std::filesystem::path XyceSimulationRunner::create_temp_netlist(std::string_view text) {
    // generate a unique temporary file path
    wxString temp_path = wxFileName::CreateTempFileName(TEMP_FILE_PREFIX);
    if (temp_path.IsEmpty())
        return {};

    // write netlist content
    std::ofstream file(temp_path.ToStdString(), std::ios::out | std::ios::binary);
    if (!file.is_open())
        return {};

    file.write(text.data(), static_cast<std::streamsize>(text.size()));
    file.close();

    return std::filesystem::path(temp_path.ToStdString());
}