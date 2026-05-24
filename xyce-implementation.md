# XYCE Implementation Review

This document summarizes the current implementation coverage of the simulation parameter models and UI editor support in `src/simulation_parameters` against the XYCE reference guide in `xyce-docs/Xyce_RG.txt`.

## Summary Table

| model | ui | SENS companion | reference guide page | issue | pass / fail | status |
|------|----|----------------|----------------------|-------|-------------|--------|
| AC | yes | no | 24–25, 128, 153 | 37 | fail | resolved |
| DC | yes | no | 27–30, 128, 153 | 37 | fail | resolved |
| TRAN | yes | no | 161–162, 128, 153 | 37 | fail | resolved |
| OP | no | n/a | 95, 128 | — | fail | pending |
| NOISE | yes | n/a | 93–94, 128 | 38 | pass | resolved |
| HB | no | n/a | 47, 116–117, 137 | — | fail | pending |
| LIN | yes | n/a | 52–53, 128 | — | pass | resolved |
| STEP | yes | n/a | 155, 128 | 7, 8, 36 | pass | resolved |
| FFT | yes | n/a | 1130–1142, 128 | 38 | pass | resolved |
| FOUR | yes | n/a | 1400–1426, 128 | — | pass | resolved |

*Note: `.SENS` is an additive directive used alongside DC, AC, or transient analyses, not a standalone analysis type. Relevant open GitHub issues are referenced in the table for any line with known gaps or related enhancement work.*

## Fail Descriptions

### OP
- The `OpSimulationParameters` model supports `.OP`, `.PRINT DC`, `.SAVE`, `.NODESET`, and initial-condition directives (`.IC` / `.DCVOLT`).
- However, the UI panel only exposes print output options, save options, and nodeset entries.
- There is no editor field for entering `.IC` or `.DCVOLT` initial-condition entries, which means users cannot edit all required OP directive parameters from the dialog.

### HB
- The `HbSimulationParameters` model correctly parses `.HB` and handles `.OPTIONS HBINT`, `.OPTIONS NONLIN-HB`, and `.OPTIONS LINSOL-HB`.
- The UI panel only exposes fundamental frequencies, harmonics, `TAHB`, `SELECTHARMS`, `STARTUPPERIODS`, and HB print options.
- There is no UI support for `.OPTIONS NONLIN-HB` or `.OPTIONS LINSOL-HB` configuration, so not all documented HB directive parameters are editable.

## Notes

- The report is based on direct inspection of the implementation in `src/simulation_parameters/` and the referenced XYCE guide sections.
- `AC`, `DC`, `TRAN`, `NOISE`, `LIN`, `STEP`, `FFT`, and `FOUR` are implemented with both model support and UI exposure for the documented directive fields.
- `.SENS` is an additive directive and should be treated as a companion to DC, AC, or TRAN analysis rather than a standalone simulation model.
- Current implementation still exposes `.SENS` in a separate Sensitivity tab, which means AC/DC/TRAN are not properly integrated with sensitivity support.
- `OP` and `HB` are additional areas with incomplete editor coverage relative to the documented syntax.
- Issue `#38` remains open because the plugin still drops `.MEASURE ..._CONT` directives during restore, even though core `.MEASURE NOISE` and `.MEASURE FFT` parsing support exists.
