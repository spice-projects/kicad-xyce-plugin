#include <filesystem>
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
#include "../netlist/file_netlist_source.h"
#include "../netlist/netlist.h"
#include "charts_panel.h"
#include "events.h"
#include "icon_manager.h"
#include "main_window.h"
#include "plugin_config_dialog.h"
#include "simulation_parameters/simulation_parameters_dialog.h"
#include "xyce_simulation_runner.h"

namespace
{
    class EditorNetlistSource : public NetlistSource
    {
    public:
        explicit EditorNetlistSource(wxStyledTextCtrl* netlist_editor, std::filesystem::path file_path) :
            m_netlist_editor(netlist_editor), m_initialized(false) {
            // create a file-based netlist source for the given path
            m_file_source = std::make_unique<FileNetlistSource>(file_path.string());
        }

        [[nodiscard]] std::string title() const override { return m_file_source->title(); }

        [[nodiscard]] bool is_read_only() const override { return false; }

        [[nodiscard]] virtual std::filesystem::path working_directory() const override { return m_file_source->working_directory(); }

        [[nodiscard]] std::tuple<bool, std::string> load_netlist() override {
            // check we have initialized the editor with the file content
            if (!m_initialized) {
                // load the netlist from the file source
                const auto [reloaded, content] = m_file_source->load_netlist();
                // mark as initialized
                m_initialized = true;
                // return the result of loading from the file source
                return {reloaded, content};
            }
            // load current text from editor
            return {false, m_netlist_editor->GetText().ToStdString()};
        }

        virtual void save_netlist(const std::string& content = "") override {
            // use the file source to save the current editor content
            m_file_source->save_netlist(content.empty() ? m_netlist_editor->GetText().ToStdString() : content);
        }

    private:
        wxStyledTextCtrl* m_netlist_editor;
        std::unique_ptr<FileNetlistSource> m_file_source;
        bool m_initialized;
    };
}; // namespace

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
    wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(800, 600)), m_kicad_session(std::move(session)), m_simulation_config(SimulationConfig::from_xyce_directives({})), m_plugin_config(PluginConfig::load()) {
    // take ownership of the netlist source in KiCad plugin mode
    if (m_kicad_session != nullptr)
        m_netlist_source = m_kicad_session->take_netlist_source();
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
    update_action_states();
    // load the schematic netlist in KiCad plugin mode
    if (m_kicad_session != nullptr)
        extract_schematic_netlist();
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
    // create main window instance
    const auto frame = new MainWindow("KiCad Xyce Plugin");
    // show main window
    frame->Show(true);
    // update file reference
    frame->update_xyce_raw_file(m_xyce_raw_file, true);
    // refresh toolbar/menu states in the new window
    frame->update_action_states();
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
            // replace the current netlist source with the file source for this path
            m_netlist_source = std::make_unique<EditorNetlistSource>(m_netlist_editor, filepath.ToStdString());
            // update title
            SetTitle(m_netlist_source->title());
            // netlist editor is now editable
            m_netlist_editor->SetReadOnly(false);
            // load the netlist content
            const auto [reloaded, content] = m_netlist_source->load_netlist();
            // set the editor content to the loaded netlist
            update_netlist_editor_content(content, false);
            // remove output file reference
            m_xyce_raw_file = std::nullopt;
            // hide/show panels
            m_content_sizer->Hide(m_charts_panel);
            m_content_sizer->Show(m_netlist_editor);
            m_content_sizer->Layout();
            // update states
            CallAfter([this]() { update_action_states(); });
            // exit
            return;
        }
        // raw file extension
        if (extension == "raw") {
            // parse file & update it in window field
            if (update_xyce_raw_file(xyce_raw_file_parser(filepath.ToStdString()), true)) {
                // update title
                SetTitle(m_xyce_raw_file.value()->title());
                // clear netlist editor content
                update_netlist_editor_content("", false);
                // hide/show panels
                m_content_sizer->Show(m_charts_panel);
                m_content_sizer->Hide(m_netlist_editor);
                m_content_sizer->Layout();
                // hide the simulation output panel for the raw file view
                if (m_body_splitter->IsSplit())
                    m_body_splitter->Unsplit(m_simulation_output_container);
                // refresh toolbar/menu states
                update_action_states();
            }
        }
    }
}

void MainWindow::on_menu_file_save(wxCommandEvent&) {
    // save content in netlist source
    m_netlist_source->save_netlist();
    // reset dirty flag
    if (update_netlist_editor_dirty_flag(false)) {
        // update state
        update_action_states();
    }
}

void MainWindow::on_show_netlist(wxCommandEvent&) {
    // hide/show panels
    m_content_sizer->Hide(m_charts_panel);
    m_content_sizer->Show(m_netlist_editor);
    m_content_sizer->Layout();
    // refresh toolbar/menu states
    update_action_states();
}

void MainWindow::on_show_charts(wxCommandEvent&) {
    // hide/show panels
    m_content_sizer->Hide(m_netlist_editor);
    m_content_sizer->Show(m_charts_panel);
    m_content_sizer->Layout();
    // refresh toolbar/menu states
    update_action_states();
}

void MainWindow::on_show_simulation_output(wxCommandEvent&) {
    // re-show the simulation output panel
    show_simulation_output_panel();
    // refresh toolbar/menu states
    update_action_states();
}

void MainWindow::on_configure_simulation(wxCommandEvent&) {
    // load the netlist content
    const auto [reloaded, content] = m_netlist_source->load_netlist();
    // check content was reloaded
    if (reloaded) {
        // update editor with reloaded content
        update_netlist_editor_content(content, false);
    }
    // parse netlist and extract topology
    const auto [sanitized_netlist, topology] = parse_netlist(content);
    // build simulation config from parsed directives
    m_simulation_config = SimulationConfig::from_xyce_directives(topology.m_directives);
    // create dialog with current config
    SimulationParametersDialog dialog(this, m_simulation_config);
    // show modal
    if (dialog.ShowModal() == wxID_OK) {
        // updated simulation config from dialog
        m_simulation_config = dialog.get_config();
        // build directives from simulation config with topology expansion
        const auto directives = m_simulation_config.to_xyce_directives(topology);
        // merge directives into sanitized netlist before .END
        const auto final_netlist = build_final_netlist(sanitized_netlist, directives, topology.m_passthrough_directives);
        // update editor with final netlist
        if (update_netlist_editor_content(final_netlist, content != final_netlist)) {
            // update state (deferred to next event loop iteration)
            CallAfter([this]() { update_action_states(); });
        }
        // exit
        return;
    }
    // update state if content was reloaded but user canceled the dialog
    if (reloaded) {
        // update state (deferred to next event loop iteration)
        CallAfter([this]() { update_action_states(); });
    }
}

void MainWindow::on_run_simulation(wxCommandEvent&) {
    // validate plugin configuration before launching
    if (!m_plugin_config.is_xyce_executable_valid()) {
        // update statusbar with error
        SetStatusText("Configured Xyce executable path is invalid");
        // exit
        return;
    }
    // netlist source content
    auto [reloaded, content] = m_netlist_source->load_netlist();
    // parse netlist and extract topology
    auto [sanitized_netlist, topology] = parse_netlist(content);
    // guard against empty netlist
    if (sanitized_netlist.empty()) {
        // update statusbar
        SetStatusText("No netlist content to simulate");
        // update editor with final netlist
        if (reloaded && update_netlist_editor_content("", false)) {
            // update state (deferred to next event loop iteration)
            CallAfter([this]() { update_action_states(); });
        }
        // exit
        return;
    }
    // initialize simulation config from parsed directives
    m_simulation_config = SimulationConfig::from_xyce_directives(topology.m_directives);
    // check if analysis is configured; if not, prompt the user
    if (std::holds_alternative<std::monostate>(m_simulation_config.analysis)) {
        // open configure dialog to set up analysis
        SimulationParametersDialog dialog(this, m_simulation_config);
        // show modal
        if (dialog.ShowModal() != wxID_OK) {
            // update editor with final netlist
            if (reloaded && update_netlist_editor_content(content, false)) {
                // update state (deferred to next event loop iteration)
                CallAfter([this]() { update_action_states(); });
            }
            // exit
            return;
        }
        // updated simulation config from dialog
        m_simulation_config = dialog.get_config();
    }
    // build directives from simulation config with topology expansion
    const auto directives = m_simulation_config.to_xyce_directives(topology);
    // merge directives into sanitized netlist before .END
    const auto final_netlist = build_final_netlist(sanitized_netlist, directives, topology.m_passthrough_directives);
    // update editor with final netlist
    if (update_netlist_editor_content(final_netlist, content != final_netlist)) {
        // update state (deferred to next event loop iteration)
        CallAfter([this]() { update_action_states(); });
    }
    // working directory
    auto working_dir = m_netlist_source->working_directory();
    // create temporary netlist file for the runner
    auto temp_path = XyceSimulationRunner::create_temp_netlist(final_netlist);
    if (temp_path.empty()) {
        // update statusbar with error
        SetStatusText("Failed to create temporary netlist file");
        // exit
        return;
    }
    // clear any previous runner
    m_simulation_runner.reset();
    // create new simulation runner
    m_simulation_runner = std::make_shared<XyceSimulationRunner>();
    // bind simulation events from the runner to this window
    m_simulation_runner->Bind(wxEVT_SIMULATION_FINISHED, &MainWindow::on_simulation_finished, this);
    m_simulation_runner->Bind(wxEVT_SIMULATION_STDOUT, &MainWindow::on_simulation_stdout, this);
    m_simulation_runner->Bind(wxEVT_SIMULATION_STDERR, &MainWindow::on_simulation_stderr, this);
    // mark the simulation as running
    m_simulation_running = true;
    // show the simulation output panel for this run
    show_simulation_output_panel();
    // reset the log for this run
    m_simulation_output_panel->ClearAll();
    // launch the simulation
    m_simulation_runner->start(m_plugin_config.xyce_executable_path(), temp_path, working_dir);
    // refresh toolbar/menu states
    update_action_states();
    // update statusbar
    SetStatusText("Simulation started...");
}

void MainWindow::on_plugin_configuration(wxCommandEvent&) {
    // create config dialog
    PluginConfigDialog dialog(this, m_plugin_config);
    // show modal
    if (dialog.ShowModal() == wxID_OK) {
        // log information
        spdlog::info("Plugin configuration updated: Xyce path = {}", m_plugin_config.xyce_executable_path());
        // update plugin configuration from dialog
        m_plugin_config = dialog.get_config();
    }
}

void MainWindow::on_simulation_finished(wxThreadEvent& event) {
    // extract exit code and canceled flag from the event
    int exit_code = event.GetInt();
    bool was_canceled = event.GetPayload<bool>();
    // mark the simulation as no longer running
    m_simulation_running = false;
    // handle canceled simulations
    if (was_canceled) {
        // update statusbar
        SetStatusText("Simulation canceled");
        // clear the runner
        m_simulation_runner.reset();
        // refresh toolbar/menu states
        update_action_states();
        // exit
        return;
    }
    // check for success
    if (exit_code == 0) {
        // ensure we have a runner reference for accessing paths
        if (!m_simulation_runner) {
            // update statusbar
            SetStatusText("Simulation finished but runner reference is missing");
            // refresh toolbar/menu states
            update_action_states();
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
                m_content_sizer->Show(m_charts_panel);
                m_content_sizer->Hide(m_netlist_editor);
                m_content_sizer->Layout();
                // update title
                SetTitle(m_xyce_raw_file.value()->title());
                // update statusbar
                SetStatusText("Simulation finished successfully");
                // clear the runner
                m_simulation_runner.reset();
                // refresh toolbar/menu states
                update_action_states();
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
    // refresh toolbar/menu states
    update_action_states();
}

void MainWindow::on_simulation_stdout(wxThreadEvent& event) {
    // stdout line from the simulation process
    const auto output_line = event.GetPayload<std::string>();
    // convert the raw output line to a wxString
    wxString output_text;
    if (!output_line.empty()) {
        // try strict UTF-8 conversion first
        output_text = wxString::FromUTF8(output_line.data(), output_line.size());
        // fall back to a byte-preserving conversion when the line is not valid UTF-8
        if (output_text.empty())
            output_text = wxString::From8BitData(output_line.data(), output_line.size());
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

void MainWindow::on_simulation_stderr(wxThreadEvent& event) {
    // capture the error line
    std::string error_line = event.GetPayload<std::string>();
    // log information
    spdlog::warn("{}", error_line);
    // update statusbar with the latest error line
    SetStatusText(wxString::Format("Simulation error: %s", error_line));
}

void MainWindow::on_netlist_editor_modified(wxStyledTextEvent&) {
    // set editor as dirty if not updating via programmatic
    if (!m_netlist_editor_updating && update_netlist_editor_dirty_flag(true)) {
        // update state
        update_action_states();
    }
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

bool MainWindow::update_netlist_editor_dirty_flag(bool flag) {
    // previous dirty state
    bool previous_dirty_state = m_netlist_editor_dirty;
    // store the dirty state
    m_netlist_editor_dirty = flag;
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
    // return true if the dirty state changed, false if it remained the same
    return previous_dirty_state != m_netlist_editor_dirty;
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

void MainWindow::show_simulation_output_panel() {
    // split the body when the simulation output panel is still hidden
    if (!m_body_splitter->IsSplit()) {
        // split splitter horizontally, top pane holds content views, bottom pane the simulation output
        m_body_splitter->SplitHorizontally(m_content_panel, m_simulation_output_container);
        // keep the log pane at a fixed portion of the body height
        m_body_splitter->SetSashPosition(m_body_splitter->GetClientSize().GetHeight() * 3 / 4);
    }
}

void MainWindow::update_action_states() {
    // netlist content is available when the editor holds text
    const bool has_netlist = m_netlist_editor != nullptr && !m_netlist_editor->GetText().IsEmpty();
    // raw output data is available
    const bool has_raw = m_xyce_raw_file.has_value();
    // charts view is the active content view
    const bool charts_shown = m_charts_panel != nullptr && m_charts_panel->IsShown();
    // simulation output can be re-shown when the splitter is hidden and the log holds content
    const bool output_hidden = m_body_splitter != nullptr && !m_body_splitter->IsSplit();
    const bool log_has_content = m_simulation_output_panel != nullptr && m_simulation_output_panel->GetLength() > 0;
    // gather the input flags describing the current window state
    ActionStateInput input;
    input.has_netlist = has_netlist;
    input.has_netlist_file = m_netlist_source != nullptr && !m_netlist_source->is_read_only();
    input.has_raw = has_raw;
    input.charts_shown = charts_shown;
    input.simulation_running = m_simulation_running;
    input.netlist_editor_dirty = m_netlist_editor_dirty;
    input.output_hidden = output_hidden;
    input.log_has_content = log_has_content;
    // derive the current application state
    const AppState state = derive_app_state(input);
    // compute the action enablement for the current state
    const ActionStateEnablement enablement = compute_action_enablement(input);
    // log state transitions
    if (state != m_app_state) {
        // store the new state
        m_app_state = state;
        // log the transition
        spdlog::debug("Application state changed to {}", app_state_name(state));
    }
    // apply toolbar states
    if (m_kicad_session == nullptr) {
        // only when not in KiCad plugin mode, the open/save actions are available
        m_open_netlist_action->Enable(enablement.open);
        m_save_netlist_action->Enable(enablement.save);
    }
    m_show_netlist_action->Enable(enablement.show_netlist);
    m_show_charts_action->Enable(enablement.show_charts);
    m_show_simulation_output_action->Enable(enablement.show_simulation_output);
    m_simulation_settings_action->Enable(enablement.configure_simulation);
    m_simulation_run_action->Enable(enablement.run_simulation);
    // mirror enablement to menu items sharing the same ids
    if (auto* menu_bar = GetMenuBar()) {
        // not available in KiCad plugin mode
        if (m_kicad_session == nullptr) {
            // file open action
            menu_bar->Enable(wxID_OPEN, enablement.open);
            // file save action
            menu_bar->Enable(wxID_SAVE, enablement.save);
        }
        // tools show charts action
        menu_bar->Enable(wxID_SHOW_CHARTS, enablement.show_charts);
        // tools view simulation output action
        menu_bar->Enable(wxID_SHOW_SIMULATION_OUTPUT, enablement.show_simulation_output);
        // tools configure simulation action
        menu_bar->Enable(wxID_CONFIGURE_SIMULATION, enablement.configure_simulation);
    }
}

void MainWindow::on_close_simulation_output(wxCommandEvent&) {
    // unsplit the bottom pane to dismiss the simulation output panel
    m_body_splitter->Unsplit(m_simulation_output_container);
    // refresh toolbar/menu states
    update_action_states();
}

bool MainWindow::extract_schematic_netlist() {
    // extract netlist from schematic
    auto [_, content] = m_netlist_source->load_netlist();
    // set content into the read-only editor
    update_netlist_editor_content(content, false);
    // show the netlist editor and hide the charts panel
    m_content_sizer->Show(m_netlist_editor);
    m_content_sizer->Hide(m_charts_panel);
    m_content_sizer->Layout();
    // refresh toolbar/menu states
    update_action_states();
    // exit
    return true;
}

bool MainWindow::update_netlist_editor_content(const std::string& content, bool dirty_flag) {
    try {
        // prevent dirty analysis
        m_netlist_editor_updating = true;
        // current read-only state of the editor
        const bool read_only = m_netlist_editor->GetReadOnly();
        // update editor content, temporarily allowing edits if the editor is read-only
        m_netlist_editor->SetReadOnly(false);
        m_netlist_editor->SetText(content);
        m_netlist_editor->SetReadOnly(read_only);
        // reset flag (on next event loop iteration)
        CallAfter([this]() { m_netlist_editor_updating = false; });
        // update editor as dirty or clean based on the provided flag
        return update_netlist_editor_dirty_flag(dirty_flag);
    }
    catch (const std::exception& e) {
        // log error
        spdlog::error("Failed to update netlist editor content: {}", e.what());
        // reset flag (on next event loop iteration)
        CallAfter([this]() { m_netlist_editor_updating = false; });
        // indicate failure
        return false;
    }
}
