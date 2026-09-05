# Simulation History Feature

## Overview
Add a collapsible simulation history browser that stores copies of generated files (`*.raw`, `*.fft`, `*.log`) under each netlist's directory in `.kicad-xyce-history/`.

## Constraints
- Reuses existing KiCad project directory (no new concept)
- Files are COPIED to history after Xyce finishes (not moved)
- Requires write access to netlist directory
- Opt-in via Plugin Config

## Done

### 1. Plugin Config
- [x] Add `simulation-history-enabled` (bool, default false)
- [x] Add `simulation-history-max-runs` (int, default 20)
- [x] Add config dialog UI for both fields
- [x] Add schema validation in config loading/saving

### 2. History Data Model
- [x] Define `SimulationHistoryFile` and `SimulationHistoryRun` structs
- [x] Expose to Slint view as model

### UI — History Tree Component
- [x] New `simulation_history_tree.slint` in `src/ui/components/`
- [x] Collapsible tree view (run folders → files)
- [x] File rows with inline `[Open]` button
- [x] Toolbar icon with "H" overlay on simulator icon
- [x] Toolbar toggle button (show/hide history pane)
- [x] Split view layout (~240px width)

### Integration — UI Wiring
- [x] Wire history callbacks to presenter:
  - Toolbar toggle → toggles history visibility
  - Click file → raw loads into charts, logs into output panel, FFT in a window
- [x] Add `history-file-selected(timestamp, file)` callback
- [x] Add `toggle-history-visibility()` callback
- [x] Ensure existing workflows unchanged when feature disabled

### History Folder Management
- [x] Determine netlist directory (from netlist source working directory)
- [x] Check write access to `.kicad-xyce-history/` folder
- [x] Create timestamped subfolder: `<YYYY-MM-DD_HH-MM-SS>/` (UTC, `_N` suffix on collision)
- [x] Copy generated files after successful simulation (`*.raw`, `*.fft*`, `*.out`)
- [x] Error handling for copy failures (reported in output panel)
- [x] Prune old runs (keep last N, configurable via max-runs)

### History Data Model (Runtime)
- [x] Scan `.kicad-xyce-history/` for existing runs (newest first)
- [x] Parse each run folder for files (type label from extension)
- [x] Populate history model on startup, file open, config change, panel show, after runs

### Storage Service
- [x] `src/history/simulation_history_store.{h,cpp}`: record/scan/prune + file type labels

## TODO (Remaining)

### Testing
- [x] Unit tests for file type mapping, timestamp pattern, record + copy, failure paths, pruning, scanning (`tests/history/simulation_history_store.test.cpp`)
- [ ] Integration test for full copy-on-success flow (presenter level)

## Out of Scope (for later)
- Preview thumbnails of waveforms in tree rows
- Export/share individual run results
- Retroactive pruning on config change
- Import/export of run folders
