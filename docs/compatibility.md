# Compatibility

The supported files are `nwn2main.exe` from the two locally investigated GOG
Neverwinter Nights 2 Complete 1.23 distributions. The bootstrap and runtime independently
verify SHA-256; the DLL also checks expected bytes before writing patch sites.

| Label | SHA-256 |
| --- | --- |
| GOG 1.23 | `5bfcdac2cdcdbb460eb7995a7b0521e9a2398bee3918f9589c4e708f385d091e` |
| GOG 1.23 v2 | `bd599aa8cb680e44516e18a8847f0edd201a331227af94baf20fc5f74ff605ae` |

These labels identify the tested builds; a matching version string is insufficient.
This does not claim vendor authenticity for every file in an installation.
Other store releases, modified executables and Enhanced Edition are not supported.

For a check without starting the game, compare this result with the table:

```powershell
Get-FileHash .\nwn2main.exe -Algorithm SHA256
```

Install beside `nwn2main.exe`; configuration is resolved relative to the DLL,
independently of a shortcut's working directory. Paths must fit the inherited
Windows 259-character limit. Only a process named `nwn2main.exe` is patched;
other executables that load the DLL receive ordinary DirectInput forwarding.

## Other runtime modifications

Combining runtime patchers has not been validated. The bootstrap refuses known
conflicting files in the game root: `nwn2fixes.dll`, `dxwrapper.dll`,
`ClientExtension.hdl`, `ClientExtension.dll`, `d3d9.dll`,
`dsound.dll`, `ddraw.dll`, `winmm.dll` and `version.dll`.
It never removes those files. Keep a backup and use a clean installation for testing.
This list is a conflict check, not proof that no other injection exists.
Game-content mods have not been systematically tested.

Do not overwrite an existing `dinput8.dll`: Patchwork uses that filename itself.
Chaining another input loader is not implemented. No third-party loader is bundled.
The system DirectInput DLL is resolved by its full system-directory path.
Initialization failure shows a message and stops the newly starting game to avoid
continuing with partially installed hooks. Patchwork does not attach to other processes.
