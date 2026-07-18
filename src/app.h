#pragma once

#include <wx/app.h>

// main application class
class App : public wxApp
{
public:
    // initialize application
    bool OnInit() override;
};
