#include "sxao.h"
#include "sberrorx.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <ctime>

#ifdef SB_WIN_BUILD
#  include <windows.h>
#else
#  include <sys/time.h>
#endif

// Hardware range: ±127 steps from centre.
const short SXAO::SX_AO_HALF_RANGE;

SXAO::SXAO(SerXInterface* pSerX)
    : m_pSerX(pSerX)
    , m_bConnected(false)
    , m_nRaPos(0)
    , m_nDecPos(0)
    , m_nDebugLevel(0)
{
    const char* home = getenv("HOME");
#ifdef SB_WIN_BUILD
    if (!home) home = getenv("USERPROFILE");
    m_sLogPath = std::string(home ? home : ".") + "\\SXAOLog.txt";
#else
    m_sLogPath = std::string(home ? home : ".") + "/SXAOLog.txt";
#endif
}

SXAO::~SXAO()
{
    if (m_bConnected)
        Disconnect();
    if (m_logFile.is_open())
        m_logFile.close();
}

// -------------------------------------------------------------------------
// Logging
// -------------------------------------------------------------------------

std::string SXAO::getTimeStamp() const
{
    char buf[32];

#ifdef SB_WIN_BUILD
    SYSTEMTIME st;
    GetSystemTime(&st);
    snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ",
             st.wYear, st.wMonth, st.wDay,
             st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm tm_utc;
    gmtime_r(&tv.tv_sec, &tm_utc);
    char tbuf[24];
    strftime(tbuf, sizeof(tbuf), "%Y-%m-%dT%H:%M:%S", &tm_utc);
    snprintf(buf, sizeof(buf), "%s.%03ldZ", tbuf, tv.tv_usec / 1000L);
#endif
    return buf;
}

std::string SXAO::hexStr(const void* data, int len) const
{
    const unsigned char* p = static_cast<const unsigned char*>(data);
    std::string s;
    char hex[4];
    for (int i = 0; i < len; i++)
    {
        snprintf(hex, sizeof(hex), "%02X ", p[i]);
        s += hex;
    }
    if (!s.empty() && s.back() == ' ')
        s.pop_back();
    return s;
}

void SXAO::logLine(const std::string& msg)
{
    if (!m_logFile.is_open())
        return;
    m_logFile << "[" << getTimeStamp() << "] " << msg << "\n";
    m_logFile.flush();
}

void SXAO::logError(const char* func, int nErr, const std::string& detail)
{
    if (m_nDebugLevel < 1 || !m_logFile.is_open())
        return;
    std::string msg = std::string("[") + func + "] ERROR " + std::to_string(nErr);
    if (!detail.empty())
        msg += ": " + detail;
    logLine(msg);
}

void SXAO::logTrace(const char* func, const std::string& msg)
{
    if (m_nDebugLevel < 2 || !m_logFile.is_open())
        return;
    logLine(std::string("[") + func + "] " + msg);
}

void SXAO::logRaw(const char* label, const void* data, int len)
{
    if (m_nDebugLevel < 3 || !m_logFile.is_open())
        return;
    logLine(std::string("[raw] ") + label + ": " + hexStr(data, len));
}

void SXAO::setDebugLevel(int nLevel)
{
    if (nLevel > 0 && !m_logFile.is_open())
        m_logFile.open(m_sLogPath.c_str(), std::ios::out | std::ios::app);
    else if (nLevel == 0 && m_logFile.is_open())
        m_logFile.close();

    m_nDebugLevel = nLevel;

    if (m_nDebugLevel >= 2 && m_logFile.is_open())
    {
        logLine("[SXAO] ---- debug level set to " + std::to_string(nLevel) +
                " (log: " + m_sLogPath + ")");
        logLine("[SXAO] built " __DATE__ " " __TIME__);
    }
}

// -------------------------------------------------------------------------
// Connection
// -------------------------------------------------------------------------

int SXAO::Connect(const std::string& sPortName)
{
    logTrace(__func__, "called, port=" + sPortName);

    if (!m_pSerX)
    {
        logError(__func__, ERR_COMMNOLINK, "no SerX interface");
        return ERR_COMMNOLINK;
    }

    int nErr = m_pSerX->open(sPortName.c_str(), 9600, SerXInterface::B_NOPARITY);
    if (nErr)
    {
        logError(__func__, nErr, "open() failed");
        return nErr;
    }

    m_pSerX->purgeTxRx();
    logTrace(__func__, "port opened, sending handshake");

    // Handshake: send 'X', expect 'Y'
    char resp[2] = {0, 0};
    nErr = sendCmd("X", 1, resp, 1, 2000);
    if (nErr)
    {
        logError(__func__, nErr, "handshake sendCmd failed");
        m_pSerX->close();
        return nErr;
    }
    if (resp[0] != 'Y')
    {
        char detail[32];
        snprintf(detail, sizeof(detail), "expected 'Y', got 0x%02X", (unsigned char)resp[0]);
        logError(__func__, ERR_CMDFAILED, detail);
        m_pSerX->close();
        return ERR_CMDFAILED;
    }

    m_bConnected = true;
    m_nRaPos  = 0;
    m_nDecPos = 0;
    logTrace(__func__, "connected OK");
    return SB_OK;
}

int SXAO::Disconnect()
{
    logTrace(__func__, "called");
    if (m_pSerX && m_bConnected)
        m_pSerX->close();
    m_bConnected = false;
    logTrace(__func__, "disconnected");
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

    if (m_nDebugLevel >= 2)
        logTrace(__func__, "TX: '" + std::string(cmd, cmdLen) + "'");
    logRaw("TX", cmd, cmdLen);

    unsigned long nWritten = 0;
    int nErr = m_pSerX->writeFile((void*)cmd, (unsigned long)cmdLen, nWritten);
    if (nErr)
    {
        logError(__func__, nErr, "writeFile failed");
        return nErr;
    }
    if ((int)nWritten != cmdLen)
    {
        logError(__func__, ERR_CMDFAILED, "short write");
        return ERR_CMDFAILED;
    }

    if (respLen <= 0)
        return SB_OK;

    m_pSerX->flushTx();

    unsigned long nRead = 0;
    nErr = m_pSerX->readFile(resp, (unsigned long)respLen, nRead, timeoutMs);
    if (nErr)
    {
        logError(__func__, nErr, "readFile failed");
        return nErr;
    }
    if ((int)nRead != respLen)
    {
        char detail[64];
        snprintf(detail, sizeof(detail), "wanted %d bytes, got %lu (timeout=%lums)",
                 respLen, nRead, timeoutMs);
        logError(__func__, ERR_RXTIMEOUT, detail);
        return ERR_RXTIMEOUT;
    }

    if (m_nDebugLevel >= 2)
        logTrace(__func__, "RX: '" + std::string(resp, nRead) + "'");
    logRaw("RX", resp, (int)nRead);

    return SB_OK;
}

// -------------------------------------------------------------------------
// Device information
// -------------------------------------------------------------------------

int SXAO::getFirmwareVersion(std::string& sVersion)
{
    logTrace(__func__, "called");

    if (!m_bConnected)
    {
        logError(__func__, ERR_COMMNOLINK);
        return ERR_COMMNOLINK;
    }

    char resp[5] = {0};
    int nErr = sendCmd("V", 1, resp, 4, 2000);
    if (nErr)
    {
        logError(__func__, nErr, "sendCmd failed");
        return nErr;
    }

    if (resp[0] != 'V')
    {
        char detail[32];
        snprintf(detail, sizeof(detail), "expected 'V', got 0x%02X", (unsigned char)resp[0]);
        logError(__func__, ERR_CMDFAILED, detail);
        return ERR_CMDFAILED;
    }

    char buf[16];
    snprintf(buf, sizeof(buf), "%c.%c.%c", resp[1], resp[2], resp[3]);
    sVersion = buf;
    logTrace(__func__, "firmware version: " + sVersion);
    return SB_OK;
}

int SXAO::getLimitStatus(unsigned char& nStatus)
{
    logTrace(__func__, "called");

    if (!m_bConnected)
    {
        logError(__func__, ERR_COMMNOLINK);
        return ERR_COMMNOLINK;
    }

    char resp[2] = {0};
    int nErr = sendCmd("L", 1, resp, 1, 2000);
    if (nErr)
    {
        logError(__func__, nErr, "sendCmd failed");
        return nErr;
    }

    nStatus = (unsigned char)(resp[0] & 0x0F);

    char detail[48];
    snprintf(detail, sizeof(detail), "limit bits=0x%X (N=%d S=%d E=%d W=%d)",
             nStatus,
             (nStatus >> 0) & 1, (nStatus >> 1) & 1,
             (nStatus >> 2) & 1, (nStatus >> 3) & 1);
    logTrace(__func__, detail);
    return SB_OK;
}

// -------------------------------------------------------------------------
// Motor limits
// -------------------------------------------------------------------------

int SXAO::getMotorLimits(unsigned short& nRaMin, unsigned short& nRaMax,
                          unsigned short& nDecMin, unsigned short& nDecMax)
{
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

int SXAO::sendStep(char dir, unsigned short nSteps, bool isRa, int sign)
{
    if (!m_bConnected)
    {
        logError(__func__, ERR_COMMNOLINK);
        return ERR_COMMNOLINK;
    }
    if (nSteps == 0)
        return SB_OK;

    char cmd[8];
    cmd[0] = 'G';
    cmd[1] = dir;
    snprintf(cmd + 2, 6, "%05u", (unsigned)nSteps);

    char detail[32];
    snprintf(detail, sizeof(detail), "G%c%05u (%s)", dir, nSteps, isRa ? "RA" : "Dec");
    logTrace(__func__, detail);

    char resp[2] = {0};
    int nErr = sendCmd(cmd, 7, resp, 1, 3000);
    if (nErr)
    {
        logError(__func__, nErr, detail);
        return nErr;
    }

    if (resp[0] == 'L')
    {
        logTrace(__func__, std::string("limit hit on ") + detail);
        if (isRa)
            m_nRaPos  = (short)(sign > 0 ?  SX_AO_HALF_RANGE : -SX_AO_HALF_RANGE);
        else
            m_nDecPos = (short)(sign > 0 ?  SX_AO_HALF_RANGE : -SX_AO_HALF_RANGE);
        return SB_OK;
    }

    if (resp[0] != 'G')
    {
        char errDetail[32];
        snprintf(errDetail, sizeof(errDetail), "unexpected response 0x%02X", (unsigned char)resp[0]);
        logError(__func__, ERR_CMDFAILED, errDetail);
        return ERR_CMDFAILED;
    }

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

    char posDetail[48];
    snprintf(posDetail, sizeof(posDetail), "pos now RA=%d Dec=%d", m_nRaPos, m_nDecPos);
    logTrace(__func__, posDetail);
    return SB_OK;
}

int SXAO::setTipTiltAbsolute(unsigned short nRa, unsigned short nDec)
{
    short targetRa  = (short)nRa  - SX_AO_HALF_RANGE;
    short targetDec = (short)nDec - SX_AO_HALF_RANGE;

    char detail[64];
    snprintf(detail, sizeof(detail), "target RA=%d Dec=%d (current RA=%d Dec=%d)",
             targetRa, targetDec, m_nRaPos, m_nDecPos);
    logTrace(__func__, detail);

    short dRa  = targetRa  - m_nRaPos;
    short dDec = targetDec - m_nDecPos;
    return setTipTiltOffset(dRa, dDec);
}

int SXAO::setTipTiltOffset(short nRaOffset, short nDecOffset)
{
    char detail[48];
    snprintf(detail, sizeof(detail), "dRA=%d dDec=%d", nRaOffset, nDecOffset);
    logTrace(__func__, detail);

    int nErr = SB_OK;

    if (nRaOffset > 0)
        nErr = sendStep('T', (unsigned short)nRaOffset,    true, +1);
    else if (nRaOffset < 0)
        nErr = sendStep('W', (unsigned short)(-nRaOffset), true, -1);

    if (nErr)
    {
        logError(__func__, nErr, "RA step failed");
        return nErr;
    }

    if (nDecOffset > 0)
        nErr = sendStep('N', (unsigned short)nDecOffset,    false, +1);
    else if (nDecOffset < 0)
        nErr = sendStep('S', (unsigned short)(-nDecOffset), false, -1);

    if (nErr)
        logError(__func__, nErr, "Dec step failed");

    return nErr;
}

int SXAO::centerAO()
{
    logTrace(__func__, "called (fast centre 'K')");

    if (!m_bConnected)
    {
        logError(__func__, ERR_COMMNOLINK);
        return ERR_COMMNOLINK;
    }

    char resp[2] = {0};
    int nErr = sendCmd("K", 1, resp, 1, 5000);
    if (nErr)
    {
        logError(__func__, nErr, "sendCmd failed");
        return nErr;
    }
    if (resp[0] != 'K')
    {
        char detail[32];
        snprintf(detail, sizeof(detail), "expected 'K', got 0x%02X", (unsigned char)resp[0]);
        logError(__func__, ERR_CMDFAILED, detail);
        return ERR_CMDFAILED;
    }

    m_nRaPos  = 0;
    m_nDecPos = 0;
    logTrace(__func__, "centred OK, position reset to 0,0");
    return SB_OK;
}

int SXAO::unjamAO()
{
    logTrace(__func__, "called (slow centre 'R', 30s timeout)");

    if (!m_bConnected)
    {
        logError(__func__, ERR_COMMNOLINK);
        return ERR_COMMNOLINK;
    }

    char resp[2] = {0};
    int nErr = sendCmd("R", 1, resp, 1, 30000);
    if (nErr)
    {
        logError(__func__, nErr, "sendCmd failed");
        return nErr;
    }
    if (resp[0] != 'K')
    {
        char detail[32];
        snprintf(detail, sizeof(detail), "expected 'K', got 0x%02X", (unsigned char)resp[0]);
        logError(__func__, ERR_CMDFAILED, detail);
        return ERR_CMDFAILED;
    }

    m_nRaPos  = 0;
    m_nDecPos = 0;
    logTrace(__func__, "unjam OK, position reset to 0,0");
    return SB_OK;
}
