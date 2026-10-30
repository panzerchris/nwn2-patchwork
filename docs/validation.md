# Testing and checksums

Patchwork is a prerelease. Testing covers the two supported GOG builds on
Windows 11, with the supplied settings and no other runtime patchers installed.

| Version | Testing summary |
| --- | --- |
| [0.3.0](releases/v0.3.0-validation.md) | Faster login and basic gameplay checked on both builds; local server and LAN checks on v2. Gameplay testing used an earlier build with the same runtime source. The final package was verified but was not retested in-game. |
| [0.2.0](releases/v0.2.0-validation.md) | Published DLL checked in-game on both builds for startup, loading, window movement, resizing and maximize/restore. Additional resolution and fullscreen checks on v2. |

Each linked page includes the release ZIP and DLL hashes, testing scope and
remaining gaps. Automated tests check configuration and patch behavior; in-game
checks also assess the visible picture and working controls.

## Known limits

- Rendering can pause while dragging; it should recover after release.
- The original HUD can stretch below 1024 pixels wide or 768 pixels high.
- Exclusive fullscreen can lose focus to EPOS Connect after a resolution change.
  [Borderless mode](configuration.md#borderless-fullscreen) is a workaround.
- Snap layouts, mixed-DPI and other monitor combinations, long sessions and
  mod combinations have not been fully tested.
- Multiplayer testing is limited; see the [multiplayer guide](multiplayer.md).

The unpublished [0.1.0 launcher candidate](releases/v0.1.0-validation.md) was
superseded by the drop-in DLL releases.
