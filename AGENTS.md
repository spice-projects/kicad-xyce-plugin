# AGENTS.md

## Language

- C++23
- Use RAII
- Avoid raw owning pointers
- Prefer std::span/std::string_view for non-owning views
- Preserve existing architecture unless explicitly requested

## Build & Unit Tests

- Build system: `cmake`
  - Configure: `cmake --preset debug`
  - Build: `cmake --build --preset debug`
- Unit Tests, avoid running unit tests using `ctest`. Execute unit tests by executing the process: `./.build-debug/tests/kicad-xyce-plugin-tests`
- Unit tests must be self-contained: no helper functions, no test utilities, no external fixtures
- Dependencies: `vcpkg`
- Debug builds enable the embedded Slint MCP server. Launch the app with `SLINT_MCP_PORT=8080 ./build/kicad-xyce-plugin` to expose an HTTP/MCP endpoint for AI-assisted UI inspection (browse the `.slint` component tree, read/write properties, invoke callbacks).

## Workflow

Before editing:
1. Search for existing implementations
2. Understand ownership and lifetime
3. Follow conventions in `STYLE-GUIDE.md` and the surrounding code
4. Make minimal changes

After editing:
1. Build the project
2. Fix compiler warnings/errors
3. Run the linter check on all changed C++ files: `bash build/check-format.sh`
4. Format any changed `.slint` files in-place: `slint-lsp format -i <path/to/file.slint>`
5. Summarize changed files
6. Do not execute unit tests unless explicitly asked

## UI (Slint)

The UI is built with Slint and lives under `src/ui`:

- Root widgets (windows/dialogs): `src/ui/widgets/*.slint`
- Shared components (imported only by roots): `src/ui/components/*.slint`
- Host C++ (`slint.h`, wiring, platform backends): flat under `src/ui/` (views/presenters, dialog wrappers, `file_dialog.*`, `clipboard.*`, `modal_manager.*`, `charts_renderer.*`)

### Build integration (`CMakeLists.txt`)

- Only `src/ui/widgets/*.slint` are compiled via `slint_target_sources(... NAMESPACE <file-stem>)`. Components under `src/ui/components/` are imported by roots only; never register them standalone.
- Each root widget becomes a C++ namespace named after its file stem (e.g. `main_window::MainWindow`); its generated header is included with angle brackets: `#include <main_window.h>`.
- Slint names are kebab-case; generated C++ accessors/callbacks are camelCase: property `charts-visible` → `get_charts_visible()`/`set_charts_visible()`; callback `show-charts` → `on_show_charts()`.
- `SLINT_STYLE=cupertino`, `SLINT_FEATURE_RENDERER_SKIA=ON`, Slint pinned to `release/1` via FetchContent.
- `@image-url()` assets are embedded with `SLINT_EMBED_RESOURCES embed-files`; image paths resolve relative to the `.slint` file (`../kicad-icons/..._48.png`).
- Platform host code uses the suffixes `.osx.mm` / `.win32.c++` / `.linux.c++`; UI colors come from `Palette` in `std-widgets.slint`.

### MVP wiring

- Keep the view adapter pattern: a `*View` implements `MainWindowViewDef` and forwards user interactions through `MainWindowViewDefEvents` (the presenter implements it and never touches the UI). The parent creates both and wires them with `view->set_event_handler(*presenter)`.
- Expose UI actions as `export global ... Actions { callback ...; }` on the root widget; bind them from `set_event_handler` via `m_window->global<...Actions>().on_<callback>(...)`.
- Views/presenters hold `slint::ComponentHandle` members and are non-copyable/non-movable; keep them alive while the event loop runs.
- Prefer `slint::SharedString` for string interop and `slint::VectorModel<T>` for models (`set_row_data` to update a row).

### Dialogs

- Slint `Dialog`s are non-modal, separate native windows. Emulate modality with `modal_manager::set_input_blocked(parent, dialog, blocked)` (per-platform); gate every dialog through the view's `begin_modal_dialog()`/`end_modal_dialog()` and wrap toolbar callbacks in `guard_modal(...)` so they are rejected while a dialog is open.
- Each dialog wrapper (e.g. `PluginConfigDialogView`) must live in its own translation unit — generated dialog headers define a `SharedGlobals` type that conflicts with the main-window header — and uses a pimpl `Impl`. It exposes `show(...)`, an `on_closed` callback and `window()`; on accept/cancel, hide the dialog, call `on_closed()`, then deliver results through `MainWindowViewDefEvents`.

### Charts renderer

- `ChartsRenderer` is platform-neutral and owns isolated ImGui/ImPlot contexts; it renders offscreen through a Skia raster surface and publishes each frame as a `slint::Image` via an injected publish function. No per-platform backends exist.
- Always use `ChartsContextScope` (RAII) around host ImGui/ImPlot calls — never assume a global context.
- Drive the redraw loop with a `slint::Timer` (16 ms) via `on_idle()`; layout/resize events schedule frames through `refresh_charts(n)` + timer, never synchronous renders.
- Slint passes chart interaction positions as `float [0..1]`; translate to a chart index with `ChartsRenderer::position_to_index()` before acting.
- Call `set_visible(bool)` when the charts panel is shown/hidden to pause/resume rendering.

### Lifecycle

- `App::run()` owns the event loop: build the view + presenter, wire them, `show()`, then `slint::run_event_loop()`.

## Architecture

- Slint owns the application lifecycle (`App` singleton + `slint::run_event_loop`)
- Rendering components should remain independent where possible
- Avoid unnecessary dependency coupling
