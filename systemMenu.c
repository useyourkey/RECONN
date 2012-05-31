#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "debugMenu.h"
#include "reconn.h"

static int on = FALSE;
static int external();
extern int enableExternalMessages;

debugMenuStruct systemDebugMenu[] =
{
    {"system", "System wide debug Menus", NULL, NULL, NULL},
    {NULL, NULL, "external", "diverts consoles messages to this debug port", external},
};

void registerSystemDebugMenu()
{
    registerDebugCommand(&systemDebugMenu[0], sizeof(systemDebugMenu)/sizeof(debugMenuStruct));
}
static int external(theSocketFd)
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
    sendSocket(theSocketFd, theMessage, strlen(theMessage), 0);
    return RECONN_SUCCESS;
}
