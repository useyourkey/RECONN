#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "debugMenu.h"
#include "reconn.h"
#include "socket.h"


static int dmmDebug();
static int powerMeterDebug();
static int specanaDebug();
static int batteryDebug();
static int lnbDebug();
static int slaveDebug();
static int externalDebug();
static int timingDebug();
static int remoteDebug();
static int eqptDebug();

extern int theDebugSocketFd;
extern int enableExternalMessages;
static char *responsePtr;

YESNO gDebugTimingEnabled = NO;
YESNO gExternalDebugEnabled = NO;
debugMenuStruct ReconnMsgsMenu[] = 
{ 
    {"msgs", "Equipment Debug Messages Menu", NULL, NULL, NULL},
    {NULL, NULL, "dmm",     "Multimeter debug", dmmDebug},
    {NULL, NULL, "pmeter",  "Power Meter debug", powerMeterDebug},
    {NULL, NULL, "spec",    "Spectrum Analyzer debug", specanaDebug},
    {NULL, NULL, "battery", "Battery status debug", batteryDebug},
    {NULL, NULL, "lnb",     "LNB debug", lnbDebug},
    {NULL, NULL, "slave",   "Slave message debug", slaveDebug},
    {NULL, NULL, "timing",  "Turns on spectrum analyzer packet timing", timingDebug},
    {NULL, NULL, "external","Enables debug socket 4084", externalDebug},
    {NULL, NULL, "eqpt",    "Enables eqpt task debug messages", eqptDebug},
    {NULL, NULL, "remote",  "Remote Monitor debug", remoteDebug}
};

void registerReconnMsgsMenu() 
{ 
    registerDebugCommand(&ReconnMsgsMenu[0], sizeof(ReconnMsgsMenu)/sizeof(debugMenuStruct)); 
}

//*************************************************************************************
//*************************************************************************************
// FUNCTION:    dmmDebug
//
// DESCRIPTION: Function used to set DMM debug settings.
//
// Parameters:
//
//*************************************************************************************
static int dmmDebug()
{
    char *responsePtr;
    YESNO invalidResponse = NO;

    while(1)
    {
        sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_CLEARSCREEN, strlen(DEBUG_CLEARSCREEN), 0);
        sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_CURSORHOME, strlen(DEBUG_CURSORHOME), 0);

        if(invalidResponse == YES)
        {
            sendSocket(theDebugSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);
            invalidResponse = NO;
        }
        else
        {
            showDebugStatus();
        }
        sprintf((char *)&debugOutputString, "\r\n\r\n0) No Debug\r\n1) toggle received packets\r\n2) toggle packets sent");
        sendSocket(theDebugSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);

        sprintf((char *)&debugOutputString, "\r\n3) toggle eqpt response messages \r\nq) quit\r\n(0, 1, 2, 3 or q)> ");
        sendSocket(theDebugSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);
        if((responsePtr = getInput(theDebugSocketFd, YES)) != NULL)
        {
            if(strcmp(responsePtr, "0") == 0)
            {
                gReconnDebugLevel = NO_DEBUG;
            }
            else if(strcmp(responsePtr, "1") == 0)
            {
                if(RECONN_DEBUG_ON(DMM_DEBUG_RCV))
                {
                    gReconnDebugLevel &= ~DMM_DEBUG_RCV;
                }
                else
                {
                    gReconnDebugLevel |= DMM_DEBUG_RCV;
                }
            }
            else if(strcmp(responsePtr, "2") == 0)
            {
                if(RECONN_DEBUG_ON(DMM_DEBUG_SND))
                {
                    gReconnDebugLevel &= ~DMM_DEBUG_SND;
                }
                else
                {
                    gReconnDebugLevel |= DMM_DEBUG_SND;
                }
            }
            else if(strcmp(responsePtr, "3") == 0)
            {
                if(RECONN_DEBUG_ON(DMM_DEBUG_EQPT))
                {
                    gReconnDebugLevel &= ~DMM_DEBUG_EQPT;
                }
                else
                {
                    gReconnDebugLevel |= DMM_DEBUG_EQPT;
                }
            }
            else if(strcmp(responsePtr, "q") == 0)
            {
                sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_CLEARSCREEN, strlen(DEBUG_CLEARSCREEN), 0);
                sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_CURSORHOME, strlen(DEBUG_CURSORHOME), 0);
                break;
            }
            else if(strcmp(responsePtr, ""))
            {
                sprintf((char *)&debugOutputString, "Invalid Response \"%s\"\r\n", responsePtr);
                invalidResponse = YES;
            }
        }
        else
        {
            reconnDebugPrint("%s: recv() failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
            break;
        }
    }
    return RECONN_SUCCESS;
}

//*************************************************************************************
//*************************************************************************************
// FUNCTION:    powerMeterDebug
//
// DESCRIPTION: Function used to set Power Meter debug settings.
//
// Parameters:
//
//*************************************************************************************
static int powerMeterDebug()
{
    YESNO invalidResponse = NO;
    while(1)
    {
        sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_CLEARSCREEN, strlen(DEBUG_CLEARSCREEN), 0);
        sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_CURSORHOME, strlen(DEBUG_CURSORHOME), 0);

        if(invalidResponse == YES)
        {
            sendSocket(theDebugSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);
            invalidResponse = NO;
        }
        else
        {
            showDebugStatus();
        }
        sprintf((char *)&debugOutputString, "\r\n\r\n0) No Debug\r\n1) toggle received packets\r\n2) toggle packets sent");
        sendSocket(theDebugSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);

        sprintf((char *)&debugOutputString, "\r\n3) toggle eqpt response messages \r\nq) quit\r\n(0, 1, 2, 3 or q)> ");
        sendSocket(theDebugSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);
        if((responsePtr = getInput(theDebugSocketFd, YES)) != NULL)
        {
            if(strcmp(responsePtr, "0") == 0)
            {
                gReconnDebugLevel = NO_DEBUG;
            }
            else if(strcmp(responsePtr, "1") == 0)
            {
                if(RECONN_DEBUG_ON(PMETER_DEBUG_RCV))
                {
                    gReconnDebugLevel &= ~PMETER_DEBUG_RCV;
                }
                else
                {
                    gReconnDebugLevel |= PMETER_DEBUG_RCV;
                }
            }
            else if(strcmp(responsePtr, "2") == 0)
            {
                if(RECONN_DEBUG_ON(PMETER_DEBUG_SND))
                {
                    gReconnDebugLevel &= ~PMETER_DEBUG_SND;
                }
                else
                {
                    gReconnDebugLevel |= PMETER_DEBUG_SND;
                }
            }
            else if(strcmp(responsePtr, "3") == 0)
            {
                gReconnDebugLevel |= PMETER_DEBUG_EQPT;
            }
            else if(strcmp(responsePtr, "q") == 0)
            {
                sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_CLEARSCREEN, strlen(DEBUG_CLEARSCREEN), 0);
                sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_CURSORHOME, strlen(DEBUG_CURSORHOME), 0);
                break;
            }
            else if(strcmp(responsePtr, ""))
            {
                sprintf((char *)&debugOutputString, "Invalid Response \"%s\"\r\n", responsePtr);
                invalidResponse = YES;
            }
        }
        else
        {
            reconnDebugPrint("%s: recv() failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
            break;
        }
    }
    return RECONN_SUCCESS;
}

//*************************************************************************************
//*************************************************************************************
// FUNCTION:    specanaDebug
//
// DESCRIPTION: Function used to set Spectrum Analyzer debug settings.
//
// Parameters:
//
//*************************************************************************************
static int specanaDebug()
{
    YESNO invalidResponse = NO;
    while(1)
    {
        sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_CLEARSCREEN, strlen(DEBUG_CLEARSCREEN), 0);
        sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_CURSORHOME, strlen(DEBUG_CURSORHOME), 0);

        if(invalidResponse == YES)
        {
            sendSocket(theDebugSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);
            invalidResponse = NO;
        }
        else
        {
            showDebugStatus();
        }
        sprintf((char *)&debugOutputString, "\r\n\r\n0) No Debug\r\n1) toggle received packets\r\n2) toggle packets sent");
        sendSocket(theDebugSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);

        sprintf((char *)&debugOutputString, "\r\n3) toggle eqpt response messages \r\nq) quit\r\n(0, 1, 2, 3 or q)> ");
        sendSocket(theDebugSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);
        if((responsePtr = getInput(theDebugSocketFd, YES)) != NULL)
        {
            if(strcmp(responsePtr, "0") == 0)
            {
                gReconnDebugLevel = NO_DEBUG;
            }
            else if(strcmp(responsePtr, "1") == 0)
            {
                if(RECONN_DEBUG_ON(SPECANA_DEBUG_RCV))
                {
                    gReconnDebugLevel &= ~SPECANA_DEBUG_RCV;
                }
                else
                {
                    gReconnDebugLevel |= SPECANA_DEBUG_RCV;
                }
            }
            else if(strcmp(responsePtr, "2") == 0)
            {
                if(RECONN_DEBUG_ON(SPECANA_DEBUG_SND))
                {
                    gReconnDebugLevel &= ~SPECANA_DEBUG_SND;
                }
                else
                {
                    gReconnDebugLevel |= SPECANA_DEBUG_SND;
                }
            }
            else if(strcmp(responsePtr, "3") == 0)
            {
                if(RECONN_DEBUG_ON(SPECANA_DEBUG_EQPT))
                {
                    gReconnDebugLevel &= ~SPECANA_DEBUG_EQPT;
                }
                else
                {
                    gReconnDebugLevel |= SPECANA_DEBUG_EQPT;
                }
            }
            else if(strcmp(responsePtr, "q") == 0)
            {
                sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_CLEARSCREEN, strlen(DEBUG_CLEARSCREEN), 0);
                sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_CURSORHOME, strlen(DEBUG_CURSORHOME), 0);
                break;
            }
            else if(strcmp(responsePtr, ""))
            {
                sprintf((char *)&debugOutputString, "Invalid Response \"%s\"\r\n", responsePtr);
                invalidResponse = YES;
            }
        }
        else
        {
            reconnDebugPrint("%s: recv() failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
            break;
        }
    }
    return RECONN_SUCCESS;
}

//*************************************************************************************
//*************************************************************************************
// FUNCTION:    batteryDebug
//
// DESCRIPTION: Function used to set Battery  debug settings.
//
// Parameters:
//
//*************************************************************************************
static int batteryDebug()
{
    YESNO invalidResponse = NO;

    while(1)
    {
        sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_CLEARSCREEN, strlen(DEBUG_CLEARSCREEN), 0);
        sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_CURSORHOME, strlen(DEBUG_CURSORHOME), 0);

        if(invalidResponse == YES)
        {
            sendSocket(theDebugSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);
            invalidResponse = NO;
        }
        else
        {
            showDebugStatus();
        }
        sprintf((char *)&debugOutputString, "\r\n\r\n0) No Debug\r\n1) toggle received packets\r\n2) toggle packets sent");
        sendSocket(theDebugSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);
        sprintf((char *)&debugOutputString, "\r\nq) quit\r\n(0, 1, 2 or q)> ");
        sendSocket(theDebugSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);

        if((responsePtr = getInput(theDebugSocketFd, YES)) != NULL)
        {
            if(strcmp(responsePtr, "0") == 0)
            {
                gReconnDebugLevel = NO_DEBUG;
            }
            else if(strcmp(responsePtr, "1") == 0)
            {
                if(RECONN_DEBUG_ON(BATTERY_DEBUG_RCV))
                {
                    gReconnDebugLevel &= ~BATTERY_DEBUG_RCV;
                }
                else
                {
                    gReconnDebugLevel |= BATTERY_DEBUG_RCV;
                }
            }
            else if(strcmp(responsePtr, "2") == 0)
            {
                if(RECONN_DEBUG_ON(BATTERY_DEBUG_SND))
                {
                    gReconnDebugLevel &= ~BATTERY_DEBUG_SND;
                }
                else
                {
                    gReconnDebugLevel |= BATTERY_DEBUG_SND;
                }
            }
            else if(strcmp(responsePtr, "q") == 0)
            {
                sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_CLEARSCREEN, strlen(DEBUG_CLEARSCREEN), 0);
                sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_CURSORHOME, strlen(DEBUG_CURSORHOME), 0);
                break;
            }
            else if(strcmp(responsePtr, ""))
            {
                sprintf((char *)&debugOutputString, "Invalid Response \"%s\"\r\n", responsePtr);
                invalidResponse = YES;
            }
        }
        else
        {
            reconnDebugPrint("%s: recv() failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
            break;
        }
    }
    return RECONN_SUCCESS;
}

//*************************************************************************************
//*************************************************************************************
// FUNCTION:    lnbDebug
//
// DESCRIPTION: Function used to set LNB debug settings.
//
// Parameters:
//
//*************************************************************************************
static int lnbDebug()
{
    YESNO invalidResponse = NO;
    while(1)
    {
        sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_CLEARSCREEN, strlen(DEBUG_CLEARSCREEN), 0);
        sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_CURSORHOME, strlen(DEBUG_CURSORHOME), 0);

        if(invalidResponse == YES)
        {
            sendSocket(theDebugSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);
            invalidResponse = NO;
        }
        else
        {
            showDebugStatus();
        }
        sprintf((char *)&debugOutputString, "\r\n\r\n0) No Debug\r\n1) toggle received packets \r\nq) quit\r\n(0, 1 or q)> ");
        sendSocket(theDebugSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);

        if((responsePtr = getInput(theDebugSocketFd, YES)) != NULL)
        {
            if(strcmp(responsePtr, "0") == 0)
            {
                gReconnDebugLevel = NO_DEBUG;
            }
            else if(strcmp(responsePtr, "1") == 0)
            {
                if(RECONN_DEBUG_ON(LNB_DEBUG_RCV))
                {
                    gReconnDebugLevel &= ~LNB_DEBUG_RCV;
                }
                else
                {
                    gReconnDebugLevel |= LNB_DEBUG_RCV;
                }
            }
            else if(strcmp(responsePtr, "q") == 0)
            {
                sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_CLEARSCREEN, strlen(DEBUG_CLEARSCREEN), 0);
                sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_CURSORHOME, strlen(DEBUG_CURSORHOME), 0);
                break;
            }
            else if(strcmp(responsePtr, ""))
            {
                sprintf((char *)&debugOutputString, "Invalid Response \"%s\"\r\n", responsePtr);
                invalidResponse = YES;
            }
        }
        else
        {
            reconnDebugPrint("%s: recv() failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
            break;
        }
    }
    return RECONN_SUCCESS;
}

//*************************************************************************************
//*************************************************************************************
// FUNCTION:    slaveDebug
//
// DESCRIPTION: Function used to set the slave debug messages. These messages are sent
//              from the master iPhone application to all connected slave devices.
//
// Parameters:
//
//*************************************************************************************
static int slaveDebug()
{
    YESNO invalidResponse = NO;
    while(1)
    {
        sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_CLEARSCREEN, strlen(DEBUG_CLEARSCREEN), 0);
        sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_CURSORHOME, strlen(DEBUG_CURSORHOME), 0);

        if(invalidResponse == YES)
        {
            sendSocket(theDebugSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);
            invalidResponse = NO;
        }
        else
        {
            showDebugStatus();
        }
        sprintf((char *)&debugOutputString, "\r\n\r\n0) No Debug\r\n1) toggle debug packets\r\nq) quit\r\n(0, 1 or q)> ");
        sendSocket(theDebugSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);

        if((responsePtr = getInput(theDebugSocketFd, YES)) != NULL)
        {
            if(strcmp(responsePtr, "0") == 0)
            {
                gReconnDebugLevel = NO_DEBUG;
            }
            else if(strcmp(responsePtr, "1") == 0)
            {
                if(RECONN_DEBUG_ON(SLAVE_DEBUG_RCV))
                {
                    gReconnDebugLevel &= ~SLAVE_DEBUG_RCV;
                }
                else
                {
                    gReconnDebugLevel |= SLAVE_DEBUG_RCV;
                }
            }
            else if(strcmp(responsePtr, "q") == 0)
            {
                sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_CLEARSCREEN, strlen(DEBUG_CLEARSCREEN), 0);
                sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_CURSORHOME, strlen(DEBUG_CURSORHOME), 0);
                break;
            }
            else if(strcmp(responsePtr, ""))
            {
                sprintf((char *)&debugOutputString, "Invalid Response \"%s\"\r\n", responsePtr);
                invalidResponse = YES;
            }
        }
        else
        {
            reconnDebugPrint("%s: recv() failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
            break;
        }
    }
    return RECONN_SUCCESS;
}

//*************************************************************************************
//*************************************************************************************
// FUNCTION:    externalDebug
//
// DESCRIPTION: Function used to turn on external debug. It uses reconnStartExternalTask()
//              to do the real work.
//
// Parameters:
//
//*************************************************************************************
static int externalDebug(int theSocketFd)
{
    ReconnErrCodes retCode;
    YESNO invalidResponse = NO;
    memset(&debugOutputString, 0, DEBUG_OUTPUT_LEN);

    while(1)
    {
        sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_CLEARSCREEN, strlen(DEBUG_CLEARSCREEN), 0);
        sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_CURSORHOME, strlen(DEBUG_CURSORHOME), 0);
        if(invalidResponse == YES)
        {
            sendSocket(theDebugSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);
            invalidResponse = NO;
        }
        else
        {
            showDebugStatus();
        }
        strcpy ((char *)&debugOutputString, "\r\ngExternalMessagesOn = ");
        strcat ((char *)&debugOutputString, (gExternalMessagesOn == YES) ? "YES": "NO");
        sendSocket(theSocketFd, (unsigned char *)&debugOutputString, strlen(debugOutputString), 0);

        memset(&debugOutputString, 0, DEBUG_OUTPUT_LEN);
        sprintf((char *)&debugOutputString, "\r\n\r\n0) No Debug\r\n1) Enable port 4084");
        sendSocket(theDebugSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);
        sprintf((char *)&debugOutputString, "\r\n2) Disable port 4084\r\nq) quit\r\n(0, 1, 2 or q)> ");
        sendSocket(theDebugSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);

        if((responsePtr = getInput(theDebugSocketFd, YES)) != NULL)
        {
            if(strcmp(responsePtr, "0") == 0)
            {
                gReconnDebugLevel = NO_DEBUG;
            }
            else if(strcmp(responsePtr, "1") == 0)
            {
                if(gExternalMessagesOn == NO)
                {
                    if((retCode = reconnStartExternalTask()) != RECONN_SUCCESS)
                    {
                        sprintf((char *)&debugOutputString, "%s: Could not open port 4084\n", __FUNCTION__);
                        gExternalMessagesOn = NO;
                        timeoutDebugMenus = TRUE;
                        gReconnDebugLevel |= EXTERNAL_DEBUG;
                    }
                    else
                    {
                        gExternalMessagesOn = YES;
                        timeoutDebugMenus = FALSE;
                        gReconnDebugLevel &= ~EXTERNAL_DEBUG;
                    }
                }
                else
                {
                    sprintf((char *)&debugOutputString, "External messages currently ON\r\n");
                    invalidResponse = YES; 
                }
            }
            else if(strcmp(responsePtr, "2") == 0) 
            {
                if(gExternalMessagesOn == YES)
                {
                    reconnStopExternalTask();
                    gExternalMessagesOn = NO;
                    timeoutDebugMenus = TRUE;
                    gReconnDebugLevel &= ~EXTERNAL_DEBUG;
                }
                else
                {
                    sprintf((char *)&debugOutputString, "External messages currently OFF\r\n");
                    invalidResponse = YES; 
                }
            }
            else if(strcmp(responsePtr, "q") == 0)
            {
                sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_CLEARSCREEN, strlen(DEBUG_CLEARSCREEN), 0);
                sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_CURSORHOME, strlen(DEBUG_CURSORHOME), 0);
                break;
            }
            else if(strcmp(responsePtr, ""))
            {
                sprintf((char *)&debugOutputString, "Invalid Response \"%s\"\r\n", responsePtr);
                invalidResponse = YES;
            }
        }
    }
    return RECONN_SUCCESS;
}
//*************************************************************************************
//*************************************************************************************
// FUNCTION:    timingDebug
//
// DESCRIPTION: Function used to turn on external debug. It uses reconnStartExternalTask()
//              to do the real work.
//
// Parameters:
//
//*************************************************************************************
static int timingDebug(int theSocketFd)
{
    YESNO invalidResponse = NO;

    while(1)
    {
        sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_CLEARSCREEN, strlen(DEBUG_CLEARSCREEN), 0);
        sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_CURSORHOME, strlen(DEBUG_CURSORHOME), 0);

        memset(&debugOutputString, 0, DEBUG_OUTPUT_LEN);
        showDebugStatus();
        strcpy ((char *)&debugOutputString, "\r\ngDebugTimingEnabled = ");
        strcat ((char *)&debugOutputString, (gDebugTimingEnabled == YES) ? "YES": "NO");
        sendSocket(theSocketFd, (unsigned char *)&debugOutputString, strlen(debugOutputString), 0);

        memset(&debugOutputString, 0, DEBUG_OUTPUT_LEN);
        sprintf((char *)&debugOutputString, "\r\n\r\n0) No Debug\r\n1) Toggle Timing");
        sendSocket(theDebugSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);
        sprintf((char *)&debugOutputString, "\r\nq) quit\r\n(0, 1, or q)> ");

        if((responsePtr = getInput(theDebugSocketFd, YES)) != NULL)
        {
            if(strcmp(responsePtr, "0") == 0)
            {
                gReconnDebugLevel = NO_DEBUG;
            }
            else if(strcmp(responsePtr, "1") == 0)
            {
                if(RECONN_DEBUG_ON(TIMING_DEBUG))
                { 
                    gReconnDebugLevel &= ~TIMING_DEBUG;
                }
                else
                {
                    gReconnDebugLevel |= TIMING_DEBUG;
                }
            }
            else if(strcmp(responsePtr, "q") == 0)
            {
                sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_CLEARSCREEN, strlen(DEBUG_CLEARSCREEN), 0);
                sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_CURSORHOME, strlen(DEBUG_CURSORHOME), 0);
                break;
            }
            else if(strcmp(responsePtr, ""))
            {
                sprintf((char *)&debugOutputString, "Invalid Response \"%s\"\r\n", responsePtr);
                invalidResponse = YES;
            }
        }
    }
    return RECONN_SUCCESS;
}
//*************************************************************************************
//*************************************************************************************
// FUNCTION:    remoteDebug
//
// DESCRIPTION: Function used to set Remote Monitor debug settings.
//
// Parameters:
//
//*************************************************************************************
static int remoteDebug()
{
    YESNO invalidResponse = NO;
    while(1)
    {
        sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_CLEARSCREEN, strlen(DEBUG_CLEARSCREEN), 0);
        sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_CURSORHOME, strlen(DEBUG_CURSORHOME), 0);

        if(invalidResponse == YES)
        {
            sendSocket(theDebugSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);
            invalidResponse = NO;
        }
        else
        {
            showDebugStatus();
        }
        sprintf((char *)&debugOutputString, "\r\n\r\n0) No Debug\r\n1) toggle debug info\r\n");
        sendSocket(theDebugSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);

        sprintf((char *)&debugOutputString, "\r\nq) quit\r\n(0, 1 or q)> ");
        sendSocket(theDebugSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);
        if((responsePtr = getInput(theDebugSocketFd, YES)) != NULL)
        {
            if(strcmp(responsePtr, "0") == 0)
            {
                gReconnDebugLevel = NO_DEBUG;
            }
            else if(strcmp(responsePtr, "1") == 0)
            {
                if(RECONN_DEBUG_ON(RM_DEBUG))
                { 
                    gReconnDebugLevel &= ~RM_DEBUG;
                }
                else
                {
                    gReconnDebugLevel |= RM_DEBUG;
                }
            }
            else if(strcmp(responsePtr, "q") == 0)
            {
                sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_CLEARSCREEN, strlen(DEBUG_CLEARSCREEN), 0);
                sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_CURSORHOME, strlen(DEBUG_CURSORHOME), 0);
                break;
            }
            else if(strcmp(responsePtr, ""))
            {
                sprintf((char *)&debugOutputString, "Invalid Response \"%s\"\r\n", responsePtr);
                invalidResponse = YES;
            }
        }
        else
        {
            reconnDebugPrint("%s: recv() failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
            break;
        }
    }
    return RECONN_SUCCESS;
}
//*************************************************************************************
//*************************************************************************************
// FUNCTION:    eqptDebug
//
// DESCRIPTION: Function used to set eqpt task settings.
//
// Parameters:
//
//*************************************************************************************
static int eqptDebug()
{
    YESNO invalidResponse = NO;
    while(1)
    {
        sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_CLEARSCREEN, strlen(DEBUG_CLEARSCREEN), 0);
        sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_CURSORHOME, strlen(DEBUG_CURSORHOME), 0);

        if(invalidResponse == YES)
        {
            sendSocket(theDebugSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);
            invalidResponse = NO;
        }
        else
        {
            showDebugStatus();
        }
        sprintf((char *)&debugOutputString, "\r\n\r\n0) No Debug\r\n1) toggle debug info\r\n");
        sendSocket(theDebugSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);

        sprintf((char *)&debugOutputString, "\r\nq) quit\r\n(0, 1 or q)> ");
        sendSocket(theDebugSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);
        if((responsePtr = getInput(theDebugSocketFd, YES)) != NULL)
        {
            if(strcmp(responsePtr, "0") == 0)
            {
                gReconnDebugLevel = NO_DEBUG;
            }
            else if(strcmp(responsePtr, "1") == 0)
            {
                if(RECONN_DEBUG_ON(EQPT_TASK))
                { 
                    gReconnDebugLevel &= ~EQPT_TASK;
                }
                else
                {
                    gReconnDebugLevel |= EQPT_TASK;
                }
            }
            else if(strcmp(responsePtr, "q") == 0)
            {
                sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_CLEARSCREEN, strlen(DEBUG_CLEARSCREEN), 0);
                sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_CURSORHOME, strlen(DEBUG_CURSORHOME), 0);
                break;
            }
            else if(strcmp(responsePtr, ""))
            {
                sprintf((char *)&debugOutputString, "Invalid Response \"%s\"\r\n", responsePtr);
                invalidResponse = YES;
            }
        }
        else
        {
            reconnDebugPrint("%s: recv() failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
            break;
        }
    }
    return RECONN_SUCCESS;
}
