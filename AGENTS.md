# Agent Guide

## Project purpose

This is a [TheSkyX X2 plugin](https://www.bisque.com/theskyx/) that adds support for the **Starlight Xpress AO** (active optics / tip-tilt mirror) unit. TheSkyX has native AO support but does not publish the required driver interface header; the interface was reconstructed by binary analysis of the TheSkyX executable. See [NOTES.md](NOTES.md) for the full methodology and findings.

The plugin is a standard C++ shared library (`libSXAO.so` / `.dylib` / `.dll`) that TheSkyX loads at runtime. It communicates with the hardware over a virtual serial port (FTDI USB-serial, 9600 8N1).

## Repository layout

```
main.h / main.cpp          Plugin entry points (sbPlugInName2, sbPlugInFactory2)
sxao.h / sxao.cpp          Low-level hardware class — serial I/O, command encoding
x2sxao.h / x2sxao.cpp      TheSkyX driver wrapper — implements all TSX interfaces
SXAO.ui                    Qt Designer UI file for the settings dialog
version.h                  Version constants; BUILD_NUMBER injected by CI
licensedinterfaces/        Vendored TheSkyX SDK headers (Software Bisque copyright)
adaptiveopticslist StarlightXpress.txt   Hardware list entry consumed by TheSkyX
install.sh                 Build-and-install helper for local development
.github/workflows/build.yml  CI: builds Linux/macOS/Windows, auto-releases on main
```

## Building

**Local (Linux or macOS):**
```sh
make
```
Produces `libSXAO.so` (Linux) or `libSXAO.dylib` (macOS). No external dependencies beyond a C++11 compiler.

**Install into TheSkyX:**
```sh
./install.sh          # build + install
./install.sh --uninstall
```

CI produces all three platform builds automatically on every push to `main` and publishes a GitHub release tagged `v1.<commit-count>`.

## Code style

- **C++11**, compiled with `-Wall -Wextra`. Keep it that way — no C++14/17 features; TheSkyX's own ABI is old and conservative.
- **No exceptions, no RTTI** beyond what the SDK interfaces use (`dynamic_cast` in `queryAbstraction` is intentional and required by the X2 protocol).
- **Two-layer architecture**: `SXAO` owns the wire protocol and knows nothing about TheSkyX; `X2SXAO` owns the TheSkyX interface and delegates all hardware ops to `SXAO`. Keep that boundary clean.
- **Error returns**: use `SB_OK` (0) and the `ERR_*` constants from `sberrorx.h`. Do not throw.
- **Serial I/O**: all commands go through `SXAO`. Do not call `SerXInterface` methods from `X2SXAO` directly.
- **UI strings**: set via `X2GUIExchangeInterface::setText`. Do not add Qt-specific code — the UI is rendered by TheSkyX's own Qt runtime.
- **Logging**: use `SXAO::log()` at the appropriate level (0 = off, 1 = errors, 2 = trace, 3 = raw hex). Log to `~/SXAOLog.txt` via the existing infrastructure; do not add a separate logging path.

## The undocumented interface problem

`AdaptiveOpticsDriverInterface` and `AdaptiveOpticsTipTiltInterface` are not in the public X2 SDK. Our definitions in `x2sxao.h` are reconstructed from vtable analysis and must match TheSkyX's internal layout exactly — if they don't, the plugin will crash or silently fail to load.

**Do not change the base class list or virtual method order in `AdaptiveOpticsDriverInterface` or `AdaptiveOpticsTipTiltInterface` without re-verifying against the binary.** See [NOTES.md](NOTES.md) for how to do that.

## Vendored headers

`licensedinterfaces/` contains 16 headers from the TheSkyX X2 SDK (copyright Software Bisque). They are vendored because the SDK is not distributed publicly. One file (`x2guiinterface.h`) has a two-line patch to add an `X2_FLAT_INCLUDES` preprocessor guard; this is noted in the file. Do not upgrade these headers without re-checking that patch.

## Version scheme

`version.h` defines `PLUGIN_VERSION_MAJOR = 1`. CI appends the git commit count as `BUILD_NUMBER`, producing versions like `v1.42`. Bump `PLUGIN_VERSION_MAJOR` manually only for significant/breaking changes.
