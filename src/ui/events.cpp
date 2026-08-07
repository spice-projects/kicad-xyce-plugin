#include "events.h"

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/event.h>
#endif

wxDEFINE_EVENT(wxEVT_NEW_WINDOW, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_SIMULATION_STDOUT, wxThreadEvent);
wxDEFINE_EVENT(wxEVT_SIMULATION_STDERR, wxThreadEvent);
wxDEFINE_EVENT(wxEVT_SIMULATION_FINISHED, wxThreadEvent);