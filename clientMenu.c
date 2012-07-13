#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <execinfo.h>

#include "debugMenu.h"
#include "reconn.h"
#include "clientApp.h"
#include "socket.h"
#include "upgrade.h"
#include "eqptResponse.h"

extern void reconnMasterIphone();
extern int theDebugSocketFd;
static int numberClients();
static int upgrade();
static int reset();

#ifdef __SIMULATION__
static int iPhoneInserted = -1;
static int iPhoneInsert();
static int iPhoneExtract();
static int clientMode();
static int masterMode();
#endif

debugMenuStruct clientDebugMenu[] =
{
    {"client", "Client debug Menus", NULL, NULL, NULL},
#ifdef __SIMULATION__
    {NULL, NULL, "insert", "Simulates front panel iPhone insertion", iPhoneInsert},
    {NULL, NULL, "extract", "Simulates front panel iPhone extraction", iPhoneExtract},
    {NULL, NULL, "client", "Simulates USB client access request", clientMode},
    {NULL, NULL, "master", "Simulates USB Master mode request", masterMode},
#endif
    {NULL, NULL, "sac", "Show the number of active client sessions", numberClients},
    {NULL, NULL, "upgrade", "Execute a software upgrade", upgrade},
    {NULL, NULL, "reset", "reset the box", reset},
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

static int clientMode()
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
    char outbuf[DEBUG_OUTPUT_LEN];

    memset((char *)&outbuf, 0, DEBUG_OUTPUT_LEN);
    sprintf((char *)&outbuf, "The number of active clients is  %d\r\n", reconnClientsRegistered());
    sendSocket(theDebugSocketFd, (unsigned char *)&outbuf, strlen(outbuf), 0);
    return RECONN_SUCCESS;
}

static int reset(void)
{
    int *i = 50;
    *i = 0;
    return RECONN_SUCCESS;
}
static int upgrade(void)
{
    char outbuf[DEBUG_OUTPUT_LEN];
    ReconnErrCodes retCode;

    if(reconnClientsRegistered() > 0)
    {
        sprintf((char *)&outbuf, "Can't upgrade because there are %d active clients.\r\n", reconnClientsRegistered());
        sendSocket(theDebugSocketFd, (unsigned char *)&outbuf, strlen((char *)&outbuf), 0);
    }
    else
    {
        if((retCode = extractBundle()) == RECONN_SUCCESS)
        {
            system("killall iphoned");
            system("killall reconn-service");
        }
        else
        {
            memset((char *)&outbuf, 0, DEBUG_OUTPUT_LEN);
            switch (retCode)
            {
                case RECONN_UPGRADE_FILE_NOT_FOUND:
                {
                    strcpy((char *)&outbuf, "UPGRADE ABORTED: upgrade file not found.\n");
                    sendSocket(theDebugSocketFd, (unsigned char *)&outbuf, strlen(outbuf), 0);
                    break;
                }
                case RECONN_UPGRADE_BAD_CHECKSUM:
                {
                    strcpy((char *)&outbuf, "UPGRADE ABORTED: /tmp/upgradeBundle has an invalid checksum.\n");
                    sendSocket(theDebugSocketFd, (unsigned char *)&outbuf, strlen(outbuf), 0);
                    break;
                }
                default:
                {
                    strcpy((char *)&outbuf, "UPGRADE ABORTED: ");
                    strcat((char *)&outbuf, strerror(errno));
                    strcat((char *)&outbuf, "\n"); 
                    sendSocket(theDebugSocketFd, (unsigned char *)&outbuf, strlen(outbuf), 0);
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
