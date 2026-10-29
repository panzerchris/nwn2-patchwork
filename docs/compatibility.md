# Compatibility

The supported files are `nwn2main.exe` from the two locally investigated GOG
Neverwinter Nights 2 Complete 1.23 distributions. The launcher and DLL independently
verify SHA-256; the DLL also checks expected bytes before writing patch sites.

| Label | SHA-256 |
| --- | --- |
| GOG 1.23 | `5bfcdac2cdcdbb460eb7995a7b0521e9a2398bee3918f9589c4e708f385d091e` |
| GOG 1.23 v2 | `bd599aa8cb680e44516e18a8847f0edd201a331227af94baf20fc5f74ff605ae` |

These labels identify the tested builds; a matching version string is insufficient.
This does not claim vendor authenticity for every file in an installation.
Other store releases, modified executables and Enhanced Edition are not supported.

For a check without starting the game:

```powershell
.\nwn2-patchwork.exe --check
```

For a package outside the game folder:

```powershell
.\nwn2-patchwork.exe --game "C:\Games\NWN2 Complete\nwn2main.exe" --check
```

Without `--check`, the same command starts the game. `--config PATH` selects an
alternative Patchwork INI. Relative command-line paths resolve from the current
directory; default paths resolve from the launcher, not the shortcut's working directory.
The default finds the game beside the launcher or one folder above it.
Paths must fit the inherited Windows 259-character limit.

## Other runtime modifications

Combining runtime patchers has not been validated. The launcher refuses known
conflicting files in the game root: `nwn2fixes.dll`, `dxwrapper.dll`,
`ClientExtension.hdl`, `ClientExtension.dll`, `d3d9.dll`, `dinput8.dll`,
`dsound.dll`, `ddraw.dll`, `winmm.dll` and `version.dll`.
It never removes those files. Keep a backup and use a clean installation for testing.
This list is a conflict check, not proof that no other injection exists.
Game-content mods have not been systematically tested.

One game process at a time is required. The launcher never attaches to or kills
an existing user game; initialization failure terminates only its own new child.
