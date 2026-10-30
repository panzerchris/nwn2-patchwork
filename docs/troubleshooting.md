# Troubleshooting

## Patchwork does not seem to load

Check that `dinput8.dll` is beside the `nwn2main.exe` your shortcut starts,
with the `nwn2-patchwork` folder beside it. A successful startup creates a log
under `%LOCALAPPDATA%\NWN2 Patchwork\logs`.

## Unsupported executable or conflicting mod

Compare your executable hash with [compatibility](compatibility.md).
Other runtime patchers and wrappers can also prevent startup. Use a supported
installation and follow the other mod's removal instructions if needed.

## Fullscreen immediately minimizes

EPOS Connect can take focus when the monitor changes resolution.
Try [borderless fullscreen](configuration.md#borderless-fullscreen).

## Rendering pauses while dragging

A pause while moving or resizing the window is expected. Report a bug if the
picture or controls stay broken after you release it.

## The HUD stretches in a small window

This is an original game limitation below 1024 pixels wide or 768 pixels high.
Use a larger window or raise the minimum size in the [configuration](configuration.md).

## Multiplayer still has the long login wait

Check that you installed Patchwork 0.3.0 and that your INI contains
`[Multiplayer]` with `SkipLegacyLoginCheck=1`. An older INI keeps the original
wait until you add the setting. The service-unavailable warning still requires OK.

For local server connections, see the [multiplayer guide](multiplayer.md).

## Configuration or initialization error

The startup message identifies the problem. Check spelling, use 0 or 1 for
switches, and keep required settings enabled together. `RevertV2` must remain
enabled on GOG v2. See [configuration](configuration.md).

If an initialization error persists, include its message and numeric code in a
bug report. Patchwork stops the launch when initialization fails.

## Antivirus detection

Keep the file quarantined and report the Patchwork version and detection name.
Do not disable protection or add an exclusion to run it.

## Report a bug

[Open an issue](https://github.com/panzerchris/nwn2-patchwork/issues/new/choose)
with:

- Your Patchwork version and GOG game build; include the executable SHA-256 if available.
- What you did, what happened and what you expected.
- Whether it happens in the menu or during gameplay.
- Windows version, relevant display/GPU details, other mods and changed Patchwork settings.

A screenshot or relevant small startup log can help. Logs are in
`%LOCALAPPDATA%\NWN2 Patchwork\logs`; check their contents before sharing.
Please do not attach game executables, saves or private game settings.
Patchwork has no telemetry or automatic log upload.
