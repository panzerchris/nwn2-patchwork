# Changelog

## 0.1.0 — initial display prerelease

- Configurable display lifecycle fixes for the exact GOG 1.23 and 1.23 v2 builds.
- Guard empty restore sizes, retain native resize lifecycle and synchronize windowed dimensions.
- Preserve window placement across mode transitions and fit windowed resolution Apply to the work area.
- Correct minimap saved-height clearing and Graphics resolution-control handling.
- Revert the identified v2 resize regression on that build only.
- Optional Alt+Enter borderless presentation, including an EPOS Connect workaround.
- Native launcher, strict configuration checks, exact executable/patch-site guards,
  source build, offline tests and automated Windows release packaging.

This version contains no audio, connection-check or DirectX timing fix.
