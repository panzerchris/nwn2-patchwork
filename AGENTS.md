# NWN2 Patchwork working rules

This is the independent public MIT release repository. Read README.md and
docs/development.md. Keep source and builds self-contained; no research submodule,
game assets or dependency on a private checkout.

## Documentation audience

Patchwork is the human-friendly publication for players and contributors.
Follow the documentation guidance in docs/development.md for every user guide
and every published release version.

Explain features, setup, configuration, supported builds, troubleshooting and
useful limitations in plain language. Keep checksums and a concise testing
summary. Put architecture and build requirements in developer documentation.
Preserve detailed experiment chronology, intermediate artifacts, checkpoint IDs,
tool failures, failed approaches and local preservation records in the independent
nwn2-research repository. Do not publish personal decisions or workstation paths.
Do not weaken evidence claims while simplifying prose.

## Runtime boundaries

Target only the two executable hashes in docs/compatibility.md. Shipped scope is
display fixes and the multiplayer login bypass. Audio and DirectX timing remain
future work; do not advertise them as shipped.

The display core is frozen pending a concrete defect. Native dragging is the
default; temporary rendering pauses are acceptable if picture and controls
recover after release. Custom dragging and experimental HUD changes stay off.
Preserve source provenance and avoid unrelated runtime rewrites.

Changes need exact expected-byte/build checks, correct ABI and ownership, and
appropriate automated and in-game verification. CTest is not gameplay proof.
Do not run a second game or attach to or terminate an existing user's game.
Preserve nwn2.ini, nwn2player.ini and ui/default/guisettings.ini byte-for-byte;
restore only after the owned game exits. Preserve saves. Keep private files under
ignored local/. Do not change unrelated software or legacy repositories.

For detailed reverse engineering or test procedures, use the independent
nwn2-research checkout's START-HERE.md and current STATUS. Resolve game paths
through that workspace's configuration; do not infer them from a repository path.

## Release preservation

Create releases as drafts until the actual packaged DLL has passed a bounded
smoke check on both supported builds. Document limitations honestly and keep
candidate testing distinct from final-package testing.

The 0.2.0 published DLL passed bounded checks on both builds. For 0.3.0, gameplay
evidence comes from an earlier build with the same runtime source; the final
package was verified but not retested in-game. These are historical coverage
limits, not instructions to repeat tests. See docs/releases/ for concise records.

Do not rebuild or replace published assets, or move tags, for documentation edits.
The unpublished 0.1.0 launcher remains held after its behavior detection.
Microsoft closed its review without reproducing the detection; this is not an
explicit behavior-rule correction or universal antivirus clearance.
Never restore, rename or repack quarantined binaries to bypass protection,
change security settings, or submit duplicate reviews as routine release work.
