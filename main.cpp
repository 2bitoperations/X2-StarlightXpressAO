#include <stdio.h>
#include "main.h"
#include "x2sxao.h"
#include "basicstringinterface.h"

extern "C" PlugInExport int sbPlugInName2(BasicStringInterface& str)
{
    str = "Starlight Xpress AO PlugIn";
    return 0;
}

extern "C" PlugInExport int sbPlugInFactory2(
    const char*                         pszDisplayName,
    const int&                          nInstanceIndex,
    SerXInterface*                      pSerXIn,
    TheSkyXFacadeForDriversInterface*   pTheSkyXIn,
    SleeperInterface*                   pSleeperIn,
    BasicIniUtilInterface*              pIniUtilIn,
    LoggerInterface*                    pLoggerIn,
    MutexInterface*                     pIOMutexIn,
    TickCountInterface*                 pTickCountIn,
    void**                              ppObjectOut)
{
    *ppObjectOut = NULL;
    X2SXAO* pDriver = new X2SXAO(
        pszDisplayName,
        nInstanceIndex,
        pSerXIn,
        pTheSkyXIn,
        pSleeperIn,
        pIniUtilIn,
        pLoggerIn,
        pIOMutexIn,
        pTickCountIn);
    *ppObjectOut = pDriver;
    return 0;
}
