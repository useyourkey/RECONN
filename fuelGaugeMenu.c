#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "debugMenu.h"
#include "reconn.h"
#include "socket.h"
#include "clientApp.h"
#include "upgrade.h"
#include "eqptResponse.h"

extern void reconnMasterIphone();
extern int theDebugSocketFd;

static char *percentageMessage = "Battery charge is at %d%%\n";

static int fuelPercent();
debugMenuStruct fuelGaugeDebugMenu[] =
{
    {"fuel", "Fuel gauge debug Menus", NULL, NULL, NULL},
    {NULL, NULL, "percent", "Returns the percentage of battery charge", fuelPercent}
};

void registerFuelGaugeDebugMenu()
{
    registerDebugCommand(&fuelGaugeDebugMenu[0], sizeof(fuelGaugeDebugMenu)/sizeof(debugMenuStruct));
}

static int fuelPercent(int theSocketFd)
{
    // Add 3 to account for the percentage number (0 - 100)
    char buf[strlen(percentageMessage+3)];
    extern short batteryPercentage;

    memset(buf, 0, strlen(percentageMessage+3));
    sprintf(buf, percentageMessage, batteryPercentage);
    sendSocket(theSocketFd, (unsigned char *)&buf, strlen(buf), 0);
    return RECONN_SUCCESS;
}
