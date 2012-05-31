#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "debugMenu.h"
#include "reconn.h"

static int iPhoneInserted = -1;

extern void reconnMasterIphone();
extern int theDebugSocketFd;

static int iPhoneInsert();
static int iPhoneExtract();
static int clientMode();
static int masterMode();
static int numberClients();
static int upgrade();

static char *percentageMessage = "Battery charge is at %d%%\n";
static char *chargerAttached =  "GPIO137 is 1\n";
static char *chargerNotAttached =  "GPIO137 is 0\n";

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

static int iPhoneExtract()
{
    iPhoneInserted = -1;
    reconnMasterIphone();
    return RECONN_SUCCESS;
}

int clientMode()
{
    ReconnPacket thePacket;
    ReconnPacket *thePacketPtr = &thePacket;

    ADD_MSGID_TO_PACKET(CLIENT_ACCESS_REQ, thePacketPtr);
    ADD_DATA_LENGTH_TO_PACKET(0, thePacketPtr);
    insertedMasterRead(&thePacket, 4);
    return RECONN_SUCCESS;
}

int masterMode()
{
    ReconnPacket thePacket;
    ReconnPacket *thePacketPtr = &thePacket;

    ADD_MSGID_TO_PACKET(MASTER_MODE_REQ, thePacketPtr);
    ADD_DATA_LENGTH_TO_PACKET(0, thePacketPtr);
    insertedMasterRead(&thePacket, 4);
    return RECONN_SUCCESS;
}

int numberClients()
{
    char outbuf[DEBUG_OUTPUT_LEN];

    memset((char *)&outbuf, 0, DEBUG_OUTPUT_LEN);
    sprintf((char *)&outbuf, "The number of active clients is  %d\r\n", reconnClientsRegistered());
    if(send(theDebugSocketFd, outbuf, strlen(outbuf)) <= 0)
    {
        reconnDebugPrint("%s: send failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
    }
    return RECONN_SUCCESS;
}

int upgrade(void)
{
    char outbuf[DEBUG_OUTPUT_LEN];
    ReconnErrCodes retCode;

    if(reconnClientsRegistered() > 0)
    {
        sprintf((char *)&outbuf, "Can't upgrade because there are %d active clients.\r\n", reconnClientsRegistered());
        send(theDebugSocketFd, (char *)&outbuf, strlen((char *)&outbuf));
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
                    send(theDebugSocketFd, &outbuf, strlen(outbuf));
                    break;
                }
                case RECONN_UPGRADE_BAD_CHECKSUM:
                {
                    strcpy((char *)&outbuf, "UPGRADE ABORTED: /tmp/upgradeBundle has an invalid checksum.\n");
                    send(theDebugSocketFd, &outbuf, strlen(outbuf));
                    break;
                }
                default:
                {
                    strcpy((char *)&outbuf, "UPGRADE ABORTED: ");
                    strcat((char *)&outbuf, strerror(errno));
                    strcat((char *)&outbuf, "\n"); 
                    send(theDebugSocketFd, &outbuf, strlen(outbuf));
                    break;
                }
            }
        }
    }
    return RECONN_SUCCESS;
}
