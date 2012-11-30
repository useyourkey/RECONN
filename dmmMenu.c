#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
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

static char *errMeterIsOn = "\r\nThe DMM is already powered on\r\n";


debugMenuStruct dmmDebugMenu[] =
{
    {"dmm", "Digital MultiMeter debug Menus", NULL, NULL, NULL},
    {NULL, NULL, "on",      "Powers up the meter", meterOn},
    {NULL, NULL, "off",     "Powers off the meter", meterOff},
    {NULL, NULL, "diags",   "Runs meter diags", meterDiags}
};

//*************************************************************************************
//*************************************************************************************
// FUNCTION:    registerDmmDebugMenu
//
// DESCRIPTION: Interface used to register the DMM diag menus with the reconn application.
//
// Parameters:
//
//*************************************************************************************
void registerDmmDebugMenu()
{
    registerDebugCommand(&dmmDebugMenu[0], sizeof(dmmDebugMenu)/sizeof(debugMenuStruct));
}

//*************************************************************************************
//*************************************************************************************
// FUNCTION:    meterOn
//
// DESCRIPTION: Function used to turn the DMM on.
//
// Parameters:
//
//*************************************************************************************
static int meterOn(int theSocketFd)
{
    short theValue;

    pthread_mutex_lock(&dmmMutex);
    if(reconnGpioAction(DMM_POWER_GPIO, READ, &theValue) == RECONN_SUCCESS)
    {
        if(theValue == GPIO_IS_INACTIVE)
        {
            dmmInit(&(gEqptDescriptors.dmmFd));
        }
        else
        { 
            sendSocket(theSocketFd, (unsigned char *)errMeterIsOn, strlen(errMeterIsOn), 0);
        }
    }
    else
    {
        reconnDebugPrint("%s: reconnGpioAction(DMM_POWER_GPIO, READ) failed. \n", __FUNCTION__);
    }
    pthread_mutex_unlock(&dmmMutex);
    return RECONN_SUCCESS;
}

//*************************************************************************************
//*************************************************************************************
// FUNCTION:    meterOff
//
// DESCRIPTION: Function used to turn the DMM off.
//
// Parameters:
//
//*************************************************************************************
static int meterOff()
{
    pthread_mutex_lock(&dmmMutex);
    dmmPowerDown();
    pthread_mutex_unlock(&dmmMutex);
    return RECONN_SUCCESS;
}

//*************************************************************************************
//*************************************************************************************
// FUNCTION:    meterDiags
//
// DESCRIPTION: Function used to executed the Digital Multimeter diagnostics.
//              The meter does not really have a diagnostics that checks its circuitry.
//              This function simplies runs the DMM through all possible configuration 
//              settings and verifies the setting outputs.
//
// Parameters:
//
//*************************************************************************************
static int meterDiags()
{
    char outbuf[DEBUG_OUTPUT_LEN];

    pthread_mutex_lock(&dmmMutex);
    memset((char *)&outbuf, 0, DEBUG_OUTPUT_LEN);
    sprintf((char *)&outbuf, "\r\nDiagnostics %s\r\n", (dmmDiags() == RECONN_SUCCESS) ? "PASSED": "FAILED");
    sendSocket(theDebugSocketFd, (unsigned char *)&outbuf, strlen(outbuf), 0);
    pthread_mutex_unlock(&dmmMutex);
    return RECONN_SUCCESS;
}
