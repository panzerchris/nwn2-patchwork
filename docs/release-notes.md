# NWN2 Patchwork 0.1.0

A small collection of configurable runtime fixes for Neverwinter Nights 2.

This initial prerelease focuses on display recovery: moving/resizing, maximize/
restore, minimize/restore, resolution Apply and Alt+Enter. It includes an optional
desktop-sized borderless workaround for EPOS Connect's fullscreen focus interaction.
Audio, connection-check and DirectX timing changes are not included.

Download **nwn2-patchwork-0.1.0-windows-x86.zip**, extract its `nwn2-patchwork`
folder into your NWN2 Complete installation, then run `nwn2-patchwork.exe` inside it.
Use the x86 package on both 32-bit and 64-bit Windows. No compiler or extra runtime
installation is needed. Edit `nwn2-patchwork.ini` to configure the fixes.

Supports only the two documented **GOG 1.23 / GOG 1.23 v2** executable hashes.
The launcher refuses unsupported executables and known runtime-patch conflicts.
No game executable or assets are included or modified on disk.

Temporary rendering pauses while dragging are acceptable; persistent post-release
freezes or broken input should be reported. Snap/mixed-DPI and arbitrary mod
combinations remain unvalidated; small-window HUD scaling remains native.
See the repository's compatibility, configuration and validation documents.

**Publication held:** Microsoft Defender quarantined the launcher during the v2
runtime check, reporting `Behavior:Win32/DefenseEvasion.A!ml`. The v2 load/move/
resize/maximize/restore checks worked, but the original-build counterpart could
not start after quarantine. No Microsoft review determination is available yet.
Keep this release in draft. Do not present it as cleared or ready to use.
See [candidate validation](https://github.com/panzerchris/nwn2-patchwork/blob/main/docs/releases/v0.1.0-validation.md).
