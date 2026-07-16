#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <wx/frame.h>

// main window class
class MainWindow : public wxFrame
{
public:
    MainWindow(const wxString& title);

private:
    void on_exit(wxCommandEvent& event);
};

#endif
