#include "x2sxao.h"
#include <cstdio>
#include <cstring>
#include <sstream>

X2SXAO::X2SXAO(const char*                       pszDriverSelection,
               const int&                         nInstanceIndex,
               SerXInterface*                     pSerX,
               TheSkyXFacadeForDriversInterface*  pTheSkyX,
               SleeperInterface*                  pSleeper,
               BasicIniUtilInterface*             pIniUtil,
               LoggerInterface*                   pLogger,
               MutexInterface*                    pIOMutex,
               TickCountInterface*                pTickCount)
{
    (void)pszDriverSelection;

    m_nInstanceIndex = nInstanceIndex;
    m_pSerX          = pSerX;
    m_pTheSkyX       = pTheSkyX;
    m_pSleeper       = pSleeper;
    m_pIniUtil       = pIniUtil;
    m_pLogger        = pLogger;
    m_pIOMutex       = pIOMutex;
    m_pTickCount     = pTickCount;

    m_bLinked = false;
    m_pAO     = new SXAO(pSerX);
}

X2SXAO::~X2SXAO()
{
    if (m_bLinked)
        m_pAO->Disconnect();

    delete m_pAO;

    if (m_pSerX)        delete m_pSerX;
    if (m_pTheSkyX)     delete m_pTheSkyX;
    if (m_pSleeper)     delete m_pSleeper;
    if (m_pIniUtil)     delete m_pIniUtil;
    if (m_pLogger)      delete m_pLogger;
    if (m_pIOMutex)     delete m_pIOMutex;
    if (m_pTickCount)   delete m_pTickCount;
}

// -------------------------------------------------------------------------
// DriverRootInterface
// -------------------------------------------------------------------------

int X2SXAO::queryAbstraction(const char* pszName, void** ppVal)
{
    *ppVal = NULL;

    if (!strcmp(pszName, AdaptiveOpticsTipTiltInterface_Name))
        *ppVal = dynamic_cast<AdaptiveOpticsTipTiltInterface*>(this);
    else if (!strcmp(pszName, SerialPortParams2Interface_Name))
        *ppVal = dynamic_cast<SerialPortParams2Interface*>(this);
    else if (!strcmp(pszName, ModalSettingsDialogInterface_Name))
        *ppVal = dynamic_cast<ModalSettingsDialogInterface*>(this);
    else if (!strcmp(pszName, X2GUIEventInterface_Name))
        *ppVal = dynamic_cast<X2GUIEventInterface*>(this);
    else if (!strcmp(pszName, LoggerInterface_Name))
        *ppVal = GetLogger();

    return SB_OK;
}

// -------------------------------------------------------------------------
// LinkInterface
// -------------------------------------------------------------------------

int X2SXAO::establishLink(void)
{
    std::string sPortName;
    getPortName(sPortName);

    int nErr = m_pAO->Connect(sPortName);
    m_bLinked = (nErr == SB_OK);
    return nErr;
}

int X2SXAO::terminateLink(void)
{
    int nErr = m_pAO->Disconnect();
    m_bLinked = false;
    return nErr;
}

bool X2SXAO::isLinked(void) const
{
    return m_bLinked;
}

// -------------------------------------------------------------------------
// HardwareInfoInterface
// -------------------------------------------------------------------------

void X2SXAO::deviceInfoNameShort(BasicStringInterface& str) const
{
    str = m_bLinked ? "SX AO" : "Not connected";
}

void X2SXAO::deviceInfoNameLong(BasicStringInterface& str) const
{
    str = "Starlight Xpress Active Optics";
}

void X2SXAO::deviceInfoDetailedDescription(BasicStringInterface& str) const
{
    str = "Starlight Xpress AO tip-tilt mirror device";
}

void X2SXAO::deviceInfoFirmwareVersion(BasicStringInterface& str)
{
    if (m_bLinked) {
        std::string sFirmware;
        m_pAO->getFirmwareVersion(sFirmware);
        str = sFirmware.c_str();
    } else {
        str = "Not connected";
    }
}

void X2SXAO::deviceInfoModel(BasicStringInterface& str)
{
    str = m_bLinked ? "SX AO" : "Not connected";
}

// -------------------------------------------------------------------------
// DriverInfoInterface
// -------------------------------------------------------------------------

void X2SXAO::driverInfoDetailedInfo(BasicStringInterface& str) const
{
    str = "Starlight Xpress AO X2 plugin";
}

double X2SXAO::driverInfoVersion(void) const
{
    return PLUGIN_VERSION;
}

// -------------------------------------------------------------------------
// AdaptiveOpticsTipTiltInterface
// -------------------------------------------------------------------------

int X2SXAO::GetMotorLimits(unsigned short& nRaMin, unsigned short& nRaMax,
                            unsigned short& nDecMin, unsigned short& nDecMax)
{
    if (!m_bLinked)
        return ERR_NOLINK;

    return m_pAO->getMotorLimits(nRaMin, nRaMax, nDecMin, nDecMax);
}

int X2SXAO::GetAOTipTilt(short* pnRa, short* pnDec)
{
    if (!m_bLinked)
        return ERR_NOLINK;

    return m_pAO->getTipTilt(pnRa, pnDec);
}

int X2SXAO::GetAOTipTiltPercent(float* pdRa, float* pdDec)
{
    if (!m_bLinked)
        return ERR_NOLINK;

    return m_pAO->getTipTiltPercent(pdRa, pdDec);
}

int X2SXAO::CCAOTipTiltAbsolute(unsigned short nRa, unsigned short nDec)
{
    if (!m_bLinked)
        return ERR_NOLINK;

    return m_pAO->setTipTiltAbsolute(nRa, nDec);
}

int X2SXAO::CCAOTipTiltOffset(short nRaOffset, short nDecOffset)
{
    if (!m_bLinked)
        return ERR_NOLINK;

    return m_pAO->setTipTiltOffset(nRaOffset, nDecOffset);
}

int X2SXAO::OnCenterAO()
{
    if (!m_bLinked)
        return ERR_NOLINK;

    return m_pAO->centerAO();
}

// -------------------------------------------------------------------------
// SerialPortParams2Interface
// -------------------------------------------------------------------------

void X2SXAO::portName(BasicStringInterface& str) const
{
    std::string sPortName;
    getPortName(sPortName);
    str = sPortName.c_str();
}

void X2SXAO::setPortName(const char* szPort)
{
    if (m_pIniUtil)
        m_pIniUtil->writeString(PARENT_KEY, CHILD_KEY_PORT_NAME, szPort);
}

void X2SXAO::getPortName(std::string& sPortName) const
{
    sPortName.assign(DEF_PORT_NAME);
    if (m_pIniUtil) {
        char port[MAX_PORT_NAME_SIZE];
        m_pIniUtil->readString(PARENT_KEY, CHILD_KEY_PORT_NAME,
                               sPortName.c_str(), port, MAX_PORT_NAME_SIZE);
        sPortName.assign(port);
    }
}

// -------------------------------------------------------------------------
// ModalSettingsDialogInterface / X2GUIEventInterface
// -------------------------------------------------------------------------

// Decode the 4-bit limit status byte into a human-readable string.
static std::string formatLimitStatus(unsigned char bits)
{
    if (bits == 0)
        return "None";

    std::string s;
    if (bits & 0x01) s += "N ";
    if (bits & 0x02) s += "S ";
    if (bits & 0x04) s += "E ";
    if (bits & 0x08) s += "W ";
    if (!s.empty() && s.back() == ' ')
        s.pop_back();
    return s;
}

// Push current firmware, limit, and position values into the open dialog.
static void refreshInfo(X2GUIExchangeInterface* dx, SXAO* pAO)
{
    char buf[32];

    std::string sFirmware;
    if (pAO->getFirmwareVersion(sFirmware) == SB_OK)
        dx->setText("label_firmware", sFirmware.c_str());
    else
        dx->setText("label_firmware", "Error");

    unsigned char limitBits = 0;
    if (pAO->getLimitStatus(limitBits) == SB_OK)
        dx->setText("label_limits", formatLimitStatus(limitBits).c_str());
    else
        dx->setText("label_limits", "Error");

    short nRa = 0, nDec = 0;
    pAO->getTipTilt(&nRa, &nDec);
    snprintf(buf, sizeof(buf), "%+d", (int)nRa);
    dx->setText("label_ra_pos", buf);
    snprintf(buf, sizeof(buf), "%+d", (int)nDec);
    dx->setText("label_dec_pos", buf);
}

int X2SXAO::execModalSettingsDialog(void)
{
    int nErr = SB_OK;
    X2ModalUIUtil uiutil(this, m_pTheSkyX);
    X2GUIInterface*         ui = uiutil.X2UI();
    X2GUIExchangeInterface* dx = NULL;
    bool bPressedOK = false;

    if (NULL == ui)
        return ERR_POINTER;

    if ((nErr = ui->loadUserInterface("SXAO.ui", deviceType(), m_nInstanceIndex)))
        return nErr;

    if (NULL == (dx = uiutil.X2DX()))
        return ERR_POINTER;

    X2MutexLocker ml(GetMutex());

    if (m_bLinked)
    {
        dx->setEnabled("pushButton_centre", true);
        dx->setEnabled("pushButton_unjam",  true);
        refreshInfo(dx, m_pAO);
    }
    else
    {
        dx->setEnabled("pushButton_centre", false);
        dx->setEnabled("pushButton_unjam",  false);
        dx->setText("label_firmware", "Not connected");
        dx->setText("label_limits",   "—");
        dx->setText("label_ra_pos",   "—");
        dx->setText("label_dec_pos",  "—");
    }
    dx->setText("label_status", "");

    if ((nErr = ui->exec(bPressedOK)))
        return nErr;

    return SB_OK;
}

void X2SXAO::uiEvent(X2GUIExchangeInterface* uiex, const char* pszEvent)
{
    if (!m_bLinked || !m_pAO)
        return;

    // Periodic refresh of position and limit status
    if (!strcmp(pszEvent, "on_timer"))
    {
        refreshInfo(uiex, m_pAO);
        return;
    }

    if (!strcmp(pszEvent, "on_pushButton_centre_clicked"))
    {
        uiex->setText("label_status", "Centring...");
        uiex->setEnabled("pushButton_centre", false);
        uiex->setEnabled("pushButton_unjam",  false);

        int nErr = m_pAO->centerAO();

        if (nErr == SB_OK)
            uiex->setText("label_status", "Centred.");
        else
        {
            char buf[64];
            snprintf(buf, sizeof(buf), "Error %d", nErr);
            uiex->setText("label_status", buf);
        }

        refreshInfo(uiex, m_pAO);
        uiex->setEnabled("pushButton_centre", true);
        uiex->setEnabled("pushButton_unjam",  true);
    }
    else if (!strcmp(pszEvent, "on_pushButton_unjam_clicked"))
    {
        uiex->setText("label_status", "Unjamming (slow centre)...");
        uiex->setEnabled("pushButton_centre", false);
        uiex->setEnabled("pushButton_unjam",  false);
        uiex->setEnabled("pushButtonOK",      false);
        uiex->setEnabled("pushButtonCancel",  false);

        int nErr = m_pAO->unjamAO();

        if (nErr == SB_OK)
            uiex->setText("label_status", "Unjam complete.");
        else
        {
            char buf[64];
            snprintf(buf, sizeof(buf), "Unjam error %d", nErr);
            uiex->setText("label_status", buf);
        }

        refreshInfo(uiex, m_pAO);
        uiex->setEnabled("pushButton_centre", true);
        uiex->setEnabled("pushButton_unjam",  true);
        uiex->setEnabled("pushButtonOK",      true);
        uiex->setEnabled("pushButtonCancel",  true);
    }
}
