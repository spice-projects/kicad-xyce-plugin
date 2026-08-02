#include <string>

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/button.h>
#include <wx/filedlg.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#endif

#include "../config/plugin_config.h"
#include "plugin_config_dialog.h"

PluginConfigDialog::PluginConfigDialog(wxWindow* parent, const PluginConfig& initial_config) :
    wxDialog(parent, wxID_ANY, "Plugin Configuration", wxDefaultPosition, wxSize(540, 200), wxDEFAULT_DIALOG_STYLE),
    m_initial_path(initial_config.xyce_executable_path()) {
    // enforce minimum size
    SetMinSize(wxSize(540, 200));

    // main vertical sizer
    auto main_sizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(main_sizer);

    // description label
    auto description = new wxStaticText(this, wxID_ANY, "Configure the Xyce executable path:");
    main_sizer->Add(description, 0, wxALL, FromDIP(12));

    // path row: label, read-only text input, and browse button
    auto path_sizer = new wxBoxSizer(wxHORIZONTAL);

    auto path_label = new wxStaticText(this, wxID_ANY, "Xyce path:");
    path_sizer->Add(path_label, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(8));

    m_path_input = new wxTextCtrl(this, wxID_ANY, wxString(m_initial_path), wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
    path_sizer->Add(m_path_input, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(8));

    auto browse_button = new wxButton(this, wxID_ANY, "Browse...");
    browse_button->Bind(wxEVT_BUTTON, &PluginConfigDialog::on_browse, this);
    path_sizer->Add(browse_button, 0, wxALIGN_CENTER_VERTICAL);

    main_sizer->Add(path_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(12));

    // error label, hidden by default
    m_error_label = new wxStaticText(this, wxID_ANY, wxEmptyString);
    m_error_label->SetForegroundColour(wxColour("#CC0000"));
    main_sizer->Add(m_error_label, 0, wxLEFT | wxRIGHT | wxTOP, FromDIP(10));
    m_error_label->Show(false);

    // platform-standard OK / Cancel buttons
    auto button_sizer = CreateButtonSizer(wxOK | wxCANCEL);
    main_sizer->Add(button_sizer, 0, wxEXPAND | wxALL, FromDIP(12));

    Bind(wxEVT_BUTTON, &PluginConfigDialog::on_ok, this, wxID_OK);

    Layout();
}

void PluginConfigDialog::on_browse(wxCommandEvent&) {
#ifdef __WXMSW__
    wxString wildcards = "Executable files (*.exe)|*.exe|All files (*.*)|*.*";
#else
    wxString wildcards = "All files (*)|*";
#endif
    // open native file picker for selecting the Xyce executable
    wxFileDialog dialog(this, "Select Xyce Executable", wxEmptyString, wxEmptyString, wildcards, wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (dialog.ShowModal() == wxID_OK) {
        wxString path = dialog.GetPath();
        // push selected path back into the text field
        m_path_input->SetValue(path);
        // clear any previous validation message after a new selection
        m_error_label->Show(false);
        Layout();
    }
}

void PluginConfigDialog::on_ok(wxCommandEvent& event) {
    wxString path = m_path_input->GetValue();
    path.Trim(true).Trim(false);
    // require a non-empty path so the plugin can launch Xyce
    if (path.IsEmpty()) {
        // render validation feedback in the dialog
        m_error_label->SetLabel("Xyce executable path is required");
        m_error_label->Show(true);
        Layout();
        return;
    }
    PluginConfig config(path.ToStdString());
    // reject path values that are not executable files
    if (!config.is_xyce_executable_valid()) {
        // render validation feedback in the dialog
        m_error_label->SetLabel("Selected path is not an executable file");
        m_error_label->Show(true);
        Layout();
        return;
    }
    // persist validated configuration
    config.save();
    // allow the default button handler to close the dialog
    event.Skip();
}

PluginConfig PluginConfigDialog::get_config() const {
    return PluginConfig(m_path_input->GetValue().ToStdString());
}