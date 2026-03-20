#include "sxao.h"
#include "sberrorx.h"

// Default motor range for SX AO (0-based, 255 positions per axis).
// These may need updating once the real device protocol is characterised.
static const unsigned short SX_AO_MIN = 0;
static const unsigned short SX_AO_MAX = 255;
static const unsigned short SX_AO_CENTER = 128;

SXAO::SXAO()
    : m_bConnected(false)
    , m_nRaPos(SX_AO_CENTER)
    , m_nDecPos(SX_AO_CENTER)
    , m_nRaMin(SX_AO_MIN)
    , m_nRaMax(SX_AO_MAX)
    , m_nDecMin(SX_AO_MIN)
    , m_nDecMax(SX_AO_MAX)
{
}

SXAO::~SXAO()
{
    if (m_bConnected)
        Disconnect();
}

int SXAO::Connect(const std::string& sPortName)
{
    // TODO: open serial port, verify device identity
    m_sPortName  = sPortName;
    m_bConnected = true;
    return SB_OK;
}

int SXAO::Disconnect()
{
    // TODO: close serial port
    m_bConnected = false;
    return SB_OK;
}

bool SXAO::isConnected() const
{
    return m_bConnected;
}

int SXAO::getFirmwareVersion(std::string& sVersion)
{
    // TODO: query device for firmware version string
    sVersion = "0.0";
    return SB_OK;
}

int SXAO::getMotorLimits(unsigned short& nRaMin, unsigned short& nRaMax,
                          unsigned short& nDecMin, unsigned short& nDecMax)
{
    nRaMin  = m_nRaMin;
    nRaMax  = m_nRaMax;
    nDecMin = m_nDecMin;
    nDecMax = m_nDecMax;
    return SB_OK;
}

int SXAO::getTipTilt(short* pnRa, short* pnDec)
{
    // Return position re-centred on zero
    if (pnRa)  *pnRa  = static_cast<short>(m_nRaPos)  - SX_AO_CENTER;
    if (pnDec) *pnDec = static_cast<short>(m_nDecPos) - SX_AO_CENTER;
    return SB_OK;
}

int SXAO::getTipTiltPercent(float* pdRa, float* pdDec)
{
    float half = static_cast<float>(SX_AO_MAX - SX_AO_MIN) / 2.0f;
    if (pdRa)  *pdRa  = (static_cast<float>(m_nRaPos)  - SX_AO_CENTER) / half;
    if (pdDec) *pdDec = (static_cast<float>(m_nDecPos) - SX_AO_CENTER) / half;
    return SB_OK;
}

int SXAO::setTipTiltAbsolute(unsigned short nRa, unsigned short nDec)
{
    // TODO: send absolute-move command to device
    m_nRaPos  = nRa;
    m_nDecPos = nDec;
    return SB_OK;
}

int SXAO::setTipTiltOffset(short nRaOffset, short nDecOffset)
{
    // TODO: send offset-move command to device (or compute absolute and send)
    int newRa  = static_cast<int>(m_nRaPos)  + nRaOffset;
    int newDec = static_cast<int>(m_nDecPos) + nDecOffset;

    // Clamp to valid range
    if (newRa  < m_nRaMin)  newRa  = m_nRaMin;
    if (newRa  > m_nRaMax)  newRa  = m_nRaMax;
    if (newDec < m_nDecMin) newDec = m_nDecMin;
    if (newDec > m_nDecMax) newDec = m_nDecMax;

    m_nRaPos  = static_cast<unsigned short>(newRa);
    m_nDecPos = static_cast<unsigned short>(newDec);
    return SB_OK;
}

int SXAO::centerAO()
{
    // TODO: send centre command to device
    m_nRaPos  = SX_AO_CENTER;
    m_nDecPos = SX_AO_CENTER;
    return SB_OK;
}
