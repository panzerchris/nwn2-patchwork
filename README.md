# NWN2 Patchwork

A small collection of configurable runtime fixes for Neverwinter Nights 2.

Patchwork helps the game recover when you move or resize its window, minimize
or maximize it, change resolution, or switch fullscreen modes. It also offers
optional borderless fullscreen and skips the long wait when signing into multiplayer.

**[Download 0.3.0](https://github.com/panzerchris/nwn2-patchwork/releases/tag/v0.3.0)**
for GOG Neverwinter Nights 2 Complete 1.23 or 1.23 v2.
Patchwork is currently a prerelease. It supports the original game, not Enhanced Edition.

## Install

1. Close NWN2 and download the **Windows x86 ZIP** from
   [Releases](https://github.com/panzerchris/nwn2-patchwork/releases).
   Use x86 on 64-bit Windows too. The GitHub “Source code” downloads are for developers.
2. Extract `dinput8.dll` and the `nwn2-patchwork` folder beside `nwn2main.exe`
   in your game installation. If `dinput8.dll` is already there, check that it
   belongs to Patchwork before replacing it; another mod may use that filename.
3. Start the game through your usual shortcut.

No separate launcher or additional runtime installation is needed. Patchwork
applies its fixes in memory; it does not modify your game executable or save files.

**Upgrading:** back up your Patchwork INI before extracting an update if you have
customized it. When upgrading from 0.2.0 and keeping your old INI, add the
[multiplayer setting](docs/configuration.md#multiplayer-login) to enable the faster login.

**Uninstalling:** close the game and remove Patchwork's `dinput8.dll`. You can
also remove the `nwn2-patchwork` folder.

## Settings and known issues

Edit `nwn2-patchwork/nwn2-patchwork.ini` with the game closed.
The supplied settings enable the display fixes and faster multiplayer login.

- **Borderless fullscreen:** enable `BorderlessAltEnter=1`, start windowed,
  then press Alt+Enter. See [configuration](docs/configuration.md) for details.
- **Multiplayer:** the service-unavailable warning still appears; click OK to
  reach the multiplayer menu. Patchwork does not restore the public server browser.
- **Dragging:** rendering may pause while you move or resize the window. The
  picture and controls should recover when you release it.
- **Small windows:** the original HUD can stretch below 1024 pixels wide or
  768 pixels high.
- **Compatibility:** only the two documented GOG executables are supported.
  Other runtime patchers, including Client Extension and graphics/input wrappers,
  are not currently supported alongside Patchwork.

See [compatibility](docs/compatibility.md), [configuration](docs/configuration.md)
and [troubleshooting](docs/troubleshooting.md) for help.
[Testing and checksums](docs/validation.md) describes the tested setups and remaining gaps.

## Contributing

Bug reports and focused fixes are welcome. See [contributing](CONTRIBUTING.md)
or [building from source](docs/development.md).

## Licence

[MIT](LICENSE). An unofficial community project; no game executable or game
assets are distributed. Neverwinter Nights and associated names belong to their
respective owners.
