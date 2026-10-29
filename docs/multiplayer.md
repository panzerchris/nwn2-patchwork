# Multiplayer login check

The source candidate adds an opt-in bypass for the obsolete BioWare Community
Login request. Runtime acceptance is in progress; this is not a release
compatibility claim.

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

This does not restore the public server browser or authentication service. It
does not change Direct Connect, LAN, real server passwords or general network
timeouts. New-account creation follows a separate branch and is not bypassed.

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

## Verification in progress

Win32 Release build and 18 offline tests pass. Offline fixtures check the two
supported build identities and reject every altered byte in the context. The
actual installer runs against an owned executable-memory page: each refusal
preserves the entire page, success changes only the five intended bytes, and
the original execute/read protection is restored. An already patched context
is refused. These checks do not simulate an OS protection/cache-flush failure.
Config
fixtures cover absent/enabled/disabled options, invalid values and unknown keys.

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

Isolated offline-network, successful server join/password rejection
and packaged-CI acceptance remain pending. No second machine/server is available
for the join/password tests. Source-level separation alone does not prove those
runtime cases. Do not describe menu access as a successful server connection.
