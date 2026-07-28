#pragma once

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/frame.h>
#include <wx/sizer.h>
#include <wx/stc/stc.h>
#endif

#include "charts_panel.h"
#include "../simulation_parameters/simulation_config.h"

class MainWindow : public wxFrame
{
public:
    explicit MainWindow(const wxString& title);

private:
    wxToolBarToolBase* m_save_netlist_action = nullptr;
    wxToolBarToolBase* m_show_netlist_action = nullptr;
    wxToolBarToolBase* m_simulation_settings_action = nullptr;
    wxToolBarToolBase* m_simulation_run_action = nullptr;

    wxBoxSizer* m_main_sizer = nullptr;
    wxStyledTextCtrl* m_netlist_editor = nullptr;
    ChartsPanel* m_charts_panel = nullptr;

    void* m_kicad_client = nullptr;

    std::optional<std::shared_ptr<XyceOutputFile>> m_xyce_raw_file;
    std::filesystem::path m_xyce_netlist_file;

    SimulationConfig m_simulation_config;

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

    void on_configure_simulation(wxCommandEvent&);

    void on_run_simulation(wxCommandEvent&);

    void on_netlist_editor_modified(wxStyledTextEvent&);

    void on_netlist_editor_style_needed(wxStyledTextEvent&);

    bool update_xyce_netlist_file(const std::filesystem::path& filename);

    bool update_xyce_raw_file(std::optional<std::shared_ptr<XyceOutputFile>> raw_file, bool delete_charts);

    void update_netlist_editor_dirty_flag(bool);

    void configure_netlist_editor();
};
