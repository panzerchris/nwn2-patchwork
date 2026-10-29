# Configuration

Edit `nwn2-patchwork.ini` while the game is closed. Options are under `[Display]`.
Use `1` or `0`; use decimal integers for dimensions. Missing options use the
defaults below. Unknown options, invalid values and invalid dependencies are refused.
Comments belong on separate lines beginning with `;`.

| Option | Default | Purpose |
| --- | --- | --- |
| RevertV2 | 1 | Revert the identified v2 resize code regression. Required on v2; no change on original. |
| GuardEmptyRestore | 1 | Avoid feeding empty restore dimensions into the resize path. |
| NativeSizing | 1 | Use the engine's full resize lifecycle when handling window size changes. |
| SyncWindowedMode | 1 | Reconcile stored windowed dimensions with the actual client/backbuffer. |
| PreservePlacement | 1 | Preserve normal/maximized placement across mode transitions. |
| ThrottleLostDevice | 1 | Reduce repeated presentation attempts while the device is lost. |
| FixMinimapHeight | 1 | Correct the minimap initialization operand that clears saved height instead of saved Y. |
| FixGraphicsControls | 1 | Correct identified resolution-list/control-type handling in Graphics options. |
| FitWindowedApply | 1 | Keep windowed resolution Apply inside the monitor work area. Requires NativeSizing and SyncWindowedMode. |
| BorderlessAltEnter | 0 | Make Alt+Enter switch between windowed and desktop-sized borderless. Requires NativeSizing and SyncWindowedMode. |
| MinClientWidth | 800 | Minimum width when NativeSizing is enabled. |
| MinClientHeight | 600 | Minimum height when NativeSizing is enabled. |

Both minimum dimensions must be zero, or both must be 1–16384. The default permits
sizes where the original HUD stretches; use at least 1024×768 if that bothers you.
Disabling a fix is available for diagnosis; arbitrary combinations do not inherit
the full default-profile runtime acceptance. Shared forwarding hooks can remain
installed when individual fixes are disabled. This is not a stock-game control.

## Borderless and EPOS Connect

With `BorderlessAltEnter=0`, Alt+Enter keeps the game's native exclusive-fullscreen
behavior. EPOS Connect has been observed showing its window after monitor mode
changes, taking focus and causing NWN2 to minimize. Desktop-resolution exclusive
recovery passed the checked sequences, but lower-resolution exclusive can still
trigger this external interaction.

To use the workaround, set `BorderlessAltEnter=1`, start with the game's **Full
Screen unchecked**, then press Alt+Enter. Press it again to restore the window.
The Graphics checkbox still requests exclusive mode. Borderless renders at desktop
resolution; it does not upscale a lower-resolution render target and may cost more GPU time.

## Internal runtime configuration

The DLL translates these public settings into a small per-launch configuration
beside its startup log in Local AppData. The inherited DLL uses an internal
`[Experiment]` section and `Initialize` export; these are implementation details.
Custom modeless dragging, trace/fault injection and the rejected small-UI redesign
remain disabled by the public configuration. No audio options are implemented yet.
