#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <net/if.h>


#include "debugMenu.h"
#include "clientApp.h"
#include "remoteMonitor.h"
#include "eqptResponse.h"
#include "socket.h"

static int remoteState(int);
static int numRemoteClients(int);
static int remoteKill(int);
static int remoteStopHb(int);
static int remoteStatus(int);

debugMenuStruct remoteMonDebugMenu[] =
{
    {"remote", "Remote Monitor debug Menus", NULL, NULL, NULL},
    {NULL, NULL, "state", "Display remote monitor state", remoteState},
    {NULL, NULL, "status","Displays remote monitor variables", remoteStatus},
    {NULL, NULL, "show", "Display the number of registered remote clients", numRemoteClients},
    {NULL, NULL, "kill", "Kills remote monitor and closes socket to webservice ", remoteKill},
    {NULL, NULL, "stop", "Stops transmission of heart beats to the webservice", remoteStopHb}
};

void registerRemoteMonDebugMenu()
{
    registerDebugCommand(&remoteMonDebugMenu[0], sizeof(remoteMonDebugMenu)/sizeof(debugMenuStruct));
}

//*************************************************************************************
//*************************************************************************************
// FUNCTION:    remoteState
//
// DESCRIPTION: Function used to display the state of Remote Monitor
//
// Parameters:
//
//************************************************************************************
static int remoteState(int theSocketFd)
{
    YESNO state;
    memset(debugOutputString, 0, DEBUG_OUTPUT_LEN);
    sprintf(debugOutputString, "\r\n\r\nRemote Monitor is %s\r\n", ((state = getRemoteMonitorState() == YES)) ? "ACTIVE": "INACTIVE");
    sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0); 

    if(state == YES)
    {
        memset(debugOutputString, 0, DEBUG_OUTPUT_LEN);


    }
    return RECONN_SUCCESS;
}

//*************************************************************************************
//*************************************************************************************
// FUNCTION:    numRemoteClients
//
// DESCRIPTION: Function to display the number of remote clients connected.
//
// Parameters:
//
//************************************************************************************
static int numRemoteClients(int theSocketFd)
{
    memset((char *)&debugOutputString, 0, DEBUG_OUTPUT_LEN);
    sprintf((char *)&debugOutputString, "\r\n\r\nThe number of registered remote clients = %d\r\n", reconnClientsRegistered(REMOTE));
    sendSocket(theSocketFd, (unsigned char *)&debugOutputString, strlen(debugOutputString), 0);
    if(reconnClientsRegistered(REMOTE) == 0)
    {
        sprintf((char *)&debugOutputString, "Web service must have data transmission turned off.\r\n");
        sendSocket(theSocketFd, (unsigned char *)&debugOutputString, strlen(debugOutputString), 0);
    }

    return RECONN_SUCCESS;
}

//*************************************************************************************
//*************************************************************************************
// FUNCTION:    remoteKill
//
// DESCRIPTION: Function used to close the socket to the remote server
//
// Parameters:
//
//************************************************************************************
static int remoteKill(int theSocketFd)
{
    YESNO wasRmDebugOn = YES;

    UNUSED_PARAM(theSocketFd);
    /*
     * Temporarily turn on remote monitor's debug -- if it is not already on.
     */
    if(!RECONN_DEBUG_ON(RM_DEBUG))
    {
        wasRmDebugOn = NO;
        gReconnDebugLevel |= RM_DEBUG;
    }
    remoteMonitorActivate(NO, NULL);
    if(wasRmDebugOn == NO)
    {
        gReconnDebugLevel &= ~RM_DEBUG;
    }
    return RECONN_SUCCESS;
}
//*************************************************************************************
//*************************************************************************************
// FUNCTION:    remoteStopHb
//
// DESCRIPTION: Function used to stop heart beat message being sent to the web service
//              this will cause the web service to close the socket.
//
// Parameters:
//
//************************************************************************************
static int remoteStopHb(int theSocketFd)
{
    YESNO wasRmDebugOn = YES;

    UNUSED_PARAM(theSocketFd);
    /*
     * Temporarily turn on remote monitor's debug -- if it is not already on.
     */
    if(!RECONN_DEBUG_ON(RM_DEBUG))
    {
        wasRmDebugOn = NO;
        gReconnDebugLevel |= RM_DEBUG;
    }
    stopRemoteMonitorHb = TRUE;
    if(wasRmDebugOn == NO)
    {
        gReconnDebugLevel &= ~RM_DEBUG;
    }

    return RECONN_SUCCESS;
}
//*************************************************************************************
//*************************************************************************************
// FUNCTION:    remoteStatus
//
// DESCRIPTION: Function used to display information about Remote monitor key variables
//
// Parameters:
//
//************************************************************************************
static int remoteStatus(int theSocketFd)
{
    REMOTE_MONITOR_DEBUG_DATA theDebugData;

    memset(&theDebugData, 0, sizeof(REMOTE_MONITOR_DEBUG_DATA));
    remoteMonitorGetDebugValues(&theDebugData);

    memset(debugOutputString, 0, DEBUG_OUTPUT_LEN);
    sprintf(debugOutputString, "\r\n%-21s %s","Variable", "Value");
    sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0); 

    memset(debugOutputString, 0, DEBUG_OUTPUT_LEN);
    sprintf(debugOutputString, "\r\n============================");
    sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0); 

    memset(debugOutputString, 0, DEBUG_OUTPUT_LEN);
    sprintf(debugOutputString, "\r\n%-21s %d","clientIndex", (theDebugData.theContextPtr) ? theDebugData.theContextPtr->index : -1);
    sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0); 

    memset(debugOutputString, 0, DEBUG_OUTPUT_LEN);
    sprintf(debugOutputString, "\r\n%-21s %d","webServerSocket",  theDebugData.webServerSocket);
    sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0); 

    memset(debugOutputString, 0, DEBUG_OUTPUT_LEN);
    sprintf(debugOutputString, "\r\n%-21s %s","IpAddress", (theDebugData.theContextPtr) ? inet_ntoa(theDebugData.theContextPtr->sourceIp.sin_addr) : "0.0.0.0");
    sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0); 

    memset(debugOutputString, 0, DEBUG_OUTPUT_LEN);
    sprintf(debugOutputString, "\r\n%-21s %u","port", (theDebugData.theContextPtr) ? ntohs(theDebugData.theContextPtr->sourceIp.sin_port): 0);
    sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0); 

    memset(debugOutputString, 0, DEBUG_OUTPUT_LEN);
    sprintf(debugOutputString, "\r\n%-21s %s","remoteMonitorDone", (theDebugData.remoteMonitorDone == FALSE) ? "FALSE" : "TRUE");
    sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0); 

    memset(debugOutputString, 0, DEBUG_OUTPUT_LEN);
    sprintf(debugOutputString, "\r\n%-21s %s","stopRemoteMonitorHb", (stopRemoteMonitorHb == FALSE) ? "FALSE" : "TRUE");
    sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0); 

    memset(debugOutputString, 0, DEBUG_OUTPUT_LEN);
    sprintf(debugOutputString, "\r\n%-21s %s","remoteMonitorActive", (theDebugData.remoteMonitorActive == YES) ? "YES" : "NO");
    sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0); 

    memset(debugOutputString, 0, DEBUG_OUTPUT_LEN);
    sprintf(debugOutputString, "\r\n%-21s %s","theState", theDebugData.theState == RM_INIT ? "INIT" : 
            theDebugData.theState == RM_GET_HANDSHAKE ? "HANDSHAKE" : 
            theDebugData.theState ==  RM_RCV_DATA ? "RCV_DATA" : 
            "UNKNOWN");
    sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0); 


    return RECONN_SUCCESS;
}
