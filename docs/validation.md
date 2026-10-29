# Validation and limits

## Research foundation

The runtime implementation was carried over from an independent display-research
workspace at checkpoint `1908353`, functional runtime source `e7b02cb`.
`research-origin.json` records the original file hashes. No game binaries,
private settings, saves, bulk traces or decompiler database are part of this repository.

Research DLL SHA-256:
`8c12db619a16fea95eb847df3ca62cea5dbc1fbddeb398e1f709d42890f7108e`.
This identifies the historical tested DLL, not a promise of byte-identical new builds.

| Behavior | Evidence |
| --- | --- |
| Native title movement and right-edge width resize, recovered image/input | D193 v2 / D194 original, loaded gameplay, four custom dragging options off |
| Maximize and drag back to normal | D193/D194 image/retry responsive; first attempted click has automation screenshot-cache uncertainty |
| Original loaded Graphics resolution Apply | D197 final quiet default |
| V2 Graphics resolution Apply | D198 final quiet default at menu then load; D189 loaded on earlier custom-drag profile |
| Maximize, exclusive Alt+Enter, actual lost-device focus loss, ordinary Alt+Tab recovery | D197 original / D198 v2, final quiet default, loaded gameplay |
| Alt+Enter return, minimize/ordinary restore, saved normal placement | D197/D198, image/input and read-only state snapshots; D198 first window-return click has tool-cache uncertainty, retry works |
| Optional borderless entry/return/minimize/restore with EPOS running | D195 v2 / D196 original, loaded image/input |

Both D197/D198 closed normally. The three user settings files were restored by
hash and each full audit found all 12,424 save files unchanged. This is research
preservation evidence; the public launcher lets game preferences save normally.

## Automated tests

CTest covers the carried-forward geometry, actual wrapper/operation fixtures,
capture/cancellation/mode ordering, resolution synchronization, Graphics guards,
borderless guard logic and actual DLL refusal of an unsupported executable.
New launcher/configuration tests cover public option validation and translation,
exact supported hashes, command-line refusal and unsupported-file preflight.
Tests run without copyrighted game files. Hardware D3D calibration is excluded.

These tests cannot prove correct gameplay rendering. A release ZIP also needs a
bounded smoke check of its actual binaries against both installed supported
executables before public promotion. Record that separately from historical
research acceptance and from CI success.

## Explicit limits

Continuous rendering during dragging is optional and unproved; temporary native
modal pauses are accepted. Snap layouts, mixed-DPI transitions, every cross-monitor
combination, long-session reliability and arbitrary mod combinations are not certified.
Lower-resolution exclusive focus loss to EPOS is not fixed; borderless is an opt-in
workaround. The stock small-window HUD distortion remains. Failed/experimental
small-UI redesign and custom dragging are disabled by the public launcher.
No audio, connection-check or DirectX timing fix is included.
