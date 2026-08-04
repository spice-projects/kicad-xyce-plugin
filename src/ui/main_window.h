#pragma once

#include <memory>

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/frame.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/stc/stc.h>
#include <wx/tbarbase.h>
#endif

#include "../config/plugin_config.h"
#include "../kicad/kicad_session.h"
#include "../netlist/netlist_source.h"
#include "../simulation_parameters/simulation_config.h"
#include "charts_panel.h"
#include "main_window_state.h"

class XyceSimulationRunner;

class MainWindow : public wxFrame
{
public:
    MainWindow(const wxString& title, std::shared_ptr<KiCadSession> session = nullptr);

private:
    wxToolBarToolBase* m_open_netlist_action = nullptr;
    wxToolBarToolBase* m_save_netlist_action = nullptr;
    wxToolBarToolBase* m_show_netlist_action = nullptr;
    wxToolBarToolBase* m_simulation_settings_action = nullptr;
    wxToolBarToolBase* m_simulation_run_action = nullptr;
    wxToolBarToolBase* m_show_simulation_output_action = nullptr;
    wxToolBarToolBase* m_show_charts_action = nullptr;

    wxBoxSizer* m_main_sizer = nullptr;
    wxBoxSizer* m_content_sizer = nullptr;
    wxSplitterWindow* m_body_splitter = nullptr;
    wxPanel* m_content_panel = nullptr;
    wxStyledTextCtrl* m_netlist_editor = nullptr;
    ChartsPanel* m_charts_panel = nullptr;
    wxPanel* m_simulation_output_container = nullptr;
    wxStyledTextCtrl* m_simulation_output_panel = nullptr;

    std::shared_ptr<KiCadSession> m_kicad_session;
    std::unique_ptr<NetlistSource> m_netlist_source;
    bool m_netlist_editor_updating = false;

    AppState m_app_state = AppState::Empty;
    bool m_simulation_running = false;
    bool m_netlist_editor_dirty = false;

    std::optional<std::shared_ptr<XyceOutputFile>> m_xyce_raw_file;

    SimulationConfig m_simulation_config;
    PluginConfig m_plugin_config;

    std::shared_ptr<XyceSimulationRunner> m_simulation_runner;

    void on_system_colour_changed(wxSysColourChangedEvent&);

    void on_display_changed(wxDisplayChangedEvent&);

    void on_exit(wxCommandEvent&);

    void on_new_window(wxCommandEvent&);

    void create_menubar();

    void create_toolbar();

    void create_statusbar();

    void on_menu_file_open(wxCommandEvent&);

    void on_menu_file_save(wxCommandEvent&);

    void on_show_netlist(wxCommandEvent&);

    void on_show_charts(wxCommandEvent&);

    void on_show_simulation_output(wxCommandEvent&);

    void on_configure_simulation(wxCommandEvent&);

    void on_run_simulation(wxCommandEvent&);

    void on_plugin_configuration(wxCommandEvent&);

    void on_simulation_finished(wxThreadEvent&);

    void on_simulation_stdout(wxThreadEvent&);

    void on_simulation_stderr(wxThreadEvent&);

    void on_close_simulation_output(wxCommandEvent&);

    void on_netlist_editor_modified(wxStyledTextEvent&);

    void on_netlist_editor_style_needed(wxStyledTextEvent&);

    bool update_xyce_raw_file(std::optional<std::shared_ptr<XyceOutputFile>>, bool);

    bool update_netlist_editor_dirty_flag(bool);

    void configure_netlist_editor();

    void update_action_states();

    void show_simulation_output_panel();

    bool extract_schematic_netlist();

    bool update_netlist_editor_content(const std::string&, bool);
};
