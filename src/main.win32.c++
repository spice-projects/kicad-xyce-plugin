#include <windows.h>

#include "app/app.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // create application
    auto& application = App::instance();
    // configure the application (command line parsing, logging, platform setup)
    // application.initialize(argc, argv);
    // create the main window and run the application event loop
    return application.run();
}
