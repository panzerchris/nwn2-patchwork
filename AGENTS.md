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
The user approved Microsoft review and completed sign-in/human verification;
submission is confirmed, determination pending. Private receipt is in
local/microsoft-submission.json. Do not submit a duplicate or ask again.
User subsequently authorized pursuing a conventional drop-in production loader;
retain the research probes/launcher for diagnostics. Prove safe initialization
timing and both-build runtime behavior before replacing the public package.

The v0.2.0 drop-in CI artifact has completed bounded both-build acceptance; see
docs/releases/v0.2.0-validation.md. Preserve that exact ZIP/DLL; documentation
updates after the tag do not change the tested binary. Do not restart broad
research permutations without a concrete defect. The old launcher review is
still pending and no universal antivirus-clearance claim is warranted.
