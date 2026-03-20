#pragma once

#include <string.h>

// These headers are found via -I../X2-Examples/licensedinterfaces in the Makefile.
// The symlink ../licensedinterfaces -> ../X2-Examples/licensedinterfaces allows the
// SDK headers' internal "../../licensedinterfaces/..." cross-references to resolve.
#include "driverrootinterface.h"
#include "linkinterface.h"
#include "deviceinfointerface.h"
#include "driverinfointerface.h"
#include "basicstringinterface.h"
#include "serxinterface.h"
#include "basiciniutilinterface.h"
#include "theskyxfacadefordriversinterface.h"
#include "sleeperinterface.h"
#include "loggerinterface.h"
#include "mutexinterface.h"
#include "tickcountinterface.h"
#include "serialportparams2interface.h"
#include "modalsettingsdialoginterface.h"
#include "x2guiinterface.h"
#include "sberrorx.h"
#include "mutexinterface.h"

#include "sxao.h"

#define PLUGIN_VERSION 1.0

#define PARENT_KEY          "StarlightXpressAO"
#define CHILD_KEY_PORT_NAME "PortName"
#define MAX_PORT_NAME_SIZE  120
#define DEF_PORT_NAME       "No port found"

// -------------------------------------------------------------------------
// AdaptiveOpticsDriverInterface
// Reconstructed from TheSkyX binary analysis. The SDK does not ship this
// header. The interface is analogous to FocuserDriverInterface, specialised
// for DT_AO devices. The virtual method list below matches the observed
// secondary vtable layout of the built-in NoAdaptiveOptics and
// AdaptiveOpticsSimulator classes.
//
// NOTE: If TSX rejects the plugin at load time, the primary vtable layout
// may need adjustment. Consult the binary analysis notes in AO_TODO.md.
// -------------------------------------------------------------------------
class AdaptiveOpticsDriverInterface
    : public DriverRootInterface
    , public LinkInterface
    , public HardwareInfoInterface
    , public DriverInfoInterface
{
public:
    virtual ~AdaptiveOpticsDriverInterface() {}

    // DriverRootInterface
    virtual DeviceType deviceType(void) { return DriverRootInterface::DT_AO; }
    virtual int queryAbstraction(const char* pszName, void** ppVal) = 0;

    // LinkInterface
    virtual int  establishLink(void)                  = 0;
    virtual int  terminateLink(void)                  = 0;
    virtual bool isLinked(void) const                 = 0;

    // HardwareInfoInterface
    virtual void deviceInfoNameShort(BasicStringInterface& str) const           = 0;
    virtual void deviceInfoNameLong(BasicStringInterface& str) const            = 0;
    virtual void deviceInfoDetailedDescription(BasicStringInterface& str) const = 0;
    virtual void deviceInfoFirmwareVersion(BasicStringInterface& str)           = 0;
    virtual void deviceInfoModel(BasicStringInterface& str)                     = 0;

    // DriverInfoInterface
    virtual void   driverInfoDetailedInfo(BasicStringInterface& str) const = 0;
    virtual double driverInfoVersion(void) const                           = 0;
};

// -------------------------------------------------------------------------
// AdaptiveOpticsTipTiltInterface
// Reconstructed from TheSkyX binary analysis. The queryAbstraction key
// observed in the binary is the full versioned string below.
// -------------------------------------------------------------------------
#define AdaptiveOpticsTipTiltInterface_Name \
    "com.bisque.TheSkyX.AdaptiveOpticsTipTiltInterface/1.0"

class AdaptiveOpticsTipTiltInterface
{
public:
    virtual ~AdaptiveOpticsTipTiltInterface() {}

    // Return the hardware motor travel limits in raw counts.
    virtual int GetMotorLimits(unsigned short& nRaMin, unsigned short& nRaMax,
                               unsigned short& nDecMin, unsigned short& nDecMax) = 0;

    // Return the current tip-tilt position in raw counts.
    virtual int GetAOTipTilt(short* pnRa, short* pnDec) = 0;

    // Return the current tip-tilt position as a fraction of full range [-1, 1].
    virtual int GetAOTipTiltPercent(float* pdRa, float* pdDec) = 0;

    // Move to an absolute position in raw counts.
    virtual int CCAOTipTiltAbsolute(unsigned short nRa, unsigned short nDec) = 0;

    // Apply a relative offset in raw counts.
    virtual int CCAOTipTiltOffset(short nRaOffset, short nDecOffset) = 0;

    // Return the mirror to its center (rest) position.
    virtual int OnCenterAO() = 0;
};

// -------------------------------------------------------------------------
// X2SXAO — the plugin driver class
// -------------------------------------------------------------------------
class X2SXAO
    : public AdaptiveOpticsDriverInterface
    , public AdaptiveOpticsTipTiltInterface
    , public SerialPortParams2Interface
    , public ModalSettingsDialogInterface
    , public X2GUIEventInterface
{
public:
    X2SXAO(const char*                       pszDriverSelection,
            const int&                        nInstanceIndex,
            SerXInterface*                    pSerX,
            TheSkyXFacadeForDriversInterface* pTheSkyX,
            SleeperInterface*                 pSleeper,
            BasicIniUtilInterface*            pIniUtil,
            LoggerInterface*                  pLogger,
            MutexInterface*                   pIOMutex,
            TickCountInterface*               pTickCount);

    virtual ~X2SXAO();

    // DriverRootInterface
    virtual DeviceType deviceType(void) { return DriverRootInterface::DT_AO; }
    virtual int queryAbstraction(const char* pszName, void** ppVal);

    // LinkInterface
    virtual int  establishLink(void);
    virtual int  terminateLink(void);
    virtual bool isLinked(void) const;

    // HardwareInfoInterface
    virtual void deviceInfoNameShort(BasicStringInterface& str) const;
    virtual void deviceInfoNameLong(BasicStringInterface& str) const;
    virtual void deviceInfoDetailedDescription(BasicStringInterface& str) const;
    virtual void deviceInfoFirmwareVersion(BasicStringInterface& str);
    virtual void deviceInfoModel(BasicStringInterface& str);

    // DriverInfoInterface
    virtual void   driverInfoDetailedInfo(BasicStringInterface& str) const;
    virtual double driverInfoVersion(void) const;

    // AdaptiveOpticsTipTiltInterface
    virtual int GetMotorLimits(unsigned short& nRaMin, unsigned short& nRaMax,
                               unsigned short& nDecMin, unsigned short& nDecMax);
    virtual int GetAOTipTilt(short* pnRa, short* pnDec);
    virtual int GetAOTipTiltPercent(float* pdRa, float* pdDec);
    virtual int CCAOTipTiltAbsolute(unsigned short nRa, unsigned short nDec);
    virtual int CCAOTipTiltOffset(short nRaOffset, short nDecOffset);
    virtual int OnCenterAO();

    // SerialPortParams2Interface
    virtual void         portName(BasicStringInterface& str) const;
    virtual void         setPortName(const char* szPort);
    // SX AO is fixed at 9600 8N1
    virtual unsigned int baudRate() const      { return 9600; }
    virtual void         setBaudRate(unsigned int) {}
    virtual bool         isBaudRateFixed() const { return true; }

    virtual SerXInterface::Parity parity() const          { return SerXInterface::B_NOPARITY; }
    virtual void                  setParity(const SerXInterface::Parity&) {}
    virtual bool                  isParityFixed() const   { return true; }

    // ModalSettingsDialogInterface
    virtual int initModalSettingsDialog(void) { return 0; }
    virtual int execModalSettingsDialog(void);

    // X2GUIEventInterface
    virtual void uiEvent(X2GUIExchangeInterface* uiex, const char* pszEvent);

private:
    SerXInterface*                    GetSerX()      { return m_pSerX; }
    TheSkyXFacadeForDriversInterface* GetTheSkyX()   { return m_pTheSkyX; }
    SleeperInterface*                 GetSleeper()   { return m_pSleeper; }
    BasicIniUtilInterface*            GetIniUtil()   { return m_pIniUtil; }
    LoggerInterface*                  GetLogger()    { return m_pLogger; }
    MutexInterface*                   GetMutex()     { return m_pIOMutex; }
    TickCountInterface*               GetTickCount() { return m_pTickCount; }

    void getPortName(std::string& sPortName) const;

    int m_nInstanceIndex;

    SerXInterface*                    m_pSerX;
    TheSkyXFacadeForDriversInterface* m_pTheSkyX;
    SleeperInterface*                 m_pSleeper;
    BasicIniUtilInterface*            m_pIniUtil;
    LoggerInterface*                  m_pLogger;
    MutexInterface*                   m_pIOMutex;
    TickCountInterface*               m_pTickCount;

    SXAO* m_pAO;
    bool  m_bLinked;
};
