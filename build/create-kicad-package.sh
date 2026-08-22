#!/bin/bash

# detect host operating system
UNAME_S=$(uname -s)

# default build directory based on host OS
DEFAULT_BUILD_DIR=".build-debug-${UNAME_S}"
if [ ! -d "$DEFAULT_BUILD_DIR" ] && [ -d ".build-debug" ]; then
    DEFAULT_BUILD_DIR=".build-debug"
fi

# project version, defaults to 0.0.1
PROJECT_VERSION=${1:-0.0.1}
# path to the compiled plugin executable, defaults to the debug build output
EXECUTABLE=${2:-${DEFAULT_BUILD_DIR}/kicad-xyce-plugin}
# plugin entrypoint name as referenced by src/plugin.json
ENTRYPOINT_NAME=${3:-kicad-xyce-plugin}

# platform, allowed values: macos, linux, windows (auto-detected if not specified)
if [ -n "$4" ]; then
    PLATFORM="$4"
else
    case "$UNAME_S" in
        Darwin) PLATFORM="macos" ;;
        Linux)  PLATFORM="linux" ;;
        MINGW*|MSYS*|CYGWIN*) PLATFORM="windows" ;;
        *) PLATFORM="macos" ;;
    esac
fi

# fail early when the executable is missing
if [ ! -f "$EXECUTABLE" ]; then
    echo "error: executable not found at $EXECUTABLE" >&2
    exit 1
fi

# destination folder
mkdir -p dist
rm -rf dist/*

# create temporary folder
temp_dir=$(mktemp -d)

# initialize folder structure
mkdir -p "$temp_dir"/plugins

# copy metadata, replace "0.0.0" with version, replace platform
sed "s/0.0.0/$PROJECT_VERSION/g; s/\"macos\"/\"$PLATFORM\"/" metadata.json > dist/metadata.json
cp dist/metadata.json "$temp_dir"/metadata.json

# copy package icon
mkdir -p "$temp_dir"/resources
cp plugin-icon-64x64.png "$temp_dir"/resources/icon.png

# copy plugin manifest and icons
sed "s/ENTRYPOINT_NAME/$ENTRYPOINT_NAME/g" src/plugin.json > "$temp_dir"/plugins/plugin.json
cp src/plugin-icon-24x24.png "$temp_dir"/plugins/

# copy the compiled executable under the entrypoint name
cp "$EXECUTABLE" "$temp_dir"/plugins/"$ENTRYPOINT_NAME"

# distribution file
output_zip="$(pwd)/dist/kicad-xyce-plugin-$PROJECT_VERSION.zip"

# create package, flat structure
(cd "$temp_dir" && zip -r "$output_zip" .)

# clean up temporary folder
rm -rf "$temp_dir"

exit 0
