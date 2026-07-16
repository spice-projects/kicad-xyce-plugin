#include "app.h"

// implement wxApp entry point
wxIMPLEMENT_APP_NO_MAIN(App); // NOLINT

int main(int argc, char * argv[]) {
    // run wxWidgets application
    return wxEntry(argc, argv);
}
