# Contributing

Please start with a reproducible issue or a focused pull request. The first
release covers display fixes on the two exact GOG executables documented in
[compatibility](docs/compatibility.md). Audio, the connection check and DirectX
timing are future work; a proposed fix needs its own investigation and validation.

For bug reports, distinguish a temporary pause during native dragging from a
game that stays stuck afterward. Include the Patchwork version, game executable
hash, menu/gameplay context, reproduction steps and relevant configuration.
The issue template asks for these details. Keep private files and game assets out
of public issues.

For code changes, build Win32 Release and run CTest using the README commands.
Tests do not need a game installation. Changes to hooks need expected-byte/build
guards and evidence that the calling convention, ownership and failure behavior
remain correct. Runtime tests need separate image and input observations; an
S_OK result alone is insufficient. See [development](docs/development.md).

Keep launcher policy separate from runtime fixes. Add future fix families as
separate modules and INI sections when proved. Keep commits and changes scoped;
do not mix cosmetic rewrites of the tested runtime with functional changes.

The current v0.1.0 candidate is held for a Defender detection and an incomplete
original-build smoke check. Keep it in draft and follow the recorded review gate.
Do not bypass quarantine or weaken security settings to run a candidate.

Contributions are provided under the project's [MIT licence](LICENSE).
