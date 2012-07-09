#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "debugMenu.h"
#include "reconn.h"
#include "socket.h"
#include "gpio.h"
#include "dmm.h"

static int meterOn();
static int meterOff();
static int meterDiags();
extern int theDebugSocketFd;


debugMenuStruct dmmDebugMenu[] =
{
    {"dmm", "Digital MultiMeter debug Menus", NULL, NULL, NULL},
    {NULL, NULL, "on",      "powers up the meter", meterOn},
    {NULL, NULL, "off",     "powers off the meter", meterOff},
    {NULL, NULL, "diags",   "runs meter diags", meterDiags}
};

void registerDmmDebugMenu()
{
    registerDebugCommand(&dmmDebugMenu[0], sizeof(dmmDebugMenu)/sizeof(debugMenuStruct));
}

static int meterOn()
{
    if(reconnGpioAction(DMM_POWER_GPIO, ENABLE, NULL) == RECONN_FAILURE)
    {
        reconnDebugPrint("%s: reconnGpioAction(DMM_POWER_GPIO, ENABLE, NULL) failed. \n", __FUNCTION__);
    }
    return RECONN_SUCCESS;
}

static int meterOff()
{
    if(reconnGpioAction(DMM_POWER_GPIO, DISABLE, NULL) == RECONN_FAILURE)
    {
        reconnDebugPrint("%s: reconnGpioAction(DMM_POWER_GPIO, DISABLE, NULL) failed. \n", __FUNCTION__);
    }
    return RECONN_SUCCESS;
}

static int meterDiags()
{
    char outbuf[DEBUG_OUTPUT_LEN];

    memset((char *)&outbuf, 0, DEBUG_OUTPUT_LEN);
    sprintf((char *)&outbuf, "Diagnostics %s\n", (dmmDiags() == RECONN_SUCCESS) ? "PASSED": "FAILED");
    sendSocket(theDebugSocketFd, (unsigned char *)&outbuf, strlen(outbuf), 0);
    return RECONN_SUCCESS;
}
