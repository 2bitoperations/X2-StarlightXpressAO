#pragma once

// Plugin version — increment when cutting a release.
#define PLUGIN_VERSION_STRING "1"

// Numeric form returned by driverInfoVersion() (TheSkyX expects a double).
#define PLUGIN_VERSION_DOUBLE  1.0

// Git commit hash injected at build time via -DGIT_HASH=\"...\".
// Falls back to "unknown" when building outside a git tree.
#ifndef GIT_HASH
#define GIT_HASH "unknown"
#endif
