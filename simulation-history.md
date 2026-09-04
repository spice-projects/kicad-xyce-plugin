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
  - Click file → logged (full implementation pending)
  - Toolbar toggle → toggles history visibility
- [x] Add `history-file-selected(timestamp, file)` callback
- [x] Add `toggle-history-visibility()` callback
- [x] Ensure existing workflows unchanged when feature disabled

## TODO (Remaining)

### History Folder Management
- [ ] Determine netlist directory (from KiCad project or loaded file)
- [ ] Check write access to `.kicad-xyce-history/` folder
- [ ] Create timestamped subfolder: `<YYYY-MM-DD_HH-MM-SS>/`
- [ ] Copy generated files after successful simulation
- [ ] Error handling for copy failures (show in output panel)
- [ ] Prune old runs (keep last N, configurable via max-runs)

### History Data Model (Runtime)
- [ ] Scan `.kicad-xyce-history/` for existing runs
- [ ] Parse each run folder for files
- [ ] Populate history model on startup / after successful runs

### Integration (Runtime)
- [ ] After successful simulation, copy output files to history
- [ ] Prune old runs after copying
- [ ] Load history runs when history panel is shown
- [ ] Click file → load selected file into charts (full implementation)

### Testing
- [ ] Write unit tests for:
  - Folder creation + write access check
  - File copying logic
  - Pruning logic (N oldest deleted)
  - History scanning/parsing
- [ ] Integration test for full copy-on-success flow

## Out of Scope (for later)
- Preview thumbnails of waveforms in tree rows
- Export/share individual run results
- Retroactive pruning on config change
- Import/export of run folders
