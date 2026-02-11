# Changelog
All notable changes to this project are documented in this file.

The format is loosely based on Keep a Changelog, and this project follows SemVer.

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
