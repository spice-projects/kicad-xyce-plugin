#pragma once

#include <wx/frame.h>

#include "charts_panel.h"

class MainWindow : public wxFrame
{
public:
    explicit MainWindow(const wxString& title);

private:
    ChartsPanel* m_charts_panel;
    std::optional<XyceOutputFile> m_xyce_raw_file;

    void on_exit(wxCommandEvent&);

    void on_display_changed(wxDisplayChangedEvent&);

    void create_menubar();

    void create_toolbar();

    void create_statusbar();

    void on_menu_open(wxCommandEvent&);
};
