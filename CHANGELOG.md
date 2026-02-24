# Changelog
All notable changes to this project are documented in this file.

The format is loosely based on Keep a Changelog, and this project follows SemVer.

## [v1.0.1] - Hotfix UI
### Fixed
- Synced tray context menu state on app startup to match the actual feature state.
- UI now updates correctly after startup.

### Tests
- Disable in UI → tray menu shows Disabled (and stays consistent after restart).

## [v1.0.0] - Release
### Added
- Microsoft Store release.
- Persistent settings.
- Store-safe build with polished tray UX.

### Notes
- Clean architecture with no known technical debt at release time.
- Store assets included: screenshots, description, minimal privacy policy, keywords.

## [v0.9.3] — Store compliance & hardening

### Added
- Reviewer notes for Store submission: `STORE_NOTES_v0.9.3.md` (runFullTrust rationale, no services/drivers/background tasks, StartupTask is user-controlled, hook privacy).
- DPI-aware UI support (Per-Monitor V2), aligned with DPIAwarenessValidation.

### Changed
- Extracted UI responsibilities into `UIPresenter` (DPI scaling, fonts, layout, painting) to simplify core app code.
- Switched UI typography to **bold** and increased font size by **+2 pt** consistently across label/text/checkbox.
- Scaled window and controls using DIPs → pixels; added proper handling for DPI changes.

### Fixed
- UI window/control sizing becoming visually smaller after enabling DPI awareness.
- Correct handling of `WM_DPICHANGED` and the recommended window rectangle.
- Incorrect arrow rendering in the Settings hint (Unicode/escape fix; no more `â†’`).

## [0.9.2] — Tray & UX polish

### Added
- Tray tooltip shows the current input language.
- Visual feedback on layout switch.

### Changed
- Tray menu updated: **Enable/Disable** toggle and **Exit**.
- Improved tray menu positioning and reliability.
- Better icon scaling across 100–400% DPI.

### Fixed
- Disabled mode no longer triggers layout switches via timers.
- More robust menu state updates (not tied to menu item positions).

### Notes
- DPI awareness enabled (Per-Monitor V2).

## [0.9.1] - Settings persistence

### Added
- `Settings`: INI persistence for `enabled` state (save/restore on startup).

### Changed
- `Settings storage`: Store-safe settings path selection:
  - Packaged (MSIX/Store) → `LocalState/settings.ini`
  - Unpackaged (Debug) → `%LOCALAPPDATA%\KeyboardAutoSwitcher\settings.ini`

### Fixed
- Early initialization: removed WinRT storage calls from static/global initialization.

## [0.9.0] - 2026-02-xx
### Added
- `StateManager`: a single source of truth for app state (enabled/disabled, last input, timeout).
- `InputTimer`: a dedicated component that owns inactivity/timer logic outside of `WndProc`.

### Changed
- Refactored: timer logic was moved out of `WndProc` into `StateManager` / `InputTimer` (thinner `WndProc`).
- Reduced “global chaos”: state-related flags/variables are encapsulated inside managers.
- Improved readability and stability via explicit state transitions and centralized handling of input/timeouts.

## [0.7.1] - 2026-02-06
### Added
- Product-grade README with clear usage sections and app behavior description.
- GitHub Release workflow: binaries are distributed via Releases (not stored in repo root).

### Changed
- Repository cleanup: removed `Release_x64.zip` from the root (source-only in main branch).

## [0.7.0] - 2026-01-xx
### Added
- StartupTask / Start with Windows option (Store-friendly autostart approach).
- Tray UI checkbox for enabling/disabling auto-start.
