# Faster multiplayer login

Patchwork 0.3.0 skips the long wait for the obsolete BioWare Community Login
service. This is enabled in the supplied configuration.

Click **Sign In**, then click **OK** on the game's service-unavailable warning.
You can then use the multiplayer menu. The login screen and warning still appear.

## Enable or disable

With the game closed, edit `nwn2-patchwork/nwn2-patchwork.ini`:

```ini
[Multiplayer]
SkipLegacyLoginCheck=1
```

Use `0` to keep the original wait. If you kept an older INI, add this section;
a missing option keeps the game's original behavior. Keep your `[Display]`
section and display settings.

## What this changes

The feature skips the old account-service check when you sign in.
It does not restore the public server browser or account creation.
Server connections, server passwords and connection timeouts are not bypassed.

For a dedicated server running on the same PC, use its advertised local network
address. The game's special handling of `127.0.0.1` can leave Direct Connect
stuck on a dimmed screen.

## Testing and limits

Login and basic gameplay were checked on both supported GOG builds. Local
dedicated-server joining, incorrect-password handling, LAN rejoining and
connection timeout recovery were also checked on GOG v2.

Offline login, original-build server joining, hostname connections, multiple
players and hosting from inside the game have not been tested. Wider network
and mod compatibility remains untested.

These gameplay checks used a pre-release build with the same runtime source;
the final 0.3.0 package was verified separately but was not retested in-game.
See [0.3.0 testing and checksums](releases/v0.3.0-validation.md).
