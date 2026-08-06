#pragma once

#include <string>

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/dialog.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#endif

#include "../config/plugin_config.h"

// modal dialog for selecting and validating the Xyce executable path
class PluginConfigDialog : public wxDialog
{
public:
    PluginConfigDialog(wxWindow* parent, const PluginConfig& initial_config);

    // retrieve the config object after a successful OK
    [[nodiscard]] PluginConfig get_config() const;

private:
    // open a native file picker to select the Xyce executable
    void on_browse(wxCommandEvent&);

    // validate the selected path and persist on OK
    void on_ok(wxCommandEvent&);

    wxTextCtrl* m_path_input = nullptr;
    wxStaticText* m_error_label = nullptr;
    std::string m_initial_path;
};