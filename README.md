# KiCad Xyce Plugin

KiCad plugin that integrates the Xyce circuit simulator into the KiCad UI, so you can configure and run simulations directly from a schematic workflow.

## Current status

- Development status: in progress
- Primary action: Run Xyce Circuit Simulator
- Runtime: Python 3.12+

## What this plugin provides

- Native KiCad plugin action to launch the simulator UI
- Simulation command dialog for transient setup
- Xyce process runner with streamed stdout and stderr handling
- Persistent plugin configuration for the Xyce executable path
- Qt/PySide6 desktop UI integrated with KiCad

## Repository layout

- plugin/: KiCad plugin runtime files
- tests/: unit tests
- docs/: project documentation
- kicad-icons/: source icon asset bundle used to populate plugin icons

## Requirements

- Python 3.12 or newer
- KiCad environment with plugin runtime support
- Xyce executable installed and available on disk

Python dependencies are declared in pyproject.toml.

## Local development setup

1. Create and activate a virtual environment
2. Install dependencies

```bash
python -m venv .venv
source .venv/bin/activate
pip install -U pip
pip install -e .
```

If you prefer non-editable install:

```bash
pip install .
```

## Running the plugin locally

The plugin entrypoint used by KiCad is plugin/run_simulator.py.

For direct local execution during development:

```bash
cd plugin
python run_simulator.py --log-level=DEBUG
```

## Building the KiCad package

```bash
hatch build --target kicad-package
```

## Testing

```bash
python -m pytest
```

## Configuration

At runtime, the plugin expects a valid path to the Xyce executable. Configure it in the plugin UI via the Configuration dialog. The value is persisted using Qt settings.

## Troubleshooting

- If simulation fails to start, verify the configured Xyce path points to an executable file
- If the plugin opens but does not run from KiCad, verify KiCad plugin discovery and runtime environment configuration
- If Qt import errors occur, verify PySide6 is installed in the active environment

## Contributing

1. Open an issue describing the proposed change
2. Implement and test in a feature branch
3. Submit a pull request with a clear summary and validation notes

## License

Project source code is licensed under Apache-2.0. See LICENSE.

This repository also bundles third-party icon assets from KiCad under CC-BY-SA 4.0 in plugin/kicad-icons. See:

- plugin/kicad-icons/LICENSE
- plugin/kicad-icons/COPYING
- THIRD_PARTY_NOTICES.txt

This repository also bundles third-party Xyce documentation PDFs in xyce-docs. See:

- xyce-docs/Xyce_RG.pdf
- xyce-docs/Xyce_UG.pdf
- xyce-docs/COPYING.XYCE
- THIRD_PARTY_NOTICES.txt

When redistributing this project, include the project LICENSE file and all third-party license and notice files listed above.

Python dependencies (including the KiCad Python API client library) are third-party components distributed under their own licenses. See THIRD_PARTY_NOTICES.txt for attribution and redistribution notes.
