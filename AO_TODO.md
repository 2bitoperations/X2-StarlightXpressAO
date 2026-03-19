# Adaptive Optics (AO) Implementation

## Investigation Chain

You can reproduce or expand this work by following these steps.

1. Search the X2-Examples directory for keywords. We used `grep -rnIE "DirectGuide|Startup|Home|FindHome"`. This found `dapiFindHome` for Domes and the error code `ERR_MOUNTNOTHOMED` (231), but nothing for mount homing.
2. Examine the TheSkyX binary and bundled mount plugins. On Linux, these are at `~/TheSkyX/TheSkyX` and `~/TheSkyX/Resources/Common/PlugIns64/MountPlugIns/*.so`.
3. Use `strings` and `c++filt` to identify demangled C++ symbols. Running `strings ~/TheSkyX/TheSkyX | c++filt` reveals vtables and virtual method declarations for classes that aren't in the SDK.
4. Identify specific interface structures. We found clear signatures for `FindHomeInterface` and `DirectGuideInterface` within the binary's symbol table.

## Adaptive Optics (AO) Interface

TheSkyX natively supports an undocumented `AdaptiveOpticsTipTiltInterface/1.0`. By reverse engineering the TSX binary, we found that plugins can expose this interface for arbitrary AO devices, not just SBIG.

### Reconstructed Interface

```cpp
class AdaptiveOpticsTipTiltInterface {
public:
    virtual ~AdaptiveOpticsTipTiltInterface() {}
    virtual int GetMotorLimits(unsigned short& nRaMin, unsigned short& nRaMax, unsigned short& nDecMin, unsigned short& nDecMax) = 0;
    virtual int GetAOTipTilt(short* pnRa, short* pnDec) = 0;
    virtual int GetAOTipTiltPercent(float* pdRa, float* pdDec) = 0;
    virtual int CCAOTipTiltAbsolute(unsigned short nRa, unsigned short nDec) = 0;
    virtual int CCAOTipTiltOffset(short nRaOffset, short nDecOffset) = 0;
    virtual int OnCenterAO() = 0;
};
```

To implement this, create an X2 plugin (typically a Camera or Mount plugin depending on the AO connection), inherit from `AdaptiveOpticsTipTiltInterface`, and intercept `"AdaptiveOpticsTipTiltInterface"` in `queryAbstraction`.

## Adaptive Optics (AO) Scaffolding

For TheSkyX to present a custom Adaptive Optics plugin in the UI and load the library, specific files and directories must exist. This is based on reverse-engineering the TSX `hardwarelist.txt` format and binary strings.

### 1. Plugin Directory

The compiled shared library (e.g., `libMyAO.so` on Linux, `libMyAO.dylib` on macOS, `MyAO.dll` on Windows) must be placed in a specific subdirectory within TheSkyX's resources:

*   **64-bit TSX:** `[TSX_Install_Dir]/Resources/Common/PlugIns64/AdaptiveOpticsPlugIns/`
*   **32-bit TSX:** `[TSX_Install_Dir]/Resources/Common/PlugIns/AdaptiveOpticsPlugIns/`

Note that this folder might not exist by default and must be created by the installer.

### 2. Hardware List File

TheSkyX enumerates devices using text files in `[TSX_Install_Dir]/Resources/Common/Miscellaneous Files/`. To register a new AO device, you must create a file named `adaptiveopticslist <YourCompany>.txt` in that directory (e.g., `adaptiveopticslist ZWO.txt`).

### 3. List File Format

The file must follow the standard X2 hardware list pipe-separated format.

Example content for `adaptiveopticslist ZWO.txt`:

```text
//Version|Manufacturer|Model|Comment|MapsTo|PlugInDllName|X2Developer|Windows|Mac|Linux|
2|ZWO|ZWO AO Device|ZWO Adaptive Optics Support| |libZWOAO|ZWO_AO_Dev||||
```

*   **Version:** `2` (supports OS-specific flags).
*   **Manufacturer:** E.g., `ZWO`.
*   **Model:** E.g., `ZWO AO Device` (This is what appears in the TSX dropdown).
*   **Comment:** Tooltip or description.
*   **MapsTo:** Unused by 3rd parties, leave as a space ` `.
*   **PlugInDllName:** The name of your compiled library without the extension (e.g., `libZWOAO` for `libZWOAO.so`).
*   **X2Developer:** Arbitrary string passed to your plugin constructor.
*   **Windows/Mac/Linux:** Leave empty to default to `1` (available on all OSs), or set to `0` to disable on a specific OS.