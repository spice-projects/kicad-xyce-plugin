#pragma once

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/event.h>
#endif

wxDECLARE_EVENT(wxEVT_NEW_WINDOW, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_SIMULATION_STDOUT, wxThreadEvent);
wxDECLARE_EVENT(wxEVT_SIMULATION_STDERR, wxThreadEvent);
wxDECLARE_EVENT(wxEVT_SIMULATION_FINISHED, wxThreadEvent);