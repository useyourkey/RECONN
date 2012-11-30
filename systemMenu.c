#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>

#include "reconn.h"
#include "debugMenu.h"
#include "powerMgmt.h"
#include "socket.h"
#include "wifi.h"
#include "version.h"

static int macSet();
static int macShow();
static int serialSet();
static int serialShow();
static int timers();
static int systemStats();
static int resetStats();
static int defaultState();
static int noDefault();
static int showVersion();


#ifndef __SIMULATION__
static char *macWriteWifiCommandString = "calibrator set nvs_mac /lib/firmware/ti-connectivity/wl1271-nvs.bin ";
#endif

debugMenuStruct systemDebugMenu[] =
{
    {"system", "System wide debug Menus", NULL, NULL, NULL},
    {NULL, NULL, "mac",         "displays the reconn mac addresses", macShow},
    {NULL, NULL, "macSet",      "changes the WiFi mac address", macSet},
    {NULL, NULL, "serial",      "displays the unit's serial number", serialShow},
    {NULL, NULL, "serialSet",   "changes the unit's serial number", serialSet},
    {NULL, NULL, "stats",       "displays some system stats", systemStats},
    {NULL, NULL, "reset",       "resets all stats", resetStats},
    {NULL, NULL, "timers",      "Displays the system's shutdown timers", timers},
    {NULL, NULL, "default",     "Sets the reconn back to default state. Also removes WIFI", defaultState},
    {NULL, NULL, "nodefault",   "Set box as non-defaulted. Also enables WIFI", noDefault},
    {NULL, NULL, "version",     "Displays the reconn software version", showVersion}
};

void registerSystemDebugMenu()
{
    registerDebugCommand(&systemDebugMenu[0], sizeof(systemDebugMenu)/sizeof(debugMenuStruct));
}

static int timers(int theSocketFd)
{
    PowerMgmtEqptCounters eqptCounters;

    if(getStandbyCounters(&eqptCounters) == RECONN_SUCCESS)
    {
        memset(debugOutputString, 0, DEBUG_OUTPUT_LEN);
        sprintf((char *)&debugOutputString, "\r\n%-25s%s\r\n", "Variable", "Count");
        sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);

        memset(debugOutputString, 0, DEBUG_OUTPUT_LEN);
        sprintf((char *)&debugOutputString, "================================\r\n");
        sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);

        memset(debugOutputString, 0, DEBUG_OUTPUT_LEN);
        sprintf((char *)&debugOutputString, "%-25s%d\r\n", "PowerMeterCounter", eqptCounters.PowerMeterCounter);
        sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);

        memset(debugOutputString, 0, DEBUG_OUTPUT_LEN);
        sprintf((char *)&debugOutputString, "%-25s%d\r\n", "DmmCounter", eqptCounters.DmmCounter);
        sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);

        memset(debugOutputString, 0, DEBUG_OUTPUT_LEN);
        sprintf((char *)&debugOutputString, "%-25s%d\r\n", "GpsCounter", eqptCounters.GpsCounter);
        sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);

        memset(debugOutputString, 0, DEBUG_OUTPUT_LEN);
        sprintf((char *)&debugOutputString, "%-25s%d\r\n", "SpectrumAnalyzerCounter", eqptCounters.SpectrumAnalyzerCounter);
        sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);

        memset(debugOutputString, 0, DEBUG_OUTPUT_LEN);
        sprintf((char *)&debugOutputString, "%-25s%d\r\n", "ReconnSystemCounter", eqptCounters.ReconnSystemCounter);
        sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);
    }
    return RECONN_SUCCESS;
}

static int serialSet(int theSocketFd)
{
    int notDone = TRUE;
    unsigned int i;
    int allDigits = TRUE;
    FILE *serialNumberFd;
    size_t bytesRead;
    char *responsePtr;

    while(notDone)
    {
        memset(&debugOutputString, 0, DEBUG_OUTPUT_LEN);
        sprintf((char *)&debugOutputString, "\r\nEnter the serial number in the form R 0-9 alpha character\n\rfollowed by 4 digits (Example R1A0001) or q to quit > ");
        sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);
        if((responsePtr = getInput(theSocketFd, YES)) != NULL)
        {
            memset(&debugOutputString, 0, DEBUG_OUTPUT_LEN);
            if(strlen(responsePtr) > RECONN_SERIALNUM_MAX_LEN)
            {
                sprintf((char *)&debugOutputString,"\r\n\r\nSerial number is too long.\r\n");
                sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);
                free(responsePtr);
                continue;
            }
            else if((responsePtr[0] == 'Q') || (responsePtr[0] == 'q'))
            {
                notDone = FALSE;
                free(responsePtr);
                continue;
            }
            else if(responsePtr[0] != 'R')
            {
                sprintf((char *)&debugOutputString,"\r\nFirst Character must be R.\r\n");
                sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);
                free(responsePtr);
                continue;
            }
            else if(!isdigit(responsePtr[1]))
            {
                sprintf((char *)&debugOutputString,"\r\n%c is not a digit.\r\n", responsePtr[1]);
                sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);
                free(responsePtr);
                continue;
            }
            else if(!isalpha(responsePtr[2]))
            {
                sprintf((char *)&debugOutputString,"\r\n%c is not a character.\r\n", responsePtr[1]);
                sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);
                free(responsePtr);
                continue;
            }
            else
            {
                for(i = 3; i < strlen(responsePtr); i++)
                {
                    if(!isdigit(responsePtr[i]))
                    {
                        sprintf((char *)&debugOutputString,"\r\n%c is not a digit.\r\n", responsePtr[i]);
                        sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);
                        allDigits = FALSE;
                        free(responsePtr);
                        continue;
                    }
                }
                if(allDigits == TRUE)
                {
                    if((serialNumberFd = fopen(RECONN_SERIALNUM_FILE_NAME, "w+")))
                    {
                        if((bytesRead = fwrite(responsePtr, 1, strlen(responsePtr), serialNumberFd)) != strlen(responsePtr))
                        {
                            reconnDebugPrint("%s: fwrite() failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
                        }
                        fclose(serialNumberFd);
                        free(responsePtr);
                        notDone = FALSE;
                    }
                    else
                    {
                        reconnDebugPrint("%s: fopen(%s, w+)() failed %d(%s)\n", __FUNCTION__, RECONN_SERIALNUM_FILE_NAME, errno, strerror(errno));
                    }
                }
            }
        }
        else
        {
            reconnDebugPrint("%s: getInput returned NULL\n", __FUNCTION__);
            notDone = FALSE;
        }
    }
    return RECONN_SUCCESS;
}
static int serialShow(int theSocketFd)
{
    char theSerialNumberString[RECONN_SERIALNUM_MAX_LEN];
    short serialBytesRead;
    FILE *serialNumberFd = NULL;

    if((serialNumberFd = fopen(RECONN_SERIALNUM_FILE_NAME, "r")))
    {
        memset(&theSerialNumberString, 0, RECONN_SERIALNUM_MAX_LEN);
        serialBytesRead = fread(&theSerialNumberString, 1, RECONN_SERIALNUM_MAX_LEN, serialNumberFd);
        if((serialBytesRead == 0) && (feof(serialNumberFd)))
        {
            memset(debugOutputString, 0, DEBUG_OUTPUT_LEN);
            sprintf(debugOutputString, "\r\nSerial number record is empty. Use serialSet command to program the serial number\n");
            sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);
            reconnDebugPrint("%s: fread () failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
        }
        else
        {
            memset(debugOutputString, 0, DEBUG_OUTPUT_LEN);
            sprintf(debugOutputString, "\r\n\r\nSerial number is %s\r\n\r\n", theSerialNumberString);
            sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);
        }
        fclose(serialNumberFd);
    }
    else
    {
        memset(debugOutputString, 0, DEBUG_OUTPUT_LEN);
        sprintf(debugOutputString, "\r\nSerial number has not been configured.\r\n");
        sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);
        reconnDebugPrint("%s: open(%s, r) failed %d(%s)\n", __FUNCTION__, RECONN_SERIALNUM_FILE_NAME, errno, 
                strerror(errno));
    }
    return RECONN_SUCCESS;
}

static int macShow(int theSocketFd)
{
    char theMacString[RECONN_MACADDR_LEN]; // aa:bb:cc:dd:ee:ff
    int tmpSocket, i, index;
    struct ifreq buffer;

    memset(&debugOutputString, 0, DEBUG_OUTPUT_LEN);
    memset(&theMacString, 0, RECONN_MACADDR_LEN);
    if(wifiGetMacAddress((char *)&theMacString) == RECONN_SUCCESS)
    {
        sprintf(debugOutputString, "\r\n\r\nWiFi mac address is ");
        strncat(debugOutputString, (char *)&theMacString, RECONN_MACADDR_LEN);
    }
    else
    {
        sprintf(debugOutputString, "\r\nCould not read WiFi mac address");
    }
    strcat(debugOutputString, "\r\n");
    sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);

    memset(&debugOutputString, 0, DEBUG_OUTPUT_LEN);
    memset(&theMacString, 0, RECONN_MACADDR_LEN);
    sprintf(debugOutputString, "RJ-45 mac address is ");
    if((tmpSocket = socket(PF_INET, SOCK_STREAM, 0)) != -1) 
    {
#ifndef __SIMULATION__
        strcpy (buffer.ifr_name, "usb0");
#else
        strcpy (buffer.ifr_name, "eth0");
#endif
        ioctl(tmpSocket, SIOCGIFHWADDR, &buffer);

        for(index = 0, i = 0; i < 6; i++)
        {
            sprintf(&(theMacString[index]), "%.2X", (unsigned char)buffer.ifr_hwaddr.sa_data[i]);
            if(i < 5)
            {
                strcat(theMacString, ":");
                index += 3;
            }
        }
        strcat((char *)&debugOutputString, theMacString);
        close(tmpSocket);
    }
    else
    {
        reconnDebugPrint("%s: socket(PF_INET, SOCK_STREAM, 0) failed %d (%s)\n", __FUNCTION__, errno, strerror(errno));
    }

    strcat(debugOutputString, "\r\n");
    sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);
    return RECONN_SUCCESS;
}

static int macSet(int theSocketFd)
{
    int Done = FALSE;
    int i, numColons, alphaNumerics, setWiFi = FALSE;
    char theCommand[100];
    char *responsePtr;
    FILE *rj45MacFd;

    memset(&debugOutputString, 0, DEBUG_OUTPUT_LEN);
    while(Done == FALSE)
    {
        sendSocket(theSocketFd, (unsigned char *)DEBUG_CLEARSCREEN, strlen(DEBUG_CLEARSCREEN), 0);
        sendSocket(theSocketFd, (unsigned char *)DEBUG_CURSORHOME, strlen(DEBUG_CURSORHOME), 0);
        sprintf((char *)&debugOutputString, "\r\n1) Wifi\r\n2) RJ-45\r\nq) quit\r\n(1, 2 or q)> ");
        sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);
        if((responsePtr = getInput(theSocketFd, YES)) != NULL)
        {
            if(strcmp(responsePtr, "1") == 0)
            {
                setWiFi = TRUE;
                Done = FALSE;
                break;
            }
            else if(strcmp(responsePtr, "2") == 0)
            {
                Done = FALSE;
                break;
            }
            else if(strcmp(responsePtr, "q") == 0)
            {
                Done = TRUE;
                sendSocket(theSocketFd, (unsigned char *)DEBUG_CLEARSCREEN, strlen(DEBUG_CLEARSCREEN), 0);
                sendSocket(theSocketFd, (unsigned char *)DEBUG_CURSORHOME, strlen(DEBUG_CURSORHOME), 0);
            }
        }
        else
        {
            reconnDebugPrint("%s: recv() failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
            Done = TRUE;
        }
    }

    /*
     * Get the mac address from the user.
     */
    while(Done == FALSE)
    {
        memset(&debugOutputString, 0, DEBUG_OUTPUT_LEN);
        sprintf((char *)&debugOutputString, "\r\nEnter the mac address in the form aa:bb:cc:dd:ee:ff or q to quit > ");
        sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);
        if((responsePtr = getInput(theSocketFd, YES)) != NULL)
        {
            memset(&debugOutputString, 0, DEBUG_OUTPUT_LEN);
            if(strlen(responsePtr) > RECONN_MACADDR_LEN)
            {
                sprintf((char *)&debugOutputString,"\r\n\r\nIncorrect mac address format\r\n");
                sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);
                continue;
            }
            else if((responsePtr[0] == 'Q') || (responsePtr[0] == 'q'))
            {
                Done = TRUE;
                continue;
            }
            /*
             * Now check the format. There has to be exactly 5 colons 12 alphanumerics
             */
            for(i = numColons =  alphaNumerics = 0; i < RECONN_MACADDR_LEN; i++)
            {
                if(responsePtr[i] == ':')
                    numColons++;
                if(isalnum(responsePtr[i]))
                    alphaNumerics++;
            }
            if((numColons != 5) || (alphaNumerics != 12))
            {
                sprintf(debugOutputString, "\r\ninvalid mac address \r\n");
                sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);
                continue;
            }

            if(setWiFi)
            {
                memset(theCommand, 0, 100);
                /*
                 * Set the mac address in the WIFI configuration file
                 */
#ifndef __SIMULATION__
                sprintf(theCommand, "%s%s", macWriteWifiCommandString, responsePtr);
#else
                sprintf(theCommand, "%s%s%s%s", "echo ", responsePtr, ">", RECONN_MACADDR_FILE_NAME);
#endif
                reconnDebugPrint("%s: %s\n", __FUNCTION__, theCommand);
                system(theCommand);

                /*
                 * Change the hardware's MAC address
                 */
#ifndef __SIMULATION__
                sprintf(theCommand, "ifconfig wlan0 down hw ether %s", responsePtr);
                system(theCommand);
                sleep(2);
#endif
                reconnDebugPrint("%s: %s\n", __FUNCTION__, theCommand);
#ifndef __SIMULATION__
                system("ifconfig wlan0 up");
#endif
                reconnDebugPrint("%s: ifconfig wlan0 up\n", __FUNCTION__);
                Done = TRUE;
            }
            else
            {
                if((rj45MacFd = fopen(RJ45_MAC_ADDRRESS_FILE_NAME, "w")))
                {
                    if (fwrite(responsePtr, 1, RECONN_MACADDR_LEN, rj45MacFd) != RECONN_MACADDR_LEN)
                    {
                        reconnDebugPrint("%s: fwrite(%s, 1, %d, %d) failed %d (%s)\n", __FUNCTION__, 
                                responsePtr, RECONN_MACADDR_LEN, rj45MacFd, errno, strerror(errno));
                        fclose(rj45MacFd);
                        Done = TRUE;
                    }
                    else
                    {
                        fclose(rj45MacFd);
                        memset(theCommand, 0, 100);
                        sprintf(theCommand, "ifconfig usb0 down hw ether %s", responsePtr);
#ifndef __SIMULATION__
                        system(theCommand);
                        sleep(2);
#endif
                        reconnDebugPrint("%s: %s\n\n", __FUNCTION__, theCommand);

                        sprintf(theCommand, "ifconfig usb0 up");
#ifndef __SIMULATION__
                        system(theCommand);
#endif
                        reconnDebugPrint("%s: %s\n\n", __FUNCTION__, theCommand);
                        Done = TRUE;
                    }
                }
                else
                { 
                    reconnDebugPrint("%s: fopen(%s, w) failed %d (%s)\n", __FUNCTION__, 
                        RJ45_MAC_ADDRRESS_FILE_NAME, errno, strerror(errno));
                    Done = TRUE;
                }
            }
        }
        else
        {
            reconnDebugPrint("%s: recv() failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
            Done = FALSE;
        }
    }
    return RECONN_SUCCESS;
}

static int systemStats(int theSocketFd)
{
    extern int gAnalyzerHigh, gPayloadHigh, gDmmHigh, gPowerMeterHigh;
    extern int gDmmResponses, gAnalyzerResponses, gMeterResponses;

    memset(debugOutputString, 0, DEBUG_OUTPUT_LEN);
    sprintf((char *)&debugOutputString, "\r\n%-20s%s\r\n", "Variable", "Value");
    sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);

    memset(debugOutputString, 0, DEBUG_OUTPUT_LEN);
    sprintf((char *)&debugOutputString, "================================\r\n");
    sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);

    memset(debugOutputString, 0, DEBUG_OUTPUT_LEN);
    sprintf((char *)&debugOutputString, "%-20s%d\r\n", "gAnalyzerHigh", gAnalyzerHigh);
    sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);

    memset(debugOutputString, 0, DEBUG_OUTPUT_LEN);
    sprintf((char *)&debugOutputString, "%-20s%d\r\n", "gDmmHigh",  gDmmHigh);
    sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);

    memset(debugOutputString, 0, DEBUG_OUTPUT_LEN);
    sprintf((char *)&debugOutputString, "%-20s%d\r\n", "gPowerMeterHigh",  gPowerMeterHigh);
    sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);

    memset(debugOutputString, 0, DEBUG_OUTPUT_LEN);
    sprintf((char *)&debugOutputString, "%-20s%d\r\n", "gPayloadHigh", gPayloadHigh);
    sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);

    memset(debugOutputString, 0, DEBUG_OUTPUT_LEN);
    sprintf((char *)&debugOutputString, "%-20s%d\r\n", "gDmmResponses", gDmmResponses);
    sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);

    memset(debugOutputString, 0, DEBUG_OUTPUT_LEN);
    sprintf((char *)&debugOutputString, "%-20s%d\r\n", "gAnalyzerResponses", gAnalyzerResponses);
    sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);

    memset(debugOutputString, 0, DEBUG_OUTPUT_LEN);
    sprintf((char *)&debugOutputString, "%-20s%d\r\n", "gMeterResponses", gMeterResponses);
    sendSocket(theSocketFd, (unsigned char *)debugOutputString, strlen(debugOutputString), 0);
    return RECONN_SUCCESS;
}

static int resetStats(int theSocketFd)
{
    extern int gAnalyzerHigh, gPayloadHigh, gDmmHigh, gPowerMeterHigh;
    extern int gDmmResponses, gAnalyzerResponses, gMeterResponses;

    UNUSED_PARAM(theSocketFd);
    gAnalyzerHigh = gPayloadHigh = gDmmHigh = gPowerMeterHigh = 0;
    gDmmResponses = gAnalyzerResponses = gMeterResponses = 0;
    return RECONN_SUCCESS;
}

static int noDefault(int theSocketFd)
{
    UNUSED_PARAM(theSocketFd);
    unlink(RECONN_DEFAULT_FILE_NAME);
    wifiSetState(WIFI_ENABLE);
    return RECONN_SUCCESS;
}

static int defaultState(int theSocketFd)
{
    char *theCommand;
    int length = strlen("touch ") + strlen(RECONN_DEFAULT_FILE_NAME) + 1;

    UNUSED_PARAM(theSocketFd);
    theCommand = malloc(length);
    if(theCommand)
    {
        memset(theCommand, 0, length);
        strcat(theCommand, "touch ");
        strcat(theCommand, RECONN_DEFAULT_FILE_NAME);
        system(theCommand);
        wifiSetState(WIFI_DISABLE);
        unlink(WIFI_ACTIVE_FILE_NAME);
        free(theCommand);
    }
    return RECONN_SUCCESS;
}
static int showVersion(int theSocketFd)
{
    sendSocket(theSocketFd, (unsigned char *)getReconnSwVersion(), strlen(getReconnSwVersion()), 0);
    return RECONN_SUCCESS;
}
