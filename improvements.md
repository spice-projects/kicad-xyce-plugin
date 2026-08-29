## Full Plan: Xyce Front-End — UI/Workflow Improvements

### Part 1 — Fixes and refinements to the existing model

**1.1 — Universal window titling from file/calculation metadata**
Problem: chart windows opened from output files show generic titles ("FFT analysis") — confirmed indistinguishable when two are open side by side.
Fix: derive window titles from data already available at open time, never from netlist/directive context (windows can be opened standalone with no netlist access):
- **FFT (Xyce-produced)**: parse header (`FFT analysis for I(L1)`, `Window: HANN`, `Start/Stop Freq`) → e.g. `FFT analysis — I(L1) (HANN, 50Hz–25.6kHz)`
- **FFT (client-calculated)**: already correct (`FFT - I(L1), P(C1)`) — confirmed working, no change needed
- **Touchstone**: parser already exposes port count, parameter type, frequency unit/reference impedance; frequency range must be derived from first/last rows of `[Network Data]` (no explicit range field in the format) → e.g. `S-Parameters — 2-port (1Hz–<last freq>Hz)`
- **RAW**: unconfirmed whether headers self-describe analysis type/range the same way — needs checking to fully close this out
- **Multi-pane windows**: concatenate signal names, as already demonstrated by the client-FFT case

**1.2 — Seed "New Window" / "Open Xyce FFT Calculation" with current state**
Problem: both mechanisms already accept a datasource + initial plot selection parameter, but currently always open empty, forcing reselection of signals the user already had visible.
Fix: pass current pane's plot selection (and split configuration, for multi-pane detach) into the new window at creation. Wiring an existing parameter, not new capability.

**1.3 — Removed.** Originally flagged as a dialog serialization bug; confirmed to be a manually hand-edited netlist error correctly caught by Xyce, not a defect.

**1.4 — Log panel default-open behavior**
Problem: every Run (success or failure) switches to Charts view with the Log panel shown by default; user must manually close it every time, even on success.
Proposed fix: show the Log panel by default only on failure/warning; on success, go straight to an unobstructed Charts view. Log stays available anytime via its toolbar toggle. *Not yet confirmed as real, repeated friction — validate before committing.*

**1.5 — Surface secondary outputs proactively**
Problem: FFT/Touchstone outputs are only reachable via right-click context menu — invisible to a user who doesn't already know to look.
Fix: after a successful run, show a small non-modal indicator (status bar/toolbar badge) listing available secondary outputs, clickable to open. Supplements, doesn't replace, the context menu.

**1.6 — Scale the plot-selection picker for large signal counts**
Confirmed: text filtering already works well (e.g., typing `I(D` narrows hundreds of signals instantly) — not a gap. Remaining gaps, confirmed with a real large-netlist screenshot:
- No category-based filtering (the Voltage/Current/Freq/Time/Power/Misc legend is a color key only, not clickable) — add as a filter, composable with text search
- No visible "currently selected" summary — items checked while filtered elsewhere aren't visible without clearing the filter and scrolling
- Long/subcircuit-qualified names (e.g. `P(XQ1:RCSDCO...)`) truncate with no way to see the full name

**1.7 — X-axis tick label precision bug at deep zoom**
Confirmed directly: consecutive X-axis labels can render identically (e.g. `48.8 ms` repeated across the whole axis) when tick spacing is finer than the display precision. Fix: dynamically increase label decimal precision when needed, or widen tick spacing so labels are always distinct. Confirmed *not* a general problem — normal zoom levels label correctly.

**1.8 — High-zoom-out rendering density for oscillating signals**
Confirmed directly and precisely scoped: when a dense/oscillating signal (e.g. switching-node current/power) is viewed at a zoom level where many samples map to one pixel column, it renders as a solid filled band rather than a legible envelope+trend — not a general large-dataset problem, since smooth/slow signals from the same size dataset render cleanly at the same zoom level. Recommended direction: standard large-dataset charting technique — render high-density regions as a min/max envelope *plus* a distinct mean/trend line, with reduced fill opacity, refining to full resolution as the user zooms in (already confirmed working correctly). Likely a bigger lift than other Part 1 items — a charting-engine change, not a small UI fix.

**Explicitly out of scope**: no warning/auto-split logic for mixed-signal or multi-unit panes — confirmed this is fully intentional user choice (e.g., the existing dB+phase dual-axis pairing is a deliberate feature), not a defect to guard against.

---

### Part 2 — Consistency

**2.1 — FFT phase output asymmetry**
Xyce-directive FFT automatically pairs magnitude + phase on dual axes; client-calculated FFT is magnitude-only (single-select "Output" dropdown). Open product decision: make "Output" multi-select to match, or confirm the asymmetry is intentional.

**2.2 — Expression consistency — confirmed working well, no action needed.**
User-defined expressions (e.g. `V(N1)+V(IN)`) are shared globally: defined once, available in every picker (Add/Remove Plots, FFT dialog), and carried forward into any window spawned from that data (New Window, detach, secondary outputs). A design strength worth preserving as-is.

---

### Part 3 — New feature: Smith chart / Touchstone visualization

Parser is complete (Touchstone v1/v2, all Xyce-supported elements) — this is now purely a visualization/UI task, built on already-proven patterns:
- Same window/pane framework: Add Chart, split panes, context menu (Zoom/Autorange, Add/Remove Plots, New Window)
- Titling per 1.1
- S-parameter selection via the same expression-grid picker, populated with S11/S21/S12/S22-style entries derived from port count
- Smith chart vs. rectangular (mag/phase) likely as a toggle within one pane (mirroring the existing dB/phase dual-axis pattern), not separate output types
- Frequency-sweep cursor/marker needs explicit design — the existing status-bar hover readout (built for rectangular X/Y charts) may not translate directly to a polar plot

---

### Part 4 — Open product decisions

| Question | Status |
|---|---|
| Does cross-run/cross-window comparison need dedicated support (overlay, run history), or does the existing multi-instance + detach model already cover it? | No direct evidence of pain; recommend validating with real usage before building |
| 1.4 — is closing the log panel every run real, repeated friction? | Not yet confirmed |
| 2.1 — should client FFT support multi-select Output (mag+phase)? | Genuine product call, not a bug |
| 1.1 (RAW) — do RAW file headers self-describe analysis type/range the way FFT/Touchstone do? | Unconfirmed — needed to fully close out 1.1 |

---

### Suggested sequencing
1. **1.1, 1.2, 1.5, 1.6, 1.7** — isolated, high-confidence, no architecture change
2. **1.4** — once confirmed as real friction
3. **2.1** — small decision, then small implementation
4. **1.8** — larger charting-engine effort, plan separately from the rest of Part 1
5. **Part 3** — Smith chart, next major feature, using proven patterns
6. **Part 4** — resolve via user feedback/data before investing engineering time