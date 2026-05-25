#!/bin/bash

# project version, defaults to 0.0.1
PROJECT_VERSION=${1:-0.0.1}

# destination folder
mkdir -p dist
rm -rf dist/*

# create wheel package
PACKAGE_VERSION=$PROJECT_VERSION python -m hatch build --target wheel

# create temporary folder
temp_dir=$(mktemp -d)

# initialze folder structure
mkdir -p "$temp_dir"/plugins
mkdir -p "$temp_dir"/resources

# copy metadata, replace "0.0.0" with version
sed "s/0.0.0/$PROJECT_VERSION/g" metadata.json > dist/metadata.json
cp dist/metadata.json "$temp_dir"/metadata.json

# copy package icon
cp plugin-icon-64x64.png "$temp_dir"/resources/icon.png

# copy plugin source files
cp src/plugin.py "$temp_dir"/plugins/
cp src/plugin.json "$temp_dir"/plugins/
cp src/plugin-icon-24x24.png "$temp_dir"/plugins/
cp src/requirements.txt "$temp_dir"/plugins/
cp dist/*.whl "$temp_dir"/plugins/
echo "__version__ = \"$PROJECT_VERSION\"" > "$temp_dir"/plugins/__version__.py

# distribution file
output_zip="$(pwd)/dist/kicad-xyce-plugin-$PROJECT_VERSION.zip"

# create package, flat structure
(cd "$temp_dir" && zip -r "$output_zip" .)

# clean up temporary folder
rm -rf "$temp_dir"

exit 0
