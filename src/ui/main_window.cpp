#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/arrstr.h>
#include <wx/artprov.h>
#include <wx/filedlg.h>
#include <wx/menu.h>
#include <wx/regex.h>
#include <wx/toolbar.h>
#endif

#include <spdlog/spdlog.h>

#include "../config/plugin_config.h"
#include "../file/xyce_raw_file.h"
#include "../netlist/netlist.h"
#include "charts_panel.h"
#include "events.h"
#include "icon_manager.h"
#include "main_window.h"
#include "plugin_config_dialog.h"
#include "simulation_parameters/simulation_parameters_dialog.h"
#include "xyce_simulation_runner.h"

enum
{
    wxID_SHOW_NETLIST = wxID_HIGHEST + 1,
    wxID_CONFIGURE_SIMULATION,
    wxID_RUN_SIMULATION,
    wxID_PLUGIN_CONFIGURATION
};

enum
{
    STYLE_SPICE_DEFAULT = 0,
    STYLE_SPICE_DIRECTIVE,
    STYLE_SPICE_COMMENT_LINE
};

const wxRegEx SPICE_COMMENTS_REGEX("^\\*.*$");
const wxRegEx SPICE_DIRECTIVE_REGEX("^(\\.\\b\\w+\\b).*$");

MainWindow::MainWindow(const wxString& title) :
    wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(800, 600)), m_simulation_config(SimulationConfig::from_xyce_directives({})), m_plugin_config(PluginConfig::load()) {
    // create menubar/toolbar/statusbar
    create_menubar();
    create_toolbar();
    create_statusbar();
    // create main vertical sizer
    m_main_sizer = new wxBoxSizer(wxVERTICAL);
    // create netlist editor (read-only if connected to KiCad)
    m_netlist_editor = new wxStyledTextCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
    m_netlist_editor->SetReadOnly(m_kicad_client != nullptr);
    m_main_sizer->Add(m_netlist_editor, 1, wxEXPAND | wxALL, 0);
    m_main_sizer->Hide(m_netlist_editor);
    // create charts panel
    m_charts_panel = new ChartsPanel(this);
    m_main_sizer->Add(m_charts_panel, 1, wxEXPAND | wxALL, 0);
    m_main_sizer->Hide(m_charts_panel);
    // set sizer
    SetSizer(m_main_sizer);
    // bind events
    Bind(wxEVT_SYS_COLOUR_CHANGED, &MainWindow::on_system_colour_changed, this);
    Bind(wxEVT_DISPLAY_CHANGED, &MainWindow::on_display_changed, this);
    Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent&) { Destroy(); });
    Bind(wxEVT_MENU, &MainWindow::on_exit, this, wxID_EXIT);
    Bind(wxEVT_MENU, &MainWindow::on_menu_file_open, this, wxID_OPEN);
    Bind(wxEVT_MENU, &MainWindow::on_menu_file_save, this, wxID_SAVE);
    // bind custom events
    Bind(wxEVT_NEW_WINDOW, &MainWindow::on_new_window, this);
    // netlist editor events
    Bind(wxEVT_STC_MODIFIED, &MainWindow::on_netlist_editor_modified, this, m_netlist_editor->GetId());
    Bind(wxEVT_STC_STYLENEEDED, &MainWindow::on_netlist_editor_style_needed, this, m_netlist_editor->GetId());
    // custom commands
    Bind(wxEVT_TOOL, &MainWindow::on_show_netlist, this, wxID_SHOW_NETLIST);
    Bind(wxEVT_TOOL, &MainWindow::on_configure_simulation, this, wxID_CONFIGURE_SIMULATION);
    Bind(wxEVT_TOOL, &MainWindow::on_plugin_configuration, this, wxID_PLUGIN_CONFIGURATION);
    Bind(wxEVT_TOOL, &MainWindow::on_run_simulation, this, wxID_RUN_SIMULATION);
    // simulation events
    Bind(wxEVT_SIMULATION_STDOUT, &MainWindow::on_simulation_stdout, this);
    Bind(wxEVT_SIMULATION_STDERR, &MainWindow::on_simulation_stderr, this);
    Bind(wxEVT_SIMULATION_FINISHED, &MainWindow::on_simulation_finished, this);
    // configure netlist editor
    configure_netlist_editor();
}

void MainWindow::on_system_colour_changed(wxSysColourChangedEvent& event) {
    // skip event
    event.Skip();
    // defer execution to the next iteration in the event loop, to let the system update the colours first
    CallAfter([this]() {
        // icon manager instance
        auto& icon_manager = IconManager::get();
        // appereance
        auto appearance = wxSystemSettings::GetAppearance();
        // dark mode
        const bool dark_mode = appearance.IsSystemDark();
        // log information
        spdlog::info("System colour changed, dark mode: {}", dark_mode);
        // TODO: update toolbar icons
        auto tool_bar = GetToolBar();
        tool_bar->SetToolNormalBitmap(wxID_OPEN, icon_manager.get_bitmap(dark_mode, "directory_open"));
        tool_bar->Realize();
    });
}

void MainWindow::on_display_changed(wxDisplayChangedEvent& event) {
    // notify charts panel that display has changed
    m_charts_panel->display_changed();
    // skip event
    event.Skip();
}

void MainWindow::on_exit(wxCommandEvent& event) {
    // close the window
    Close(true);
    // skip event
    event.Skip();
}

void MainWindow::on_new_window(wxCommandEvent& event) {
    // create main window instance
    const auto frame = new MainWindow("KiCad Xyce Plugin");
    // show main window
    frame->Show(true);
    // update file reference
    frame->update_xyce_raw_file(m_xyce_raw_file, true);
    // skip event
    event.Skip();
}

void MainWindow::create_menubar() {
    // create menubar
    const auto menu_bar = new wxMenuBar();

    // file menu
    const auto file_menu = new wxMenu();
    menu_bar->Append(file_menu, "&File");

    // section only available when not connected to KiCad
    if (!m_kicad_client) {
        // file / open
        file_menu->Append(wxID_OPEN, "&Open\tCtrl-O", "Open Xyce File");
        // file / save
        file_menu->Append(wxID_SAVE, "&Save\tCtrl-S", "Save Xyce Netlist File");
    }

    // tools menu
    const auto tools_menu = new wxMenu();
    menu_bar->Append(tools_menu, "&Tools");

    // tools / view simulation output
    tools_menu->Append(wxID_ANY, "&View Simulation Output\tCtrl-Alt-V", "View Simulation Output");

    // tools / configure simulation (matches toolbar action)
    tools_menu->Append(wxID_CONFIGURE_SIMULATION, "&Configure Simulation...\tCtrl-Shift-S", "Configure simulation parameters");

    // tools / configuration
    tools_menu->Append(wxID_PLUGIN_CONFIGURATION, "&Configuration\tCtrl-Alt-C", "Configure Xyce Plugin");

    // set menu bar for frame
    wxFrameBase::SetMenuBar(menu_bar);
}

void MainWindow::create_toolbar() {
    // icon manager instance
    auto& icon_manager = IconManager::get();
    // appereance
    auto appearance = wxSystemSettings::GetAppearance();
    // dark mode
    const bool dark_mode = appearance.IsDark();
    // create toolbar
    const auto tool_bar = wxFrame::CreateToolBar(wxTB_HORIZONTAL);
    // section only available when not connected to KiCad
    if (!m_kicad_client) {
        // open action
        tool_bar->AddTool(wxID_OPEN, "Open Xyce File", icon_manager.get_bitmap(dark_mode, "directory_open"));
        // save action(disable by default)
        m_save_netlist_action = tool_bar->AddTool(wxID_SAVE, "Save", icon_manager.get_bitmap(dark_mode, "save"));
        m_save_netlist_action->Enable(false);
        // separator
        tool_bar->AddSeparator();
    }
    // show netlist action (enabled if connected to KiCad)
    m_show_netlist_action = tool_bar->AddTool(wxID_SHOW_NETLIST, "Show Netlist", icon_manager.get_bitmap(dark_mode, "netlist"));
    m_show_netlist_action->Enable(m_kicad_client != nullptr);
    // simulation settings action (enabled if connected to KiCad)
    m_simulation_settings_action = tool_bar->AddTool(wxID_CONFIGURE_SIMULATION, "Configure simulation parameters", icon_manager.get_bitmap(dark_mode, "sim_command"));
    m_simulation_settings_action->Enable(m_kicad_client != nullptr);
    // simulation run action (enabled if connected to KiCad)
    m_simulation_run_action = tool_bar->AddTool(wxID_RUN_SIMULATION, "Run the simulation", icon_manager.get_bitmap(dark_mode, "sim_run"));
    m_simulation_run_action->Enable(m_kicad_client != nullptr);
    // separator
    tool_bar->AddSeparator();

    // configuration action
    tool_bar->AddTool(wxID_PLUGIN_CONFIGURATION, "Plugin configuration", icon_manager.get_bitmap(dark_mode, "preference"));
    // separator
    tool_bar->AddSeparator();

    // add tool button for exit
    tool_bar->AddTool(wxID_EXIT, "Exit", icon_manager.get_bitmap(dark_mode, "exit"));

    // realize toolbar
    tool_bar->Realize();
}

void MainWindow::create_statusbar() {
    // create statusbar
    wxFrame::CreateStatusBar(1);
    // set statusbar text
    wxFrame::SetStatusText("Welcome to KiCad Xyce Plugin");
}

void MainWindow::on_menu_file_open(wxCommandEvent& event) {
    // file types
    const wxString wildcards = "Netlist files (*.cir)|*.cir|Xyce output files (*.raw, *.fftX)|*.raw;*.fft?;*.fft??";
    // define open file dialog
    wxFileDialog dialog(this, "Select Xyce input/output file", wxEmptyString, wxEmptyString, wildcards, wxFD_OPEN);
    // center on screen
    dialog.Centre(wxCENTER_ON_SCREEN);
    // show and wait for ok
    if (dialog.ShowModal() == wxID_OK) {
        // selected file
        const auto filepath = dialog.GetPath();
        // analyze file extension
        const auto extension = filepath.AfterLast('.');
        // netlist file extension
        if (extension == "cir") {
            // load netlist file into editor
            if (update_xyce_netlist_file(filepath.ToStdString())) {
                // disable save action (enabled when editor is modified)
                m_save_netlist_action->Enable(false);
                // enable simulation actions (analyzing netlist)
                m_show_netlist_action->Enable(true);
                m_simulation_settings_action->Enable(true);
                m_simulation_run_action->Enable(true);
                // update title
                SetTitle(m_xyce_netlist_file.filename().string());
                // remove output file reference
                m_xyce_raw_file = std::nullopt;
                // hide/show panels
                m_main_sizer->Hide(m_charts_panel);
                m_main_sizer->Show(m_netlist_editor);
                m_main_sizer->Layout();
            }
            // skip event
            event.Skip();
            // exit
            return;
        }
        // raw file extension
        if (extension == "raw") {
            // parse file & update it in window field
            if (update_xyce_raw_file(xyce_raw_file_parser(filepath.ToStdString()), true)) {
                // disable simulation actions (analyzing existing output file)
                m_show_netlist_action->Enable(false);
                m_simulation_settings_action->Enable(false);
                m_simulation_run_action->Enable(false);
                // update title
                SetTitle(m_xyce_raw_file.value()->title());
                // remove existing netlist file
                m_xyce_netlist_file = std::filesystem::path();
                m_netlist_editor->ClearAll();
                // hide/show panels
                m_main_sizer->Show(m_charts_panel);
                m_main_sizer->Hide(m_netlist_editor);
                m_main_sizer->Layout();
                // disable the save netlist action (enabled when editor is modified)
                CallAfter([this]() { update_netlist_editor_dirty_flag(false); });
            }
            // skip event
            event.Skip();
            // exit
            return;
        }
    }
    // skip event
    event.Skip();
}

void MainWindow::on_menu_file_save(wxCommandEvent& event) {
    // open stream in write mode
    std::ofstream file(m_xyce_netlist_file, std::ios::out | std::ios::trunc);
    if (file.is_open()) {
        // write content into file
        file << m_netlist_editor->GetText().ToStdString();
        // close stream
        file.close();
        // reset dirty flag
        update_netlist_editor_dirty_flag(false);
    }
    // skip event
    event.Skip();
}

void MainWindow::on_show_netlist(wxCommandEvent& event) {
    // hide/show panels
    m_main_sizer->Hide(m_charts_panel);
    m_main_sizer->Show(m_netlist_editor);
    m_main_sizer->Layout();
    // skip event
    event.Skip();
}

void MainWindow::on_configure_simulation(wxCommandEvent& event) {
    // parse netlist editor content for initial config
    if (m_netlist_editor && !m_netlist_editor->GetText().IsEmpty()) {
        // netlist text
        const auto netlist_text = m_netlist_editor->GetText().ToStdString();
        // parse netlist and extract topology
        const auto [sanitized_netlist, topology] = parse_netlist(netlist_text);
        // store topology for later use
        m_topology = std::move(topology);
        // store sanitized netlist for later serialization
        m_sanitized_netlist = std::move(sanitized_netlist);
        // build simulation config from parsed directives
        m_simulation_config = SimulationConfig::from_xyce_directives(m_topology.m_directives);
    }
    // create dialog with current config
    SimulationParametersDialog dialog(this, m_simulation_config);
    // show modal
    if (dialog.ShowModal() == wxID_OK) {
        // updated simulation config from dialog
        m_simulation_config = dialog.get_config();
        // build directives from simulation config with topology expansion
        const auto directives = m_simulation_config.to_xyce_directives(&m_topology);
        // merge directives into sanitized netlist before .END
        const auto final_netlist = build_final_netlist(m_sanitized_netlist, directives, m_topology.m_passthrough_directives);
        // update stored netlist with merged content
        m_sanitized_netlist = final_netlist;
        // update editor with final netlist
        m_netlist_editor->SetText(m_sanitized_netlist);
    }
    // skip event
    event.Skip();
}

void MainWindow::on_plugin_configuration(wxCommandEvent& event) {
    // create config dialog
    PluginConfigDialog dialog(this, m_plugin_config);
    // show modal
    if (dialog.ShowModal() == wxID_OK) {
        // log information
        spdlog::info("Plugin configuration updated: Xyce path = {}", m_plugin_config.xyce_executable_path());
        // update plugin configuration from dialog
        m_plugin_config = dialog.get_config();
    }
    // skip event
    event.Skip();
}

void MainWindow::on_run_simulation(wxCommandEvent& event) {
    // guard against empty editor content
    if (!m_netlist_editor || m_netlist_editor->GetText().IsEmpty()) {
        // update statusbar
        SetStatusText("No netlist content to simulate");
        // skip event
        event.Skip();
        // exit
        return;
    }
    // validate plugin configuration before launching
    if (!m_plugin_config.is_xyce_executable_valid()) {
        // update statusbar with error
        SetStatusText("Configured Xyce executable path is invalid");
        // skip event
        event.Skip();
        // exit
        return;
    }
    // re-parse editor content for fresh netlist + topology
    const auto netlist_text = m_netlist_editor->GetText().ToStdString();
    // parse netlist and extract topology
    auto [sanitized_netlist, topology] = parse_netlist(netlist_text);
    // store topology for later use
    m_topology = std::move(topology);
    // store sanitized netlist for later serialization
    m_sanitized_netlist = std::move(sanitized_netlist);
    // initialize simulation config from parsed directives
    m_simulation_config = SimulationConfig::from_xyce_directives(m_topology.m_directives);
    // guard against empty netlist after dialog interaction
    if (m_sanitized_netlist.empty()) {
        // skip when no netlist has been parsed
        event.Skip();
        // exit
        return;
    }
    // check if analysis is configured; if not, prompt the user
    if (std::holds_alternative<std::monostate>(m_simulation_config.analysis)) {
        // open configure dialog to set up analysis
        SimulationParametersDialog dialog(this, m_simulation_config);
        // show modal
        if (dialog.ShowModal() != wxID_OK) {
            // user cancelled, skip
            event.Skip();
            // exit
            return;
        }
        // store updated config
        m_simulation_config = dialog.get_config();
    }
    // build directives from simulation config with topology expansion
    const auto directives = m_simulation_config.to_xyce_directives(&m_topology);
    // merge directives into sanitized netlist before .END
    const auto final_netlist = build_final_netlist(m_sanitized_netlist, directives, m_topology.m_passthrough_directives);
    // update stored netlist with merged content
    m_sanitized_netlist = final_netlist;
    // update editor with final netlist
    m_netlist_editor->SetText(m_sanitized_netlist);
    // determine working directory (netlist parent, or current directory)
    std::filesystem::path working_dir;
    if (!m_xyce_netlist_file.empty())
        working_dir = m_xyce_netlist_file.parent_path();
    // create temporary netlist file for the runner
    auto temp_path = XyceSimulationRunner::create_temp_netlist(m_sanitized_netlist);
    if (temp_path.empty()) {
        // update statusbar with error
        SetStatusText("Failed to create temporary netlist file");
        // skip event
        event.Skip();
        // exit
        return;
    }
    // clear any previous runner
    m_simulation_runner.reset();
    // create new simulation runner
    auto runner = std::make_shared<XyceSimulationRunner>();
    // bind simulation events from the runner to this window
    runner->Bind(wxEVT_SIMULATION_FINISHED, &MainWindow::on_simulation_finished, this);
    runner->Bind(wxEVT_SIMULATION_STDOUT, &MainWindow::on_simulation_stdout, this);
    runner->Bind(wxEVT_SIMULATION_STDERR, &MainWindow::on_simulation_stderr, this);
    // store the runner
    m_simulation_runner = std::move(runner);
    // launch the simulation
    m_simulation_runner->start(m_plugin_config.xyce_executable_path(), temp_path, working_dir);
    // update statusbar
    SetStatusText("Simulation started...");
    // skip event
    event.Skip();
}

void MainWindow::on_simulation_finished(wxThreadEvent& event) {
    // extract exit code and canceled flag from the event
    int exit_code = event.GetInt();
    bool was_canceled = event.GetPayload<bool>();
    // handle canceled simulations
    if (was_canceled) {
        // update statusbar
        SetStatusText("Simulation canceled");
        // clear the runner
        m_simulation_runner.reset();
        // skip event
        event.Skip();
        // exit
        return;
    }
    // check for success
    if (exit_code == 0) {
        // ensure we have a runner reference for accessing paths
        if (!m_simulation_runner) {
            // update statusbar
            SetStatusText("Simulation finished but runner reference is missing");
            // skip event
            event.Skip();
            // exit
            return;
        }
        // compute expected raw output file path
        auto raw_path = m_simulation_config.raw_output_file_path(m_simulation_runner->working_directory(), m_simulation_runner->netlist_file_path());
        // try to load the raw file when a path was computed
        if (raw_path.has_value() && std::filesystem::exists(*raw_path)) {
            // parse the raw file
            auto raw_file = xyce_raw_file_parser(raw_path->string());
            if (raw_file.has_value()) {
                // update charts panel with the parsed data
                update_xyce_raw_file(std::move(raw_file), true);
                // switch to charts view
                m_main_sizer->Show(m_charts_panel);
                m_main_sizer->Hide(m_netlist_editor);
                m_main_sizer->Layout();
                // update title
                SetTitle(m_xyce_raw_file.value()->title());
                // update statusbar
                SetStatusText("Simulation finished successfully");
                // clear the runner
                m_simulation_runner.reset();
                // skip event
                event.Skip();
                // exit
                return;
            }
        }
        // raw file not found or failed to parse
        SetStatusText("Simulation finished but output raw file could not be found");
    }
    else {
        // simulation failed with a non-zero exit code
        SetStatusText(wxString::Format("Simulation failed (exit code %d)", exit_code));
    }
    // clear the runner
    m_simulation_runner.reset();
    // skip event
    event.Skip();
}

void MainWindow::on_simulation_stdout(wxThreadEvent& event) {
    // log stdout lines from the simulation process
    spdlog::info("{}", event.GetPayload<std::string>());
    // skip event
    event.Skip();
}

void MainWindow::on_simulation_stderr(wxThreadEvent& event) {
    // capture the error line
    std::string error_line = event.GetPayload<std::string>();
    // log information
    spdlog::warn("{}", error_line);
    // update statusbar with the latest error line
    SetStatusText(wxString::Format("Simulation error: %s", error_line));
    // skip event
    event.Skip();
}

void MainWindow::on_netlist_editor_modified(wxStyledTextEvent& event) {
    // set editor as dirty
    update_netlist_editor_dirty_flag(true);
    // skip event
    event.Skip();
}

void MainWindow::on_netlist_editor_style_needed(wxStyledTextEvent& event) {
    // determine the position style is required
    int start_pos = m_netlist_editor->GetEndStyled();
    int end_pos = event.GetPosition();
    // start and end line numbers for the range that needs styling
    int start_line = m_netlist_editor->LineFromPosition(start_pos);
    int end_line = m_netlist_editor->LineFromPosition(end_pos);
    // start at first character in start line and end at last character in end line
    start_pos = m_netlist_editor->PositionFromLine(start_line);
    end_pos = m_netlist_editor->GetLineEndPosition(end_line);
    // text that needs styling
    wxString text = m_netlist_editor->GetTextRange(start_pos, end_pos);
    // split text into lines
    auto lines = wxSplit(text, '\n');
    // loop lines and apply styles
    for (auto line : lines) {
        // check if line is a comment
        if (SPICE_COMMENTS_REGEX.Matches(line)) {
            // apply comment style
            m_netlist_editor->StartStyling(start_pos);
            m_netlist_editor->SetStyling(line.Length(), STYLE_SPICE_COMMENT_LINE);
            // increment start position for next line
            start_pos += line.Length() + 1;
            // next
            continue;
        }
        // directive
        if (SPICE_DIRECTIVE_REGEX.Matches(line)) {
            // actual directive is in capture group 1
            wxString directive = SPICE_DIRECTIVE_REGEX.GetMatch(line, 1);
            // apply directive style
            m_netlist_editor->StartStyling(start_pos);
            m_netlist_editor->SetStyling(directive.Length(), STYLE_SPICE_DIRECTIVE);
            // default style for the rest of the line
            m_netlist_editor->SetStyling(line.Length() - directive.Length(), STYLE_SPICE_DEFAULT);
            // increment start position for next line
            start_pos += line.Length() + 1;
            // next
            continue;
        }
        // default style
        m_netlist_editor->StartStyling(start_pos);
        m_netlist_editor->SetStyling(line.Length(), STYLE_SPICE_DEFAULT);
        // increment start position for next line
        start_pos += line.Length() + 1;
    }
    // skip event
    event.Skip();
}

bool MainWindow::update_xyce_netlist_file(const std::filesystem::path& filename) {
    // check if file exists
    if (!std::filesystem::exists(filename))
        return false;
    // open stream in read mode
    std::ifstream file(filename, std::ios::in);
    if (!file.is_open())
        return false;
    // pre-allocate buffer for file content
    std::string content;
    content.reserve(std::filesystem::file_size(filename));
    // read file content into buffer
    content.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    // set content into editor & set file
    m_netlist_editor->SetText(content);
    m_xyce_netlist_file = filename;
    // reset dirty flag, wait for next iteration of event loop to allow the text editor event to be processed first
    CallAfter([this]() { update_netlist_editor_dirty_flag(false); });
    // indicate success
    return true;
}

bool MainWindow::update_xyce_raw_file(std::optional<std::shared_ptr<XyceOutputFile>> raw_file, bool delete_charts) {
    // store reference
    m_xyce_raw_file = std::move(raw_file);
    // check file is present
    if (m_xyce_raw_file.has_value()) {
        // file instance
        auto& file = m_xyce_raw_file.value();
        // cleanup charts if new file might be different with old one
        if (delete_charts) {
            // delete all charts when file changes
            m_charts_panel->delete_all_charts();
        }
        // update references
        m_charts_panel->update(file->expression_manager(), file->step_information(), "", file->abscissa_scale());
        // indicate success
        return true;
    }
    // indicate failure
    return false;
}

void MainWindow::update_netlist_editor_dirty_flag(bool flag) {
    // current title
    const auto current_title = GetTitle();
    // check flag
    if (flag) {
        // update title with asterisk to indicate unsaved changes
        if (!current_title.StartsWith("* "))
            SetTitle("* " + current_title);
    }
    else if (current_title.StartsWith("* "))
        SetTitle(current_title.substr(2));
    // enable/disable save action
    m_save_netlist_action->Enable(flag);
}

void MainWindow::configure_netlist_editor() {
    // reset existing styles
    m_netlist_editor->StyleClearAll();
    // set wrap mode
    m_netlist_editor->SetWrapMode(wxSTC_WRAP_WORD);
    // set lexer to custom
    m_netlist_editor->SetLexer(wxSTC_LEX_CONTAINER);
    // margin for line numbers
    m_netlist_editor->SetMarginType(0, wxSTC_MARGIN_NUMBER);
    m_netlist_editor->SetMarginWidth(0, 35);
    // default style
    m_netlist_editor->StyleSetForeground(STYLE_SPICE_DEFAULT, m_netlist_editor->GetForegroundColour());
    m_netlist_editor->StyleSetBackground(STYLE_SPICE_DEFAULT, m_netlist_editor->GetBackgroundColour());
    // comment line style
    m_netlist_editor->StyleSetForeground(STYLE_SPICE_COMMENT_LINE, wxColour(0, 128, 0));
    m_netlist_editor->StyleSetItalic(STYLE_SPICE_COMMENT_LINE, true);
    // directive style
    m_netlist_editor->StyleSetForeground(STYLE_SPICE_DIRECTIVE, wxColour(0, 0, 255));
    m_netlist_editor->StyleSetBold(STYLE_SPICE_DIRECTIVE, true);
}
