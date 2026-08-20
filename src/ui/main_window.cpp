#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <utility>

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

// #include "../app.h"
#include "../config/plugin_config.h"
#include "../netlist/netlist_source.h"
#include "charts_panel.h"
#include "events.h"
#include "icon_manager.h"
#include "main_window.h"
#include "main_window_presenter.h"
#include "plugin_config_dialog.h"
#include "simulation_parameters/simulation_parameters_dialog.h"
#include "xyce_simulation_runner.h"

enum
{
    wxID_SHOW_NETLIST = wxID_HIGHEST + 1,
    wxID_CONFIGURE_SIMULATION,
    wxID_RUN_SIMULATION,
    wxID_PLUGIN_CONFIGURATION,
    wxID_CLOSE_SIMULATION_OUTPUT,
    wxID_SHOW_SIMULATION_OUTPUT,
    wxID_SHOW_CHARTS
};

enum
{
    STYLE_SPICE_DEFAULT = 0,
    STYLE_SPICE_DIRECTIVE,
    STYLE_SPICE_COMMENT_LINE
};

const wxRegEx SPICE_COMMENTS_REGEX("^\\*.*$");
const wxRegEx SPICE_DIRECTIVE_REGEX("^(\\.\\b\\w+\\b).*$");

MainWindow::MainWindow(const wxString& title, std::shared_ptr<KiCadSession> session) :
    wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(800, 600)), m_kicad_session(std::move(session)) {
    // register this application frame before it can receive close events
    // if (auto* app = dynamic_cast<App*>(wxTheApp)) {
    //     app->register_frame();
    //     m_registered = true;
    // }
    // take ownership of the netlist source in KiCad plugin mode
    std::unique_ptr<NetlistSource> netlist_source;
    if (m_kicad_session != nullptr)
        netlist_source = m_kicad_session->take_netlist_source();
    // create the presenter, which owns the business logic
    m_presenter = std::make_unique<MainWindowPresenter>(*this, std::move(netlist_source), PluginConfig::load());
    // create menubar/toolbar/statusbar
    create_menubar();
    create_toolbar();
    create_statusbar();
    // create main vertical sizer
    m_main_sizer = new wxBoxSizer(wxVERTICAL);
    // create body splitter, top pane holds the content views, bottom pane the simulation output
    m_body_splitter = new wxSplitterWindow(this, wxID_ANY);
    // content panel hosts the exclusive netlist/charts views
    m_content_panel = new wxPanel(m_body_splitter, wxID_ANY);
    // create main content sizer
    m_content_sizer = new wxBoxSizer(wxVERTICAL);
    // create netlist editor
    m_netlist_editor = new wxStyledTextCtrl(m_content_panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
    m_netlist_editor->SetReadOnly(true);
    m_content_sizer->Add(m_netlist_editor, 1, wxEXPAND | wxALL, 0);
    m_content_sizer->Hide(m_netlist_editor);
    // create charts panel
    m_charts_panel = new ChartsPanel(m_content_panel);
    m_content_sizer->Add(m_charts_panel, 1, wxEXPAND | wxALL, 0);
    m_content_sizer->Hide(m_charts_panel);
    // set FFT result callback to spawn new window
    m_charts_panel->set_fft_result_callback([this](std::shared_ptr<XyceOutputFile> raw_file) { spawn_raw_file_window(std::move(raw_file)); });
    // set sizer for content panel
    m_content_panel->SetSizer(m_content_sizer);
    // create simulation output container, holds a header bar and the log area
    m_simulation_output_container = new wxPanel(m_body_splitter, wxID_ANY);
    // create vertical sizer for the simulation output container
    auto* output_container_sizer = new wxBoxSizer(wxVERTICAL);
    // create header bar with title and close button
    auto* output_header_sizer = new wxBoxSizer(wxHORIZONTAL);
    // title of the simulation output panel
    auto* output_title = new wxStaticText(m_simulation_output_container, wxID_ANY, "Simulation Output");
    output_header_sizer->Add(output_title, 0, wxALIGN_CENTER_VERTICAL | wxALL, FromDIP(6));
    // push the close button to the right edge of the header
    output_header_sizer->AddStretchSpacer();
    // close button to dismiss the simulation output panel
    auto* output_close_button = new wxButton(m_simulation_output_container, wxID_CLOSE_SIMULATION_OUTPUT, "\u2715", wxDefaultPosition, wxSize(FromDIP(24), FromDIP(24)));
    output_close_button->SetToolTip("Close the simulation output panel");
    output_header_sizer->Add(output_close_button, 0, wxALIGN_CENTER_VERTICAL | wxALL, FromDIP(2));
    output_container_sizer->Add(output_header_sizer, 0, wxEXPAND);
    // create simulation output panel (blank, read-only log area)
    m_simulation_output_panel = new wxStyledTextCtrl(m_simulation_output_container, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
    // use a monospaced font for readable log output
    m_simulation_output_panel->StyleSetFont(wxSTC_STYLE_DEFAULT, wxFont(wxFontInfo(wxNORMAL_FONT->GetPointSize()).Family(wxFONTFAMILY_TELETYPE)));
    // use explicit system colours so log text is always readable
    m_simulation_output_panel->StyleSetForeground(wxSTC_STYLE_DEFAULT, wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
    m_simulation_output_panel->StyleSetBackground(wxSTC_STYLE_DEFAULT, wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
    // make the simulation output panel read-only
    m_simulation_output_panel->SetReadOnly(true);
    // keep each output line intact on a single visual line
    m_simulation_output_panel->SetWrapMode(wxSTC_WRAP_NONE);
    // propagate default style attributes to all other styles
    m_simulation_output_panel->StyleClearAll();
    output_container_sizer->Add(m_simulation_output_panel, 1, wxEXPAND | wxALL, 0);
    // set sizer for the simulation output container
    m_simulation_output_container->SetSizer(output_container_sizer);
    // hide the simulation output container until a simulation is started
    m_simulation_output_container->Hide();
    // initialize splitter with the content panel only, simulation output panel stays hidden
    m_body_splitter->Initialize(m_content_panel);
    // keep the bottom pane fixed size when the window is resized
    m_body_splitter->SetSashGravity(1.0);
    // prevent the panes from collapsing entirely
    m_body_splitter->SetMinimumPaneSize(FromDIP(80));
    // add splitter to main sizer
    m_main_sizer->Add(m_body_splitter, 1, wxEXPAND | wxALL, 0);
    // set sizer
    SetSizer(m_main_sizer);
    // bind events
    Bind(wxEVT_SYS_COLOUR_CHANGED, &MainWindow::on_system_colour_changed, this);
    Bind(wxEVT_DISPLAY_CHANGED, &MainWindow::on_display_changed, this);
    // Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent&) {
    //     // remove this frame from the application lifetime once
    //     if (m_registered) {
    //         auto* app = dynamic_cast<App*>(wxTheApp);
    //         if (app)
    //             app->unregister_frame();
    //         m_registered = false;
    //     }
    //     // allow wxWidgets to destroy the frame after the close event completes
    //     Destroy();
    // });
    Bind(wxEVT_MENU, &MainWindow::on_exit, this, wxID_EXIT);
    Bind(wxEVT_MENU, &MainWindow::on_menu_file_open, this, wxID_OPEN);
    Bind(wxEVT_MENU, &MainWindow::on_menu_file_save, this, wxID_SAVE);
    // bind custom events
    Bind(wxEVT_NEW_WINDOW, &MainWindow::on_new_window, this);
    Bind(wxEVT_OPEN_XYCE_FFT_CALCULATION, &MainWindow::on_open_xyce_fft_calculation, this);
    Bind(wxEVT_CHART_HOVER, &MainWindow::on_chart_hover, this);
    // netlist editor events
    Bind(wxEVT_STC_MODIFIED, &MainWindow::on_netlist_editor_modified, this, m_netlist_editor->GetId());
    Bind(wxEVT_STC_STYLENEEDED, &MainWindow::on_netlist_editor_style_needed, this, m_netlist_editor->GetId());
    // custom commands
    Bind(wxEVT_TOOL, &MainWindow::on_show_netlist, this, wxID_SHOW_NETLIST);
    Bind(wxEVT_TOOL, &MainWindow::on_show_charts, this, wxID_SHOW_CHARTS);
    Bind(wxEVT_TOOL, &MainWindow::on_show_simulation_output, this, wxID_SHOW_SIMULATION_OUTPUT);
    Bind(wxEVT_TOOL, &MainWindow::on_configure_simulation, this, wxID_CONFIGURE_SIMULATION);
    Bind(wxEVT_TOOL, &MainWindow::on_plugin_configuration, this, wxID_PLUGIN_CONFIGURATION);
    Bind(wxEVT_TOOL, &MainWindow::on_run_simulation, this, wxID_RUN_SIMULATION);
    // simulation events
    Bind(wxEVT_SIMULATION_STDOUT, &MainWindow::on_simulation_stdout, this);
    Bind(wxEVT_SIMULATION_STDERR, &MainWindow::on_simulation_stderr, this);
    Bind(wxEVT_SIMULATION_FINISHED, &MainWindow::on_simulation_finished, this);
    // simulation output panel events
    Bind(wxEVT_BUTTON, &MainWindow::on_close_simulation_output, this, wxID_CLOSE_SIMULATION_OUTPUT);
    // configure netlist editor
    configure_netlist_editor();
    // set initial action states
    m_presenter->refresh_action_states();
    // load the schematic netlist in KiCad plugin mode
    if (m_kicad_session != nullptr)
        m_presenter->extract_schematic_netlist();
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

void MainWindow::on_exit(wxCommandEvent&) {
    // close the window
    Close(true);
}

void MainWindow::on_new_window(wxCommandEvent&) {
    // check a raw file is available to seed the new window
    const auto& raw = m_presenter->raw_file();
    // spawn a new window when a raw file is present
    if (raw.has_value())
        spawn_raw_file_window(raw.value());
}

void MainWindow::on_open_xyce_fft_calculation(wxCommandEvent&) {
    // open a new window for each parsed FFT calculation output file
    for (const auto& fft_file : m_presenter->fft_files())
        spawn_raw_file_window(fft_file);
}

void MainWindow::on_chart_hover(wxCommandEvent& event) {
    // hover text from the charts panel
    const std::string text = event.GetString().ToStdString();
    // empty text restores the permanent status message
    if (text.empty()) {
        // preserve the last presenter-set text
        wxFrame::SetStatusText(m_last_status_text);
    }
    else {
        wxFrame::SetStatusText(text);
    }
}

void MainWindow::create_menubar() {
    // create menubar
    const auto menu_bar = new wxMenuBar();

    // file menu
    const auto file_menu = new wxMenu();
    menu_bar->Append(file_menu, "&File");

    // section only available when not connected to KiCad
    if (m_kicad_session == nullptr) {
        // file / open
        file_menu->Append(wxID_OPEN, "&Open\tCtrl-O", "Open Xyce File");
        // file / save
        file_menu->Append(wxID_SAVE, "&Save\tCtrl-S", "Save Xyce Netlist File");
    }

    // tools menu
    const auto tools_menu = new wxMenu();
    menu_bar->Append(tools_menu, "&Tools");

    // tools / show charts (matches toolbar action)
    tools_menu->Append(wxID_SHOW_CHARTS, "&Show Charts", "Show Charts");

    // tools / view simulation output (matches toolbar action)
    tools_menu->Append(wxID_SHOW_SIMULATION_OUTPUT, "&View Simulation Output\tCtrl-Alt-V", "View Simulation Output");

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
    if (m_kicad_session == nullptr) {
        // open action
        m_open_netlist_action = tool_bar->AddTool(wxID_OPEN, "Open Xyce File", icon_manager.get_bitmap(dark_mode, "directory_open"));
        // save action(disable by default)
        m_save_netlist_action = tool_bar->AddTool(wxID_SAVE, "Save", icon_manager.get_bitmap(dark_mode, "save"));
        // separator
        tool_bar->AddSeparator();
    }
    // show netlist action
    m_show_netlist_action = tool_bar->AddTool(wxID_SHOW_NETLIST, "Show Netlist", icon_manager.get_bitmap(dark_mode, "netlist"));
    // show charts action
    m_show_charts_action = tool_bar->AddTool(wxID_SHOW_CHARTS, "Show Charts", icon_manager.get_bitmap(dark_mode, "simulator"));
    // show simulation output action
    m_show_simulation_output_action = tool_bar->AddTool(wxID_SHOW_SIMULATION_OUTPUT, "Show Simulation Output", icon_manager.get_bitmap(dark_mode, "sim_command"));
    // separator
    tool_bar->AddSeparator();
    // simulation run action
    m_simulation_run_action = tool_bar->AddTool(wxID_RUN_SIMULATION, "Run the simulation", icon_manager.get_bitmap(dark_mode, "sim_run"));
    // simulation settings action
    m_simulation_settings_action = tool_bar->AddTool(wxID_CONFIGURE_SIMULATION, "Configure simulation parameters", icon_manager.get_bitmap(dark_mode, "sim_tune"));
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

void MainWindow::on_menu_file_open(wxCommandEvent&) {
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
            // forward the netlist file to the presenter
            m_presenter->open_netlist_file(filepath.ToStdString());
            // exit
            return;
        }
        // raw file extension
        if (extension == "raw") {
            // forward the raw file to the presenter
            m_presenter->open_raw_file(filepath.ToStdString());
        }
    }
}

void MainWindow::on_menu_file_save(wxCommandEvent&) {
    // forward the save to the presenter
    m_presenter->save_netlist();
}

void MainWindow::on_show_netlist(wxCommandEvent&) {
    // switch to the netlist view
    show_netlist_view();
    // refresh toolbar/menu states
    m_presenter->refresh_action_states();
}

void MainWindow::on_show_charts(wxCommandEvent&) {
    // switch to the charts view
    show_charts_view();
    // refresh toolbar/menu states
    m_presenter->refresh_action_states();
}

void MainWindow::on_show_simulation_output(wxCommandEvent&) {
    // show the simulation output panel
    show_simulation_output_panel();
    // refresh toolbar/menu states
    m_presenter->refresh_action_states();
}

void MainWindow::on_configure_simulation(wxCommandEvent&) {
    // forward to the presenter
    m_presenter->configure_simulation();
}

void MainWindow::on_run_simulation(wxCommandEvent&) {
    // forward to the presenter
    m_presenter->run_simulation();
}

void MainWindow::on_plugin_configuration(wxCommandEvent&) {
    // forward to the presenter
    m_presenter->configure_plugin();
}

void MainWindow::on_simulation_finished(wxThreadEvent& event) {
    // translate the wx event into a presenter call
    m_presenter->handle_simulation_finished(event.GetInt(), event.GetPayload<bool>());
}

void MainWindow::on_simulation_stdout(wxThreadEvent& event) {
    // translate the wx event into a presenter call
    m_presenter->handle_simulation_stdout(event.GetPayload<std::string>());
}

void MainWindow::on_simulation_stderr(wxThreadEvent& event) {
    // translate the wx event into a presenter call
    m_presenter->handle_simulation_stderr(event.GetPayload<std::string>());
}

void MainWindow::on_close_simulation_output(wxCommandEvent&) {
    // hide the simulation output panel
    hide_simulation_output_panel();
    // refresh toolbar/menu states
    m_presenter->refresh_action_states();
}

void MainWindow::on_netlist_editor_modified(wxStyledTextEvent&) {
    // skip programmatic updates
    if (!m_netlist_editor_updating)
        m_presenter->handle_netlist_editor_modified();
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

// ========================================================================================
// main window view interface
// ========================================================================================

void MainWindow::set_title(const std::string& title) {
    // set the frame title
    SetTitle(title);
}

void MainWindow::set_status_text(const std::string& text) {
    // store the permanent presenter message
    m_last_status_text = text;
    // set the statusbar text
    SetStatusText(text);
}

void MainWindow::apply_action_enablement(const ActionStateEnablement& enablement) {
    // open/save actions are only available outside KiCad plugin mode
    if (m_kicad_session == nullptr) {
        // enable the file open action
        m_open_netlist_action->Enable(enablement.open);
        // enable the file save action
        m_save_netlist_action->Enable(enablement.save);
    }
    // enable the content-view actions
    m_show_netlist_action->Enable(enablement.show_netlist);
    m_show_charts_action->Enable(enablement.show_charts);
    m_show_simulation_output_action->Enable(enablement.show_simulation_output);
    m_simulation_settings_action->Enable(enablement.configure_simulation);
    m_simulation_run_action->Enable(enablement.run_simulation);
    // mirror enablement to menu items sharing the same ids
    if (auto* menu_bar = GetMenuBar()) {
        // open/save actions are only available outside KiCad plugin mode
        if (m_kicad_session == nullptr) {
            // enable the file open action
            menu_bar->Enable(wxID_OPEN, enablement.open);
            // enable the file save action
            menu_bar->Enable(wxID_SAVE, enablement.save);
        }
        // enable the show charts action
        menu_bar->Enable(wxID_SHOW_CHARTS, enablement.show_charts);
        // enable the view simulation output action
        menu_bar->Enable(wxID_SHOW_SIMULATION_OUTPUT, enablement.show_simulation_output);
        // enable the configure simulation action
        menu_bar->Enable(wxID_CONFIGURE_SIMULATION, enablement.configure_simulation);
    }
}

void MainWindow::show_netlist_view() {
    // hide the charts panel and show the netlist editor
    m_content_sizer->Hide(m_charts_panel);
    m_content_sizer->Show(m_netlist_editor);
    m_content_sizer->Layout();
}

void MainWindow::show_charts_view() {
    // hide the netlist editor and show the charts panel
    m_content_sizer->Hide(m_netlist_editor);
    m_content_sizer->Show(m_charts_panel);
    m_content_sizer->Layout();
}

void MainWindow::set_netlist_editor_content(const std::string& content) {
    // prevent dirty analysis during programmatic updates
    m_netlist_editor_updating = true;
    // current read-only state of the editor
    const bool read_only = m_netlist_editor->GetReadOnly();
    // update editor content, temporarily allowing edits if the editor is read-only
    m_netlist_editor->SetReadOnly(false);
    m_netlist_editor->SetText(content);
    m_netlist_editor->SetReadOnly(read_only);
    // reset flag (on next event loop iteration)
    CallAfter([this]() { m_netlist_editor_updating = false; });
}

std::string MainWindow::netlist_editor_content() const {
    // return the editor text as a plain string
    return m_netlist_editor != nullptr ? m_netlist_editor->GetText().ToStdString() : std::string();
}

void MainWindow::set_netlist_editor_read_only(bool read_only) {
    // set the editor read-only state
    m_netlist_editor->SetReadOnly(read_only);
}

bool MainWindow::charts_shown() const {
    // the charts panel is the active content view
    return m_charts_panel != nullptr && m_charts_panel->IsShown();
}

void MainWindow::show_simulation_output_panel() {
    // split the body when the simulation output panel is still hidden
    if (!m_body_splitter->IsSplit()) {
        // split the splitter horizontally, top pane holds content views, bottom pane the simulation output
        m_body_splitter->SplitHorizontally(m_content_panel, m_simulation_output_container);
        // keep the log pane at a fixed portion of the body height
        m_body_splitter->SetSashPosition(m_body_splitter->GetClientSize().GetHeight() * 3 / 4);
    }
}

void MainWindow::hide_simulation_output_panel() {
    // unsplit the bottom pane when it is present
    if (m_body_splitter->IsSplit())
        m_body_splitter->Unsplit(m_simulation_output_container);
}

void MainWindow::clear_simulation_output() {
    // clear all log content
    m_simulation_output_panel->ClearAll();
}

void MainWindow::append_simulation_output_line(const std::string& line) {
    // convert the raw output line to a wxString
    wxString output_text;
    if (!line.empty()) {
        // try strict UTF-8 conversion first
        output_text = wxString::FromUTF8(line.data(), line.size());
        // fall back to a byte-preserving conversion when the line is not valid UTF-8
        if (output_text.empty())
            output_text = wxString::From8BitData(line.data(), line.size());
    }
    // allow programmatic inserts, Scintilla rejects them while read-only
    m_simulation_output_panel->SetReadOnly(false);
    // terminate the line, the runner strips the trailing newline
    output_text.Append('\n');
    // append the line to the end of the simulation output panel
    m_simulation_output_panel->InsertText(m_simulation_output_panel->GetLength(), output_text);
    // restore read-only state for user interaction
    m_simulation_output_panel->SetReadOnly(true);
    // scroll to the end so the latest output is visible
    m_simulation_output_panel->GotoPos(m_simulation_output_panel->GetLength());
}

bool MainWindow::simulation_output_panel_hidden() const {
    // the output panel is hidden when the splitter holds only the content panel
    return m_body_splitter != nullptr && !m_body_splitter->IsSplit();
}

bool MainWindow::simulation_output_has_content() const {
    // the log holds content when the output panel text is not empty
    return m_simulation_output_panel != nullptr && m_simulation_output_panel->GetLength() > 0;
}

void MainWindow::update_charts(ExpressionManager& expression_manager, const StepInformation& step_information, AbscissaScale abscissa_scale, const std::vector<std::vector<std::string>>& suggested_plots) {
    // refresh the charts panel with the given data
    m_charts_panel->update(expression_manager, step_information, abscissa_scale, suggested_plots);
}

void MainWindow::delete_all_charts() {
    // clear all charts from the panel
    m_charts_panel->delete_all_charts();
}

void MainWindow::set_open_fft_calculation_files(const std::vector<std::shared_ptr<XyceOutputFile>>& files) {
    // forward the parsed FFT calculation files to the charts panel for the context menu
    m_charts_panel->set_open_fft_calculation_files(files);
}

std::optional<SimulationConfig> MainWindow::show_simulation_parameters_dialog(const SimulationConfig& current) {
    // create the dialog with the current config
    SimulationParametersDialog dialog(this, current);
    // evaluate the dialog result
    if (dialog.ShowModal() == wxID_OK)
        return dialog.get_config();
    // dialog canceled
    return std::nullopt;
}

std::optional<PluginConfig> MainWindow::show_plugin_config_dialog(const PluginConfig& current) {
    // create the config dialog
    PluginConfigDialog dialog(this, current);
    // evaluate the dialog result
    if (dialog.ShowModal() == wxID_OK)
        return dialog.get_config();
    // dialog canceled
    return std::nullopt;
}

void MainWindow::start_simulation_process(const std::string& program, const std::filesystem::path& netlist_path, const std::filesystem::path& working_directory) {
    // get the active simulation runner from the presenter
    std::shared_ptr<XyceSimulationRunner> runner = m_presenter->simulation_runner();
    // bind simulation events from the runner to this window
    runner->Bind(wxEVT_SIMULATION_FINISHED, &MainWindow::on_simulation_finished, this);
    runner->Bind(wxEVT_SIMULATION_STDOUT, &MainWindow::on_simulation_stdout, this);
    runner->Bind(wxEVT_SIMULATION_STDERR, &MainWindow::on_simulation_stderr, this);
    // launch the simulation
    runner->start(program, netlist_path, working_directory);
}

void MainWindow::set_simulation_running(bool /*running*/) {
    // the wx toolbar keeps a single run action; the Slint ui toggles Run/Stop
}

void MainWindow::cancel_simulation_process() {
    // get the active simulation runner from the presenter
    const auto runner = m_presenter->simulation_runner();
    // request a graceful shutdown when a runner is active
    if (runner)
        runner->cancel();
}

void MainWindow::spawn_raw_file_window(std::shared_ptr<XyceOutputFile> raw_file) {
    // create a new main window instance
    const auto frame = new MainWindow("KiCad Xyce Plugin");
    // show the new window
    frame->Show(true);
    // load the raw file into the new window's presenter
    frame->m_presenter->load_raw_file(std::move(raw_file));
}
