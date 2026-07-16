#include <wx/artprov.h>
#include <wx/menu.h>
#include <wx/toolbar.h>

#include "main_window.h"

MainWindow::MainWindow(const wxString& title)
    : wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(800, 600)) {
    // create file menu
    auto file_menu = new wxMenu();
    // append exit option to file menu
    file_menu->Append(wxID_EXIT, "E&xit\tAlt-X", "Quit this application");
    // create menu bar
    const auto menu_bar = new wxMenuBar();
    // append file menu to menu bar
    menu_bar->Append(file_menu, "&File");
    // set menu bar for frame
    wxFrameBase::SetMenuBar(menu_bar);
    // create toolbar
    const auto tool_bar = wxFrame::CreateToolBar(wxTB_HORIZONTAL | wxTB_FLAT);
    // add tool button for exit
    tool_bar->AddTool(wxID_EXIT, "Exit", wxArtProvider::GetBitmap(wxART_QUIT, wxART_TOOLBAR));
    // realize toolbar
    tool_bar->Realize();
    // create statusbar
    wxFrame::CreateStatusBar(1);
    // set statusbar text
    wxFrame::SetStatusText("Welcome to KiCad Xyce Plugin");
    // bind exit event
    Bind(wxEVT_MENU, &MainWindow::on_exit, this, wxID_EXIT);
    // bind close window event
    Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent&) {
        // destroy the window
        Destroy();
    });
}

void MainWindow::on_exit(wxCommandEvent&) {
    // close the window
    Close(true);
}
