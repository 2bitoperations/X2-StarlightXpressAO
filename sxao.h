#pragma once

#include <string>
#include "serxinterface.h"

// -------------------------------------------------------------------------
// SXAO — low-level Starlight Xpress AO hardware abstraction
//
// Serial protocol: 9600 8N1, no parity, no flow control.
// All commands and responses are ASCII characters.
//
// Key commands (from SX AO-USB Handbook, Issue 3):
//   X            → Handshake        ← Y
//   V            → Firmware version ← V + 3 ASCII digits (e.g. "V123")
//   K            → Find Centre      ← K
//   R            → Centre (slow/unjam) ← K
//   L            → Limit status     ← byte 0x30..0x3F (bits 0-3 = N,S,E,W limits)
//   GN00001      → 1 step North     ← G (ok) or L (limit hit)
//   GS00001      → 1 step South     ← G or L
//   GT00001      → 1 step East      ← G or L
//   GW00001      → 1 step West      ← G or L
//   G{dir}NNNNN  → N steps (NNNNN = 5-char zero-padded ASCII decimal)
// -------------------------------------------------------------------------

class SXAO
{
public:
    explicit SXAO(SerXInterface* pSerX);
    ~SXAO();

    // Connection management
    int  Connect(const std::string& sPortName);
    int  Disconnect();
    bool isConnected() const;

    // Device information
    int getFirmwareVersion(std::string& sVersion);

    // Limit switch status: bits 0-3 = N, S, E, W limits asserted
    int getLimitStatus(unsigned char& nStatus);

    // Motor limits: raw count range for each axis.
    int getMotorLimits(unsigned short& nRaMin, unsigned short& nRaMax,
                       unsigned short& nDecMin, unsigned short& nDecMax);

    // Current tip-tilt position in raw counts (signed, centred at 0).
    int getTipTilt(short* pnRa, short* pnDec);

    // Current tip-tilt position as a normalised fraction in [-1.0, 1.0].
    int getTipTiltPercent(float* pdRa, float* pdDec);

    // Move mirror to an absolute raw position (0 = centre, signed).
    int setTipTiltAbsolute(unsigned short nRa, unsigned short nDec);

    // Apply a relative offset in raw counts (signed).
    int setTipTiltOffset(short nRaOffset, short nDecOffset);

    // Return mirror to its mechanical centre (fast, sends 'K').
    int centerAO();

    // Return mirror to its mechanical centre slowly, overcoming any jam ('R').
    int unjamAO();

private:
    // Send cmd (cmdLen bytes), read respLen bytes into resp.
    // Returns SB_OK or ERR_COMMNOLINK / ERR_COMMTIMEOUT.
    int sendCmd(const char* cmd, int cmdLen, char* resp, int respLen,
                unsigned long timeoutMs = 2000);

    // Send a step command: 'G' + dir + 5-char count.
    // dir is one of 'N','S','T','W'.  Updates m_nRaPos / m_nDecPos on success.
    int sendStep(char dir, unsigned short nSteps, bool isRa, int sign);

    SerXInterface*  m_pSerX;
    bool            m_bConnected;

    // Software position tracking (signed, centred at 0 after K/R)
    short           m_nRaPos;
    short           m_nDecPos;

    // Fixed hardware range (steps from centre to limit)
    static const short SX_AO_HALF_RANGE = 127;
};
