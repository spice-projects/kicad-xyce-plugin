import os

def generate_icon_header():
    # define paths relative to the script location
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.abspath(os.path.join(script_dir, "..", ".."))
    png_path = os.path.join(project_root, "src", "kicad_xyce_plugin", "window-icon-512x512.png")
    out_path = os.path.join(project_root, "src", "ui", "icon_data.h")

    # read png data
    with open(png_path, "rb") as f:
        data = f.read()

    # format hex bytes
    hex_bytes = [f"0x{b:02x}" for b in data]
    
    # split into lines of 12 bytes
    lines = []
    for i in range(0, len(hex_bytes), 12):
        lines.append("    " + ", ".join(hex_bytes[i:i+12]))

    # generate file content
    content = f'''#ifndef ICON_DATA_H
#define ICON_DATA_H

// generated using: python3 src/ui/resources/generate_icon.py
// window-icon-512x512.png byte array
inline const unsigned char window_icon_512x512_png[] = {{
{',\n'.join(lines)}
}};
inline const unsigned int window_icon_512x512_png_len = {len(data)};

#endif
'''

    # write to icon_data.h
    with open(out_path, "w") as f:
        f.write(content)

    print(f"Generated {out_path}")

if __name__ == "__main__":
    generate_icon_header()
