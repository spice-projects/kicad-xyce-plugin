import os
import glob
import re
from collections import defaultdict

def filename_to_symbol(filename):
    """
    Converts a filename into a valid C identifier name.
    Example: 'window-icon-512x512.png' -> 'window_icon_512x512_png'
             'cancel_24.png' -> 'cancel_24_png'
    """
    symbol = re.sub(r'[^a-zA-Z0-9]', '_', filename)
    if symbol and symbol[0].isdigit():
        symbol = '_' + symbol
    return symbol

def get_family_info(png_path, script_dir):
    """
    Determines family key and family var_name.
    Strips resolution suffixes like _16, _24, _32, _48, _64 or -512x512 from stem.
    Example: 'kicad-icons/cancel_16.png' -> family: 'kicad-icons/cancel', var_name: 'cancel_png'
    """
    rel_path = os.path.relpath(png_path, script_dir)
    stem, ext = os.path.splitext(rel_path)
    
    # Strip resolution patterns like _16, _24, _512x512, -512x512
    family_stem = re.sub(r'[_\-](?:\d+x\d+|\d+)$', '', stem)
    
    filename = os.path.basename(family_stem + ext)
    family_var_name = filename_to_symbol(filename)
    
    return family_stem, family_var_name

def generate_icon_header():
    # define directory containing this script (src/ui)
    script_dir = os.path.dirname(os.path.abspath(__file__))
    out_path = os.path.join(script_dir, "icon_data.h")

    # Find all PNG files under script_dir recursively
    png_files = sorted(glob.glob(os.path.join(script_dir, "**", "*.png"), recursive=True))
    
    if not png_files:
        print(f"No PNG files found under {script_dir}")
        return

    sections = []
    seen_symbols = {}
    families = defaultdict(list)

    for png_path in png_files:
        filename = os.path.basename(png_path)
        var_name = filename_to_symbol(filename)

        # Handle duplicate filenames in different subdirectories safely
        if var_name in seen_symbols:
            rel_dir = os.path.dirname(os.path.relpath(png_path, script_dir))
            prefix = filename_to_symbol(rel_dir)
            var_name = f"{prefix}_{var_name}"

        seen_symbols[var_name] = png_path

        family_stem, family_var_name = get_family_info(png_path, script_dir)
        families[family_stem].append((var_name, family_var_name))

        with open(png_path, "rb") as f:
            data = f.read()

        hex_bytes = [f"0x{b:02x}" for b in data]
        lines = []
        for i in range(0, len(hex_bytes), 12):
            lines.append("    " + ", ".join(hex_bytes[i:i+12]))

        rel_path = os.path.relpath(png_path, script_dir)
        section = f"// {rel_path} byte array\n" \
                  f"inline const unsigned char {var_name}[] = {{\n" \
                  f"{',\n'.join(lines)}\n" \
                  f"}};\n" \
                  f"inline const unsigned int {var_name}_len = {len(data)};\n\n" \
                  f"inline wxBitmap get_{var_name}_bitmap() {{\n" \
                  f"    wxMemoryInputStream stream({var_name}, {var_name}_len);\n" \
                  f"    return wxBitmap(wxImage(stream, wxBITMAP_TYPE_PNG));\n" \
                  f"}}"
        sections.append(section)

    bundle_sections = []
    seen_bundle_names = set()
    manager_entries = []

    for family_stem, members in sorted(families.items()):
        _, family_var_name = members[0]

        # Get exact base filename without extension & without size suffix (preserving hyphens)
        raw_base = os.path.basename(family_stem)
        map_key = re.sub(r'[_\-](?:\d+x\d+|\d+)$', '', raw_base)

        # Ensure unique bundle names if different dirs have same family name
        bundle_func_name = f"get_{family_var_name}_bundle"
        if bundle_func_name in seen_bundle_names:
            dir_prefix = filename_to_symbol(os.path.dirname(family_stem))
            bundle_func_name = f"get_{dir_prefix}_{family_var_name}_bundle"
            dir_key = os.path.dirname(family_stem)
            map_key = f"{dir_key}/{map_key}"

        seen_bundle_names.add(bundle_func_name)
        manager_entries.append((map_key, bundle_func_name))

        if len(members) == 1:
            member_var_name, _ = members[0]
            bundle_section = f"// Bundle for {family_stem}\n" \
                             f"inline wxBitmapBundle {bundle_func_name}() {{\n" \
                             f"    return wxBitmapBundle::FromBitmap(get_{member_var_name}_bitmap());\n" \
                             f"}}"
        else:
            vector_push_code = "\n    ".join(f"bitmaps.push_back(get_{m[0]}_bitmap());" for m in members)
            bundle_section = f"// Bundle for {family_stem} ({len(members)} resolutions)\n" \
                             f"inline wxBitmapBundle {bundle_func_name}() {{\n" \
                             f"    wxVector<wxBitmap> bitmaps;\n" \
                             f"    {vector_push_code}\n" \
                             f"    return wxBitmapBundle::FromBitmaps(bitmaps);\n" \
                             f"}}"
        bundle_sections.append(bundle_section)

    header_guard = "ICON_DATA_H"
    content = f'''#pragma once

#include <wx/wx.h>
#include <wx/mstream.h>
#include <wx/bmpbndl.h>

// Generated automatically by generate_icon.py
// Contains raw byte arrays, wxBitmap, and wxBitmapBundle helper functions for all PNG icons under src/ui

{'\n\n'.join(sections)}

// ============================================================================
// wxBitmapBundle Families
// ============================================================================

{'\n\n'.join(bundle_sections)}
'''

    with open(out_path, "w") as f:
        f.write(content)

    print(f"Generated {out_path} with {len(png_files)} icons across {len(families)} icon bundles.")

    # Generate icon_manager.cpp
    manager_cpp_path = os.path.join(script_dir, "icon_manager.cpp")
    manager_map_code = "\n    ".join(f'm_bitmap_bundles["{key}"] = {func}();' for key, func in manager_entries)
    manager_content = f'''#include <wx/wx.h>

#include "icon_data.h"
#include "icon_manager.h"

IconManager::IconManager() {{
    // automatically generated icon bundle map
    {manager_map_code}
}}
'''

    with open(manager_cpp_path, "w") as f:
        f.write(manager_content)

    print(f"Generated {manager_cpp_path}")

if __name__ == "__main__":
    generate_icon_header()
