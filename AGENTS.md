# NWN2 Patchwork working rules

This is the independent public release repository. Read README.md and
docs/development.md. Target only the two hashes in docs/compatibility.md.
Keep the source and build self-contained; no research submodule or game assets.

The user chose MIT and the description in README. Initial scope is the proved
display fixes. Audio, connection-check and DirectX timing fixes are future work.
Rendering may pause while dragging if image/input recover after release.
Native dragging is the default; custom dragging and failed UI redesign stay off.

Preserve runtime source provenance. Changes need exact expected-byte/build checks
and appropriate offline/runtime verification. Never treat CTest as gameplay proof.
Do not run a second game or terminate an existing user's process. Runtime tests
must preserve/restore the user's nwn2.ini, nwn2player.ini and ui/default/guisettings.ini
after confirming the owned game has exited. Keep private logs/binaries under local/.
Do not archive the old remote or change EPOS as part of routine release work.

Create releases as drafts until the actual packaged DLL has passed a
bounded smoke check on both supported builds. Document limitations honestly.
The v0.1.0 candidate is held: Defender quarantined the launcher after the v2
smoke check; original check is incomplete. Read docs/releases/v0.1.0-validation.md.
Do not restore/repack/rename to bypass quarantine or change Defender settings.
Microsoft has completed the old-launcher case without reproducing a detection;
cloud/client show no malware, while the file-level determination still says
Pending. This is not an explicit correction of the original behavior detection.
Private receipt is in local/microsoft-submission.json. Do not submit a duplicate.
User subsequently authorized pursuing a conventional drop-in production loader;
retain the research probes/launcher for diagnostics. Prove safe initialization
timing and both-build runtime behavior before replacing the public package.

The v0.2.0 drop-in CI artifact has completed bounded both-build acceptance; see
docs/releases/v0.2.0-validation.md. Preserve that exact ZIP/DLL; documentation
updates after the tag do not change the tested binary. Do not restart broad
research permutations without a concrete defect. The old launcher remains held;
no universal antivirus-clearance claim is warranted.

For a fresh local chat doing reverse engineering/probing, the independent research
checkout's START-HERE.md maps the source, inspectors, preserved probes and optional
Ghidra MCP. On this machine it is D:/workspace/nwn2-research; game files are in
G:/GoG Galaxy/Games/NWN2 Complete. The release build has no dependency on that repo.
For ordinary release work use this repo's docs/development.md and docs/loading.md.
