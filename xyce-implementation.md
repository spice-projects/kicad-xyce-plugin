# XYCE Implementation Review

This document summarizes the current implementation coverage of the simulation parameter models and UI editor support in `src/simulation_parameters` against the XYCE reference guide in `xyce-docs/Xyce_RG.txt`.

## Summary Table

| model | ui | SENS companion | reference guide page | issue | pass / fail | status |
|------|----|----------------|----------------------|-------|-------------|--------|
| AC | yes | no | 24–25, 128, 153 | 37 | fail | resolved |
| DC | yes | no | 27–30, 128, 153 | 37 | fail | resolved |
| TRAN | yes | no | 161–162, 128, 153 | 37 | fail | resolved |
| OP | yes | n/a | 95, 128 | 41 | pass | resolved |
| NOISE | yes | n/a | 93–94, 128 | 38 | pass | resolved |
| HB | no | n/a | 47, 116–117, 137 | — | fail | pending |
| LIN | yes | n/a | 52–53, 128 | — | pass | resolved |
| STEP | yes | n/a | 155, 128 | 7, 8, 36 | partial | pending |
| FFT | yes | n/a | 1130–1142, 128 | 38 | pass | resolved |
| FOUR | yes | n/a | 1400–1426, 128 | — | pass | resolved |

*Note: `.SENS` is an additive directive used alongside DC, AC, or transient analyses, not a standalone analysis type. Relevant open GitHub issues are referenced in the table for any line with known gaps or related enhancement work.*

## Current Open Issue Gaps

The following open GitHub issues represent the highest-priority work items aligned with the current XYCE implementation gaps:

- `#38`: `.MEASURE ..._CONT` continuous results and missing measure subtypes. **RESOLVED**: Correctly identifies and retains `.TRAN_CONT`, `.DC_CONT`, `.AC_CONT`, and `.NOISE_CONT` directives during import/restore.
- `#39`: Missing `.PRINT` subtypes such as `.PRINT ES`, `.PRINT PCE`, `.PRINT HOMOTOPY`, `.PRINT AC_IC`, and `.PRINT TRANADJOINT`. **RESOLVED**: Added UI and model support for common subtypes (`TRANADJOINT`, `AC_IC`, `HOMOTOPY`). Advanced UQ subtypes (`ES`, `PCE`, `SAMPLING`) are correctly identified and preserved during import/restore via a generic catch-all mechanism.
- `#36`: Analysis directives missing from the parser and UI, including `.STEP`, `.DATA`, `.SAMPLING`, `.EMBEDDEDSAMPLING`, `.PCE`, and related companion directives.
- `#7` / `#8`: `.STEP` parametric sweep and in-dialog `.DATA` table editor work remain open, indicating Step support is not yet fully complete.
- `#32`, `#33`, `#34`: Advanced UQ analysis support for `.SAMPLING`, `.EMBEDDEDSAMPLING`, and `.PCE` is currently incomplete.

## Recommended Next Work

1. Prioritize `#38` as the next task: fix `.MEASURE ..._CONT` handling and restore flow. This is the most direct implementation gap affecting correctness and is explicitly noted in the current review as still broken.
2. Follow with `#39` to add missing `.PRINT` subtypes, especially those required by uncertainty quantification and homotopy analyses.
3. Then address the parser/UI analysis directive gaps in `#36`, `#7`, and `#8` to complete `.STEP` / `.DATA` and general directive extraction support.
4. Finally, expand advanced UQ support with `#32`, `#33`, and `#34` once the core parser/model restoration gaps are fixed.

## Fail Descriptions

### OP
- Resolved: the `OpSimulationParameters` model and UI now support `.IC` / `.DCVOLT` initial-condition entries through the OP panel.
- The dialog now exposes an editor field for `.IC` / `.DCVOLT`, supporting full operating-point directive coverage.

### HB
- The `HbSimulationParameters` model correctly parses `.HB` and handles `.OPTIONS HBINT`, `.OPTIONS NONLIN-HB`, and `.OPTIONS LINSOL-HB`.
- The UI panel only exposes fundamental frequencies, harmonics, `TAHB`, `SELECTHARMS`, `STARTUPPERIODS`, and HB print options.
- There is no UI support for `.OPTIONS NONLIN-HB` or `.OPTIONS LINSOL-HB` configuration, so not all documented HB directive parameters are editable.

## Notes

- The report is based on direct inspection of the implementation in `src/simulation_parameters/` and the referenced XYCE guide sections.
- `AC`, `DC`, `TRAN`, `NOISE`, `LIN`, `STEP`, `FFT`, and `FOUR` are implemented with both model support and UI exposure for the documented directive fields.
- `.SENS` is an additive directive and should be treated as a companion to DC, AC, or TRAN analysis rather than a standalone simulation model.
- Current implementation still exposes `.SENS` in a separate Sensitivity tab, which means AC/DC/TRAN are not properly integrated with sensitivity support.
- `HB` remains an additional area with incomplete editor coverage relative to the documented syntax.
- Issue #41 is closed with UI support added for `.IC` / `.DCVOLT`.
- Issue #38 is resolved: the plugin now correctly preserves `.MEASURE ..._CONT` directives during restore.

