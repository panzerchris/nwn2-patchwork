# Development

## Build from source

Install Visual Studio 2022 or Build Tools with **Desktop development with C++**,
the MSVC x86/x64 tools and a Windows SDK. CMake 3.24 or later is required.

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A Win32
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
powershell -NoProfile -File scripts/package.ps1 -BuildDir build
```

CMake also generates a Visual Studio solution under `build/`.
Use Win32: the hooks depend on verified x86 calling conventions and code locations.
The CRT is statically linked. Tests do not require a game installation.

Packaging requires a clean, committed checkout. It produces a ZIP, SHA-256
sidecar and file manifest under `dist/`. Use a new output directory if one
already exists. Only allowlisted release files are packaged.

## Source layout

- `src/probe.cpp` and geometry headers: display fixes.
- `src/multiplayer.cpp` / `.h`: guarded multiplayer login patch.
- `src/dropin.cpp` / `.def`: DirectInput forwarding and initialization.
- `src/runtime_support.h`: executable hashing and startup records.
- `src/config.h`: public INI validation and runtime configuration.
- `src/supported_builds.h`: supported executable hashes.
- `tests/`: automated configuration, patch, geometry and forwarding checks.
- `config/`: supplied configuration.
- `scripts/package.ps1`: packaging and checksums.

See [loading architecture](loading.md) for initialization and failure handling.
The display module retains its original filename and internal configuration
format; avoid unrelated rewrites when changing a fix.
Source provenance is recorded in `docs/research-origin.json` in the checkout.
The build is independent of the research repository.

The optional `PATCHWORK_BUILD_DIAGNOSTIC_LAUNCHER=ON` CMake flag enables a
separate diagnostic launcher and its tests. It is excluded from release packages.
The historical 0.1.0 launcher candidate remains unpublished; see
[its status](releases/v0.1.0-validation.md).

## Changing runtime behavior

Check the exact supported executable hash and expected instruction bytes before
patching. Preserve calling conventions, object ownership and failure handling.
Add new fix families as separate modules and INI sections.

Run relevant automated tests, then check affected behavior in-game on both
supported builds. Verify the visible picture and actual controls; successful
graphics API calls alone do not establish recovery.

Use only an owned test session. Preserve `nwn2.ini`, `nwn2player.ini` and
`ui/default/guisettings.ini` before testing, restore them after the game exits,
and preserve saves. Keep game files, private logs and test artifacts out of Git.

## Releases

CI builds, tests and packages pushes and pull requests. Version tags also
create a draft prerelease.

1. Update the CMake version, changelog and `docs/release-notes.md`.
2. Commit, tag `vX.Y.Z` and push the tag. CI checks that the tag matches the version.
3. Download the draft's exact ZIP and verify its checksum.
4. Test that packaged DLL in-game on both supported builds. Record the artifact
   identity, tested behavior and important gaps.
5. Resolve any antivirus detection before publishing a known blocked build.
   Do not bypass quarantine or weaken protection.
6. Add the concise testing summary, review the player-facing release notes,
   then publish the draft.

CI success alone does not establish gameplay compatibility. If testing used a
different artifact, say so explicitly. Do not silently turn earlier source
testing into a claim that the final package was tested in-game.

Keep published ZIPs, checksums and tags unchanged when editing documentation.
Update the repository and GitHub release text; documentation inside an existing
ZIP remains the version originally shipped.

## Documentation

Patchwork is the publication for players and contributors. The independent
`nwn2-research` repository holds the detailed investigation record.

- **User guides:** explain what the player sees, how to install or configure it,
  supported versions, useful troubleshooting and relevant limits.
- **Release notes:** lead with that version's changes, then installation or upgrade
  instructions and important caveats. Link to a short testing/checksum summary.
- **Developer docs:** retain build instructions, architecture, patch safety and
  enough rationale to maintain the source.
- **Research:** keep experiment chronology, intermediate artifacts, checkpoint
  IDs, per-action timings, tool problems, failed approaches, local environment
  details and preservation receipts there.

Write for the reader's task. Avoid internal process vocabulary in user-facing
text. Do not repeat historical caveats on every page, advertise unshipped work,
or copy personal decisions and machine-specific paths into this repository.
Keep testing limitations honest and proportionate. Release summaries should
remain understandable without access to research notes.

Apply this guidance to every published release page as well as the current
README. Preserve detailed evidence in research before shortening existing records.
