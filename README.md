# Kiro Image Bridge

Paste a Windows clipboard image straight into Kiro running under WSL.
Kiro reads pasted images from the X11 clipboard (via arboard), and WSLg does not bridge images across from Windows.
This tool reads the Windows clipboard image (or a copied image file), decodes and caps it, and serves it on the X11 CLIPBOARD selection so Ctrl+V in Kiro attaches it.

This may work for other similar tools, but was designed/tested for Kiro-CLI.

## Requirements

- Windows 11, or Windows 10 with WSLg, and WSL2.
- `xclip` in your default WSL distro: `sudo apt install xclip`
- `image-bridge.exe` (prebuilt, or [build the included Visual Studio project](#building) as x64 Release) and the [`image-bridge-serve`](image-bridge-serve) script (included).

## Install

1. WSL side. Install the serve script into your default distro:

       sudo install -m 0755 image-bridge-serve /usr/local/bin/image-bridge-serve

   No sudo: put it anywhere on your side, for example `~/.local/bin/image-bridge-serve`, and point the tool at it with the `serve_script` [config key](#config) or `--serve-script`.

2. Windows side. Put `image-bridge.exe` wherever you like, for example `%LOCALAPPDATA%\Programs\image-bridge\image-bridge.exe` (no admin needed). Create a shortcut somewhere if you plan to always manually run it.
   It streams the image to WSL in memory and writes nothing to disk.

## Use

Run it. Double-click `image-bridge.exe` and it sits in the system tray with a focus-gated global hotkey (default `ctrl+u`):

    image-bridge.exe

Copy an image on Windows, focus Kiro in your terminal, and press the hotkey.
A chord is ctrl/alt/shift/win plus one letter or digit.
Right-click the tray icon to quit. Only one instance runs at a time.

Note: To prevent this always catching the hotkey, it only fires if the terminal is focused.
The default terminal it looks for is WindowsTerminal, this can be changed via [config](#config).

One-shot. Paste the current clipboard image once and exit, for binding to your own launcher (e.g. PowerToys Keyboard Manager):

    image-bridge.exe --once

The image attaches to the prompt.

## Config

Optional. The tool reads `image-bridge.conf` next to the exe, or the path given by `--config`.
A missing file or key falls back to a built-in default, and command-line arguments override the file.
See [`image-bridge.conf.example`](image-bridge.conf.example).

Keys:

- `terminal`: process name the hotkey is gated on (default `WindowsTerminal.exe`). Set this for a different terminal.
- `hotkey`: the global hotkey chord, e.g. `ctrl+u`.
- `serve_script`: WSL path to the serve script.
- `max_dim`: longest-side pixel cap (default 2000).

## Start on login

Windows (recommended). Press Win+R, run `shell:startup`, and drop a shortcut to `image-bridge.exe` in that folder.
It starts in the tray at each login.

The daemon is a Windows process, so there is nothing to start inside WSL.
If you would rather launch it when you open a WSL terminal, add this to your shell profile, pointing at wherever you installed the exe.
Only one instance runs, so launching it again from another terminal is harmless:

    "/mnt/c/Users/<you>/AppData/Local/Programs/image-bridge/image-bridge.exe" &

## Without the daemon

If you already run PowerToys: Keyboard Manager can map to launch this.
Add a keybinding for `image-bridge.exe --once`. This does not start a daemon.

With this approach, you don't need to have this [start on login](#start-on-login).
It also gives better keybinding support.

## Building

Open the solution in Visual Studio with vcpkg enabled and build x64 Release.
The dependencies (stb, libwebp) are handled by vcpkg.
