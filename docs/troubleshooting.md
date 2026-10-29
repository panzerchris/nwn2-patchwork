# Troubleshooting

**The normal shortcut has no fixes:** check that `dinput8.dll` is beside the actual
`nwn2main.exe` it launches, with the `nwn2-patchwork` subfolder beside it. Look for
a new session under the log path below. Another loader or unsupported EXE may prevent startup.

**Unsupported executable:** check the hash against compatibility.md. Restore the
supported game through your installer/client if appropriate; do not rename a
modified executable to bypass the check. The newly starting game is stopped.

**Another patch/proxy is present:** Patchwork refuses known combinations. It does
not uninstall anything. Use a separate clean installation or preserve and remove
the other mod following that mod's own instructions.

**Fullscreen immediately minimizes:** EPOS Connect may be taking focus after the
monitor changes resolution. Try the optional borderless setting in configuration.md.
That workaround has been checked with EPOS running on both supported EXEs.

**Rendering pauses while dragging:** expected with native modal moving/resizing.
Report a problem if rendering or input remains broken after releasing the window.

**HUD stretches in a small window:** an original UI-scaling limitation below
1024 wide or 768 high. This release does not redesign the game's HUD.

**Configuration refusal:** keep switches at 0 or 1, check section/key spelling,
and keep dependent settings enabled together. The startup message names the
relevant dependency. RevertV2 cannot be disabled on v2 in this implementation.

**Initialization refused with a numeric code:** 2/3 identify an unsupported image
base/hash; 5/6/12/16/20/21/23 identify expected-code mismatch or hook preparation;
7/8 identify missing/unwritable startup logging; 9/10/13 identify a failed patch
write. 4/11/14/15/17/18/19/22 are configuration guards. Do not bypass the checks.
The newly starting game is stopped if initialization fails.

**Reporting a bug:** include the release version, executable SHA-256, relevant
INI settings, Windows/GPU/display setup and the exact operation that leaves the
game broken. State whether it happens at the menu or during gameplay, and whether
EPOS/overlays/other mods are running. Include the relevant small startup log from
`%LOCALAPPDATA%\NWN2 Patchwork\logs` if useful; inspect files before sharing.
Logs are local; Patchwork has no telemetry or automatic upload.
