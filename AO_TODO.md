# Adaptive Optics (AO) Implementation

## Investigation Chain

You can reproduce or expand this work by following these steps.

1. Search the X2-Examples directory for keywords. We used `grep -rnIE "DirectGuide|Startup|Home|FindHome"`. This found `dapiFindHome` for Domes and the error code `ERR_MOUNTNOTHOMED` (231), but nothing for mount homing.
2. Examine the TheSkyX binary and bundled mount plugins. On Linux, these are at `~/TheSkyX/TheSkyX` and `~/TheSkyX/Resources/Common/PlugIns64/MountPlugIns/*.so`. On macOS, the binary is at `/Applications/TheSkyX Professional Edition.app/Contents/MacOS/TheSkyX`.
3. Use `strings` and `c++filt` to identify demangled C++ symbols. Running `strings ~/TheSkyX/TheSkyX | c++filt` reveals vtables and virtual method declarations for classes that aren't in the SDK.
4. Identify specific interface structures. We found clear signatures for `FindHomeInterface` and `DirectGuideInterface` within the binary's symbol table.
5. For AO specifically: run `strings <TSX binary> | grep -i "adaptive\|tiptilt"` and `nm -arch x86_64 <TSX binary> | c++filt | grep -i adaptive` to find the interface names and typeinfo symbols.

## Adaptive Optics (AO) Interface

TheSkyX natively supports an undocumented `AdaptiveOpticsTipTiltInterface/1.0`. By reverse engineering the TSX binary, we found that plugins can expose this interface for arbitrary AO devices, not just SBIG.

### Confirmed Facts from Binary Analysis (macOS TheSkyX Professional Edition)

- `DT_AO = 11` **already exists** in the SDK's `driverrootinterface.h`. The device type is officially defined; only the driver interface header is missing from the SDK.
- The `queryAbstraction` name is the **full versioned string** `"com.bisque.TheSkyX.AdaptiveOpticsTipTiltInterface/1.0"` — confirmed directly from binary strings. Earlier notes suggesting just `"AdaptiveOpticsTipTiltInterface"` are incorrect.
- `AdaptiveOpticsDriverInterface` is the **base class** for AO plugins (analogous to `FocuserDriverInterface` for focusers). It is not shipped in the SDK and must be defined locally.
- The factory entry points are the standard X2 ones: `sbPlugInName2` and `sbPlugInFactory2`.
- Three built-in TSX classes inherit from `AdaptiveOpticsDriverInterface`:
  - `NoAdaptiveOptics` — the "no device selected" stub
  - `AdaptiveOpticsSimulator` — the built-in software simulator
  - `EmbeddedAODevice` — SBIG's camera-integrated AO

### Confirmed Facts from Vtable Analysis

`nm -arch x86_64 <TSX binary> | c++filt` exposes typeinfo symbols:
- `AdaptiveOpticsDriverInterface` typeinfo at `0x1027c4a30` (x86_64, no ASLR)
- `AdaptiveOpticsTipTiltInterface` typeinfo at `0x1027c4a20`

Dumping the `__DATA_CONST __const` section with `otool -arch x86_64 -s __DATA_CONST __const` and locating the `NoAdaptiveOptics` vtable reveals its secondary vtable structure. The vtable for `AdaptiveOpticsTipTiltInterface` is a **6-entry secondary vtable**, which exactly matches the 6 pure virtual methods documented below. This confirms the method list.

The `NoAdaptiveOptics` primary vtable has **22 entries** and `AdaptiveOpticsSimulator` has **30 entries**. The primary vtable layout for `AdaptiveOpticsDriverInterface` is not fully resolved (no exported vtable symbols). Our best reconstruction based on the pattern from other driver interfaces is that it inherits from `DriverRootInterface`, `LinkInterface`, `HardwareInfoInterface`, and `DriverInfoInterface`.

**If TSX crashes on plugin load**, the primary vtable layout may be wrong. Compare the entry counts above and adjust the `AdaptiveOpticsDriverInterface` definition in `x2sxao.h`.

### Reconstructed AdaptiveOpticsTipTiltInterface

```cpp
// queryAbstraction key: "com.bisque.TheSkyX.AdaptiveOpticsTipTiltInterface/1.0"
class AdaptiveOpticsTipTiltInterface {
public:
    virtual ~AdaptiveOpticsTipTiltInterface() {}
    virtual int GetMotorLimits(unsigned short& nRaMin, unsigned short& nRaMax,
                               unsigned short& nDecMin, unsigned short& nDecMax) = 0;
    virtual int GetAOTipTilt(short* pnRa, short* pnDec) = 0;
    virtual int GetAOTipTiltPercent(float* pdRa, float* pdDec) = 0;
    virtual int CCAOTipTiltAbsolute(unsigned short nRa, unsigned short nDec) = 0;
    virtual int CCAOTipTiltOffset(short nRaOffset, short nDecOffset) = 0;
    virtual int OnCenterAO() = 0;
};
```

### Reconstructed AdaptiveOpticsDriverInterface

This is our best reconstruction. It is not from the SDK. It may need adjustment if TSX rejects the plugin.

```cpp
class AdaptiveOpticsDriverInterface
    : public DriverRootInterface
    , public LinkInterface
    , public HardwareInfoInterface
    , public DriverInfoInterface
{
public:
    virtual ~AdaptiveOpticsDriverInterface() {}
    virtual DeviceType deviceType(void) { return DriverRootInterface::DT_AO; }
    virtual int queryAbstraction(const char* pszName, void** ppVal) = 0;
    // LinkInterface, HardwareInfoInterface, DriverInfoInterface pure virtuals...
};
```

## Adaptive Optics (AO) Scaffolding

For TheSkyX to present a custom Adaptive Optics plugin in the UI and load the library, specific files and directories must exist. This is based on reverse-engineering the TSX `hardwarelist.txt` format and binary strings.

### 1. Plugin Directory

The compiled shared library (`libSXAO.so` on Linux, `libSXAO.dylib` on macOS, `libSXAO.dll` on Windows) must be placed in:

- **macOS:** `/Applications/TheSkyX Professional Edition.app/Contents/PlugIns/AdaptiveOpticsPlugIns/`

This directory **exists but is empty** in the stock TSX installation — it was designed for third-party plugins. No creation step is needed on macOS.

The earlier note about `PlugIns64/` was based on Linux documentation. On macOS the path is just `PlugIns/AdaptiveOpticsPlugIns/`.

### 2. Hardware List File

TheSkyX enumerates devices using text files in:

- **macOS:** `/Applications/TheSkyX Professional Edition.app/Contents/Resources/Common/Miscellaneous Files/`

The built-in list lives at `adaptiveopticslist.txt` in that directory. For a third-party plugin, create a separate file named `adaptiveopticslist <YourCompany>.txt` (e.g., `adaptiveopticslist StarlightXpress.txt`).

### 3. List File Format

The file must follow the standard X2 hardware list pipe-separated format. The built-in `adaptiveopticslist.txt` uses **Version 1** (no OS flags). Third-party plugins should use **Version 2** (with OS flags in the last three columns).

Our device list file (`adaptiveopticslist StarlightXpress.txt`):

```text
//Version|Manufacturer|Model|Comment|MapsTo|PlugInDllName|X2Developer|Windows|Mac|Linux|
2|StarlightXpress|SX AO|Starlight Xpress Active Optics| |libSXAO|SX_AO_Dev||||
```

- **Version:** `2` (supports OS-specific flags).
- **Manufacturer:** `StarlightXpress`
- **Model:** `SX AO` (appears in the TSX dropdown).
- **Comment:** Tooltip or description.
- **MapsTo:** Unused by 3rd parties, leave as a space ` `.
- **PlugInDllName:** Library name without extension (`libSXAO` for `libSXAO.dylib`).
- **X2Developer:** Arbitrary string passed to your plugin constructor.
- **Windows/Mac/Linux:** Leave empty to default to `1` (available on all OSs).

## Build Setup Notes

The X2 SDK licensed interface headers use `../../licensedinterfaces/foo.h` for internal cross-references between headers in the same directory. This is designed for plugins placed **two levels below** a directory containing `licensedinterfaces/` (i.e., inside `X2-Examples/{type}plugins/{pluginname}/`).

For projects living outside X2-Examples (like this one), the Makefile automatically creates a symlink:

```
../licensedinterfaces  →  ../X2-Examples/licensedinterfaces
```

This lets the SDK headers' internal `../../licensedinterfaces/` references resolve correctly. The Makefile uses flat includes (`-I../X2-Examples/licensedinterfaces`) in our own source files.

This same issue affects the OnStep plugin (`../OnStep/`): its pre-built `.o` files are stale from a previous location; it cannot currently be rebuilt in place without the same fix.


    Serial Protocol                                              

    Port settings: 9600 baud, 8 data bits, 1 stop bit, no parity.                            

    The device connects as a virtual serial port (FTDI USB-serial inside the SX AO USB unit).

    Command Table                                                                                                           
                                                                                                                            
    ┌─────────────────────────┬─────────────────────────────┬──────────────────────────────────────────────────────────────┐
    │  Command (bytes sent)   │         Description         │                           Response                           │
    ├─────────────────────────┼─────────────────────────────┼──────────────────────────────────────────────────────────────┤
    │ X                       │ Handshake                   │ Y                                                            │
    ├─────────────────────────┼─────────────────────────────┼──────────────────────────────────────────────────────────────┤
    │ V                       │ Get firmware version        │ V + 3 ASCII digits (e.g. V123)                               │
    ├─────────────────────────┼─────────────────────────────┼──────────────────────────────────────────────────────────────┤
    │ K                       │ Find centre (home, fast)    │ K                                                            │
    ├─────────────────────────┼─────────────────────────────┼──────────────────────────────────────────────────────────────┤
    │ R                       │ Centre at low speed (unjam) │ K                                                            │
    ├─────────────────────────┼─────────────────────────────┼──────────────────────────────────────────────────────────────┤
    │ L                       │ Get limit switch status     │ 1 byte, ASCII 0x30–0x3F; bits encode which limits are active │
    ├─────────────────────────┼─────────────────────────────┼──────────────────────────────────────────────────────────────┤
    │ G N 0 0 0 0 N (7 bytes) │ AO N steps North            │ G (ok) or L (hit limit)                                      │
    ├─────────────────────────┼─────────────────────────────┼──────────────────────────────────────────────────────────────┤
    │ G S 0 0 0 0 N (7 bytes) │ AO N steps South            │ G or L                                                       │
    ├─────────────────────────┼─────────────────────────────┼──────────────────────────────────────────────────────────────┤
    │ G T 0 0 0 0 N (7 bytes) │ AO N steps East             │ G or L                                                       │
    ├─────────────────────────┼─────────────────────────────┼──────────────────────────────────────────────────────────────┤
    │ G W 0 0 0 0 N (7 bytes) │ AO N steps West             │ G or L                                                       │
    ├─────────────────────────┼─────────────────────────────┼──────────────────────────────────────────────────────────────┤
    │ M N 0 0 0 0 N (7 bytes) │ Mount bump N steps North    │ M                                                            │
    ├─────────────────────────┼─────────────────────────────┼──────────────────────────────────────────────────────────────┤
    │ M S 0 0 0 0 N (7 bytes) │ Mount bump N steps South    │ M                                                            │
    ├─────────────────────────┼─────────────────────────────┼──────────────────────────────────────────────────────────────┤
    │ M T 0 0 0 0 N (7 bytes) │ Mount bump N steps East     │ M                                                            │
    ├─────────────────────────┼─────────────────────────────┼──────────────────────────────────────────────────────────────┤
    │ M W 0 0 0 0 N (7 bytes) │ Mount bump N steps West     │ M                                                            │
    └─────────────────────────┴─────────────────────────────┴──────────────────────────────────────────────────────────────┘

    G/M Command Format                              

    7 ASCII bytes: [G|M][N|S|T|W][0][0][0][0][count]                                                                                     
                       
    The step count occupies bytes 3–7 as a zero-padded 5-character ASCII decimal integer. For 1 step: GN00001. For 10 steps: GN00010. For
    255 steps: GN00255.                                                              

    T = East, W = West (the manual uses compass headings for the optic, not the sky).

    Notes                                                                                                                          
                                       
    - The device does not expose an absolute position query. Position must be tracked in software by counting steps sent. After a K
    command, position resets to centre.                                                                                                  
    - Do not send a new command until the previous one has returned its response character.
    - Mount bump commands (M) can interleave with AO commands in serial mode, allowing the AO to back off while the mount corrects — this
    is not available in parallel (pulse-width) mode.

