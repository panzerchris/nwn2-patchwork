# Contributing

Bug reports and focused fixes are welcome.

## Report a problem

Open an issue with the Patchwork version, game build, steps to reproduce and
what you expected to happen. Include relevant settings and system details.
See [troubleshooting](docs/troubleshooting.md) for useful information to collect.

For display problems, say whether the picture and controls recover after you
release the window. For multiplayer problems, say which login or connection
step fails. Keep private files and game assets out of public issues.

## Code changes

Follow [development](docs/development.md) to build Win32 Release and run the
automated tests. No game installation is needed for those tests.

Changes to patches must check supported executable hashes and expected bytes,
preserve calling conventions and handle failures safely. Changes affecting
gameplay need in-game checks as well as automated tests.

Keep changes focused. Separate loading and configuration code from runtime
fixes, and add new fix families as separate modules and INI sections.
The supported scope is display fixes and the multiplayer login bypass.

## Documentation

Write user guides and release notes around what players need to know.
Keep build instructions and architecture in developer documentation.
Detailed investigations belong in the independent research repository.
See [the documentation guidance](docs/development.md#documentation).

Contributions are provided under the project's [MIT licence](LICENSE).
