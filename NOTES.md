# Implementation Notes

## Undocumented AO Interfaces

TheSkyX ships with AO support (`DT_AO = 11` is in the public SDK's `driverrootinterface.h`) but does not publish the corresponding driver interface header. The interfaces below were reconstructed by binary analysis.

### How to reproduce / extend this analysis

```sh
# macOS binary
TSX="/Applications/TheSkyX Professional Edition.app/Contents/MacOS/TheSkyX"

# Find AO-related symbols
strings "$TSX" | grep -i "adaptive\|tiptilt"
nm -arch x86_64 "$TSX" | c++filt | grep -i adaptive

# Typeinfo addresses (no ASLR):
#   AdaptiveOpticsDriverInterface   0x1027c4a30
#   AdaptiveOpticsTipTiltInterface  0x1027c4a20

# Dump vtable section to count virtual method entries
otool -arch x86_64 -s __DATA_CONST __const "$TSX" | less
```

Key findings:
- `queryAbstraction` key is the full versioned string `"com.bisque.TheSkyX.AdaptiveOpticsTipTiltInterface/1.0"` (confirm directly from `strings` output — earlier community notes using the short form are wrong).
- Three built-in classes implement `AdaptiveOpticsDriverInterface`: `NoAdaptiveOptics`, `AdaptiveOpticsSimulator`, `EmbeddedAODevice` (SBIG camera-integrated).
- `NoAdaptiveOptics` primary vtable: **22 entries**. `AdaptiveOpticsSimulator`: **30 entries**. The `AdaptiveOpticsTipTiltInterface` secondary vtable has exactly **6 entries**, matching the 6 pure virtuals below.
- Factory entry points are the standard X2 ones: `sbPlugInName2` / `sbPlugInFactory2`.

### Reconstructed interfaces

These live in `x2sxao.h`. The `AdaptiveOpticsDriverInterface` is our best reconstruction — if TSX crashes on plugin load, the primary vtable layout is wrong; compare entry counts above and adjust the base class list.

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

// Not in the SDK. Pattern matches FocuserDriverInterface for focusers.
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
    // ... LinkInterface, HardwareInfoInterface, DriverInfoInterface pure virtuals
};
```

---

## SX AO Serial Protocol

Port settings: **9600 baud, 8N1**, no flow control. The device presents as a virtual serial port via an FTDI USB-serial chip inside the SX AO USB unit.

### Command table

| Command (bytes sent)    | Description              | Response                                          |
|-------------------------|--------------------------|---------------------------------------------------|
| `X`                     | Handshake                | `Y`                                               |
| `V`                     | Get firmware version     | `V` + 3 ASCII digits (e.g. `V123`)               |
| `K`                     | Centre mirror (fast)     | `K`                                               |
| `R`                     | Centre mirror (slow/unjam) | `K` (takes ~30 s)                               |
| `L`                     | Get limit switch status  | 1 byte, ASCII `0x30`–`0x3F`; bits 0–3 = N/S/E/W |
| `GN00001`–`GN00127`     | AO N steps North         | `G` (ok) or `L` (hit limit)                      |
| `GS00001`–`GS00127`     | AO N steps South         | `G` or `L`                                       |
| `GT00001`–`GT00127`     | AO N steps East          | `G` or `L`                                       |
| `GW00001`–`GW00127`     | AO N steps West          | `G` or `L`                                       |
| `MN00001`–`MN00127`     | Mount bump N steps North | `M`                                               |
| `MS00001`–`MS00127`     | Mount bump N steps South | `M`                                               |
| `MT00001`–`MT00127`     | Mount bump N steps East  | `M`                                               |
| `MW00001`–`MW00127`     | Mount bump N steps West  | `M`                                               |

### G/M command format

7 ASCII bytes: `[G|M] [N|S|T|W] [5-digit zero-padded step count]`

Examples: `GN00001` (1 step North), `GT00010` (10 steps East), `GS00127` (127 steps South, max).

`T` = East, `W` = West — the manual uses optic-frame compass headings, not sky directions.

### Notes

- No absolute position query exists. Position must be tracked in software by accumulating steps sent; resets to centre after `K` or `R`.
- Do not send a new command until the previous response byte has been received.
- Mount bump commands (`M`) can interleave with AO commands (`G`) in serial mode, allowing the AO to back off while the mount corrects. This interleaving is not available in parallel (pulse-width) mode.
- Hardware travel limit is ±127 steps from centre in each axis.
