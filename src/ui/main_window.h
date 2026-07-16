#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <wx/frame.h>

// main window class
class MainWindow : public wxFrame
{
public:
    explicit MainWindow(const wxString& title);

private:
    void on_exit(wxCommandEvent& event);

    void create_menubar();

    void create_toolbar();

    void create_statusbar();

    void on_menu_open(wxCommandEvent&);
};

#endif
