#pragma once

#include <cstddef>

#include <wx/app.h>

class App : public wxApp
{
public:
    bool OnInit() override;

    void OnInitCmdLine(wxCmdLineParser&) override;

    bool OnCmdLineParsed(wxCmdLineParser&) override;

    void register_frame();

    void unregister_frame();

private:
    size_t m_frame_count = 0;

    static void setup_logger(const wxString&);
};
