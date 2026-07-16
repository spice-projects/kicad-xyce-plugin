#include "app.h"
#include "ui/main_window.h"

bool App::OnInit() {
    // create main window instance
    const auto frame = new MainWindow("KiCad Xyce Plugin");
    // show main window
    frame->Show(true);
    // return true to continue running the app
    return true;
}
