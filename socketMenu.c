#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "debugMenu.h"
#include "reconn.h"
#include "socket.h"

short socketPrint = FALSE;

static int showComms();
debugMenuStruct socketDebugMenu[] =
{
    {"socket", "Socket debug Menus", NULL, NULL, NULL},
    {NULL, NULL, "show", "Turns on socket communication printing (a lot of data)", showComms}
};

void registerSocketDebugMenu()
{
    registerDebugCommand(&socketDebugMenu[0], sizeof(socketDebugMenu)/sizeof(debugMenuStruct));
}

static int showComms(int theSocketFd)
{
    extern short socketPrint;


    socketPrint = (socketPrint == TRUE)  ? FALSE : TRUE;
    
    return RECONN_SUCCESS;
}
