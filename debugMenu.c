//******************************************************************************
//******************************************************************************
// FILE:        debugMenu.c
//
// CLASSES:     
//
// DESCRIPTION: This file contains the main debug menu code plus supporting functions
//         
//******************************************************************************
//
//                       CONFIDENTIALITY NOTICE:
//
// THIS FILE CONTAINS MATERIAL THAT IS "HARRIS PROPRIETARY INFORMATION"  ANY 
// REVIEW, RELIANCE, DISTRIBUTION, DISCLOSURE, OR FORWARDING WITHOUT EXPRESSED 
// PERMISSION IS STRICTLY PROHIBITED.  PLEASE BE SURE TO PROPERLY DISPOSE ANY 
// HARDCOPIES OF THIS DOCUMENT.
//         
//******************************************************************************
//
// Government Use Rights:
//
//           (Applicable only for source code delivered under U. S.
//           Government contracts)
//
//                           RESTRICTED RIGHTS LEGEND
//           Use, duplication, or disclosure is subject to restrictions
//           stated in the Government's contract with Harris Corporation,
//           RF Communications Division. The applicable contract number is
//           indicated on the media containing this software. As a minimum,
//           the Government has restricted rights in the software as
//           defined in DFARS 252.227-7013.
//
// Commercial Use Rights:
//
//           (Applicable only for source code procured under contracts other
//           than with the U. S. Government)
//
//                           TRADE SECRET
//           Contains proprietary information of Harris Corporation.
//
// Copyright:
//           Protected as an unpublished copyright work,
//                    (c) Harris Corporation
//           First fixed in 2004, all rights reserved.
//
//******************************************************************************
//
// HISTORY: Created <MM>/<DD>/<YYYY> by <USER>
// $Header:$
// $Revision: 1.3 $
// $Log:$// 
//******************************************************************************
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>

#include "reconn.h"
#include "debugMenu.h"
#include "dmm.h"
#include "powerMgmt.h"
#include "socket.h"
#include "fuel_gauge.h"
#include "powerMeter.h"

int externalDebugSocketFd = -1;
int gReconnDebugLevel = NO_DEBUG;
int timeoutDebugMenus = TRUE;
char debugOutputString[DEBUG_OUTPUT_LEN];
int theDebugSocketFd= -1;
static char lastCommand[DEBUG_INPUT_LEN]; // 50 should be enough

static int debugListenFd = -1;
static int externalDebugListenSocket = -1;
static char debugPrompt[DEBUG_PROMPT_MAX_SIZE];
static int externalTaskDone = TRUE;
YESNO gExternalMessagesOn = NO;

extern short socketPrint;
extern YESNO swUpgradeInProgress;

static void *externalDebugTask(void *args);
#define lineFeed "\r"

//*************************************************************************************
//*************************************************************************************
// FUNCTION:    reconnDebugPrint
// 
// DESCRIPTION: The reconn application's debug interface. It sends the passed in message 
//              to the external debug socket and, if enabled, the debug console.
//
// Parameters:
//              fmt - pointer to the format and string to be printed
//
//*************************************************************************************
void reconnDebugPrint(const char *fmt, ...)
{
    unsigned char theBuf[200];
    va_list ap;

    memset(theBuf, 0, 200);
    va_start(ap, fmt);
    vsprintf((char *)theBuf, fmt ,ap);
    va_end(ap);

    if((gExternalMessagesOn == YES) && (externalDebugSocketFd != -1))
    {
        send(externalDebugSocketFd, (unsigned char *)theBuf, strlen((char *)theBuf), 0);
    }
    printf("%s", theBuf);
}
static debugMenusTableStruct_t *debugMenusTable = 0;

//*************************************************************************************
//*************************************************************************************
// FUNCTION:    debugCleanUp
//
// DESCRIPTION: Interface called to clean up all of the debug menu resources.
//
// Parameters:
//
//*************************************************************************************
static void debugCleanUp()
{
    reconnDebugPrint("%s: **** Called\n", __FUNCTION__);
    struct TableEntry_t *currentTablePosition;
    int i;
    gExternalMessagesOn = NO;

    if(debugListenFd != -1)
    {
        reconnDebugPrint("%s: **** shutdown debugListenFd\n", __FUNCTION__);
        if(shutdown(debugListenFd, SHUT_RDWR) != 0)
        {
            reconnDebugPrint("%s: shutdown(%d, 2) failed %d (%s)\n", __FUNCTION__, debugListenFd, 
                    errno , strerror(errno));
        }

        reconnDebugPrint("%s: **** closing debugListenFd\n", __FUNCTION__);
        shutdown(theDebugSocketFd, SHUT_RDWR);
        if(close(debugListenFd) != 0)
        {
            reconnDebugPrint("%s: close(%d) failed %d (%s)\n", __FUNCTION__, debugListenFd, 
                    errno , strerror(errno));
        }
        debugListenFd = -1;
    }

    if(theDebugSocketFd != -1)
    {
        reconnDebugPrint("%s: **** Closing theDebugSocketFd\n", __FUNCTION__);
        shutdown(theDebugSocketFd, SHUT_RDWR);
        if(close(theDebugSocketFd) != 0)
        {
            reconnDebugPrint("%s: close(%d) failed %d (%s)\n", __FUNCTION__, debugMenuTask, 
                    errno , strerror(errno));
        }
        theDebugSocketFd = -1;
    }
    if(debugMenusTable)
    {
        while(debugMenusTable->numTables)
        {
            currentTablePosition = debugMenusTable->aMenuEntry;
            for(i = 0; i < (debugMenusTable->numTables - 1); i++) 
            {
                currentTablePosition = currentTablePosition->nextTableEntry;
            }
            free(currentTablePosition);
            currentTablePosition->nextTableEntry = NULL;
            debugMenusTable->numTables--;
        }
        free(debugMenusTable);
    }
}

//*************************************************************************************
//*************************************************************************************
// FUNCTION:    getInput
//
// DESCRIPTION: General purpose interface used to get input from the debug socket port 4083
//
// Parameters:
//              theDebugSocketFd - the debug socket's file descriptor
//              echoOn - boolean to echo character back to the user via theDebugSocketFd 
//
//*************************************************************************************
char * getInput(int theDebugSocketFd, YESNO echoOn)
{
    char *line = malloc(100), *linep = line;
    extern int fuelModelTaskDone;
    size_t lenmax = 100, len = lenmax;
    fd_set debugFdSet;
    struct timeval timeout;
    char c;
    int retCode;
    
    if(line == NULL)
        return NULL;
    
    for(;;)
    {
        timeout.tv_sec = DEBUG_INACTIVITY_TIMEOUT; 
        timeout.tv_usec = 0;
        
        FD_ZERO(&debugFdSet);
        FD_SET(theDebugSocketFd, &debugFdSet);
        if(select(theDebugSocketFd+1, &debugFdSet, NULL, NULL, &timeout) <= 0)
        {
            if(timeoutDebugMenus == TRUE)
            {
                sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_TIMEOUT_MESSAGE, strlen(DEBUG_TIMEOUT_MESSAGE), 0);

                reconnStopExternalTask();
                gReconnDebugLevel = NO_DEBUG;
                fuelModelTaskDone = TRUE;
                shutdown(theDebugSocketFd, SHUT_RDWR);
                close(theDebugSocketFd);
                theDebugSocketFd = -1;
                timeoutDebugMenus = FALSE;
                socketPrint = FALSE;
                fuelModelCleanUp();
                free(linep);
                return 0;
            }
            continue;
        }
        if((retCode = recv((int)theDebugSocketFd, &c, 1, 0)) <= 0)
        {
            reconnDebugPrint("%s: recv returned %d\n", __FUNCTION__, retCode);
            reconnStopExternalTask();
            gReconnDebugLevel = NO_DEBUG;
            fuelModelTaskDone = TRUE; 
            shutdown(theDebugSocketFd, SHUT_RDWR);
            close(theDebugSocketFd);
            theDebugSocketFd = -1;
            timeoutDebugMenus = FALSE;
            socketPrint = FALSE;
            fuelModelCleanUp();
            free(linep);
            return 0;
        }
        if(c == DEBUG_DELETE)
        {
            /*
             * backspace entered
             */
            if(len < 100)
            {
                /*
                 * Remove the last character.
                 */
                sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_BACKSPACE, strlen(DEBUG_BACKSPACE), 0);
                sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_CLEARLINE, strlen(DEBUG_CLEARLINE), 0);
                len++;
                line--;
                *line = 0;
            }
            continue;
        }
        if(!isascii(c))
        {
            continue;
        }
        if(--len == 0)
        {
            char * linen = realloc(linep, lenmax *= 2);
            len = lenmax;
            
            if(linen == NULL)
            {
                free(linep);
                return NULL;
            }
            line = linen + (line - linep);
            linep = linen;
        }
        
        if(c == '\r')
        {
            /*
             * this will remove any \n character
             */
            recv((int)theDebugSocketFd, &c, 1, 0);
            break;
        }

        *line++ = c;
        if(echoOn == YES)
        {
            sendSocket(theDebugSocketFd, (unsigned char *)&c, 1, 0);
        }
    }
    *line = '\0';
    return linep;
}

//*************************************************************************************
//*************************************************************************************
// FUNCTION:    registerDebugCommand
//
// DESCRIPTION: Interface called to register a particular debug menu.
//
// Parameters:
//              theMenu - a debugMenuStruct pointer to the registering menu
//              numMenuEntires - the number of menus theMenu is pointing to
//
//*************************************************************************************
void registerDebugCommand(debugMenuStruct *theMenu, int numMenuEntries)
{
    static struct TableEntry_t *currentTablePosition = 0;
    // get the subsystem name and store it along with the tables address
    if(!debugMenusTable)
    {
        debugMenusTable = malloc(sizeof(debugMenusTableStruct_t));
        debugMenusTable->aMenuEntry = malloc(sizeof(struct TableEntry_t));
        debugMenusTable->aMenuEntry->theMenu = theMenu;
        debugMenusTable->aMenuEntry->numMenuItems = numMenuEntries;
        debugMenusTable->aMenuEntry->nextTableEntry = NULL;
        currentTablePosition = debugMenusTable->aMenuEntry;
    }
    else
    {
        currentTablePosition->nextTableEntry = malloc(sizeof(struct TableEntry_t));
        currentTablePosition = currentTablePosition->nextTableEntry;
        currentTablePosition->theMenu = theMenu;
        currentTablePosition->numMenuItems = numMenuEntries;
        currentTablePosition->nextTableEntry = NULL;
    }
    debugMenusTable->numTables++;
}
//*************************************************************************************
//*************************************************************************************
// FUNCTION:    debugMenuTask
//
// DESCRIPTION: The main work horse of the debug menus. This task takes user input
//              and looks to see if there is a matching menu or commands. If there is,
//              the menu is displayed or the sub-command is executed.
//
// Parameters:  argument - a phony pointer to nothing.
//
//*************************************************************************************
void *debugMenuTask(void *argument) 
{
    int connected = TRUE;
    int commandFound = FALSE;
    int needHelp;
    char buff[DEBUG_INPUT_LEN];
    char *inputString;
    char echoAnswer[DEBUG_INPUT_LEN];
    int debugPort, retCode,   optval = 1, i;
    ReconnErrCodes commandStatus;
    struct sockaddr_in server_addr, client_addr;
    unsigned int client_len;
    fd_set debugFdSet;
    struct timeval timeout;
    struct TableEntry_t *tableEntry = NULL;
    struct TableEntry_t *activeTableEntry = NULL;
    
    UNUSED_PARAM(argument);
    atexit(debugCleanUp);
    /* Create the incoming (server) socket */
    if((debugListenFd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    {
        reconnDebugPrint("%s: Failed to initialize the incoming socket %d(%s)\n", __FUNCTION__, errno, strerror(errno));
        return (0);
    }
    if(setsockopt(debugListenFd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
    {
        reconnDebugPrint("%s: setsockopt failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
    }
    
    bzero((unsigned char *) &server_addr, sizeof(server_addr));
    /* bind the socket */
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    debugPort = DEBUG_PORT;
    server_addr.sin_port = htons(debugPort);
    
    if (bind(debugListenFd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) 
    {
        reconnDebugPrint("%s: Failed to bind the socket %d (%s)\n", __FUNCTION__, errno, strerror(errno));
        return (0);
    }
    while (swUpgradeInProgress == NO)
    {
        memset(debugPrompt, 0, DEBUG_PROMPT_MAX_SIZE);
        strcpy(debugPrompt, DEBUG_MAIN_PROMPT);
        /* pend on the incoming socket */
        reconnDebugPrint("\n\n%s: Listening\n\n", __FILE__);
        if(listen(debugListenFd, 5) == 0)
        {
            client_len = sizeof(client_addr);
            /* sit here and wait for a new connection request */
            if((theDebugSocketFd = accept(debugListenFd, (struct sockaddr *) &client_addr, &client_len)) < 0)
            {
                reconnDebugPrint("%s: Failed to open a new Client socket %d %s.\n", __FUNCTION__, errno , strerror(errno));
                continue;
            }
            if (theDebugSocketFd != 0)
            {
                reconnDebugPrint("%s: newSocketFd = %d\r\n", __FUNCTION__, theDebugSocketFd);
                sendSocket(theDebugSocketFd, (unsigned char *)"\377\375\042\377\373\001", 6, 0);
                memset(echoAnswer, 0, DEBUG_INPUT_LEN);
                retCode = recv((int)theDebugSocketFd, &echoAnswer, DEBUG_INPUT_LEN, 0);
                retCode = recv((int)theDebugSocketFd, &echoAnswer, DEBUG_INPUT_LEN, 0);
                //
                // Ask for password
                //
                sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_CLEARSCREEN, strlen(DEBUG_CLEARSCREEN), 0);
                sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_CURSORHOME, strlen(DEBUG_CURSORHOME), 0);
                sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_QUESTION, strlen(DEBUG_QUESTION), 0);
                memset(buff, 0, DEBUG_INPUT_LEN);

                timeout.tv_sec = 30;
                timeout.tv_usec = 0;

                FD_ZERO(&debugFdSet);
                FD_SET(theDebugSocketFd, &debugFdSet);
                if(select(theDebugSocketFd+1, &debugFdSet, NULL, NULL, &timeout) <= 0)
                {
                    sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_TIMEOUT_MESSAGE, strlen(DEBUG_TIMEOUT_MESSAGE), 0);
                    reconnStopExternalTask();
                    gReconnDebugLevel = NO_DEBUG;
                    shutdown(theDebugSocketFd, SHUT_RDWR);
                    close(theDebugSocketFd);
                    theDebugSocketFd = -1;
                    continue;
                }
                if((inputString = getInput(theDebugSocketFd, NO)) == NULL)
                {
                    reconnDebugPrint("%s: getInput returned %d\n", __FUNCTION__, retCode);
                    reconnStopExternalTask();
                    gReconnDebugLevel = NO_DEBUG;
                    continue;
                }
                //sendSocket(theDebugSocketFd, (unsigned char *)echo_on, strlen(echo_on), 0);
                //retCode = recv((int)theDebugSocketFd, &echoAnswer, DEBUG_INPUT_LEN, 0);

                if(strcmp(DEBUG_PASSWORD, inputString) == 0)
                {
                    connected = TRUE;
                    needHelp = TRUE;
                    while(connected)
                    {
                        if(needHelp)
                        {
                            if(!activeTableEntry)
                            {
                                if(debugMenusTable)
                                {
                                    tableEntry = debugMenusTable->aMenuEntry;
                                }
                                memset(debugOutputString, 0, DEBUG_OUTPUT_LEN);
                                sprintf((char *) debugOutputString, "\r\n\r\n%-20s%-25s\r\n", "Menu Item", "Help");
                                sendSocket(theDebugSocketFd, (unsigned char *)&debugOutputString, strlen(debugOutputString), 0);
                                sprintf((char *) debugOutputString, "=====================================\r\n");
                                sendSocket(theDebugSocketFd, (unsigned char *)&debugOutputString, strlen(debugOutputString), 0);
                                while(tableEntry)
                                {
                                    if(tableEntry->theMenu->subSystemName)
                                    {
                                        sprintf((char *) debugOutputString, "%-20s", tableEntry->theMenu->subSystemName);
                                        sendSocket(theDebugSocketFd, (unsigned char *)&debugOutputString, strlen(debugOutputString), 0);
                                    }
                                    if(tableEntry->theMenu->subSystemHelp)
                                    {
                                        sprintf((char *) debugOutputString, "%-20s\r\n", tableEntry->theMenu->subSystemHelp);
                                        sendSocket(theDebugSocketFd, (unsigned char *)&debugOutputString, strlen(debugOutputString), 0);
                                    }
                                    tableEntry = tableEntry->nextTableEntry;
                                }
                                sprintf((char *) debugOutputString, "%-20s%-25s\r\n", "quit", "Exit debug mode");
                                sendSocket(theDebugSocketFd, (unsigned char *)&debugOutputString, strlen(debugOutputString), 0);
                            }
                            else
                            {
                                sprintf((char *) debugOutputString, "\r\n\r\n%-20s%-25s\r\n", "Command", "Help");
                                sendSocket(theDebugSocketFd, (unsigned char *)&debugOutputString, strlen(debugOutputString), 0);
                                sprintf((char *) debugOutputString, "=====================================\r\n");
                                sendSocket(theDebugSocketFd, (unsigned char *)&debugOutputString, strlen(debugOutputString), 0);
                                for(i = 0; i < activeTableEntry->numMenuItems; i++)
                                {
                                    if(activeTableEntry->theMenu[i].commandName)
                                    {
                                        sprintf((char *) debugOutputString, "%-20s", activeTableEntry->theMenu[i].commandName);
                                        sendSocket(theDebugSocketFd, (unsigned char *)&debugOutputString, strlen(debugOutputString), 0);
                                    }
                                    if(activeTableEntry->theMenu[i].commandHelp)
                                    {
                                        if(strlen(activeTableEntry->theMenu[i].commandHelp) > DEBUG_OUTPUT_LEN)
                                        {
                                            sprintf((char *)debugOutputString, "%-20s%d bytes too long\r\n", " ***** Help text is ", strlen(activeTableEntry->theMenu[i].commandHelp) - DEBUG_OUTPUT_LEN);
                                        }
                                        else
                                        { 
                                            sprintf((char *)debugOutputString, "%-20s\r\n", activeTableEntry->theMenu[i].commandHelp);
                                        }
                                        sendSocket(theDebugSocketFd, (unsigned char *)&debugOutputString, strlen(debugOutputString), 0);
                                    }
                                }
                                sprintf((char *) debugOutputString, "%-20s%-25s\r\n", "quit", "go back to main menu");
                                sendSocket(theDebugSocketFd, (unsigned char *)&debugOutputString, strlen(debugOutputString), 0);
                            }
                        }
                        sendSocket(theDebugSocketFd, (unsigned char *)&debugPrompt, strlen(debugPrompt), 0);

                        commandFound = FALSE;
                        if((inputString = getInput(theDebugSocketFd, YES)) != 0)
                        {

                            if(activeTableEntry)
                            {
                                // loop through the command tables looking for a match between 
                                // the input string and a subsystem string. If found, point to
                                // the current debug menu table
                                if((strcasecmp(inputString, "quit") == 0) || (strncasecmp(inputString, "q", 1) == 0))
                                {
                                    activeTableEntry = NULL;
                                    memset(debugPrompt, 0, DEBUG_PROMPT_MAX_SIZE);
                                    memset(&lastCommand, 0, DEBUG_INPUT_LEN);
                                    strcpy(debugPrompt, DEBUG_MAIN_PROMPT);
                                    free(inputString);
                                    sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_CLEARSCREEN, strlen(DEBUG_CLEARSCREEN), 0);
                                    sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_CURSORHOME, strlen(DEBUG_CURSORHOME), 0);
                                    continue;
                                }
                                for(i = 1; i < activeTableEntry->numMenuItems; i++)
                                {
                                    /*
                                     * Does the user input match a command in the tables or the last command entered
                                     */
                                    if((activeTableEntry->theMenu[i].commandName) && 
                                            ((strcmp(activeTableEntry->theMenu[i].commandName, inputString) == 0)
                                             ||
                                             (strcmp(activeTableEntry->theMenu[i].commandName, (char *)&lastCommand) == 0)))
                                    {
                                        commandFound = TRUE;
                                        if(strcmp(inputString, ""))
                                        {
                                            memset(&lastCommand, 0, DEBUG_INPUT_LEN);
                                            strcpy((char *)&lastCommand, inputString);
                                        }

                                        sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_CLEARSCREEN, 
                                                strlen(DEBUG_CLEARSCREEN), 0);
                                        sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_CURSORHOME,
                                                strlen(DEBUG_CURSORHOME), 0);
                                        /*
                                         * Command found so execute it.
                                         */
                                        if((commandStatus = activeTableEntry->theMenu[i].func(theDebugSocketFd)) != RECONN_SUCCESS)
                                        {
                                            if(commandStatus == RECONN_DEBUG_DONE)
                                            {
                                                activeTableEntry = NULL;
                                                connected = 0;
                                                break;
                                            }
                                            else
                                            {
                                                char *p;
                                                if((p = malloc(strlen(activeTableEntry->theMenu[i].commandName + 10))))
                                                {
                                                    memset(p , 0, strlen(activeTableEntry->theMenu[i].commandName + 10));
                                                    strcat(p, activeTableEntry->theMenu[i].commandName);
                                                    strcat(p, " Failed\r\n");
                                                    sendSocket(theDebugSocketFd, (unsigned char *)p, strlen(p), 0);

                                                    free (p); 
                                                    //needHelp = TRUE;
                                                }
                                            }
                                        }
                                        needHelp = TRUE;
                                    }
                                }
                                if(commandFound == FALSE)
                                {
                                    sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_CLEARSCREEN, strlen(DEBUG_CLEARSCREEN), 0);
                                    sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_CURSORHOME, strlen(DEBUG_CURSORHOME), 0);
                                    if(strcmp(inputString, ""))
                                    {
                                        sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_INVALID_COMMAND, strlen(DEBUG_INVALID_COMMAND), 0);
                                    }
                                }
                            }
                            else
                            {
                                if(debugMenusTable)
                                {
                                    tableEntry = debugMenusTable->aMenuEntry;
                                }
                                if((strcasecmp(inputString, "quit") == 0)  || (strncasecmp(inputString, "q", 1) == 0))
                                {
                                    free(inputString);
                                    reconnStopExternalTask();
                                    gReconnDebugLevel = NO_DEBUG;
                                    shutdown(theDebugSocketFd, SHUT_RDWR);
                                    close(theDebugSocketFd);
                                    theDebugSocketFd = -1;
                                    socketPrint = FALSE;
                                    needHelp = FALSE;
                                    timeoutDebugMenus = TRUE;
                                    connected = 0;
                                }
                                else
                                {
                                    sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_CLEARSCREEN, strlen(DEBUG_CLEARSCREEN), 0);
                                    sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_CURSORHOME, strlen(DEBUG_CURSORHOME), 0);
                                    while(tableEntry)
                                    {
                                        if(strcmp(tableEntry->theMenu->subSystemName, inputString) == 0)
                                        {
                                            activeTableEntry = tableEntry;
                                            memset(debugPrompt, 0, DEBUG_PROMPT_MAX_SIZE);
                                            strcpy(debugPrompt, activeTableEntry->theMenu->subSystemName);
                                            strcat(debugPrompt, " debug > ");
                                            needHelp = TRUE;
                                            commandFound = TRUE;
                                            break;
                                        }
                                        tableEntry = tableEntry->nextTableEntry;
                                    }
                                    if(commandFound == FALSE)
                                    {
                                        sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_CLEARSCREEN, strlen(DEBUG_CLEARSCREEN), 0);
                                        sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_CURSORHOME, strlen(DEBUG_CURSORHOME), 0);
                                         if(strcmp(inputString, ""))
                                         {
                                             sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_INVALID_COMMAND, strlen(DEBUG_INVALID_COMMAND), 0);
                                         }
                                    }
                                    free(inputString);
                                }
                            }
                        }
                        else
                        {
                            socketPrint = FALSE;
                            activeTableEntry = NULL;
                            connected = FALSE;
                        }
                    } // end while(connected)
                }
                else
                {
                    sendSocket((int )theDebugSocketFd, (unsigned  char *)"Invalid Password entered\r\n", 26, 0);
                    shutdown(theDebugSocketFd, SHUT_WR);
                    if(close(theDebugSocketFd) != 0)
                    {
                        sprintf((char *) debugOutputString, "%s: close returned  =%d(%s) \r\n", __FUNCTION__, errno, strerror(errno));
                        sendSocket(theDebugSocketFd, (unsigned char *)&debugOutputString, strlen(debugOutputString), 0);
                    }
                    theDebugSocketFd = -1;
                }
            }
        }
        else
        {
            reconnDebugPrint("%s: listen returned  =%d(%s) \n", __FUNCTION__, errno, strerror(errno));
            reconnStopExternalTask();
            gReconnDebugLevel = NO_DEBUG;
            close(debugListenFd);
            debugListenFd = -1;
            break;
        }
    }
    reconnDebugPrint("%s: Exiting *******************\n",__FUNCTION__);
    return (0);
}

//*************************************************************************************
//*************************************************************************************
// FUNCTION:    showDebugStatus
//
// DESCRIPTION: Function called to display the global debug state.
//
// Parameters:
//
//*************************************************************************************
void showDebugStatus()
{
    memset(debugOutputString, 0, DEBUG_OUTPUT_LEN);
    strcat((char *)&debugOutputString, "gReconnDebugLevel = ");
    if(gReconnDebugLevel == NO_DEBUG)
    {
        strcat((char *)&debugOutputString, "NO DEBUG");
    }
    else
    {
        if(RECONN_DEBUG_ON(DMM_DEBUG_RCV))
        {
            strcat((char *)&debugOutputString, "DMM_RCV ");
        }
        if(RECONN_DEBUG_ON(DMM_DEBUG_SND))
        {
            strcat((char *)&debugOutputString, "DMM_SND ");
        }
        if(RECONN_DEBUG_ON(DMM_DEBUG_EQPT))
        {
            strcat((char *)&debugOutputString, "DMM_EPQT ");
        }
        if(RECONN_DEBUG_ON(PMETER_DEBUG_RCV))
        {
            strcat((char *)&debugOutputString, "PMETER_RCV ");
        }
        if(RECONN_DEBUG_ON(PMETER_DEBUG_SND))
        {
            strcat((char *)&debugOutputString, "PMETER_SND ");
        }
        if(RECONN_DEBUG_ON(PMETER_DEBUG_EQPT))
        {
            strcat((char *)&debugOutputString, "PMETER_EQPT ");
        }
        if(RECONN_DEBUG_ON(BATTERY_DEBUG_RCV))
        {
            strcat((char *)&debugOutputString, "BATTERY_RCV ");
        }
        if(RECONN_DEBUG_ON(BATTERY_DEBUG_SND))
        {
            strcat((char *)&debugOutputString, "BATTERY_SND ");
        }
        if(RECONN_DEBUG_ON(SPECANA_DEBUG_RCV))
        {
            strcat((char *)&debugOutputString, "SPECANA_RCV ");
        }
        if(RECONN_DEBUG_ON(SPECANA_DEBUG_SND))
        {
            strcat((char *)&debugOutputString, "SPECANA_SND ");
        }
        if(RECONN_DEBUG_ON(SPECANA_DEBUG_EQPT))
        {
            strcat((char *)&debugOutputString, "SPECANA_EQPT ");
        }
        if(RECONN_DEBUG_ON(LNB_DEBUG_RCV))
        {
            strcat((char *)&debugOutputString, "LNB_RCV ");
        }
        if(RECONN_DEBUG_ON(SLAVE_DEBUG_RCV))
        {
            strcat((char *)&debugOutputString, "SLAVE_RCV ");
        }
        if(RECONN_DEBUG_ON(TIMING_DEBUG))
        {
            strcat((char *)&debugOutputString, "TIMING ");
        }
        if(RECONN_DEBUG_ON(EXTERNAL_DEBUG))
        {
            strcat((char *)&debugOutputString, "EXTERNAL ");
        }
        if(RECONN_DEBUG_ON(RM_DEBUG))
        {
            strcat((char *)&debugOutputString, "REMOTE_MONITOR ");
        }
        if(RECONN_DEBUG_ON(EQPT_TASK))
        {
            strcat((char *)&debugOutputString, "EQPT_TASK_DEBUG");
        }
    }
    sendSocket(theDebugSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);
}

//*************************************************************************************
//*************************************************************************************
// FUNCTION:    reconnStartExternalTask
//
// DESCRIPTION: Interface used to start the external debug listeneing task
//
// Parameters:
//
//*************************************************************************************
ReconnErrCodes reconnStartExternalTask()
{
    ReconnErrCodes retCode = RECONN_SUCCESS;

    if(reconnThreadIds[RECONN_XTERNAL_DEBUG_TASK] == (pthread_t)-1)
    {
        reconnDebugPrint("%s: ****** Creating Task\n", __FUNCTION__);
        if(pthread_create(&(reconnThreadIds[RECONN_XTERNAL_DEBUG_TASK]), NULL, externalDebugTask, (void *)0) < 0)
        {
            reconnDebugPrint("%s: Could not start externalDebugTask %d %s\n", __FUNCTION__, errno, strerror(errno));
            retCode = RECONN_FAILURE;
        }
        externalTaskDone = FALSE;
        reconnDebugPrint("%s: Function exiting %s\n", __FUNCTION__,
                (retCode == RECONN_SUCCESS) ? "SUCCESS" : "FAILURE");
    }
    else
    {
        reconnDebugPrint("%s: externalDebugTask already started.\n", __FUNCTION__);
    }
    return(retCode);
}

//*************************************************************************************
//*************************************************************************************
// FUNCTION:    reconnStopExternalTask
//
// DESCRIPTION: Interface used to stop the external debug listeneing task.
//
// Parameters:
//
//*************************************************************************************
void reconnStopExternalTask()
{
    externalTaskDone = TRUE;
    shutdown(externalDebugListenSocket, SHUT_RDWR);
}

//*************************************************************************************
//*************************************************************************************
// FUNCTION:        externalDebugTask
//
// DESCRIPTION: Task that listens to port 4084. Once connected debug message are sent
//              out that port
//
// Parameters:
//
//*************************************************************************************
static void *externalDebugTask(void *args)
{
    socklen_t  clientLen;
    struct sockaddr_in server_addr, clientAddr;
    int optval = 1;
    char c; // dummy character
    static int taskState = 0;
    fd_set theFileDescriptor;
    struct timeval waitTime;

    UNUSED_PARAM(args);
    reconnDebugPrint("%s: ****** started\n", __FUNCTION__);
    /* Create the incoming (server) socket */
    if((externalDebugListenSocket= socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        reconnDebugPrint("%s: Failed to initialize the incoming socket %d(%s)\n", __FUNCTION__, errno, strerror(errno));
    }
    else
    {
        if(setsockopt(externalDebugListenSocket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
        {
            reconnDebugPrint("%s: setsockopt SO_REUSEADDR failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
        }
        bzero((unsigned char *) &server_addr, sizeof(server_addr));
        /* bind the socket */
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(4084);

        reconnDebugPrint("%s: binding to socket 4084\n", __FUNCTION__);
        if (bind(externalDebugListenSocket, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)
        {
            reconnDebugPrint("%s: Task Failed to bind socket %d (%s)\n",  __FUNCTION__, errno, strerror(errno));
        }
        else
        {
            //atexit(remoteMonitorCleanup);
            reconnDebugPrint("%s: ****** Listening on port %d\n", __FUNCTION__, 4084);
            /* pend on the incoming socket */
            if(listen(externalDebugListenSocket, 1) == 0)
            {
                clientLen = sizeof(clientAddr);
                if((externalDebugSocketFd = accept(externalDebugListenSocket,
                                (struct sockaddr *) &clientAddr, &clientLen)) < 0)
                {
                    reconnDebugPrint("%s: accept failed %d(%s)\n",  __FUNCTION__,  errno, strerror(errno));
                }
                while (externalTaskDone == FALSE)
                {
                    waitTime.tv_sec = 0;
                    waitTime.tv_usec = 500000;
                    FD_ZERO(&theFileDescriptor);
                    FD_SET(externalDebugSocketFd, &theFileDescriptor);
                    if(select(externalDebugSocketFd +1,  &theFileDescriptor, NULL, NULL, &waitTime) > 0)
                    {
                        if(recv(externalDebugSocketFd, &c, 1, 0) <= 0)
                        {
                            externalTaskDone = TRUE;
                        }
                    }
                }
                gExternalMessagesOn = NO;
                if(externalDebugSocketFd != -1)
                {
                    if(close(externalDebugSocketFd) != 0)
                    {
                        reconnDebugPrint("%s: close(externalDebugSocketFd) failed %d(%s)\n",  __FUNCTION__,  errno, strerror(errno));
                    }
                    externalDebugSocketFd = -1;
                }
                if(externalDebugListenSocket != -1)
                {
                    if(close(externalDebugListenSocket) != 0)
                    {
                        reconnDebugPrint("%s: close(externalDebugListenSocket) failed %d(%s)\n",  __FUNCTION__,  errno, strerror(errno));
                    }
                    externalDebugListenSocket = -1;
                }
            }
        }
    }
    reconnThreadIds[RECONN_XTERNAL_DEBUG_TASK] = -1;
    return (&taskState);
}
