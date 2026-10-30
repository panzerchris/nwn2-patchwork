# Changelog

## 0.3.0 — Faster multiplayer login

- Skip the long wait for the old multiplayer login service. Click OK on the
  game's warning to continue to the multiplayer menu.
- Enabled in the supplied INI. When keeping an older INI, add
  `[Multiplayer] SkipLegacyLoginCheck=1` to enable it.
- Display fixes are unchanged from 0.2.0.

This does not restore the public server browser or account service.
See [multiplayer](docs/multiplayer.md) for details and
[testing and checksums](docs/releases/v0.3.0-validation.md) for tested coverage.

## 0.2.0 — First public release

- Install a drop-in `dinput8.dll` and start the game through its usual shortcut.
- Improve display recovery after window movement, resizing, maximize/restore,
  minimize/restore, resolution changes and Alt+Enter.
- Fix minimap saved-height handling and Graphics resolution controls.
- Offer optional desktop-sized borderless fullscreen through Alt+Enter.
- Configure individual fixes in an INI file.

Supports the documented GOG 1.23 and 1.23 v2 executables.
See [testing and checksums](docs/releases/v0.2.0-validation.md).

## 0.1.0 — Unpublished candidate

Initial display fixes using a separate launcher. This candidate was not
published and was superseded by 0.2.0.
