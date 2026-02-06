# KeyboardAutoSwitcher

KeyboardAutoSwitcher is a small portable Windows tray app that lets you temporarily switch to a secondary keyboard layout and then automatically return to your default layout. :contentReference[oaicite:1]{index=1}

![KeyboardAutoSwitcher](KeyboardAutoSwitcher.png)

## What it does
- Runs in the system tray (no installer).
- Switches the current keyboard layout on demand (hotkey).
- Automatically returns to the default layout after a short delay.

Typical use case: keep **English** as your default layout, quickly switch to a second layout to type a short message, then auto-return back to English.

## How it works
- Press hotkey → switch to secondary layout  
- After **20 seconds** → auto return to default layout  
- Optional: **Start with Windows** (tray checkbox setting)

## How to run
1. Go to **Releases** and download `Release_x64.zip`.
2. Unzip it anywhere (e.g. `Downloads\KeyboardAutoSwitcher\`).
3. Run the app (it will appear in the system tray).
4. Use the tray menu to view/adjust settings and enable auto-start.

> Note: If Windows SmartScreen shows a warning, click **More info** → **Run anyway** (common for unsigned portable tools).

## Options / Settings
Settings are available from the tray menu.
- **Auto return timeout:** 20 seconds (v0.7.1)
- **Start with Windows:** enable/disable via tray checkbox

## Auto-start
To launch the app automatically after login:
1. Open the tray menu.
2. Enable **Start with Windows**.
3. If Windows disables it, re-enable it in **Settings → Apps → Startup**.

## Limitations / Known issues
- Timer-based behavior: it does **not** track per-window/per-app layout (it’s a simple global switch + timer).
- Works only inside a normal Windows session (secure desktops like UAC prompts / lock screen are not supported).
- Requires at least two keyboard layouts installed in Windows, otherwise switching has no effect.

## Build from source (optional)
- Open `KeyboardAutoSwitcher.sln` in Visual Studio.
- Build configuration: **Release / x64**
- Output: `KeyboardAutoSwitcher.exe` (and related files) can be packed into `Release_x64.zip`.

## License
MIT
