#import <AppKit/AppKit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

#include <filesystem>
#include <optional>
#include <string>

#include <spdlog/spdlog.h>

#include "file_dialog.h"

std::optional<std::filesystem::path> FileDialog::open_xyce_file() {
    // build the content types accepted by the dialog from the extensions
    // matching the wxWidgets wildcard "Netlist files (*.cir)|*.cir|Xyce output
    // files (*.raw, *.fftX)|*.raw;*.fft?;*.fft??"
    NSMutableArray<UTType*>* content_types = [NSMutableArray array];
    for (NSString* extension in @[ @"cir", @"raw" ]) {
        // skip extensions that map to no known type
        UTType* type = [UTType typeWithFilenameExtension:extension];
        if (type != nil) {
            [content_types addObject:type];
        }
    }
    // create the native open panel
    NSOpenPanel* panel = [NSOpenPanel openPanel];
    // configure the dialog for a single existing file
    panel.title = @"Select Xyce input/output file";
    panel.canChooseFiles = YES;
    panel.canChooseDirectories = NO;
    panel.allowsMultipleSelection = NO;
    panel.allowsOtherFileTypes = YES;
    panel.allowedContentTypes = content_types;
    // run the dialog modally
    if ([panel runModal] != NSModalResponseOK) {
        // user canceled the dialog
        return std::nullopt;
    }
    // selected file url
    NSURL* url = [panel URL];
    // convert the cocoa path to std::filesystem::path
    return std::filesystem::path(std::string(url.path.UTF8String));
}

std::optional<std::filesystem::path> FileDialog::open_xyce_executable() {
    // create the native open panel
    NSOpenPanel* panel = [NSOpenPanel openPanel];
    // configure the dialog for a single existing file; no content type filter
    // so any executable file is selectable (mirrors the wx "All files (*)|*"
    // wildcard used for the Xyce executable)
    panel.title = @"Select Xyce Executable";
    panel.canChooseFiles = YES;
    panel.canChooseDirectories = NO;
    panel.allowsMultipleSelection = NO;
    panel.allowedContentTypes = nil;
    // run the dialog modally
    if ([panel runModal] != NSModalResponseOK) {
        // user canceled the dialog
        return std::nullopt;
    }
    // selected file url
    NSURL* url = [panel URL];
    // convert the cocoa path to std::filesystem::path
    return std::filesystem::path(std::string(url.path.UTF8String));
}
