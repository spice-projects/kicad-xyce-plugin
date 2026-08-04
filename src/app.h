#pragma once

#include <wx/app.h>

class App : public wxApp
{
public:
    bool OnInit() override;
    void OnInitCmdLine(wxCmdLineParser&) override;
    bool OnCmdLineParsed(wxCmdLineParser&) override;

private:
    static void setup_logger(const wxString&);
};
