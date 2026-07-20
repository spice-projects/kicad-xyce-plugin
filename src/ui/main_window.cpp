#include <spdlog/spdlog.h>
#include <wx/artprov.h>
#include <wx/filedlg.h>
#include <wx/menu.h>
#include <wx/toolbar.h>

#include "main_window.h"
#include "charts_panel.h"
#include "../file/xyce_raw_file.h"

MainWindow::MainWindow(const wxString& title)
    : wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(800, 600)) {
    // create menubar/toolbar/statusbar
    create_menubar();
    create_toolbar();
    create_statusbar();

    spdlog::set_level(spdlog::level::debug);

    // create charts panel, it should cover all the available area
    m_charts_panel = new ChartsPanel(this);

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

void MainWindow::create_menubar() {
    // create menubar
    const auto menu_bar = new wxMenuBar();

    // file
    const auto file_menu = new wxMenu();
    file_menu->Append(wxID_EXIT, "E&xit\tAlt-X", "Quit this application");
    menu_bar->Append(file_menu, "&File");

    // file / open
    file_menu->Append(wxID_OPEN, "&Open\tAlt-X", "Open Xyce File");
    Bind(wxEVT_MENU, &MainWindow::on_menu_open, this, wxID_OPEN);

    // set menu bar for frame
    wxFrameBase::SetMenuBar(menu_bar);
}

void MainWindow::create_toolbar() {
    // create toolbar
    const auto tool_bar = wxFrame::CreateToolBar(wxTB_HORIZONTAL | wxTB_FLAT);

    // add tool button for exit
    tool_bar->AddTool(wxID_EXIT, "Exit", wxArtProvider::GetBitmap(wxART_QUIT, wxART_TOOLBAR));

    // realize toolbar
    tool_bar->Realize();
}

void MainWindow::create_statusbar() {
    // create statusbar
    wxFrame::CreateStatusBar(1);
    // set statusbar text
    wxFrame::SetStatusText("Welcome to KiCad Xyce Plugin");
}

void MainWindow::on_menu_open(wxCommandEvent&) {
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
        const auto filename = dialog.GetFilename();

        wxFrame::SetStatusText(filepath + filename);

        // parse file
        m_xyce_raw_file = xyce_raw_file_parser(filepath.ToStdString());
        if (m_xyce_raw_file.has_value()) {
            // file reference
            auto& file = m_xyce_raw_file.value();
            // TODO: disable simulation actions
            // update title

            // update all charts
            m_charts_panel->update_charts(file.expression_manager(), file.step_information(), "", file.abscissa_scale(), 100);

            // add chart
            const auto chart = m_charts_panel->add_chart();

            auto& em = file.expression_manager();

            auto i = em.evaluate("I(R213)");
            auto v = em.evaluate("V(VCC)");

            chart->plot_series({v, i});

            m_charts_panel->refresh_charts();
        }
    }
}
