#pragma once

#include <string>

// -------------------------------------------------------------------------
// SXAO — low-level Starlight Xpress AO hardware abstraction
//
// All methods are stubs. The SX AO communicates over USB-serial at 9600
// baud using a simple ASCII command protocol. The actual command set and
// response parsing will be filled in as the protocol is documented.
// -------------------------------------------------------------------------
class SXAO
{
public:
    SXAO();
    ~SXAO();

    // Connection management
    int  Connect(const std::string& sPortName);
    int  Disconnect();
    bool isConnected() const;

    // Device information
    int getFirmwareVersion(std::string& sVersion);

    // Motor limits: raw count range for each axis.
    // For the SX AO the range is typically 0..255 on each axis.
    int getMotorLimits(unsigned short& nRaMin, unsigned short& nRaMax,
                       unsigned short& nDecMin, unsigned short& nDecMax);

    // Current tip-tilt position in raw counts (signed, centered at 0).
    int getTipTilt(short* pnRa, short* pnDec);

    // Current tip-tilt position as a normalised fraction in [-1.0, 1.0].
    int getTipTiltPercent(float* pdRa, float* pdDec);

    // Move mirror to an absolute raw position.
    int setTipTiltAbsolute(unsigned short nRa, unsigned short nDec);

    // Apply a relative offset in raw counts (signed).
    int setTipTiltOffset(short nRaOffset, short nDecOffset);

    // Return mirror to its mechanical centre.
    int centerAO();

private:
    bool          m_bConnected;
    std::string   m_sPortName;
    unsigned short m_nRaPos;
    unsigned short m_nDecPos;
    unsigned short m_nRaMin;
    unsigned short m_nRaMax;
    unsigned short m_nDecMin;
    unsigned short m_nDecMax;
};
