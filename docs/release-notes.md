# NWN2 Patchwork 0.3.0

Multiplayer Sign In now skips the obsolete authentication request through the
game's native service-unavailable path. Acknowledge the existing warning to reach
the multiplayer menu immediately. The supplied INI enables this with
`[Multiplayer] SkipLegacyLoginCheck=1`; set `0` to retain the native wait. Existing
display-only INIs keep the native check until the option is added.

Uses the same drop-in **dinput8.dll** as 0.2.0. No separate launcher, debugger or
on-disk game EXE patch is needed. The existing display fixes remain unchanged.
Download **nwn2-patchwork-0.3.0-windows-x86.zip**. Close NWN2, verify any existing
`dinput8.dll` belongs to Patchwork, then extract the DLL and `nwn2-patchwork` folder
beside `nwn2main.exe`. Use the x86 package on 64-bit Windows too. Keep your custom
display settings when upgrading and add the new multiplayer section if retaining
an older INI. Do not overwrite another mod's loader.

Supports only the exact documented GOG 1.23 and GOG 1.23 v2 executable hashes.
The login patch checks the build and full instruction context before changing
five bytes in memory. It does not restore the public browser or authentication
service. Actual game-server connections, passwords and timeouts have separate
native paths.

The branch CI DLL passes bounded login/gameplay checks on both builds. V2 also
passes actual local dedicated-server joining, wrong-password rejection, LAN
rejoin and movement input. All 18 offline tests pass, including actual installer
write/refusal/protection fixtures. The versioned CI package has a separate bounded
smoke record linked from [validation](https://github.com/panzerchris/nwn2-patchwork/blob/main/docs/validation.md).

This is a prerelease with documented limits: isolated offline login, actual
original-build server joining, hostname connections, multiple players and wider
network/mod combinations remain untested. See [multiplayer evidence](https://github.com/panzerchris/nwn2-patchwork/blob/main/docs/multiplayer.md).
No audio or DirectX timing changes are included. Native dragging can briefly
pause rendering; image/input should recover after release. Existing display and
antivirus limitations remain in the validation records.
