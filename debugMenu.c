#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <arpa/telnet.h>

#include "reconn.h"
#include "debugMenu.h"
#include "powerMgmt.h"

static int debugListenFd;
int theDebugSocketFd;
static char debugPrompt[DEBUG_PROMPT_MAX_SIZE];

static void debugCleanUp()
{
    printf("%s: **** Called\n", __FUNCTION__);
    close(debugListenFd);
}

debugMenusTableStruct_t *debugMenusTable = 0;


static char * getInput(int theDebugSocketFd)
{
    char *line = malloc(100), *linep = line;
    size_t lenmax = 100, len = lenmax;
    fd_set debugFdSet;
    struct timeval timeout;
    char c;
    int retCode;
    
    if(line == NULL)
        return NULL;
    
    for(;;)
    {
        timeout.tv_sec = 1200; // 10 minutes
        timeout.tv_usec = 0;
        
        FD_ZERO(&debugFdSet);
        FD_SET(theDebugSocketFd, &debugFdSet);
        if(select(theDebugSocketFd+1, &debugFdSet, NULL, NULL, &timeout) <= 0)
        {
            sendSocket(theDebugSocketFd, DEBUG_TIMEOUT_MESSAGE, strlen(DEBUG_TIMEOUT_MESSAGE), 0);

            close(theDebugSocketFd);
            return 0;
        }
        if((retCode = recv((int)theDebugSocketFd, &c, 1, 0)) <= 0)
        {
            printf("%s: recv returned %d\n", __FUNCTION__, retCode);
            close(theDebugSocketFd);
        }
        if(c == EOF)
            break;
        
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
        
        if((*line++ = c) == '\n')
        {
            line--;
            break;
        }
    }
    *line = '\0';
    return linep;
}

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
void *debugMenuTask(void *argument) 
{
    int connected = TRUE;
    int needHelp;
    char *inputString;
    char buff[DEBUG_INPUT_LEN];
    char echoAnswer[DEBUG_INPUT_LEN];
    char outbuff[DEBUG_OUTPUT_LEN];
    int debugPort, retCode,   optval = 1, i;
    struct sockaddr_in server_addr, client_addr;
    unsigned int client_len;
    fd_set debugFdSet;
    struct timeval timeout;
    struct TableEntry_t *tableEntry = NULL;
    struct TableEntry_t *activeTableEntry = NULL, *tmpItem;
    char echo_off[] = { (char)IAC, (char) WILL, (char) TELOPT_ECHO, (char) 0 };
    char echo_on[] = { (char)IAC, (char) WONT, (char) TELOPT_ECHO, (char) TELOPT_NAOFFD, (char)TELOPT_NAOCRD, (char) 0 };
    
    atexit(debugCleanUp);
    /* Create the incoming (server) socket */
    if((debugListenFd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    {
        printf("%s: Server Failed to initialize the incoming socket %d(%s)\n", __FUNCTION__, errno, strerror(errno));
        exit (1);
    }
    if(setsockopt(debugListenFd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
    {
        printf("%s: setsockopt failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
    }
    
    bzero((unsigned char *) &server_addr, sizeof(server_addr));
    /* bind the socket */
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    debugPort = 4083;
    server_addr.sin_port = htons(debugPort);
    
    if (bind(debugListenFd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) 
    {
        printf("%s: Server Failed to bind the socket\n", __FUNCTION__);
        exit (0);
    }
    memset(debugPrompt, 0, DEBUG_PROMPT_MAX_SIZE);
    strcpy(debugPrompt, "reconn debug >");
    while (1)
    {
        /* pend on the incoming socket */
        printf("\n\n%s: Listening\n\n", __FILE__);
        if(listen(debugListenFd, 5) == 0)
        {
            client_len = sizeof(client_addr);
            /* sit here and wait for a new connection request */
            if((theDebugSocketFd = accept(debugListenFd, (struct sockaddr *) &client_addr, &client_len)) < 0)
            {
                printf("%s: Failed to open a new Client socket %d %s.\n", __FUNCTION__, errno , strerror(errno));
                continue;
            }
            if (theDebugSocketFd != 0)
            {
                printf("%s: newSocketFd = %d\r\n", __FUNCTION__, theDebugSocketFd);
                
                sendSocket(theDebugSocketFd, echo_off, strlen(echo_off), 0);
                retCode = recv((int)theDebugSocketFd, &echoAnswer, DEBUG_INPUT_LEN, 0);
                //
                // Ask for password
                //
                sendSocket(theDebugSocketFd, DEBUG_QUESTION, strlen(DEBUG_QUESTION), 0);
                memset(buff, 0, DEBUG_INPUT_LEN);

                timeout.tv_sec = 30;
                timeout.tv_usec = 0;

                FD_ZERO(&debugFdSet);
                FD_SET(theDebugSocketFd, &debugFdSet);
                if(select(theDebugSocketFd+1, &debugFdSet, NULL, NULL, &timeout) <= 0)
                {
                    sendSocket(theDebugSocketFd, DEBUG_TIMEOUT_MESSAGE, strlen(DEBUG_TIMEOUT_MESSAGE), 0);
                    close(theDebugSocketFd);
                    continue;
                }
                if((retCode = recv((int)theDebugSocketFd, &buff, DEBUG_INPUT_LEN, 0)) <= 0)
                {
                    printf("%s: recv returned %d\n", __FUNCTION__, retCode);
                    close(theDebugSocketFd);
                    continue;
                }
                sendSocket(theDebugSocketFd, echo_on, strlen(echo_on), 0);
                retCode = recv((int)theDebugSocketFd, &echoAnswer, DEBUG_INPUT_LEN, 0);

                if(strcmp(DEBUG_PASSWORD, buff) == 0)
                {
                    connected = TRUE;
                    while(connected)
                    {
                        needHelp = 1;
                        sendSocket(theDebugSocketFd, &debugPrompt, strlen(debugPrompt), 0);
                        if((inputString = getInput(theDebugSocketFd)) != 0)
                        
                        {
                            if(activeTableEntry)
                            {
                                // loop through the command tables looking for a match between 
                                // the input string and a subsystem string. If found, point to
                                // the current debug menu table
                                if((strncasecmp(inputString, "quit", 4) == 0) || (strncasecmp(inputString, "q", 1) == 0))
                                {
                                    activeTableEntry = NULL;
                                    memset(debugPrompt, 0, DEBUG_PROMPT_MAX_SIZE);
                                    strcpy(debugPrompt, "reconn");
                                    strcat(debugPrompt, " debug > ");
                                    free(inputString);
                                    continue;
                                }
                                for(i = 1; i < activeTableEntry->numMenuItems; i++)
                                {
                                    if((activeTableEntry->theMenu[i].commandName) &&
                                            (strncmp(activeTableEntry->theMenu[i].commandName, inputString, strlen(activeTableEntry->theMenu[i].commandName)) == 0))
                                    {
                                        if(activeTableEntry->theMenu[i].func(theDebugSocketFd) != RECONN_SUCCESS)
                                        {
                                            char *p;
                                            if(p = malloc(strlen(activeTableEntry->theMenu[i].commandName + 10)))
                                            {
                                                memset(p , 0, strlen(activeTableEntry->theMenu[i].commandName + 10));
                                                strcat(p, activeTableEntry->theMenu[i].commandName);
                                                strcat(p, " Failed\n");
                                                sendSocket(theDebugSocketFd, p, strlen(p), 0);
                                                free (p);
                                            }
                                        }
                                        needHelp = 0;
                                    }
                                }
                            }
                            else
                            {
                                if(debugMenusTable)
                                {
                                    tableEntry = debugMenusTable->aMenuEntry;
                                }
                                if((strncasecmp(inputString, "quit", 4) == 0)  || (strncasecmp(inputString, "q", 1) == 0))
                                {
                                    free(inputString);
                                    close(theDebugSocketFd);
                                    needHelp = 0;
                                    connected = 0;
                                }
                                else
                                {
                                    while(tableEntry)
                                    {
                                        if(strncmp(tableEntry->theMenu->subSystemName, inputString, strlen(tableEntry->theMenu->subSystemName)) == 0)
                                        {
                                            activeTableEntry = tableEntry;
                                            memset(debugPrompt, 0, DEBUG_PROMPT_MAX_SIZE);
                                            strcpy(debugPrompt, activeTableEntry->theMenu->subSystemName);
                                            strcat(debugPrompt, " debug > ");
                                            needHelp = 0;
                                            break;
                                        }
                                        tableEntry = tableEntry->nextTableEntry;
                                    }
                                    free(inputString);
                                }
                            }
                            if(needHelp)
                            {
                                if(!activeTableEntry)
                                {
                                    if(debugMenusTable)
                                    {
                                        tableEntry = debugMenusTable->aMenuEntry;
                                    }
                                    sprintf((char *) outbuff, "\n\n%-25s%-25s\n", "Command", "Help");
                                    sendSocket(theDebugSocketFd, &outbuff, strlen(outbuff), 0);
                                    sprintf((char *) outbuff, "=====================================\n");
                                    sendSocket(theDebugSocketFd, &outbuff, strlen(outbuff), 0);
                                    while(tableEntry)
                                    {
                                        if(tableEntry->theMenu->subSystemName)
                                        {
                                            sprintf((char *) outbuff, "%-25s", tableEntry->theMenu->subSystemName);
                                            sendSocket(theDebugSocketFd, &outbuff, strlen(outbuff), 0);
                                        }
                                        if(tableEntry->theMenu->subSystemHelp)
                                        {
                                            sprintf((char *) outbuff, "%-25s\n", tableEntry->theMenu->subSystemHelp);
                                            sendSocket(theDebugSocketFd, &outbuff, strlen(outbuff), 0);
                                        }
                                        tableEntry = tableEntry->nextTableEntry;
                                    }
                                    sprintf((char *) outbuff, "%-25s%-25s\n", "quit", "Exit debug mode");
                                    sendSocket(theDebugSocketFd, &outbuff, strlen(outbuff), 0);
                                }
                                else
                                {
                                    sprintf((char *) outbuff, "\n\n%-25s%-25s\n", "Command", "Help");
                                    sendSocket(theDebugSocketFd, &outbuff, strlen(outbuff), 0);
                                    sprintf((char *) outbuff, "=====================================\n");
                                    sendSocket(theDebugSocketFd, &outbuff, strlen(outbuff), 0);
                                    for(i = 0; i < activeTableEntry->numMenuItems; i++)
                                    {
                                        if(activeTableEntry->theMenu[i].commandName)
                                        {
                                            sprintf((char *) outbuff, "%-25s", activeTableEntry->theMenu[i].commandName);
                                            sendSocket(theDebugSocketFd, &outbuff, strlen(outbuff), 0);
                                        }
                                        if(activeTableEntry->theMenu[i].commandHelp)
                                        {
                                            sprintf((char *) outbuff, "%-25s\n", activeTableEntry->theMenu[i].commandHelp);
                                            sendSocket(theDebugSocketFd, &outbuff, strlen(outbuff), 0);
                                        }
                                    }
                                    sprintf((char *) outbuff, "%-25s%-25s\n", "quit", "go back to main menu\n");
                                    sendSocket(theDebugSocketFd, &outbuff, strlen(outbuff), 0);
                                }
                            }
                        }
                        else
                        {
                            continue;
                        }
                    }
                }
                else
                {
                    sendSocket((int )theDebugSocketFd, "Invalid Password entered\n", 26, 0);
                    if(close(theDebugSocketFd) != 0)
                    {
                        sprintf((char *) outbuff, "%s: close returned  =%d(%s) \n", __FUNCTION__, errno, strerror(errno));
                        sendSocket(theDebugSocketFd, &outbuff, strlen(outbuff), 0);
                    }
                }
            }
        }
        else
        {
            printf("%s: listen returned  =%d(%s) \n", __FUNCTION__, errno, strerror(errno));
            close(debugListenFd);
            break;
        }
    }
    printf("%s: Exiting *******************\n",__FUNCTION__);
    exit (0);
}
