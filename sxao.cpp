#include "sxao.h"
#include "sberrorx.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>

// Hardware range: ±127 steps from centre.
const short SXAO::SX_AO_HALF_RANGE;

SXAO::SXAO(SerXInterface* pSerX)
    : m_pSerX(pSerX)
    , m_bConnected(false)
    , m_nRaPos(0)
    , m_nDecPos(0)
{
}

SXAO::~SXAO()
{
    if (m_bConnected)
        Disconnect();
}

// -------------------------------------------------------------------------
// Connection
// -------------------------------------------------------------------------

int SXAO::Connect(const std::string& sPortName)
{
    if (!m_pSerX)
        return ERR_COMMNOLINK;

    // Open at 9600 8N1
    int nErr = m_pSerX->open(sPortName.c_str(), 9600, SerXInterface::B_NOPARITY);
    if (nErr)
        return nErr;

    m_pSerX->purgeTxRx();

    // Handshake: send 'X', expect 'Y'
    char resp[2] = {0, 0};
    nErr = sendCmd("X", 1, resp, 1, 2000);
    if (nErr)
    {
        m_pSerX->close();
        return nErr;
    }
    if (resp[0] != 'Y')
    {
        m_pSerX->close();
        return ERR_CMDFAILED;
    }

    m_bConnected = true;
    m_nRaPos  = 0;
    m_nDecPos = 0;
    return SB_OK;
}

int SXAO::Disconnect()
{
    if (m_pSerX && m_bConnected)
        m_pSerX->close();
    m_bConnected = false;
    return SB_OK;
}

bool SXAO::isConnected() const
{
    return m_bConnected;
}

// -------------------------------------------------------------------------
// Serial helpers
// -------------------------------------------------------------------------

int SXAO::sendCmd(const char* cmd, int cmdLen, char* resp, int respLen,
                  unsigned long timeoutMs)
{
    if (!m_pSerX)
        return ERR_COMMNOLINK;

    unsigned long nWritten = 0;
    int nErr = m_pSerX->writeFile((void*)cmd, (unsigned long)cmdLen, nWritten);
    if (nErr)
        return nErr;
    if ((int)nWritten != cmdLen)
        return ERR_CMDFAILED;

    if (respLen <= 0)
        return SB_OK;

    m_pSerX->flushTx();

    unsigned long nRead = 0;
    nErr = m_pSerX->readFile(resp, (unsigned long)respLen, nRead, timeoutMs);
    if (nErr)
        return nErr;
    if ((int)nRead != respLen)
        return ERR_RXTIMEOUT;

    return SB_OK;
}

// -------------------------------------------------------------------------
// Device information
// -------------------------------------------------------------------------

int SXAO::getFirmwareVersion(std::string& sVersion)
{
    if (!m_bConnected)
        return ERR_COMMNOLINK;

    // Send 'V', receive 'V' + 3 ASCII digit chars (e.g. "V123" = v1.2.3)
    char resp[5] = {0};
    int nErr = sendCmd("V", 1, resp, 4, 2000);
    if (nErr)
        return nErr;

    if (resp[0] != 'V')
        return ERR_CMDFAILED;

    // Format as "M.m.p"
    char buf[16];
    snprintf(buf, sizeof(buf), "%c.%c.%c", resp[1], resp[2], resp[3]);
    sVersion = buf;
    return SB_OK;
}

int SXAO::getLimitStatus(unsigned char& nStatus)
{
    if (!m_bConnected)
        return ERR_COMMNOLINK;

    // Send 'L', receive one byte in range 0x30..0x3F
    char resp[2] = {0};
    int nErr = sendCmd("L", 1, resp, 1, 2000);
    if (nErr)
        return nErr;

    // Lower 4 bits encode N, S, E, W limit switches
    nStatus = (unsigned char)(resp[0] & 0x0F);
    return SB_OK;
}

// -------------------------------------------------------------------------
// Motor limits
// -------------------------------------------------------------------------

int SXAO::getMotorLimits(unsigned short& nRaMin, unsigned short& nRaMax,
                          unsigned short& nDecMin, unsigned short& nDecMax)
{
    // The device range is ±SX_AO_HALF_RANGE steps.
    // TSX expects unsigned values; we centre the unsigned range at the midpoint.
    nRaMin  = 0;
    nRaMax  = (unsigned short)(SX_AO_HALF_RANGE * 2);
    nDecMin = 0;
    nDecMax = (unsigned short)(SX_AO_HALF_RANGE * 2);
    return SB_OK;
}

// -------------------------------------------------------------------------
// Position queries
// -------------------------------------------------------------------------

int SXAO::getTipTilt(short* pnRa, short* pnDec)
{
    if (pnRa)  *pnRa  = m_nRaPos;
    if (pnDec) *pnDec = m_nDecPos;
    return SB_OK;
}

int SXAO::getTipTiltPercent(float* pdRa, float* pdDec)
{
    float half = (float)SX_AO_HALF_RANGE;
    if (pdRa)  *pdRa  = (float)m_nRaPos  / half;
    if (pdDec) *pdDec = (float)m_nDecPos / half;
    return SB_OK;
}

// -------------------------------------------------------------------------
// Motion
// -------------------------------------------------------------------------

// Send a step command: GN/GS/GT/GW + 5-char zero-padded count.
// sign: +1 = positive axis direction, -1 = negative.
// isRa: true updates m_nRaPos, false updates m_nDecPos.
int SXAO::sendStep(char dir, unsigned short nSteps, bool isRa, int sign)
{
    if (!m_bConnected)
        return ERR_COMMNOLINK;
    if (nSteps == 0)
        return SB_OK;

    // Command is exactly 7 ASCII bytes: G + dir + 5-digit count
    char cmd[8];
    cmd[0] = 'G';
    cmd[1] = dir;
    snprintf(cmd + 2, 6, "%05u", (unsigned)nSteps);

    char resp[2] = {0};
    int nErr = sendCmd(cmd, 7, resp, 1, 3000);
    if (nErr)
        return nErr;

    if (resp[0] == 'L')
    {
        // Hit a limit — still update position to the limit value
        if (isRa)
            m_nRaPos  = (short)(sign > 0 ?  SX_AO_HALF_RANGE : -SX_AO_HALF_RANGE);
        else
            m_nDecPos = (short)(sign > 0 ?  SX_AO_HALF_RANGE : -SX_AO_HALF_RANGE);
        return SB_OK; // limit hit is not an error
    }

    if (resp[0] != 'G')
        return ERR_CMDFAILED;

    // Update tracked position
    int delta = sign * (int)nSteps;
    if (isRa)
    {
        m_nRaPos = (short)(m_nRaPos + delta);
        if (m_nRaPos >  SX_AO_HALF_RANGE)  m_nRaPos =  SX_AO_HALF_RANGE;
        if (m_nRaPos < -SX_AO_HALF_RANGE)  m_nRaPos = -SX_AO_HALF_RANGE;
    }
    else
    {
        m_nDecPos = (short)(m_nDecPos + delta);
        if (m_nDecPos >  SX_AO_HALF_RANGE)  m_nDecPos =  SX_AO_HALF_RANGE;
        if (m_nDecPos < -SX_AO_HALF_RANGE)  m_nDecPos = -SX_AO_HALF_RANGE;
    }
    return SB_OK;
}

int SXAO::setTipTiltAbsolute(unsigned short nRa, unsigned short nDec)
{
    // nRa / nDec are unsigned with centre at SX_AO_HALF_RANGE
    short targetRa  = (short)nRa  - SX_AO_HALF_RANGE;
    short targetDec = (short)nDec - SX_AO_HALF_RANGE;
    short dRa  = targetRa  - m_nRaPos;
    short dDec = targetDec - m_nDecPos;
    return setTipTiltOffset(dRa, dDec);
}

int SXAO::setTipTiltOffset(short nRaOffset, short nDecOffset)
{
    int nErr = SB_OK;

    // RA axis: positive = East ('T'), negative = West ('W')
    if (nRaOffset > 0)
        nErr = sendStep('T', (unsigned short)nRaOffset,  true, +1);
    else if (nRaOffset < 0)
        nErr = sendStep('W', (unsigned short)(-nRaOffset), true, -1);

    if (nErr)
        return nErr;

    // Dec axis: positive = North ('N'), negative = South ('S')
    if (nDecOffset > 0)
        nErr = sendStep('N', (unsigned short)nDecOffset,   false, +1);
    else if (nDecOffset < 0)
        nErr = sendStep('S', (unsigned short)(-nDecOffset), false, -1);

    return nErr;
}

int SXAO::centerAO()
{
    if (!m_bConnected)
        return ERR_COMMNOLINK;

    char resp[2] = {0};
    int nErr = sendCmd("K", 1, resp, 1, 5000);
    if (nErr)
        return nErr;
    if (resp[0] != 'K')
        return ERR_CMDFAILED;

    m_nRaPos  = 0;
    m_nDecPos = 0;
    return SB_OK;
}

int SXAO::unjamAO()
{
    if (!m_bConnected)
        return ERR_COMMNOLINK;

    // 'R' centres at low speed — may take longer, use generous timeout
    char resp[2] = {0};
    int nErr = sendCmd("R", 1, resp, 1, 30000);
    if (nErr)
        return nErr;
    if (resp[0] != 'K')
        return ERR_CMDFAILED;

    m_nRaPos  = 0;
    m_nDecPos = 0;
    return SB_OK;
}
