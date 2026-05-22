# Xyce Simulation Configuration Dialog — UI Specification

## Overview

A modal dialog (or panel) for configuring Xyce circuit simulator directives. The user selects a simulation type and all relevant parameters, output settings, and contextual add-ons are presented in a single unified view. The UI must feel native, flat, and clean — no gradients, no drop shadows, no decorative effects.

---

## Layout

The dialog uses a **two-column layout**:

- **Left column** — Simulation type selector (sidebar)
- **Right column** — Configuration panel (scrollable, divided into sections)

A **footer bar** spans the full width below both columns, containing a live directive preview and action buttons.

### Overall dimensions

- Minimum width: 680px
- Minimum height: 520px
- Border: 1px solid, light gray (`#E0E0E0` in light mode)
- Border radius: 12px
- Background: white
- Overflow: hidden on the outer shell (so sidebar and panel fill it cleanly)

---

## Left Column — Simulation Type Selector

### Dimensions

- Width: 180px fixed
- Full height of the dialog (minus the footer)
- Background: slightly off-white / light gray surface (one shade darker than the main panel)
- Right border: 1px solid light gray, same as outer border

### Header label

- Text: "Simulation type"
- Font size: 11px
- Color: muted gray (tertiary text)
- All-caps with slight letter-spacing (0.06em)
- Padding: 8px horizontal, 8px top, 4px bottom

### Simulation type buttons

One button per simulation type, listed vertically with 2px gaps. The list is:

1. `.OP` — Operating point
2. `.TRAN` — Transient
3. `.DC` — DC sweep
4. `.AC` — AC small-signal
5. `.NOISE` — Noise
6. `.HB` — Harmonic balance
7. `.LIN` — Linear network

Each button:
- Full width of the sidebar, left-aligned
- Padding: 8px 12px
- Border radius: 8px
- Background: transparent (default), white with a light gray border on hover and active
- Font size: 13px
- Color: muted gray (default), dark/primary text on hover and active
- Font weight: 400 default, 500 when active
- Contains two elements inline:
  1. A **tag badge** — the directive string (`.OP`, `.TRAN`, etc.) in monospace font, 11px, with a light blue background and blue text. When the button is active, the badge background becomes a solid blue (`#185FA5`) and text becomes white.
  2. A **text label** — human-readable name ("Operating point", "Transient", etc.)

**Important behavioral note:** `.SENS` is NOT in this list. Sensitivity analysis is a contextual add-on that appears inside certain simulation type panels, not a standalone simulation type.

Only one simulation type can be active at a time. Clicking a button immediately updates the right panel content.

---

## Right Column — Configuration Panel

### Dimensions

- Flexible width (fills remaining space)
- Vertically scrollable
- Padding: 20px on all sides
- Background: white

### Panel header

At the top of the panel, two lines:

1. **Title** — e.g. ".TRAN — Transient analysis", 16px, font weight 500
2. **Description** — short one-line description of what the simulation does, 12px, muted gray

These update immediately when a new simulation type is selected.

---

### Sections

The panel body consists of stacked **sections**. Each section is a card-like container:

- Border: 0.5px solid light gray
- Border radius: 8px
- Overflow: hidden
- No drop shadow

Each section has:
- A **section header** — 8px 12px padding, slightly off-white background (same surface color as the sidebar), bottom border 0.5px solid light gray, 12px font, font weight 500
- A **section body** — 12px padding, white background

The sections displayed vary by simulation type (detailed below).

---

## Section 1: Parameters

Always shown (except for `.OP`, which has no parameters and hides this section entirely).

### Section header

Text: "Parameters" — with a small muted badge on the right reading "required fields marked *"

### Section body

A **two-column grid** of labeled input fields. Each field:
- Label above, 12px, muted gray
- Text input below, 13px, full width of its grid cell
- Input border: 0.5px solid medium gray
- Border radius: 8px
- Padding: 5px 8px
- On focus: border color turns blue, with a subtle blue box shadow ring

Fields vary per simulation type:

**`.TRAN`**
- Initial step * (default: `1u`)
- Final time * (default: `1m`)
- Start time (default: `0`)
- Step ceiling (optional, placeholder text only)
- Operating point mode — full-width `<select>` dropdown, options: "Default (compute OP)", "Skip OP", "UIC — use initial conditions"

**`.DC`**
- Source / param * (e.g. `V1`)
- Start *
- Stop *
- Step *

**`.AC`**
- Sweep type * (text input, placeholder "DEC / LIN / OCT")
- Points / decade *
- Start frequency *
- Stop frequency *

**`.NOISE`**
- Output variable * (e.g. `V(out)`)
- Input source * (e.g. `V1`)
- Sweep type
- Points per decade

**`.HB`**
- Fundamental frequency *
- Number of harmonics

**`.LIN`**
- Sweep type *
- Points per decade
- Start frequency *
- Stop frequency *

---

### Adaptive Schedule sub-section (`.TRAN` only)

Appears below the parameter fields, separated by a thin horizontal divider line.

A **toggle row**: a checkbox on the left, label "Enable adaptive schedule (time, max_step pairs)" — 13px text. When unchecked (default), nothing else is visible.

When checked, a textarea appears below:
- Full width
- 2 rows tall, resizable vertically
- Monospace font, 12px
- Placeholder text: "0.5e-3,1e-3  1e-6,2e-3  0"
- Below the textarea: a hint line in 11px muted text reading "Space-separated pairs of time, max_step values"

---

## Section 2: Sensitivity Analysis (`.SENS` add-on)

Shown only for simulation types that support it: `.TRAN`, `.DC`, `.AC`.

This section is visually distinguished with a **2px solid blue left border** (accent line) on the section header, instead of the default style.

### Section header

- Left side: text ".SENS — Sensitivity analysis" with a small muted pill/badge reading "contextual add-on"
- Right side: a checkbox + label "Enable" — inline, right-aligned

When the checkbox is unchecked (default), the section body is hidden. When checked, the body expands.

### Section body (when enabled)

Two full-width fields (spanning both grid columns):

1. **Objective function(s)** — text input, placeholder `{V(out)} {I(R1)}`
2. **Parameters to differentiate** — text input, placeholder `R1:R C1:C`

---

## Section 3: Output (.PRINT)

Always shown for all simulation types.

### Section header

- Left side: text ".PRINT — Output"
- Right side: a checkbox + label "Enabled" — checked by default

When unchecked, the section body becomes visually dimmed (opacity ~40%) and non-interactive.

### Section body

**Capture sub-header** — small label "Capture" in 12px muted gray above the pill group.

**Pill group** — a row of toggleable pill buttons. Each pill:
- Border: 0.5px solid medium gray
- Border radius: 20px (fully rounded, pill shape)
- Padding: 4px 10px
- Font size: 12px
- Default (off) state: transparent background, muted text
- Active (on) state: light blue background, blue text, blue border

The pills available vary by simulation type:

- `.TRAN`, `.DC`, `.OP`: "V(\*) all voltages", "I(\*) all currents", "P(\*) power"
- `.AC`: "V(\*) magnitude", "V(\*) phase", "I(\*) magnitude"
- `.NOISE`: "ONOISE", "INOISE"
- `.HB`, `.LIN`: "V(\*) all voltages", "I(\*) all currents"

**Additional signals field** — below the pill group, a labeled text input:
- Label: "Additional signals", 12px muted
- Placeholder: "e.g. V(out) I(R1) — pre-filled from netlist"
- Full width

**Format and output file row** — a two-column grid:
- Left: dropdown labeled "Format" — options: RAW, CSV, PROBE, GNUPLOT
- Right: text input labeled "Output file" — placeholder "optional (e.g. output.raw)"

---

### FFT sub-section (`.TRAN` only, inside the .PRINT section)

Appears at the bottom of the .PRINT section body, separated by a thin horizontal divider.

A toggle row: checkbox + label "Include .FFT (Fast Fourier Transform)". Default: unchecked.

When checked, a two-column field grid expands below:
- Variable — text input, placeholder `V(out)`
- Fundamental frequency — text input, placeholder `1k`
- Number of harmonics — text input, placeholder `10`
- Window function — dropdown: HANN, RECT, BLACKMAN, HARRIS

---

## Section 4: Preprocessor

Always shown for all simulation types. Placed last.

### Section header

Text: "Preprocessor"

### Section body

A single toggle row: checkbox + label "Replace ground node (.PREPROCESS REPLACEGROUND TRUE)" — unchecked by default.

---

## Footer Bar

Spans the full width of the dialog, below both columns. Height approximately 48px.

- Top border: 0.5px solid light gray
- Background: white
- Padding: 12px 20px
- Horizontal layout with space-between alignment

### Left side — Live directive preview

A read-only text display showing the Xyce directive being constructed in real time, updating as the user changes parameter values.

- Background: slightly off-white/gray surface
- Padding: 4px 8px
- Border radius: 8px
- Font: monospace, 11px, muted gray text
- Max width: ~360px, text truncated with ellipsis if too long
- Example: `.TRAN 1u 1m`

The preview reflects the primary directive only (not .PRINT or .SENS). It updates live on any parameter field change.

### Right side — Action buttons

Two buttons, right-aligned, with an 8px gap:

1. **Cancel** — transparent background, 0.5px border, muted text, 13px
2. **Apply** — solid blue background (`#185FA5`), white text, 13px, font weight 500. Darkens on hover.

Both buttons: border radius 8px, padding 6px 16px.

---

## Interaction Behavior

### Switching simulation types

When the user clicks a different simulation type button in the sidebar:
- The active button state updates immediately
- The panel header title and description update
- The Parameters section fields update (labels, default values, placeholders) to match the new simulation type
- Fields not used by the new sim type are hidden
- The `.SENS` section is shown or hidden based on whether the selected type supports it; its checkbox resets to unchecked and body collapses
- The pill buttons in `.PRINT` are replaced with the correct set for the new sim type, all defaulting to the "on" state
- The `.FFT` toggle (if present) resets to unchecked and its body collapses
- The live directive preview updates

### `.SENS` toggle

Clicking the "Enable" checkbox in the .SENS section header shows/hides the section body with its input fields. The section header and its accent border are always visible (when the sim type supports it), regardless of the checkbox state.

### `.PRINT` enabled toggle

Unchecking "Enabled" in the .PRINT section header dims the entire section body and makes all its inputs non-interactive. Rechecking restores normal state.

### Pill toggles

Each pill button independently toggles between on and off states on click. At least visually — there is no enforced minimum; all can be off.

### FFT toggle

Checking/unchecking the .FFT checkbox shows/hides the four FFT parameter fields below it, with no animation required (instant show/hide is acceptable).

### Adaptive schedule toggle (`.TRAN` only)

Checking/unchecking shows/hides the textarea and its hint text below the checkbox row.

---

## Simulation Type Quick Reference Table

| Sim type | Parameters section | .SENS add-on | .FFT in .PRINT | Adaptive schedule |
|---|---|---|---|---|
| .OP | Hidden | No | No | No |
| .TRAN | Initial step, Final time, Start time, Step ceiling, OP mode | Yes | Yes | Yes |
| .DC | Source/param, Start, Stop, Step | Yes | No | No |
| .AC | Sweep type, Points/decade, Start freq, Stop freq | Yes | No | No |
| .NOISE | Output var, Input source, Sweep type, Points/decade | No | No | No |
| .HB | Fundamental freq, Number of harmonics | No | No | No |
| .LIN | Sweep type, Points/decade, Start freq, Stop freq | No | No | No |

---

## Typography

- Font family: system sans-serif (or the application's default UI font)
- Body / inputs: 13–14px, weight 400
- Labels: 12px, weight 400, muted color
- Section headers: 12px, weight 500
- Panel header title: 16px, weight 500
- Sidebar label: 11px, all-caps, letter-spacing 0.06em
- Directive badge in sidebar: 11px, monospace
- Live preview: 11px, monospace

---

## Color Reference (light mode)

| Role | Value |
|---|---|
| Background — main panel | `#FFFFFF` |
| Background — sidebar / section headers | `#F5F5F3` |
| Border — default | `#E0E0DC` (0.5px) |
| Border — emphasis | `#C0C0BB` (0.5px) |
| Text — primary | `#1A1A18` |
| Text — secondary / labels | `#6B6B66` |
| Text — tertiary / hints | `#9A9A94` |
| Accent blue — button bg, active badge, .SENS border | `#185FA5` |
| Accent blue — light (pill on-state bg, input focus ring) | `#E6F1FB` |
| Accent blue — mid (pill on-state border, input focus border) | `#378ADD` |
| Accent blue — text on light blue bg | `#185FA5` |

The dialog must also support a dark mode. In dark mode, surface colors invert to dark grays, borders become lighter semi-transparent, and text reverses. Blue accents remain similar. Implement using CSS custom properties / variables so the theme switches automatically with the system preference.
