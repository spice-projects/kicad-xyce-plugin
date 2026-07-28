#pragma once

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#endif

// bordered card container with a header row (title + optional badge) and a body area
class SimulationCard : public wxPanel
{
public:
    // construct a simulation card with a title and optional badge
    SimulationCard(wxWindow* parent, const wxString& title, const wxString& badge = "");

    // return the content panel where callers can add child controls
    [[nodiscard]] wxPanel* get_content() const;

private:
    wxStaticText* m_title_text = nullptr;
    wxStaticText* m_badge_text = nullptr;
    wxPanel* m_content = nullptr;
};
