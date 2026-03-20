#include "x2sxao.h"

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
    m_pAO     = new SXAO();
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

int X2SXAO::execModalSettingsDialog(void)
{
    // TODO: implement settings UI
    return SB_OK;
}

void X2SXAO::uiEvent(X2GUIExchangeInterface* /*uiex*/, const char* /*pszEvent*/)
{
    // TODO: handle UI events
}
