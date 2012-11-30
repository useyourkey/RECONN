#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>
#include <net/if.h>

#include "debugMenu.h"
#include "reconn.h"
#include "clientApp.h"
#include "socket.h"
#include "upgrade.h"
#include "eqptResponse.h"
#include "remoteMonitor.h"
#include "libiphoned.h"


extern void reconnMasterIphone();
extern int theDebugSocketFd;
static int numberClients();
static int upgrade();
static int showClient();

#ifdef __SIMULATION__
static int iPhoneInserted = -1;
static int iPhoneInsert();
static int iPhoneExtract();
static int slaveMode();
static int masterMode();
#endif

static char *errMsg = "\r\nInvalid client index requested\r\n";
static char *sessionQuestion = "\r\nEnter client index > ";
pthread_mutex_t clientListMutex = PTHREAD_MUTEX_INITIALIZER;
CLIENTCONTEXT *activeClientsList[RECONN_MAX_NUM_CLIENTS];

debugMenuStruct clientDebugMenu[] =
{
    {"client", "Client debug Menus", NULL, NULL, NULL},
#ifdef __SIMULATION__
    {NULL, NULL, "insert", "Simulates front panel iPhone insertion", iPhoneInsert},
    {NULL, NULL, "extract", "Simulates front panel iPhone extraction", iPhoneExtract},
    {NULL, NULL, "slave", "Simulates USB client access request", slaveMode},
    {NULL, NULL, "master", "Simulates USB Master mode request", masterMode},
#endif
    {NULL, NULL, "list", "Display all active client source IPs", clientList},
    {NULL, NULL, "session",  "Display a clients session info ", showClient},
    {NULL, NULL, "sac", "Show the number of active client sessions", numberClients},
    {NULL, NULL, "upgrade", "Execute a software upgrade", upgrade},
};

void registerClientDebugMenu()
{
    registerDebugCommand(&clientDebugMenu[0], sizeof(clientDebugMenu)/sizeof(debugMenuStruct));
}

#ifdef __SIMULATION__
static int iPhoneInsert()
{
    iPhoneInserted = 0;
    reconnMasterIphone();
    return RECONN_SUCCESS;
}

static int iPhoneExtract()
{
    iPhoneInserted = -1;
    reconnMasterIphone();
    return RECONN_SUCCESS;
}

static int slaveMode()
{
    ReconnPacket thePacket;
    ReconnPacket *thePacketPtr = &thePacket;

    ADD_MSGID_TO_PACKET(CLIENT_ACCESS_REQ, thePacketPtr);
    ADD_DATA_LENGTH_TO_PACKET(0, thePacketPtr);
    insertedMasterRead((unsigned char *)&thePacket, 4);
    return RECONN_SUCCESS;
}

static int masterMode()
{
    ReconnPacket thePacket;
    ReconnPacket *thePacketPtr = &thePacket;

    ADD_MSGID_TO_PACKET(MASTER_MODE_REQ, thePacketPtr);
    ADD_DATA_LENGTH_TO_PACKET(0, thePacketPtr);
    insertedMasterRead((unsigned char *)&thePacket, 4);
    return RECONN_SUCCESS;
}
#endif

static int numberClients()
{
    memset((char *)&debugOutputString, 0, DEBUG_OUTPUT_LEN);
    sprintf((char *)&debugOutputString, "\r\nThe number of active clients is  %d\r\n", reconnClientsRegistered(ALL));
    sendSocket(theDebugSocketFd, (unsigned char *)&debugOutputString, strlen(debugOutputString), 0);
    return RECONN_SUCCESS;
}

static int upgrade(void)
{
    ReconnErrCodes retCode;

    if(reconnClientsRegistered(ALL) > 0)
    {
        sprintf((char *)&debugOutputString, "\r\nCan't upgrade because there are %d active clients.\r\n", reconnClientsRegistered(ALL));
        sendSocket(theDebugSocketFd, (unsigned char *)&debugOutputString, strlen((char *)&debugOutputString), 0);
    }
    else
    {
        if((retCode = extractBundle()) == RECONN_SUCCESS)
        {
            libiphoned_stop();
            system("killall -SIGTERM reconn-service");
        }
        else
        {
            memset((char *)&debugOutputString, 0, DEBUG_OUTPUT_LEN);
            switch (retCode)
            {
                case RECONN_UPGRADE_FILE_NOT_FOUND:
                {
                    strcpy((char *)&debugOutputString, "\r\nUPGRADE ABORTED: upgrade file not found.\r\n");
                    sendSocket(theDebugSocketFd, (unsigned char *)&debugOutputString, strlen(debugOutputString), 0);
                    break;
                }
                case RECONN_UPGRADE_BAD_CHECKSUM:
                {
                    strcpy((char *)&debugOutputString, "\r\nUPGRADE ABORTED: /tmp/upgradeBundle has an invalid checksum.\r\n");
                    sendSocket(theDebugSocketFd, (unsigned char *)&debugOutputString, strlen(debugOutputString), 0);
                    break;
                }
                default:
                {
                    strcpy((char *)&debugOutputString, "\r\nUPGRADE ABORTED: ");
                    strcat((char *)&debugOutputString, strerror(errno));
                    strcat((char *)&debugOutputString, "\r\n"); 
                    sendSocket(theDebugSocketFd, (unsigned char *)&debugOutputString, strlen(debugOutputString), 0);
                    break;
                }
            }
        }
    }
    return RECONN_SUCCESS;
}
#ifdef __SIMULATION__
int simulate_isiphonepresent()
{
    return iPhoneInserted;
}
#endif
int clientList(int theSocketFd)
{
    int i;
    CLIENTCONTEXT *aContext;

    memset(debugOutputString, 0, DEBUG_OUTPUT_LEN);
    sprintf(debugOutputString, "\r\n%-8s%-16s%-8s%-16s%-9s\r\n", "Client", "IP", "Port", "Client Mode", "SocketFd"); 
    sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0); 
    memset(debugOutputString, 0, DEBUG_OUTPUT_LEN);
    sprintf(debugOutputString, "=========================================================\r\n"); 
    sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0); 

#ifdef DEBUG_MUTEX
    reconnDebugPrint("%s: Calling pthread_mutex_lock(&clientListMutex) %d %d %d\r\n", __FUNCTION__, clientListMutex.__data.__lock, clientListMutex.__data.__count, clientListMutex.__data.__owner);
#endif
    pthread_mutex_lock(&clientListMutex);

    for(i = 0; i < RECONN_MAX_NUM_CLIENTS; i++, aContext++)
    {
        aContext = activeClientsList[i];
        if(aContext)
        {
            memset(debugOutputString, 0, DEBUG_OUTPUT_LEN);
            if(i == 0)
            {
                sprintf(debugOutputString, "%-8d%-16s%-8s%-16s%-9s\r\n", aContext->index, "Usb Connector", "NA", "Inserted Master", "NA");
            }
            else
            { 
                sprintf(debugOutputString, "%-8d%-16s%-8d%-16s%-9d\r\n", aContext->index, inet_ntoa(aContext->sourceIp.sin_addr), aContext->sourceIp.sin_port, (activeClientsList[i]->mode == SLAVEMODE) ? "SLAVE" : (activeClientsList[i]->mode == MASTERMODE) ? "WIFI MASTER": "REMOTE", aContext->socketFd);
            }
            sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0); 
        }
    }
#ifdef DEBUG_MUTEX
    reconnDebugPrint("%s: Calling pthread_mutex_unlock(&clientListMutex) %d %d %d\r\n", __FUNCTION__, clientListMutex.__data.__lock, clientListMutex.__data.__count, clientListMutex.__data.__owner);
#endif
    pthread_mutex_unlock(&clientListMutex);
    return RECONN_SUCCESS;
}
static int showClient(int theSocketFd)
{
    int index;
    char *theAnswer;
    CLIENTCONTEXT *theContext = NULL;


    // Ask the user for an index
    sendSocket(theDebugSocketFd, (unsigned char *)sessionQuestion, strlen(sessionQuestion), 0);
    theAnswer = getInput(theSocketFd, YES);
    index = atoi(theAnswer);
    sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_CLEARSCREEN, strlen(DEBUG_CLEARSCREEN), 0);
    sendSocket(theDebugSocketFd, (unsigned char *)DEBUG_CURSORHOME, strlen(DEBUG_CURSORHOME), 0);
    if((index < 0) || (index > RECONN_MAX_NUM_CLIENTS))
    {
        sendSocket(theSocketFd, (unsigned char *)errMsg, strlen(errMsg), 0); 
    }
    else
    {
#ifdef DEBUG_MUTEX
        reconnDebugPrint("%s: Calling pthread_mutex_lock(&clientListMutex) %d %d %d\r\n", __FUNCTION__, clientListMutex.__data.__lock, clientListMutex.__data.__count, clientListMutex.__data.__owner);
#endif

        pthread_mutex_lock(&clientListMutex);
        theContext = activeClientsList[index];
        if(theContext)
        {
            memset(debugOutputString, 0, DEBUG_OUTPUT_LEN);
            sprintf(debugOutputString, "\r\n%-17s %s\r\n", "Variable", "Value"); 
            sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);

            memset(debugOutputString, 0, DEBUG_OUTPUT_LEN);
            sprintf(debugOutputString, "=========================\r\n"); 
            sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);

            memset(debugOutputString, 0, DEBUG_OUTPUT_LEN);
            sprintf(debugOutputString, "%-17s 0x%x\r\n", "thisContext", (unsigned int)theContext->thisContext); 
            sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);

            memset(debugOutputString, 0, DEBUG_OUTPUT_LEN);
            sprintf(debugOutputString, "%-17s %d\r\n", "index", theContext->index); 
            sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);

            memset(debugOutputString, 0, DEBUG_OUTPUT_LEN);
            sprintf(debugOutputString,  "%-17s 0x%x\r\n", "theEqptFd", (unsigned int)theContext->eqptDescriptors); 
            sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);

            memset(debugOutputString, 0, DEBUG_OUTPUT_LEN);
            sprintf(debugOutputString,  "%-17s 0x%x\r\n","socketFd", theContext->socketFd); 
            sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);

            memset(debugOutputString, 0, DEBUG_OUTPUT_LEN);
            sprintf(debugOutputString,  "%-17s %d\r\n", "connectionOpen", theContext->connectionOpen);
            sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);

            memset(debugOutputString, 0, DEBUG_OUTPUT_LEN);
            sprintf(debugOutputString,  "%-17s %d\r\n", "pktLength", theContext->pktLength); 
            sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);

            memset(debugOutputString, 0, DEBUG_OUTPUT_LEN);
            sprintf(debugOutputString,  "%-17s %d\r\n", "length", theContext->length); 
            sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);

            memset(debugOutputString, 0, DEBUG_OUTPUT_LEN);
            sprintf(debugOutputString,  "%-17s %d\r\n", "responseNeeded", theContext->responseNeeded); 
            sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);

            memset(debugOutputString, 0, DEBUG_OUTPUT_LEN);
            sprintf(debugOutputString,  "%-17s %d\r\n", "retStatus", theContext->retStatus); 
            sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);

#ifdef DEBUG_CLIENT
            memset(debugOutputString, 0, DEBUG_OUTPUT_LEN);
            sprintf(debugOutputString,  "%-17s %d\r\n", "debugIndex", theContext->debugIndex); 
            sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);
#endif

            memset(debugOutputString, 0, DEBUG_OUTPUT_LEN); sprintf(debugOutputString,  "%-17s 0x%x\r\n", "cmdid", theContext->cmdid);
            sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);

            memset(debugOutputString, 0, DEBUG_OUTPUT_LEN);sprintf(debugOutputString,  "%-17s %d\r\n", "responseId", theContext->responseId); sprintf(debugOutputString,  "%-17s %d\r\n", "responseId", theContext->responseId);
            sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);

            memset(debugOutputString, 0, DEBUG_OUTPUT_LEN); sprintf(debugOutputString,  "%-17s %d\r\n", "retCode", theContext->retCode);
            sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);

            memset(debugOutputString, 0, DEBUG_OUTPUT_LEN); sprintf(debugOutputString,  "%-17s %s\r\n", "mode", (theContext->mode == MASTERMODE) ? "Master": (theContext->mode == SLAVEMODE) ? "Slave" : "Inserted Master" );
            sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);

            memset(debugOutputString, 0, DEBUG_OUTPUT_LEN); sprintf(debugOutputString,  "%-17s %d\r\n", "DmmFd", theContext->eqptDescriptors->dmmFd);
            sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);

            memset(debugOutputString, 0, DEBUG_OUTPUT_LEN); sprintf(debugOutputString,  "%-17s %d\r\n", "powerMeterFd", theContext->eqptDescriptors->powerMeterFd);
            sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);

            memset(debugOutputString, 0, DEBUG_OUTPUT_LEN); sprintf(debugOutputString,  "%-17s %d\r\n", "dmmFd", theContext->eqptDescriptors->dmmFd);

            memset(debugOutputString, 0, DEBUG_OUTPUT_LEN);
            sprintf(debugOutputString,  "%-17s %d\r\n", "analyzerFd", theContext->eqptDescriptors->analyzerFd); sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);
        }
        else
        {
            sendSocket(theSocketFd, (unsigned char *)errMsg, strlen(errMsg), 0); 
        }
#ifdef DEBUG_MUTEX
        reconnDebugPrint("%s: Calling pthread_mutex_unlock(&clientListMutex) %d %d %d\r\n", __FUNCTION__, clientListMutex.__data.__lock, clientListMutex.__data.__count, clientListMutex.__data.__owner);
#endif
        pthread_mutex_unlock(&clientListMutex);
    }
    free(theAnswer);
    return RECONN_SUCCESS;
} 
