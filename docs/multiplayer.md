# Multiplayer login check

The unreleased source candidate adds an opt-in bypass for the obsolete BioWare
Community Login request. The exact CI DLL passes bounded login and general
regression checks on both supported builds. Real server joining and password
rejection remain untested; this is not full multiplayer compatibility acceptance.

While the game is closed, add:

```ini
[Multiplayer]
SkipLegacyLoginCheck=1
```

An absent section or `0` retains the native check. Existing display-only INIs
therefore keep their current behavior. The existing `[Display]` section remains
required.

With the option enabled, ordinary Sign In takes the game's authentication
unavailable path before starting its auth request. The native service-down
warning and OK acknowledgement remain. The username/password screen still
accepts input, and the native continuation opens the multiplayer menu.

This does not restore the public server browser or authentication service. The
patch targets ordinary login; Direct Connect, LAN, real server passwords and
general network timeouts have no patch sites. Their complete runtime behavior
still needs the connection coverage below. New-account creation follows a
separate branch and is not bypassed.

## Implementation evidence

The supported original/v2 SHA-256 values remain those in compatibility.md.
Independent extraction from both files finds the same context at RVA `0x31b60e`:

```
8b ce e8 8b d3 ff ff 84 c0 8b 0d 9c e1 a0 00 74 de
```

The five-byte call at RVA `0x31b610` is replaced with `32 c0 90 90 90`, clearing
AL and preserving instruction length. The caller tests AL and follows its
existing auth-unavailable branch. There are no stack arguments at this callsite;
the original call receives the connection object in ECX. The new-account branch
at VA `0x71b5ca` takes another path. The frame dispatcher maps login operation1
to VA `0x71b510`; actual server connection is operation2, VA `0x71d1b0`.

The loader checks the build and complete context before the display initializer,
then the separate multiplayer module checks again before writing. Its failures
use the existing startup refusal policy. The public diagnostic launcher does
not integrate this feature and remains excluded from the package.

## Offline verification

Win32 Release build and 18 offline tests pass. Offline fixtures check the two
supported build identities and reject every altered byte in the context. The
actual installer runs against an owned executable-memory page: each refusal
preserves the entire page, success changes only the five intended bytes, and
the original execute/read protection is restored. An already patched context
is refused. These checks do not simulate an OS protection/cache-flush failure.
Configuration fixtures cover absent/enabled/disabled options, invalid values
and unknown keys.

## Exact CI artifact checks — 2026-10-29

Tested source: `41b518265d83cb5e3033246c479e9e9396f4b036`.
[CI run 40965988302](https://github.com/panzerchris/nwn2-patchwork/actions/runs/40965988302)
passes build, all 18 tests and packaging. The downloaded ZIP matches its checksum;
all 16 manifest files, source commit, x86/Release identity and absence of extra
files verify. The installed package INI matches the downloaded INI before the
explicit test opt-in.

- ZIP SHA-256: `6dbe43280aa04bbfad50a30ffb6cc2599d1a980933b60f878e27bd5db3a7265a`.
- DLL SHA-256: `fc34facce8a26838c7046b9ba8eca0434a66d63ad8fb215526d48b65836507d1`.

This branch artifact retains project version 0.2.0. It is an unreleased candidate,
distinct from the published 0.2.0 ZIP/DLL; no candidate tag or release was created.
These results apply to the hashes above, not a future rebuilt package.

| Exact CI DLL check | Observation |
| --- | --- |
| Original enabled | Native unavailable warning/OK visible by 1.810s; OK-to-menu capture 1.053s. Existing Tutorial load, movement input and native title-move recovery pass. Interactive beyond 219s after login without a late warning. |
| v2 enabled | Native warning/OK visible by 1.000s, 1.405s and 0.937s in separate preserved sessions. Final OK-to-menu capture 1.413s. Tutorial load, movement input and native title-move recovery pass. Menu input remains usable beyond 203s after login. |
| v2 unreachable Direct Connect | Bare local alias `127.0.0.2` resolves visibly to `127.0.0.2:5121`. Its separate find-server operation retains a running countdown and completes with native result 5 about 30.84s after OK; visible message is “Server is not responding (timed out).” OK returns to Direct Connect; Cancel returns to usable multiplayer menu. |
| v2 UI boundaries | Login Cancel/reopen and empty new-account form/Cancel work; no account is submitted. LAN browser/Cancel and hosting setup/Cancel work; no server is started. |

The unreachable endpoint is owned loopback; a UDP 5121 bind precheck succeeds and
the socket closes before the attempt. The connection target is local; no public
server fixture is used. Read-only state sampling is separate from visible/input
observations and uses no debugger or injected helper. A live server handshake was
not exercised.

Exact `127.0.0.1` is unsuitable for this negative test: both stock v2 and candidate
can leave a dimmed Direct Connect screen without an active operation. Static
analysis confirms native special handling of that exact string before operation
startup; the precise dynamic branch was not captured. An earlier malformed port
separator resolved to `255.255.255.255` and is excluded from controlled endpoint
acceptance. Neither attempt is counted as a successful connection test.

All owned sessions exit normally. Preserved user/package settings restore
byte-for-byte, save audits find all 12,424 files unchanged, canonical v2 is
restored after comparisons, and the previous installed 0.2.0 DLL is restored.

## Earlier local candidate comparisons

Initial local runtime candidate DLL SHA256 (before the installer test seam):
`d5ad28062054d2418d9437c0e518f2f6136d8fa534fb786531f2cdf194b18ff7`.

| Local candidate check | Observation |
| --- | --- |
| v2 enabled | Three login/back cycles; first warning by 1.111s, third by 0.838s. First OK-to-menu by 0.769s. Direct Connect/Cancel and LAN browser work. Responsive beyond 150s after first login. |
| Original enabled | Three cycles; warning by 0.195/1.732/1.686s. Direct Connect/Cancel and LAN browser work. Tutorial loads, movement clicks work, native title move recovers visible image/input. Responsive beyond 137s after third login. |
| Original disabled | Original call bytes intact; native countdown and result 18 completion observed about 31s after Sign In. Native warning/OK reaches the same menu and Back accepts input. |
| v2 disabled | Original call bytes intact; native countdown and result 18 completion observed about 31s after Sign In. Native warning/OK reaches the same menu and Back accepts input. |

Tool input/capture overhead is included; these are upper observation bounds,
not frame-accurate latency measurements. Some captures exceed the proposed
one-second target but remain under the two-second investigation threshold.
The owned sessions exit normally through Alt+F4; preserved settings restore
byte-for-byte and all 12,424 save files audit unchanged. Canonical v2 is restored
after the original comparisons.

## Remaining acceptance gates

Successful joining, actual server-password rejection, hostname connection and
LAN/hosting with another player remain untested. No authorized server or second
machine is available. The controlled unreachable-endpoint check above covers v2
only. Source-level separation and menu access do not establish a successful join.

Isolated offline login remains untested: the test shell lacks elevation for a
per-process firewall denial. No shared adapter, global DNS/hosts or security
settings were changed. A suitable isolated environment is needed for this case.

The bypass precedes request/timer creation, so it creates no authentication
request to cancel or deliver a late completion. Repeated login/back and delayed
interaction checks pass as identified above; manually cancelling a pending native
auth request and an injected completion-race experiment were not performed.

Keep the feature opt-in and the PR in draft while these gates remain open. A
future versioned release needs its own exact packaged-artifact acceptance; do not
promote the current evidence into a full multiplayer compatibility claim.
