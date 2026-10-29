# Development and releases

`src/probe.cpp` and its geometry headers preserve the tested runtime implementation.
The legacy filename/internal export/schema remain to keep source provenance clear;
the public binary is named `nwn2-patchwork.dll`. Avoid cosmetic runtime rewrites
while establishing release parity. Runtime fixes are separate from launcher policy.

- `src/launcher.cpp`: public paths/configuration/build checks and startup.
- `src/inject_child.h`: the tested own-child entry-point injection machinery.
- `src/config.h`: public [Display] options mapped to the inherited runtime schema.
- `src/supported_builds.h`: public preflight allowlist; runtime independently verifies it.
- `tests/`: offline fixtures; no game installation is needed for CTest.
- `config/`: supplied public configuration.
- `scripts/package.ps1`: explicit install, manifest, archive and checksum.

Add future fix families with separate source modules and INI sections when their
behavior is proved. Do not advertise proposed fixes as shipped functionality.
Research remains independent; there is no submodule and no build dependency on it.

## Build and packaging

Use the README commands or open the generated Visual Studio solution. Use Win32,
not x64: hooks include x86 assembly and verified fixed-address ABI assumptions.
The CRT is statically linked. Debug and release builds are distinct.
Never distribute a modified game EXE or game assets.

The default workflow builds/tests/packages on pushes and pull requests. Tagged
versions also create a **draft prerelease** with the ZIP and its checksum. This
keeps CI compilation separate from manual real-game acceptance. Release creation
has contents-write permission only on the tagged release job.

1. Update the CMake project version and CHANGELOG.
2. Commit, tag `vX.Y.Z` and push the tag. CI checks tag/version consistency.
3. Wait for CI; download the draft release's exact ZIP and verify SHA-256.
4. Smoke-test those binaries on both supported EXEs. Record commit/hash/actions,
   correct image/input, normal exit and any limits in a validation record.
5. Resolve any antivirus detection rather than publishing a known blocked build.
   Request a vendor review with the exact binary/source hashes; do not work around
   quarantine or ask users to weaken security settings.
6. Add the validation record and release notes, then publish the draft prerelease.

A green CI run alone is insufficient for a real-game compatibility claim. Wider
user testing can turn a prerelease into a stable release later. Do not build from
private research paths or reuse its relocated CMake cache.
