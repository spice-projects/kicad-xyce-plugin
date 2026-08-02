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

    void drain_stream(wxInputStream* stream, std::string& buffer, wxEventType event_type, wxEvtHandler* handler) {
        // check stream
        if (!stream || !stream->CanRead())
            return;
        // read available bytes
        while (stream->CanRead()) {
            // read a chunk of data
            char chunk[4096];
            size_t bytes_read = stream->Read(chunk, sizeof(chunk)).LastRead();
            if (bytes_read == 0)
                break;
            // append to buffer
            buffer.append(chunk, bytes_read);
        }
        // split on newlines and emit complete lines
        size_t pos = 0;
        while (true) {
            // find '\n'
            size_t newline = buffer.find('\n', pos);
            if (newline == std::string::npos)
                break;
            // extract line (without '\n')
            std::string line = buffer.substr(pos, newline - pos);
            // strip trailing carriage return (\r\n)
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            // create event and emit
            wxThreadEvent event(event_type);
            event.SetPayload(line);
            handler->ProcessEvent(event);
            // move to next position
            pos = newline + 1;
        }
        // keep the remaining partial line
        if (pos > 0)
            buffer.erase(0, pos);
    }

    struct CwdGuard
    {
        std::filesystem::path previous_cwd;

        explicit CwdGuard(const std::filesystem::path& new_cwd) :
            // store current working directory
            previous_cwd(std::filesystem::current_path()) {
            // change to new working directory
            std::error_code ec;
            std::filesystem::current_path(new_cwd, ec);
        }

        ~CwdGuard() {
            // restore previous working directory
            std::error_code ec;
            std::filesystem::current_path(previous_cwd, ec);
        }

        CwdGuard(const CwdGuard&) = delete;

        CwdGuard& operator=(const CwdGuard&) = delete;
    };
} // namespace

void XyceProcess::OnTerminate(int /*pid*/, int status) {
    // store exit code
    m_exit_code = status;
    // take a strong reference so the runner stays alive until the deferred callback runs
    auto runner = m_runner.lock();
    // check runner reference
    if (runner) {
        // send notification to the runner on the main thread
        runner->CallAfter([runner, status]() { runner->notify_process_ended(status); });
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
        // log information
        spdlog::warn("XyceSimulationRunner::start called while already running");
        // exit
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
    process->set_runner(shared_from_this());
    // prepare command-line arguments as a null-terminated argv array
    std::string prog_path = program;
    std::string net_path = netlist_path.string();
    // arguments: [program, netlist_path, nullptr]
    const char* argv[3] = {prog_path.c_str(), net_path.c_str(), nullptr};
    // set working directory so Xyce writes output files relative to it
    CwdGuard cwd_guard(working_directory);
    // execute the process asynchronously
    long pid = wxExecute(argv, wxEXEC_ASYNC, process);
    if (pid <= 0) {
        // log information
        spdlog::error("XyceSimulationRunner::start failed to launch: {} {}", program, netlist_path.string());
        // delete object to avoid memory leak
        delete process;
        // exit
        return;
    }
    // store process and pid
    m_process = process;
    m_pid = pid;
    // state
    m_running = true;
    // emit started event
    wxThreadEvent started_event(wxEVT_SIMULATION_STARTED);
    ProcessEvent(started_event);
    // start I/O polling timer
    m_io_timer.Start(IO_POLL_MS);
    // log information
    spdlog::info("Xyce simulation started (pid {})", static_cast<int>(pid));
}

void XyceSimulationRunner::cancel() {
    // check if already canceled or not running
    if (!m_running || m_canceled)
        return;
    // update state
    m_canceled = true;
    // log information
    spdlog::info("Canceling simulation (pid {})", static_cast<int>(m_pid));
    // send SIGTERM for graceful shutdown
    wxProcess::Kill(m_pid, wxSIGTERM, wxKILL_CHILDREN);
    // schedule SIGKILL fallback after 2 s if still running
    m_kill_timer.StartOnce(KILL_TIMEOUT_MS);
}

void XyceSimulationRunner::notify_process_ended(int status) {
    // store exit code
    m_exit_code = status;
    // state
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
    // log information
    spdlog::info("Xyce simulation finished (exit code {}, canceled {})", m_exit_code, m_canceled);
}

void XyceSimulationRunner::on_timer(wxTimerEvent&) {
    // check process state
    if (!m_process || !m_running)
        return;
    // drain stdout
    drain_stream(m_process->GetInputStream(), m_stdout_buffer, wxEVT_SIMULATION_STDOUT, this);
    // drain stderr
    drain_stream(m_process->GetErrorStream(), m_stderr_buffer, wxEVT_SIMULATION_STDERR, this);
}

void XyceSimulationRunner::on_kill_timeout(wxTimerEvent&) {
    // check process state
    if (!m_running)
        return;
    // log information
    spdlog::warn("SIGKILL fallback for pid {}", static_cast<int>(m_pid));
    // send SIGKILL to the process and its children
    wxProcess::Kill(m_pid, wxSIGKILL, wxKILL_CHILDREN);
}

void XyceSimulationRunner::emit_buffered_lines() {
    // flush remaining stdout content as a partial line
    if (!m_stdout_buffer.empty()) {
        // emit the remaining stdout content as a single event
        wxThreadEvent event(wxEVT_SIMULATION_STDOUT);
        event.SetPayload(m_stdout_buffer);
        ProcessEvent(event);
        // clear the buffer
        m_stdout_buffer.clear();
    }
    // flush remaining stderr content as a partial line
    if (!m_stderr_buffer.empty()) {
        // emit the remaining stderr content as a single event
        wxThreadEvent event(wxEVT_SIMULATION_STDERR);
        event.SetPayload(m_stderr_buffer);
        ProcessEvent(event);
        // clear the buffer
        m_stderr_buffer.clear();
    }
}

void XyceSimulationRunner::cleanup_temp_netlist() {
    // check if temp netlist path is set
    if (m_temp_netlist_path.empty())
        return;
    // attempt to remove the temporary netlist file
    std::error_code ec;
    std::filesystem::remove(m_temp_netlist_path, ec);
    if (ec) {
        // log information
        spdlog::warn("Failed to remove temp netlist: {}", m_temp_netlist_path.string());
    }
    // clear the path
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
    // write the text to the file
    file.write(text.data(), static_cast<std::streamsize>(text.size()));
    file.close();
    // return the path as std::filesystem::path
    return std::filesystem::path(temp_path.ToStdString());
}
