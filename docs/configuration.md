# Configuration

Edit `nwn2-patchwork/nwn2-patchwork.ini` with the game closed.
The supplied settings are recommended for normal play.

Use `1` to enable a switch and `0` to disable it. Dimensions use whole numbers
in pixels. Comments go on separate lines beginning with `;`.
Unknown settings, invalid values and missing dependencies produce a startup error.

## Display

These options belong under `[Display]`. Missing display options use the defaults
below. Most are individual fixes; leave them enabled unless troubleshooting.

| Option | Default | What it does |
| --- | --- | --- |
| RevertV2 | 1 | Fixes a window-resizing regression in GOG v2. Required on v2; has no effect on the original build. |
| GuardEmptyRestore | 1 | Prevents invalid zero-size resize requests when restoring the window. |
| NativeSizing | 1 | Lets the game update its display properly when the window changes size. |
| SyncWindowedMode | 1 | Keeps the game's windowed resolution in sync with the window size. |
| PreservePlacement | 1 | Remembers the window's position, size and maximized state across display-mode changes. |
| ThrottleLostDevice | 1 | Reduces repeated rendering attempts while the display is temporarily unavailable. |
| FixMinimapHeight | 1 | Fixes a mistake in how the minimap's saved height is initialized. |
| FixGraphicsControls | 1 | Fixes resolution selection and related controls in Graphics options. |
| FitWindowedApply | 1 | Keeps the window within the desktop work area when applying a resolution. Requires NativeSizing and SyncWindowedMode. |
| BorderlessAltEnter | 0 | Makes Alt+Enter switch between windowed and desktop-sized borderless mode. Requires NativeSizing and SyncWindowedMode. |
| MinClientWidth | 800 | Minimum window content width when NativeSizing is enabled. |
| MinClientHeight | 600 | Minimum window content height when NativeSizing is enabled. |

Set both minimum dimensions to zero to remove the minimum, or set both to values
from 1 to 16384. A minimum of 1024 by 768 helps avoid the original HUD's stretching
at smaller sizes.

Testing uses the supplied defaults and the optional borderless mode.
Other combinations have not been fully tested. To disable Patchwork entirely,
close the game and remove its `dinput8.dll`.

## Borderless fullscreen

With `BorderlessAltEnter=0`, Alt+Enter uses the game's normal exclusive fullscreen.

To use borderless mode:

1. Set `BorderlessAltEnter=1` in the Patchwork INI.
2. Start the game with **Full Screen unchecked** in Graphics options.
3. Press Alt+Enter. Press it again to return to the window.

The game's Full Screen checkbox still selects exclusive fullscreen.
Borderless renders at your desktop resolution, which may require more GPU power
than playing at a lower resolution. It does not upscale a lower-resolution image.

Borderless can also help when EPOS Connect takes focus and minimizes NWN2 after
an exclusive-fullscreen resolution change.

## Multiplayer login

Version 0.3.0 enables faster login in the supplied INI:

```ini
[Multiplayer]
SkipLegacyLoginCheck=1
```

Click Sign In as usual, then OK on the game's service-unavailable warning to reach
the multiplayer menu without the long wait. This does not restore the public
server browser or the old account service.

Set `SkipLegacyLoginCheck=0` to keep the original login wait. If you kept a 0.2.0
INI, add the section above to enable the feature; a missing option leaves the
original behavior in place. Keep the existing `[Display]` section.

This setting requires Patchwork 0.3.0; 0.2.0 rejects it.
See [multiplayer](multiplayer.md) for feature limits.
