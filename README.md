# NWN2 Patchwork

A small collection of configurable runtime fixes for Neverwinter Nights 2.

The first release focuses on display recovery: moving and resizing the window,
maximize/restore, minimize/restore, resolution changes and Alt+Enter. Individual
fixes can be enabled or disabled in an INI file. Audio, the defunct connection
check and DirectX timing are possible future work; this release does not change them.

## Download and run

Download the **Windows x86 ZIP** from [Releases](https://github.com/panzerchris/nwn2-patchwork/releases).
The automatically generated GitHub “Source code” archives contain source, not a runnable build.

1. Close NWN2. Extract the ZIP into your **NWN2 Complete game folder**. Keep its
   `nwn2-patchwork` subfolder intact, next to `nwn2main.exe`.
2. Open that subfolder and run **`nwn2-patchwork.exe`**. You can make a shortcut to it.
3. The default settings enable the tested display fixes. Edit
   `nwn2-patchwork.ini` with the game closed to change them.

No compiler, Python, PowerShell wrapper or additional Visual C++ runtime install
is needed. The game remains 32-bit; the x86 package is also correct on 64-bit Windows.
Patchwork starts its own game process and applies changes in memory before game
entry. It does not replace or modify `nwn2main.exe`, change saves, or install a proxy DLL.
Game preferences continue to save normally, as with an ordinary game launch.

To play without Patchwork, use the game's normal launcher or `nwn2main.exe`.
To uninstall, close the game and remove only the `nwn2-patchwork` subfolder.
Small startup logs are stored in `%LOCALAPPDATA%\NWN2 Patchwork\logs`.

## Supported games

**GOG Neverwinter Nights 2 Complete 1.23 and GOG 1.23 v2**, using the exact
`nwn2main.exe` hashes in [compatibility](docs/compatibility.md). Other executables
are refused before Patchwork starts the game. Version labels alone do not establish compatibility.
This package is for the original game, not Neverwinter Nights 2: Enhanced Edition.

Development/runtime checks were performed on Windows 11. The automated build uses
Windows Server 2022; other Windows versions have not been runtime-validated.
Start from an installation without other runtime patchers, Client Extension or
local graphics/input/audio proxy DLLs. Those combinations are not supported yet.

## Display behavior and limits

- A temporary rendering pause while dragging is normal for the native Windows
  move/resize loop. Rendering and input should recover after release.
- Moving, width resizing, maximize/restore, minimize/restore, resolution Apply and
  native Alt+Enter recovery have research evidence on both supported EXEs.
- EPOS Connect can take focus when exclusive fullscreen changes the monitor's
  resolution. Optional `BorderlessAltEnter=1` avoids that change: start windowed,
  then press Alt+Enter for desktop-sized borderless presentation. The Graphics
  Full Screen checkbox continues to request native exclusive fullscreen.
- Snap layouts, mixed-DPI transitions and long-session reliability are not fully
  validated. The original HUD can stretch below 1024 wide or 768 high.

See [configuration](docs/configuration.md), [troubleshooting](docs/troubleshooting.md)
and [validation](docs/validation.md) for details and the scope of the evidence.
The initial public release is a prerelease for wider testing, not a promise that
every display setup or mod combination works.

## Build from source

Install Visual Studio 2022 or Build Tools with **Desktop development with C++**,
the MSVC x86/x64 tools and a Windows SDK. CMake 3.24+ is required.

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A Win32
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
powershell -NoProfile -File scripts/package.ps1 -BuildDir build
```

CMake generates a Visual Studio solution under `build/` if you prefer the IDE.
The package script creates a ZIP, SHA-256 checksum and build manifest under `dist/`.
It packages only explicitly installed release files; tests, private game files,
research data and build intermediates are excluded. See [development](docs/development.md).

## Licence

[MIT](LICENSE). An unofficial community project; no game executable or game assets
are distributed. Neverwinter Nights and associated names belong to their respective owners.
