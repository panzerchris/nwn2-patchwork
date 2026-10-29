# Changelog

## 0.3.0 — multiplayer login

- Supply `[Multiplayer] SkipLegacyLoginCheck=1` to take the native
  authentication-unavailable path before starting the obsolete ordinary-login
  request. The native warning and OK acknowledgement remain.
- Enable it in the shipped INI; `0` or an absent section/option preserves the
  native check for existing configurations.
- Guard the five-byte change with both supported executable hashes and the full
  expected instruction context. Server connections and new-account requests use
  separate paths. See [evidence and limits](docs/multiplayer.md).

The branch CI DLL passes bounded login/gameplay checks on both supported builds.
V2 additionally passes actual local-server joining, password rejection, LAN rejoin
and gameplay, plus a controlled unreachable timeout. Isolated offline login,
original-build actual joining and wider network/mod combinations remain untested.
Versioned package identity and its bounded smoke checks are recorded separately.

## 0.2.0 — drop-in loading

- Replace the distributed launcher with a self-contained `dinput8.dll` forwarding
  library. Start NWN2 through its ordinary shortcut; no debugger or remote-process
  injection is used by the release DLL.
- Initialize on the game's first DirectInput call, outside `DllMain`, with exact
  executable and patch-site checks. Forward all six system exports.
- Retain the previous launcher as an opt-in diagnostic source target; omit it
  from the release ZIP. Keep the tested display implementation unchanged.
- Package the DLL beside `nwn2main.exe`, with configuration/docs in `nwn2-patchwork`.

The 0.1.0 launcher remains held for Microsoft review. A different loading method
is not a guarantee against antivirus false positives.

## 0.1.0 — initial display candidate (held, unpublished)

- Configurable display lifecycle fixes for the exact GOG 1.23 and 1.23 v2 builds.
- Guard empty restore sizes, retain native resize lifecycle and synchronize windowed dimensions.
- Preserve window placement across mode transitions and fit windowed resolution Apply to the work area.
- Correct minimap saved-height clearing and Graphics resolution-control handling.
- Revert the identified v2 resize regression on that build only.
- Optional Alt+Enter borderless presentation, including an EPOS Connect workaround.
- Native launcher, strict configuration checks, exact executable/patch-site guards,
  source build, offline tests and automated Windows release packaging.

This version contains no audio, connection-check or DirectX timing fix.
