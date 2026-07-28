# Migration Plan: Python/QML UI → C++ wxWidgets

## Important

Read the `AGENTS.md` document and the `STYLE-GUIDE.md` document, the instructions in these documents are mandatory.

## Overview

Migrate the simulation parameters UI from Python (PySide6/QtQuick/QML) to C++ (wxWidgets).
The data model layer (`src/simulation_parameters/`) is already migrated; this plan covers only
the UI layer, which consumes those C++ model types.

| Current (Python) | Target (C++) |
|---|---|
| `src/kicad_xyce_plugin/simulation_parameters/` (Python panels + QML) | `src/ui/simulation_parameters/` (wxWidgets panels) |
| SimulationCard.qml | `SimulationCard` — reusable bordered card |
| Print sections per panel | `PrintSectionPanel` — reusable .PRINT section widget |
| SensitivitySection.qml + .py | `SensitivitySectionPanel` — .SENS panel with embedded print |
| 7 `*_panel.py` + 7 `*_panel.qml` files | 7 `wxPanel` subclasses (apply + handle_submit merged inline) |
| `simulation_parameters_dialog.py` + `.qml` | `SimulationParametersDialog` (wxDialog) |
| `data_table_dialog.py` + `.qml` | `DataTableDialog` (wxDialog) |

---

## Design Rules

### Follow System Style — No Hardcoded Colors

All wxWidgets implementations **must not use hardcoded colors, fonts, or dimensions**.
Rely on the native platform theme:

- Use default `wxSystemSettings::GetColour()` for text, backgrounds, and highlights
- Use `wxWindow::SetBackgroundColour()` only with `wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW)`
- Do **not** replicate the Qt/QML styling (white backgrounds, gray sidebar, blue accents, rounded corners)
- Card borders: use `wxStaticBox` (natively styled) instead of custom `wxPanel` with drawn borders
- Fonts: use `wxNORMAL_FONT` / `wxSMALL_FONT` / `wxBOLD_FONT` — never specify family or size
- Layout spacing: use `wxSizer::SetSpacing()` with standard values (5-10px), not hardcoded pixel margins
- Dynamic spacing: prefer `wxBoxSizer` with proportion flags over fixed-position layouts

---

## Phase 1: Shared Infrastructure

### 1.1 SimulationCard (`wxPanel`)

A bordered card container with a header row (title + optional badge) and a body area.
Used by every analysis panel to group related controls.

**Reference**: `src/kicad_xyce_plugin/simulation_parameters/simulation_card.qml`

- `SimulationCard(wxWindow* parent, const wxString& title, const wxString& badge = "")`
- Internally uses `wxStaticBox` as the card border (native styling)
- Header uses `wxStaticText` with default bold font
- `GetContent()` returns the content `wxPanel*` so callers can add children

### 1.2 GlobalSettingsPanel

A tiny panel with the "Replace ground (GND) with 0" checkbox and an explanatory label.
Shared across all 7 analysis panels.

**Reference**: "Global Settings" section in every `*_panel.qml` file

- `GlobalSettingsPanel(wxWindow* parent)`
- `bool GetReplaceGround() const`
- `void SetReplaceGround(bool)`

---

## Phase 2: Print Section Widget

### 2.1 PrintSectionPanel (`wxPanel`)

Reusable .PRINT section shared across all analysis types. Customized via constructor parameters.

**Reference**: Print section in `src/kicad_xyce_plugin/simulation_parameters/op_panel.py` (lines 96-116),
`src/kicad_xyce_plugin/simulation_parameters/dc_panel.py` (lines 208-227),
`src/kicad_xyce_plugin/simulation_parameters/tran_panel.py` (lines 207-227)

```
PrintSectionPanel(wxWindow* parent,
                  const wxString& analysis_prefix,    // e.g. "TRAN", "DC", "AC"
                  std::vector<wxString> print_types,  // combo model, e.g. {"TRAN", "TRANADJOINT"}
                  bool show_power,                    // show P(*) checkbox
                  bool show_bjt_fet,                  // show BJT/FET lead checkboxes
                  bool show_print_type_combo          // show print type selector)
```

- Enable/disable checkbox
- Print type combo (conditional)
- Wildcard checkboxes: V(*), I(*), P(*), BJT leads, FET leads
- "Additional variables" text field
- Format combo + Output file field
- `BuildPrintParameters() -> std::optional<PrintParameters>`
- `Apply(const PrintParameters* params, bool has_bjt, bool has_fet)`

---

## Phase 3: Sensitivity Section

### 3.1 SensitivitySectionPanel (`wxPanel`)

Embedded .SENS section shared by AC, DC, and TRAN panels. Contains its own print subsection.

**Reference**: `src/kicad_xyce_plugin/simulation_parameters/sensitivity_section.py`
`src/kicad_xyce_plugin/simulation_parameters/sensitivity_section.qml`

- Objective mode combo (objfunc / objvars / acobjfunc)
- Objective values + parameters text fields
- Direct/Adjoint checkboxes
- Embedded `PrintSectionPanel` for .PRINT SENS
- `BuildSensParameter(const wxString& analysis_type) -> std::optional<SensParameter>`
- `Apply(const SensParameter* params)`

---

## Phase 4: Analysis Panels (7 wxPanel subclasses)

Each panel follows the same pattern:

- **Constructor**: Creates all child controls inside `wxBoxSizer` layouts
- **`Apply(const ParametersType* params, bool has_bjt = false, bool has_fet = false)`**:
  Populates controls from model (or sets defaults when `params` is null). Clears error text.
- **`HandleSubmit(...) -> std::optional<ParametersType>`**:
  Reads controls, validates, returns constructed model or `std::nullopt` on failure.
  On failure, sets error text on a shared error label.

### 4.1 OpPanel

**Reference**: `src/kicad_xyce_plugin/simulation_parameters/op_panel.py`
`src/kicad_xyce_plugin/simulation_parameters/op_panel.qml`

- PrintSectionPanel (DC type, all wildcards, BJT/FET)
- .SAVE section: enable checkbox, IC/NODESET radio buttons, file path
- .NODESET text field
- .IC / .DCVOLT text field
- GlobalSettingsPanel
- `HandleSubmit(...) -> OpSimulationParameters`

### 4.2 TranPanel

**Reference**: `src/kicad_xyce_plugin/simulation_parameters/tran_panel.py`
`src/kicad_xyce_plugin/simulation_parameters/tran_panel.qml`

- .TRAN fields: initial step, final time, start time, step ceiling
- Operating point combo (Default / NOOP / UIC)
- Schedule enable + schedule pairs text area
- .FFT + .FOUR + .MEASURE text areas (monospace, multiline)
- PrintSectionPanel (TRAN/TRANADJOINT types, power, BJT/FET)
- SensitivitySectionPanel
- GlobalSettingsPanel
- `HandleSubmit(...) -> std::optional<TransientSimulationParameters>`

### 4.3 DcPanel

**Reference**: `src/kicad_xyce_plugin/simulation_parameters/dc_panel.py`
`src/kicad_xyce_plugin/simulation_parameters/dc_panel.qml`

- Sweep mode combo (LIN/DEC/OCT/LIST/DATA)
- Primary variable + range fields (start/stop/step or points, conditional on mode)
- LIST values text area (visible in LIST mode)
- DATA table name + "Edit Table..." button (visible in DATA mode)
- Secondary sweep section (nested, conditional on mode, same field pattern)
- .MEASURE text area
- PrintSectionPanel (DC/HOMOTOPY types, power, BJT/FET)
- SensitivitySectionPanel
- GlobalSettingsPanel
- `HandleSubmit(...) -> std::optional<DCSimulationParameters>`

### 4.4 AcPanel

**Reference**: `src/kicad_xyce_plugin/simulation_parameters/ac_panel.py`
`src/kicad_xyce_plugin/simulation_parameters/ac_panel.qml`

- Sweep mode combo (LIN/DEC/OCT/DATA)
- Sweep fields: points, start, end frequency (or data table name for DATA)
- .MEASURE text area
- PrintSectionPanel (AC/AC_IC types, no power, no BJT/FET)
- SensitivitySectionPanel
- GlobalSettingsPanel
- `HandleSubmit(...) -> std::optional<AcSimulationParameters>`

### 4.5 NoisePanel

**Reference**: `src/kicad_xyce_plugin/simulation_parameters/noise_panel.py`
`src/kicad_xyce_plugin/simulation_parameters/noise_panel.qml`

- Output node, reference node, input source text fields
- Sweep mode combo + sweep fields (same as AC)
- Device noise operators: dynamic list of rows (device name text + DNI/DNO combo + optional source text + remove button) with "Add Device Operator" button
- .MEASURE text area
- PrintSectionPanel (NOISE type, includes INOISE/ONOISE checkboxes)
- GlobalSettingsPanel
- `HandleSubmit(...) -> std::optional<NoiseSimulationParameters>`

### 4.6 HbPanel

**Reference**: `src/kicad_xyce_plugin/simulation_parameters/hb_panel.py`
`src/kicad_xyce_plugin/simulation_parameters/hb_panel.qml`

- Frequencies text field (space-separated)
- Harmonics text field (space-separated integers)
- TAHB combo (Off/Transient/DC)
- SELECTHARMS combo (Hybrid/Box/Diamond)
- Startup periods text field (integer validator)
- Nonlinear solver options text field (key=value pairs)
- Linear solver options text field (key=value pairs)
- PrintSectionPanel (HB/HB_FD/HB_TD types, no power, no BJT/FET)
- GlobalSettingsPanel
- `HandleSubmit(...) -> std::optional<HbSimulationParameters>`

### 4.7 LinPanel

**Reference**: `src/kicad_xyce_plugin/simulation_parameters/lin_panel.py`
`src/kicad_xyce_plugin/simulation_parameters/lin_panel.qml`

- SPARCALC enable checkbox
- Format combo (TOUCHSTONE2/TOUCHSTONE), Parameter type combo (S/Y/Z), Data format combo (RI/MA/DB)
- Output file, Width, Precision text fields
- Sweep mode combo + sweep fields (same as AC)
- PrintSectionPanel (LIN type, no power, no BJT/FET)
- GlobalSettingsPanel
- `HandleSubmit(...) -> std::optional<LinSimulationParameters>`

---

## Phase 5: Main Dialog

### 5.1 SimulationParametersDialog (`wxDialog`)

**Reference**: `src/kicad_xyce_plugin/simulation_parameters/simulation_parameters_dialog.py`
`src/kicad_xyce_plugin/simulation_parameters/simulation_parameters_dialog.qml`

- **Layout**: `wxBoxSizer` horizontal split
  - **Left sidebar** (`wxPanel`): 7 buttons styled as sidebar items (.OP, .TRAN, .DC, .AC, .NOISE, .HB, .LIN) with badge + label, highlights active selection
  - **Right content area** (`wxSimplebook`): 7 pages, one per analysis panel
- **Footer** (`wxPanel`):
  - Directive preview text (monospace, read-only)
  - Cancel/Apply buttons
  - Error text label (uses `wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT)` — no hardcoded red)
- **Step parameters section**: Shared across all tabs, positioned below the simplebook or in footer area. Fields: enable checkbox, sweep mode, variable, range (conditional on mode), list values, data table name.
- **Data blocks list**: Stored as member `std::vector<DataBlock>`, edited via `DataTableDialog`
- **Ownership**: owns all 7 panels + step section + sensitivity sections
- **`GetConfig() -> SimulationConfig`**: Assembles result from current panel + step + data blocks
- **Signal wiring**: No Qt signals — direct method calls. Panel `HandleSubmit()` called on Apply, dialog reads result or shows validation error.
- **Integration**: Instantiated from `MainWindow::on_configure_simulation()` with current `SimulationConfig` and `NetlistTopology`. Returns updated config on accept.

---

## Phase 6: Data Table Editor

### 6.1 DataTableDialog (`wxDialog`)

**Reference**: `src/kicad_xyce_plugin/simulation_parameters/data_table_dialog.py`
`src/kicad_xyce_plugin/simulation_parameters/data_table_dialog.qml`

- Table name text field
- Column management: dynamic list of column name text fields + "Remove" buttons, "+ Add Column" button
- Row grid: dynamic list of rows, each row has a text field per column + "Remove" button, "+ Add Row" button
- Validation (client-side):
  - Table name: required, legal identifier, not reserved, unique
  - Columns: at least 1, legal identifiers, unique
  - Rows: at least 1, one value per column, all numeric (accepts SPICE suffixes)
- Cancel/OK buttons
- `GetDataBlock() -> DataBlock`

---

## Phase 7: Integration

1. Wire `SimulationParametersDialog` into `MainWindow::OnConfigureSimulation()`
2. The CMakeLists.txt already globs `src/ui/*.cpp` — no build system changes needed for new files
3. Remove Python/QML import once migration is verified and stable

---

## Design Guidelines

- **RAII**: All controls parented to wxWindow tree; no manual `delete`
- **No raw owning pointers**: Use parent-child ownership model
- **`std::optional`** for nullable model parameters (same as existing C++ models)
- **Validation pattern**: `HandleSubmit()` validates, returns `std::nullopt` on failure, sets error label text. Dialog checks return before assembling result.
- **Layout style**: `wxBoxSizer` + `wxFlexGridSizer` for form grids. Map the QML grid structure directly.
- **Monospace text areas**: `.FFT`, `.FOUR`, `.MEASURE`, schedule, list values use `wxTextCtrl` with `wxTE_MULTILINE` and `wxFont(wxFontInfo().Family(wxFONTFAMILY_TELETYPE))`.
- **Dynamic list controls**: Noise device operators use `wxScrolledWindow` with add/remove buttons (rebuilt on each mutation, small dataset).
- **No hardcoded colors, fonts, or dimensions**: Always use `wxSystemSettings` defaults. No `.qml` styling (rounded corners, gray palettes, accent colors) should be replicated.

---

## Files to Create

All under `src/ui/simulation_parameters/`:

| File | Content |
|---|---|
| `simulation_card.h` / `.cpp` | Bordered card container widget |
| `print_section_panel.h` / `.cpp` | Reusable .PRINT section |
| `sensitivity_section_panel.h` / `.cpp` | .SENS section with embedded print |
| `op_panel.h` / `.cpp` | OP analysis panel |
| `tran_panel.h` / `.cpp` | TRAN analysis panel |
| `dc_panel.h` / `.cpp` | DC sweep panel |
| `ac_panel.h` / `.cpp` | AC analysis panel |
| `noise_panel.h` / `.cpp` | NOISE analysis panel |
| `hb_panel.h` / `.cpp` | HB analysis panel |
| `lin_panel.h` / `.cpp` | LIN analysis panel |
| `simulation_parameters_dialog.h` / `.cpp` | Main dialog shell |
| `data_table_dialog.h` / `.cpp` | DATA table editor |

The existing CMakeLists.txt glob (`src/ui/*.cpp`) captures any `.cpp` under `src/ui/`,
so no build system changes are required.
