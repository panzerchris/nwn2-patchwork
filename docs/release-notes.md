# NWN2 Patchwork 0.3.0

**Skip the long wait when signing into multiplayer.** Click OK on the game's
service-unavailable warning to reach the multiplayer menu. This is enabled by
default; the display fixes from 0.2.0 are unchanged.

Download **nwn2-patchwork-0.3.0-windows-x86.zip** below. Use x86 on 64-bit Windows too.

1. Close NWN2.
2. Extract `dinput8.dll` and the `nwn2-patchwork` folder beside `nwn2main.exe`.
   Do not overwrite another mod's `dinput8.dll`.
3. Start the game through your usual shortcut.

**Upgrading from 0.2.0?** Back up your Patchwork INI before extracting if you
have customized it. If you keep the old INI, add:

```ini
[Multiplayer]
SkipLegacyLoginCheck=1
```

Set the option to `0` to keep the original login wait.

Supports the [documented GOG 1.23 and 1.23 v2 builds](https://github.com/panzerchris/nwn2-patchwork/blob/main/docs/compatibility.md),
not Enhanced Edition. This does not restore the public server browser or
account service. Server passwords and connection timeouts still apply.

This is a prerelease with limited multiplayer coverage. Gameplay checks used an
earlier build with the same runtime source; the final package was verified but
was not retested in-game.
See [testing and checksums](https://github.com/panzerchris/nwn2-patchwork/blob/main/docs/releases/v0.3.0-validation.md)
for coverage and remaining gaps, or the
[user guide](https://github.com/panzerchris/nwn2-patchwork#readme) for settings and known issues.

ZIP SHA-256:
```text
9f5374bfb7ed439f2737e0e4183b7e433483caf38fd7611b2cd00f834a4969cd
```
