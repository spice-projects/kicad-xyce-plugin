#import <AppKit/AppKit.h>

#include "app.h"
#include "ui/main_window.h"
#include "ui/icon_data.h"

bool App::OnInit() {
    // initialize image handlers
    wxInitAllImageHandlers();
    // create main window instance
    const auto frame = new MainWindow("KiCad Xyce Plugin");
    // show main window
    frame->Show(true);
    // set application dock icon from embedded png bytes
    NSData* png_data = [NSData dataWithBytes:window_icon_512x512_png length:window_icon_512x512_png_len];
    // create nsimage from png data
    NSImage* image = [[NSImage alloc] initWithData:png_data];
    // check if image was loaded
    if (image) {
        // set the application icon in the dock
        [NSApp setApplicationIconImage:image];
    }
    // return true to continue running the app
    return true;
}
