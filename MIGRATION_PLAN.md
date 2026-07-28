# Migration Plan: Python/QML UI → C++ wxWidgets

## Important

Read the `AGENTS.md` document and the `STYLE-GUIDE.md` document, the instructions in these documents are mandatory.

## Status Legend

| Marker | Meaning |
|---|---|
| ✅ **COMPLETE** | Implementation exists, tests exist, builds |
| ⚠️ **PARTIAL** | Implementation exists but has gaps vs. plan spec |
| 🏗 **IN PROGRESS** | File exists but is incomplete (e.g. empty .cpp) |
| ❌ **NOT STARTED** | No implementation exists |
| 🔲 **BLOCKED** | Blocked by a missing prerequisite |

## Overview

Migrate the simulation parameters UI from Python (PySide6/QtQuick/QML) to C++ (wxWidgets).
The data model layer (`src/simulation_parameters/`) is already migrated; this plan covers only
the UI layer, which consumes those C++ model types.

| Current (Python) | Target (C++) | Status |
|---|---|---|
| `src/kicad_xyce_plugin/simulation_parameters/` (Python panels + QML) | `src/ui/simulation_parameters/` (wxWidgets panels) | ✅ |
| SimulationCard.qml | `SimulationCard` — reusable bordered card | ✅ |
| Print sections per panel | `PrintSectionPanel` — reusable .PRINT section widget | ✅ |
| SensitivitySection.qml + .py | `SensitivitySectionPanel` — .SENS panel with embedded print | ✅ |
| 7 `*_panel.py` + 7 `*_panel.qml` files | 7 `wxPanel` subclasses | ✅ |
| `simulation_parameters_dialog.py` + `.qml` | `SimulationParametersDialog` (wxDialog) | ✅ |
| `data_table_dialog.py` + `.qml` | `DataTableDialog` (wxDialog) | ❌ |

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

## Phase 1: Shared Infrastructure ✅ COMPLETE

### 1.1 SimulationCard (`wxPanel`) ✅

**Files**: `src/ui/simulation_parameters/simulation_card.h`, `simulation_card.cpp`
**Tests**: `tests/ui/simulation_parameters/simulation_card.test.cpp`

A bordered card container with a header row (title + optional badge) and a body area.
Used by every analysis panel to group related controls.

- `SimulationCard(wxWindow* parent, const wxString& title, const wxString& badge = "")`
- Internally uses `wxStaticBox` as the card border (native styling)
- Header uses `wxStaticText` with default bold font
- `get_content()` returns the content `wxPanel*` so callers can add children

### 1.2 GlobalSettingsPanel ✅

**Files**: `src/ui/simulation_parameters/global_settings_panel.h`, `global_settings_panel.cpp`
**Tests**: `tests/ui/simulation_parameters/global_settings_panel.test.cpp`

A tiny panel with the "Replace ground (GND) with 0" checkbox and an explanatory label.
Shared across all 7 analysis panels.

- `GlobalSettingsPanel(wxWindow* parent)`
- `bool get_replace_ground() const`
- `void set_replace_ground(bool)`

---

## Phase 2: Print Section Widget ✅ COMPLETE

### 2.1 PrintSectionPanel (`wxPanel`) ✅

**Files**: `src/ui/simulation_parameters/print_section_panel.h`, `print_section_panel.cpp`
**Tests**: `tests/ui/simulation_parameters/print_section_panel.test.cpp`

Reusable .PRINT section shared across all analysis types. Customized via constructor parameters.

```
PrintSectionPanel(wxWindow* parent,
                  const wxString& analysis_prefix,
                  std::vector<wxString> print_types,
                  bool show_power,
                  bool show_bjt_fet,
                  bool show_print_type_combo)
```

- Enable/disable checkbox
- Print type combo (conditional)
- Wildcard checkboxes: V(*), I(*), P(*), BJT leads, FET leads
- "Additional variables" text field
- Format combo + Output file field
- `build_print_parameters() -> std::optional<PrintParameters>`
- `apply(const PrintParameters* params, bool has_bjt, bool has_fet)`

---

## Phase 3: Sensitivity Section ✅ COMPLETE

### 3.1 SensitivitySectionPanel (`wxPanel`) ✅

**Files**: `src/ui/simulation_parameters/sensitivity_section_panel.h`, `sensitivity_section_panel.cpp`
**Tests**: `tests/ui/simulation_parameters/sensitivity_section_panel.test.cpp` (15 tests)

Embedded .SENS section shared by AC, DC, and TRAN panels. Contains its own print subsection.

**Reference**: `src/kicad_xyce_plugin/simulation_parameters/sensitivity_section.py`
`src/kicad_xyce_plugin/simulation_parameters/sensitivity_section.qml`

- Objective mode combo (objfunc / objvars / acobjfunc)
- Objective values + parameters text fields
- Direct/Adjoint checkboxes
- Embedded `PrintSectionPanel` for .PRINT SENS
- `build_sens_parameter(const wxString& analysis_type) -> std::optional<SensParameter>`
- `apply(const SensParameter* params)`

**Dependency**: The C++ `SensParameter` model type in `src/simulation_parameters/sens_parameter.h`
needs verification — the Python version depends on `NetlistTopology` for topology-aware
serialization, and the C++ version may need the same.

**Integration**: Ready to be embedded into AC, DC, and TRAN panels.

---

## Phase 4: Analysis Panels (7 wxPanel subclasses) ✅ COMPLETE

Each panel follows the same pattern:

- **Constructor**: Creates all child controls inside `wxBoxSizer` layouts
- **`apply(const ParametersType& params)`**: Populates controls from model
- **`build_*_parameters() -> ParametersType`**: Reads controls, returns constructed model

> **Note**: The actual implementation uses `build_*_parameters()` / `apply()` (lowercase),
> not `HandleSubmit()` / `Apply()` as originally specified. All panels skip validation
> (no `std::nullopt` return) and directly return a populated model with empty strings for
> missing values. Error labels are managed by the dialog, not individual panels.

### 4.1 OpPanel ✅

**Files**: `src/ui/simulation_parameters/op_parameters_panel.h`, `op_parameters_panel.cpp`
**Tests**: `tests/ui/simulation_parameters/op_parameters_panel.test.cpp`

- PrintSectionPanel (DC type, all wildcards, BJT/FET) ✅
- .SAVE section: enable checkbox, IC/NODESET radio buttons, file path ✅
- .NODESET text field ✅
- .IC / .DCVOLT text field ✅
- GlobalSettingsPanel ✅
- `build_op_parameters() -> OpSimulationParameters` ✅

### 4.2 TranPanel ✅

**Files**: `src/ui/simulation_parameters/transient_parameters_panel.h`, `transient_parameters_panel.cpp`
**Tests**: `tests/ui/simulation_parameters/transient_parameters_panel.test.cpp`

| Feature | Status | Notes |
|---|---|---|
| .TRAN fields (initial step, final time, start time, step ceiling) | ✅ | |
| Operating point combo (Default / NOOP / UIC) | ✅ | |
| Schedule points text area | ✅ | |
| .FFT text area | ✅ | Multi-line, one directive per line |
| .FOUR text area | ✅ | Multi-line, one directive per line |
| .MEASURE text area | ✅ | Multi-line, one directive per line |
| PrintSectionPanel (TRAN/TRANADJOINT, power, BJT/FET) | ✅ | |
| SensitivitySectionPanel | ✅ | Ready for integration |
| GlobalSettingsPanel | ✅ | |
| `build_transient_parameters() -> TransientSimulationParameters` | ✅ | |

### 4.3 DcPanel ✅

**Files**: `src/ui/simulation_parameters/dc_parameters_panel.h`, `dc_parameters_panel.cpp`
**Tests**: `tests/ui/simulation_parameters/dc_parameters_panel.test.cpp`

| Feature | Status | Notes |
|---|---|---|
| Sweep mode combo (LIN/DEC/OCT/LIST/DATA) | ✅ | |
| Primary variable + range fields | ✅ | |
| LIST values text area | ✅ | |
| DATA table name + "Edit Table..." button | ⚠️ | Text field exists, but button is non-functional without DataTableDialog |
| Secondary sweep section | ✅ | |
| .MEASURE text area | ✅ | Multi-line, one directive per line |
| PrintSectionPanel (DC/HOMOTOPY, power, BJT/FET) | ✅ | |
| SensitivitySectionPanel | ✅ | Ready for integration |
| GlobalSettingsPanel | ✅ | |
| `build_dc_parameters() -> DCSimulationParameters` | ✅ | |

### 4.4 AcPanel ✅

**Files**: `src/ui/simulation_parameters/ac_parameters_panel.h`, `ac_parameters_panel.cpp`
**Tests**: `tests/ui/simulation_parameters/ac_parameters_panel.test.cpp`

| Feature | Status | Notes |
|---|---|---|
| Sweep mode combo (LIN/DEC/OCT/DATA) | ✅ | |
| Sweep fields (points, start, end, data table) | ✅ | |
| .MEASURE text area | ✅ | Multi-line, one directive per line |
| PrintSectionPanel (AC/AC_IC, no power, no BJT/FET) | ✅ | |
| SensitivitySectionPanel | ✅ | Ready for integration |
| GlobalSettingsPanel | ✅ | |
| `build_ac_parameters() -> AcSimulationParameters` | ✅ | |

### 4.5 NoisePanel ✅

**Files**: `src/ui/simulation_parameters/noise_parameters_panel.h`, `noise_parameters_panel.cpp`
**Tests**: `tests/ui/simulation_parameters/noise_parameters_panel.test.cpp`

| Feature | Status | Notes |
|---|---|---|
| Output node, reference node, input source text fields | ✅ | |
| Sweep mode combo + sweep fields | ✅ | |
| Device noise operators | ✅ | Text-area based (one-per-line) |
| PrintSectionPanel (NOISE type, INOISE/ONOISE) | ✅ | |
| GlobalSettingsPanel | ✅ | |
| `build_noise_parameters() -> NoiseSimulationParameters` | ✅ | |

### 4.6 HbPanel ✅

**Files**: `src/ui/simulation_parameters/hb_parameters_panel.h`, `hb_parameters_panel.cpp`
**Tests**: `tests/ui/simulation_parameters/hb_parameters_panel.test.cpp`

| Feature | Status | Notes |
|---|---|---|
| Frequencies text field | ✅ | |
| Harmonics text field | ✅ | |
| TAHB combo | ✅ | (None), 0 (off), 1 (auto), 2, 5, 10, 20 |
| SELECTHARMS combo | ✅ | (None), ALL, 1, 2, 3, 5, 10 |
| Startup periods text field | ✅ | |
| Nonlinear solver options text area | ✅ | |
| Linear solver options text area | ✅ | |
| PrintSectionPanel (HB/HB_FD/HB_TD) | ✅ | No power, no BJT/FET |
| GlobalSettingsPanel | ✅ | |
| `build_hb_parameters() -> HbSimulationParameters` | ✅ | |

### 4.7 LinPanel ✅

**Files**: `src/ui/simulation_parameters/lin_parameters_panel.h`, `lin_parameters_panel.cpp`
**Tests**: `tests/ui/simulation_parameters/lin_parameters_panel.test.cpp`

- SPARCALC enable checkbox ✅
- Format combo (TOUCHSTONE2/TOUCHSTONE), Parameter type combo (S/Y/Z), Data format combo (RI/MA/DB) ✅
- Output file, Width, Precision text fields ✅
- Sweep mode combo + sweep fields (same as AC) ✅
- PrintSectionPanel (LIN type, no power, no BJT/FET) ✅
- GlobalSettingsPanel ✅
- `build_lin_parameters() -> LinSimulationParameters` ✅

---

## Phase 5: Main Dialog ✅ COMPLETE

### 5.1 SimulationParametersDialog (`wxDialog`) ✅

**Files**: `src/ui/simulation_parameters/simulation_parameters_dialog.h` ✅
`src/ui/simulation_parameters/simulation_parameters_dialog.cpp` ✅ (487 lines)
**Tests**: No C++ tests exist

**Reference**: `src/kicad_xyce_plugin/simulation_parameters/simulation_parameters_dialog.py`

**Implementation**:
- Constructor builds full layout: sidebar (7 toggle buttons) + wxSimplebook (7 analysis panels) + sensitivity section (shown for TRAN/DC/AC) + step parameters section (.STEP) + preview text area + error label + Apply/Cancel buttons
- Sidebar buttons navigate the simplebook; `on_page_changed` manages button highlighting, sensitivity visibility, and preview
- `apply_config` resets panels to defaults then applies the matching analysis type from the variant, including sensitivity for TRAN/DC/AC
- `get_config` / `build_preview_config` reads the active panel's parameters + sensitivity (for relevant types) + step parameters + data blocks, assembles a `SimulationConfig`
- `update_preview` serializes the current configuration to Xyce directives in real-time
- Step parameters are managed via dedicated helper methods

**Integration**: Ready to be wired in `MainWindow::on_configure_simulation` (Phase 7)
- Data blocks integration with `DataTableDialog` (blocked on Phase 6)

**Constructor signature** (from header): `SimulationParametersDialog(wxWindow* parent, const SimulationConfig& config)`
**Plan spec** says it should also accept `NetlistTopology` — header needs updating if this is required.

---

## Phase 6: Data Table Editor ❌ NOT STARTED

### 6.1 DataTableDialog (`wxDialog`) ❌

**No implementation exists.**

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
- `get_data_block() -> DataBlock`

**Note**: The existing `DataBlock` C++ model type (`src/simulation_parameters/data_block.h`)
uses `std::vector<std::string>` for column parameters and
`std::vector<std::vector<std::string>>` for row records.

---

## Phase 7: Integration ✅

1. **`MainWindow::on_configure_simulation`** — Implemented:
   - Parses current netlist editor content via `SimulationConfig::from_xyce_directives()` for initial config
   - Instantiates `SimulationParametersDialog` modally with current config
   - On accept, stores returned `SimulationConfig` in `m_simulation_config` member
   - "Configure Simulation" menu item added to Tools menu (`Ctrl-Shift-S`) bound to same handler as toolbar button
   - `m_simulation_config` initialized with an empty config in constructor initializer list

2. **C++ `NetlistTopology` type** — Still missing (only a Python `@dataclass` at
   `src/kicad_xyce_plugin/netlist_parser.py`). The C++ dialog constructor does not
   require it (`SimulationParametersDialog(wxWindow*, const SimulationConfig&)`),
   so integration works without it. Topology-aware serialization can be added later.

3. The CMakeLists.txt already globs `src/ui/*.cpp` — no build system changes needed for new files.

4. Remove Python/QML import once migration is verified and stable.

---

## Design Guidelines

- **RAII**: All controls parented to wxWindow tree; no manual `delete`
- **No raw owning pointers**: Use parent-child ownership model
- **`std::optional`** for nullable model parameters (same as existing C++ models)
- **Validation pattern**: Return model directly with default/empty values for invalid fields.
  Dialog-level validation can check and show error text before assembling the final config.
- **Layout style**: `wxBoxSizer` + `wxFlexGridSizer` for form grids. Map the QML grid structure directly.
- **Monospace text areas**: `.FFT`, `.FOUR`, `.MEASURE`, schedule, list values use `wxTextCtrl` with `wxTE_MULTILINE` and `wxFont(wxFontInfo().Family(wxFONTFAMILY_TELETYPE))`.
- **Dynamic list controls**: Noise device operators use `wxScrolledWindow` with add/remove buttons (rebuilt on each mutation, small dataset).
- **No hardcoded colors, fonts, or dimensions**: Always use `wxSystemSettings` defaults. No `.qml` styling (rounded corners, gray palettes, accent colors) should be replicated.

---

## Files Status

All under `src/ui/simulation_parameters/`:

| File | Content | Status |
|---|---|---|
| `simulation_card.h` / `.cpp` | Bordered card container widget | ✅ |
| `print_section_panel.h` / `.cpp` | Reusable .PRINT section | ✅ |
| `global_settings_panel.h` / `.cpp` | Replace-ground checkbox panel | ✅ |
| `sensitivity_section_panel.h` / `.cpp` | .SENS section with embedded print | ✅ |
| `op_parameters_panel.h` / `.cpp` | OP analysis panel | ✅ |
| `transient_parameters_panel.h` / `.cpp` | TRAN analysis panel | ✅ |
| `dc_parameters_panel.h` / `.cpp` | DC sweep panel | ✅ |
| `ac_parameters_panel.h` / `.cpp` | AC analysis panel | ✅ |
| `noise_parameters_panel.h` / `.cpp` | NOISE analysis panel | ✅ |
| `hb_parameters_panel.h` / `.cpp` | HB analysis panel | ✅ |
| `lin_parameters_panel.h` / `.cpp` | LIN analysis panel | ✅ |
| `simulation_parameters_dialog.h` / `.cpp` | Main dialog shell | ✅ |
| `data_table_dialog.h` / `.cpp` | DATA table editor | ❌ **NEEDS CREATION** |

**Also needed** (not in original file table):

| File | Content | Status |
|---|---|---|
| `src/simulation_parameters/netlist_topology.h` / `.cpp` | C++ `NetlistTopology` type | ❌ **NEEDS CREATION** (currently Python-only) |

The existing CMakeLists.txt glob (`src/ui/*.cpp` and `src/simulation_parameters/*.cpp`)
captures any `.cpp` added, so no build system changes are required for new files.

## Test Coverage Summary

| Component | C++ Tests | Status |
|---|---|---|
| SimulationCard | `tests/ui/simulation_parameters/simulation_card.test.cpp` | ✅ |
| GlobalSettingsPanel | `tests/ui/simulation_parameters/global_settings_panel.test.cpp` | ✅ |
| PrintSectionPanel | `tests/ui/simulation_parameters/print_section_panel.test.cpp` | ✅ |
| SensitivitySectionPanel | `tests/ui/simulation_parameters/sensitivity_section_panel.test.cpp` | ✅ |
| OpParametersPanel | `tests/ui/simulation_parameters/op_parameters_panel.test.cpp` | ✅ |
| TransientParametersPanel | `tests/ui/simulation_parameters/transient_parameters_panel.test.cpp` | ✅ |
| DcParametersPanel | `tests/ui/simulation_parameters/dc_parameters_panel.test.cpp` | ✅ |
| AcParametersPanel | `tests/ui/simulation_parameters/ac_parameters_panel.test.cpp` | ✅ |
| NoiseParametersPanel | `tests/ui/simulation_parameters/noise_parameters_panel.test.cpp` | ✅ |
| HbParametersPanel | `tests/ui/simulation_parameters/hb_parameters_panel.test.cpp` | ✅ |
| LinParametersPanel | `tests/ui/simulation_parameters/lin_parameters_panel.test.cpp` | ✅ |
| SimulationParametersDialog | — | ❌ |
| DataTableDialog | — | ❌ |
| SimulationConfig (C++ model) | — | ❌ (only Python tests exist) |