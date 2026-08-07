#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <string>

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/frame.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/stc/stc.h>
#include <wx/tbarbase.h>
#endif

#include "../config/plugin_config.h"
#include "../expression/expression_manager.h"
#include "../file/xyce_output_file.h"
#include "../kicad/kicad_session.h"
#include "../simulation_parameters/simulation_config.h"
#include "../step_information.h"
#include "charts_panel.h"
#include "main_window_state.h"
#include "main_window_view.h"

class MainWindowPresenter;

class MainWindow : public wxFrame, public MainWindowView
{
public:
    MainWindow(const wxString& title, std::shared_ptr<KiCadSession> session = nullptr);

    // main window view interface
    void set_title(const std::string& title) override;
    void set_status_text(const std::string& text) override;
    void apply_action_enablement(const ActionStateEnablement& enablement) override;

    void show_netlist_view() override;
    void show_charts_view() override;
    void set_netlist_editor_content(const std::string& content) override;
    std::string netlist_editor_content() const override;
    void set_netlist_editor_read_only(bool read_only) override;
    bool charts_shown() const override;

    void show_simulation_output_panel() override;
    void hide_simulation_output_panel() override;
    void clear_simulation_output() override;
    void append_simulation_output_line(const std::string& line) override;
    bool simulation_output_panel_hidden() const override;
    bool simulation_output_has_content() const override;

    void update_charts(ExpressionManager& expression_manager, const StepInformation& step_information, const std::string& abscissa_label, AbscissaScale abscissa_scale) override;
    void delete_all_charts() override;

    std::optional<SimulationConfig> show_simulation_parameters_dialog(const SimulationConfig& current) override;
    std::optional<PluginConfig> show_plugin_config_dialog(const PluginConfig& current) override;

    void start_simulation_process(const std::string& program, const std::filesystem::path& netlist_path, const std::filesystem::path& working_directory) override;

    void spawn_raw_file_window(std::shared_ptr<XyceOutputFile> raw_file) override;

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
    std::unique_ptr<MainWindowPresenter> m_presenter;
    bool m_netlist_editor_updating = false;
    bool m_registered = false;

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

    void configure_netlist_editor();
};
