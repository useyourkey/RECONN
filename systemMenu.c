#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "reconn.h"
#include "debugMenu.h"
#include "powerMgmt.h"
#include "socket.h"

static int on = FALSE;
static int external();
static int timers();
extern int enableExternalMessages;

debugMenuStruct systemDebugMenu[] =
{
    {"system", "System wide debug Menus", NULL, NULL, NULL},
    {NULL, NULL, "external", "diverts consoles messages to this debug port", external},
    {NULL, NULL, "timers", "Displays the system's shutdown timers", timers}
};

void registerSystemDebugMenu()
{
    registerDebugCommand(&systemDebugMenu[0], sizeof(systemDebugMenu)/sizeof(debugMenuStruct));
}

static int external(int theSocketFd)
{
    char *theMessage;
    if(on == TRUE)
    {
        theMessage = "Disabling console messages to this port\n";
        on = enableExternalMessages = FALSE;
    }
    else
    {
        theMessage = "Enabling console messages to this port\n";
        on = enableExternalMessages = TRUE;
    }
    sendSocket(theSocketFd, (unsigned char *)&theMessage, strlen(theMessage), 0);
    return RECONN_SUCCESS;
}

static int timers(int theSocketFd)
{
    PowerMgmtEqptCounters eqptCounters;
    char theString[RECONN_PAYLOAD_SIZE]; // RECONN_PAYLOAD_SIZE (50) should be enough

    if(getStandbyCounters(&eqptCounters) == RECONN_SUCCESS)
    {
        memset(theString, 0, RECONN_PAYLOAD_SIZE);
        sprintf(theString, "PowerMeterCounter = %d\n", eqptCounters.PowerMeterCounter);
        sendSocket(theSocketFd, (unsigned char *)theString, strlen(theString), 0);

        memset(theString, 0, RECONN_PAYLOAD_SIZE);
        sprintf(theString, "DmmCounter = %d\n", eqptCounters.DmmCounter);
        sendSocket(theSocketFd, (unsigned char *)theString, strlen(theString), 0);

        memset(theString, 0, RECONN_PAYLOAD_SIZE);
        sprintf(theString, "GpsCounter = %d\n", eqptCounters.GpsCounter);
        sendSocket(theSocketFd, (unsigned char *)theString, strlen(theString), 0);

        memset(theString, 0, RECONN_PAYLOAD_SIZE);
        sprintf(theString, "SpectrumAnalyzerCounter = %d\n", eqptCounters.SpectrumAnalyzerCounter);
        sendSocket(theSocketFd, (unsigned char *)theString, strlen(theString), 0);

        memset(theString, 0, RECONN_PAYLOAD_SIZE);
        sprintf(theString, "ReconnSystemCounter = %d\n", eqptCounters.ReconnSystemCounter);
        sendSocket(theSocketFd, (unsigned char *)theString, strlen(theString), 0);
    }
    return RECONN_SUCCESS;
}
