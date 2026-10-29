# Loading architecture

The game already imports `DirectInput8Create` from `dinput8.dll`. The release places
a forwarding DLL beside the game and configures Patchwork inside that process
when the game makes its first DirectInput call. There is no bootstrap work in
`DllMain`, no separate launcher and no process creation/debugging/injection API
in this delivery path. The original diagnostic path remains a separate source target.

The forwarding library resolves Windows' own `dinput8.dll` using the full
`GetSystemDirectoryW` path and `LOAD_LIBRARY_SEARCH_SYSTEM32`. A basename-only
lookup could resolve the already-loaded local DLL recursively. All six system
exports are forwarded with the x86 names, calling conventions and ordinals.

An `INIT_ONCE` gate initializes the fixes only in `nwn2main.exe`. Startup checks
the exact EXE SHA-256, reads the supplied INI relative to the DLL, rejects known
conflicting proxies and generates a small session configuration/log under Local
AppData. The existing runtime independently checks the image hash and expected
patch bytes. A failure shows a startup message and terminates that newly starting
process, because a write failure could have installed only some hooks.

Other executables using DirectInput receive forwarding only. There is no ASI
plugin search, arbitrary plugin loading, proxy chaining or third-party loader.
Do not replace an existing `dinput8.dll` without resolving that mod conflict.

Offline tests load the proxy first and compare actual creation results against
Windows' implementation. Real-game acceptance must additionally record the first
caller/thread and verify that initialization precedes the relevant display
operations on both builds. `bootstrap.json` provides the caller/thread receipt;
the quiet runtime log identifies the enabled fixes. CTest alone cannot prove timing.

This design removes the old debugger/remote-process injection behavior from the
distributed package. It does not guarantee an antivirus verdict: the display
fixes still install in-process hooks. Do not weaken security settings if a build
is detected; record its exact hashes and request vendor review.
