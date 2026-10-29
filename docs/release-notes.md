# NWN2 Patchwork 0.2.0

A small collection of configurable runtime fixes for Neverwinter Nights 2.

Display recovery fixes for moving/resizing, maximize/restore, minimize/restore,
resolution Apply and Alt+Enter, plus an optional borderless workaround for EPOS
Connect's fullscreen focus interaction. Audio, connection-check and DirectX timing
changes are future work.

Download **nwn2-patchwork-0.2.0-windows-x86.zip**. With NWN2 closed, extract
`dinput8.dll` and the `nwn2-patchwork` folder beside `nwn2main.exe`, then start
the game normally. Do not overwrite another mod's `dinput8.dll`. Use this x86
package on 64-bit Windows too. Edit `nwn2-patchwork/nwn2-patchwork.ini` to configure
the fixes. Remove Patchwork's `dinput8.dll` to disable it.

The forwarding DLL replaces the separate launcher used in the unpublished 0.1.0
candidate. It initializes inside the normally starting game, outside `DllMain`,
then forwards DirectInput to Windows. No game EXE or assets are included or
modified on disk. The diagnostic launcher remains available in source only.

Supports only the documented **GOG 1.23 / GOG 1.23 v2** executable hashes.
Temporary native dragging pauses are expected; image/input should recover after
release. Snap/mixed-DPI, long sessions and arbitrary mod combinations remain
unvalidated. The native small-window HUD scaling limit remains.

The previous launcher's Microsoft review is pending. This DLL uses a different
loading architecture; it is not represented as antivirus-certified. A known
blocked binary must not be published. See the repository's validation records.

Built by Windows CI. Both-build acceptance of the exact package is recorded
separately from the historical display research and automated test results.

The exact 176,748-byte CI ZIP passed normal startup, loaded native width resizing,
title movement and maximize/restore checks on both supported EXEs. V2 additionally
passed Graphics resolution Apply and loaded native fullscreen entry/return. All
17 offline tests passed. The exact DLL's local Defender scan found no threats,
and neither gameplay session produced a new detection/quarantine event. These
observations are not a guarantee of future antivirus verdicts.

See [0.2.0 validation](https://github.com/panzerchris/nwn2-patchwork/blob/main/docs/releases/v0.2.0-validation.md)
for precise scope, preservation checks and uncertainties, including the original
game's unexplained nonzero exit status after Alt+F4. The ZIP is the original CI
artifact; this acceptance record was added after that artifact was built.
