# Compatibility

Patchwork supports **GOG Neverwinter Nights 2 Complete 1.23 and 1.23 v2**.
It does not support Enhanced Edition, other store releases or modified game
executables.

Patchwork checks your executable before applying fixes. Only the following
`nwn2main.exe` files are supported; the version label alone is not enough.

| Game build | SHA-256 |
| --- | --- |
| GOG 1.23 | `5bfcdac2cdcdbb460eb7995a7b0521e9a2398bee3918f9589c4e708f385d091e` |
| GOG 1.23 v2 | `bd599aa8cb680e44516e18a8847f0edd201a331227af94baf20fc5f74ff605ae` |

To check your file, run this in PowerShell from the game folder:

```powershell
Get-FileHash .\nwn2main.exe -Algorithm SHA256
```

Use the x86 package even on 64-bit Windows. Gameplay checks were performed on
Windows 11; other Windows versions have not been tested in-game.

## Other mods

Other runtime patchers and wrappers are not currently supported alongside
Patchwork. It refuses to start when it finds these known conflicting files
in the game folder:

`nwn2fixes.dll`, `dxwrapper.dll`, `ClientExtension.hdl`,
`ClientExtension.dll`, `d3d9.dll`, `dsound.dll`, `ddraw.dll`,
`winmm.dll` or `version.dll`.

Patchwork does not remove these files. Follow the other mod's uninstall
instructions or use a separate clean installation.

Do not overwrite another mod's `dinput8.dll`; Patchwork uses that filename too.
Loading multiple mods through it is not supported. Other combinations, including
game-content mods, have not been systematically tested.

## Installation requirements

Place `dinput8.dll` beside `nwn2main.exe`, with the `nwn2-patchwork` folder
beside it. Avoid installation paths longer than 259 characters.

If a compatibility or initialization check fails, Patchwork displays an error
and stops that game launch. See [troubleshooting](troubleshooting.md).
