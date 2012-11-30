#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <ctype.h>
#include <net/if.h>


#include "debugMenu.h"
#include "reconn.h"
#include "socket.h"
#include "eqptResponse.h"
#include "clientApp.h"

short socketPrint = FALSE;
short clientToPrint = -2;

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
    char *responsePtr;
    int numActiveClients = reconnClientsRegistered(ALL);

    if(numActiveClients == 0)
    {
        memset(&debugOutputString, 0, DEBUG_OUTPUT_LEN); 
        sprintf((char *)&debugOutputString, "\r\nThere are no active clients\r\n"); 
        sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);
    }
    else
    {
        if(socketPrint == FALSE)
        {
            /*
             * Ask the user which socket data they want to print
             */
            clientList(theSocketFd);

            memset(&debugOutputString, 0, DEBUG_OUTPUT_LEN);
            sprintf((char *)&debugOutputString, "\r\nEnter client number to display or \"all\" > ");
            sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);
            if((responsePtr = getInput(theSocketFd, YES)) != NULL)
            {
                if(strcmp(responsePtr, "all") == 0)
                {
                    clientToPrint = -1;
                    socketPrint = TRUE;
                }
                else if(!(((clientToPrint = atoi(responsePtr)) > 0) && (clientToPrint <= numActiveClients)))
                {
                    memset(&debugOutputString, 0, DEBUG_OUTPUT_LEN);
                    sprintf((char *)&debugOutputString, "\r\nInvalid selection(%d) entered\r\n ", clientToPrint); 
                    sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);
                    socketPrint = FALSE;
                }
                else
                {
                    socketPrint = TRUE;
                }
            }
            else
            {
                reconnDebugPrint("%s: recv() failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
                socketPrint = FALSE;
            }
        }
        else
        {
            socketPrint = FALSE;
            clientToPrint = -1;
        }
    }
    return RECONN_SUCCESS;
}
