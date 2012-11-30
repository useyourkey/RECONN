//******************************************************************************
//******************************************************************************
// FILE:        wifi.c
//
// FUNCTION:        wifiUpdateConfFile
//
// DESCRIPTION: This file contains interfaces used by the reconn embedded software
//              to update wifi related data and hardware
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
// $Log:$
// 
//******************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
//#include <sys/stat.h>
#include <sys/ioctl.h>
//#include <arpa/inet.h>
#include <linux/if.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "reconn.h"
#include "clientApp.h"
#include "wifi.h"
#include "debugMenu.h"
#include "remoteMonitor.h"
#include "eqptResponse.h"

static int wifiSocketFd = -1;

#if 0
#ifndef __SIMULATION__
static char *macReadCommandString = "calibrator get nvs_mac /lib/firmware/ti-connectivity/wl1271-nvs.bin > ";
#else
static char *macReadCommandString = "echo  MAC addr from NVS: de:ad:be:ef:00:12 > ";
#endif
#endif

static int taskDone = FALSE;


static WIFIENUM theWiFiState = WIFI_DISABLE;
//******************************************************************************
//******************************************************************************
// FUNCTION:        wifiCleanUp
//
// DESCRIPTION: Performs all wifi cleanup.
//
// PARAMETERS:
//
//******************************************************************************
void wifiCleanUp()
{
    reconnDebugPrint("%s: Function entered wifiSocketFd = %d\n", __FUNCTION__, wifiSocketFd);
    if(wifiSocketFd != -1)
    {
        if(shutdown(wifiSocketFd, 2) != 0)
        {
            reconnDebugPrint("%s: shutdown(%d, 2) failed %d (%s)\n", __FUNCTION__, wifiSocketFd,
                    errno , strerror(errno));
        }

        reconnDebugPrint("%s: closing wifiSocketFd %d\n", __FUNCTION__, wifiSocketFd);
        if(close(wifiSocketFd) != 0)
        {
            reconnDebugPrint("%s close(%d) failed %d (%s)\n", __FUNCTION__, wifiSocketFd, errno, strerror(errno));
        }
        wifiSocketFd = -1;
    }
    taskDone = TRUE;
}
//******************************************************************************
//******************************************************************************
// FUNCTION:        wifiStartConnectionTask
//
// CLASSES:     
//
// DESCRIPTION: This is called to start the task that accepts reconn connections 
//              over the wifi interface. 
//              theWifiState is set at initialization at the top of this file. 
//              If WIFI_ACTIVE_FILE_NAME is present
//              then the reconn device has been configured to have wifi turned on.
//
// PARAMETERS:
//
//******************************************************************************
ReconnErrCodes wifiStartConnectionTask()
{
    ReconnErrCodes retCode = RECONN_SUCCESS;
    struct stat fileStat;

    reconnDebugPrint("%s: Function Entered\n", __FUNCTION__);
    taskDone = FALSE;
    atexit(wifiCleanUp);


    if(stat(WIFI_ACTIVE_FILE_NAME, &fileStat) == 0) 
    {
        /*
         * The presence of WIFI_ACTIVE_FILE_NAME means the user has enabled wifi via
         * the iPhone application.
         */
        theWiFiState = WIFI_ENABLE;
        reconnDebugPrint("%s: Starting wifiConnectionTask\n", __FUNCTION__);
        if(pthread_create(&(reconnThreadIds[RECONN_WIFI_CONNECTION_TASK]), NULL, wifiConnectionTask, (void *)0) < 0)
        {
            reconnDebugPrint("%s: Could not start wifiConnectionTask %d %s\n", __FUNCTION__, errno, strerror(errno));
            retCode = RECONN_FAILURE;
        }
    }
    else if(errno == ENOENT)
    {
        reconnDebugPrint("%s: WIFI is disabled\n", __FUNCTION__);
        retCode = RECONN_INVALID_STATE;
    }
    else
    {
        reconnDebugPrint("%s: stat(%s) failed %d (%s) \n", __FUNCTION__, WIFI_ACTIVE_FILE_NAME, errno,
                strerror(errno));
        retCode = RECONN_FAILURE;
    }
    reconnDebugPrint("%s: Function exiting %s\n", __FUNCTION__, 
            (retCode == RECONN_SUCCESS) ? "SUCCESS" : "FAILURE");
    return(retCode);
}

//******************************************************************************
//******************************************************************************
// FUNCTION:        wifiGetState
//
// CLASSES:     
//
// DESCRIPTION: This interface returns the Wifi's state which was set vi 
//              wifiSetState().
//
// PARAMETERS:
//
//******************************************************************************
WIFIENUM wifiGetState()
{
    return (theWiFiState); 
}

//******************************************************************************
//******************************************************************************
// FUNCTION:        wifiSetState
//
// CLASSES:     
//
// DESCRIPTION: This interface is called to make changes to the enable disable
//              state of the wifi interface. The reconn device does not have the
//              hardware capability to power down the wifi chip itself. This interface
//              turns the interface up/down.
//
// PARAMETERS:
//              newState - WIFI_ENABLE/WIFI_DISABLE
//
//******************************************************************************
ReconnErrCodes wifiSetState(WIFIENUM newState)
{
    ReconnErrCodes retCode = RECONN_SUCCESS;
    FILE *tmpFd;
#ifndef __SIMULATION__
    WIFIENUM state;
    int systemRetCode;
#endif

    reconnDebugPrint("%s: Function entered with newState = %d, theWiFiState == %d\n", __FUNCTION__, newState, theWiFiState);
    /*
     * We are turning WIFI on
     */
    if(newState == WIFI_ENABLE)
    {
        if(theWiFiState == WIFI_DISABLE)
        {
#ifndef __SIMULATION__
            reconnDebugPrint("%s: Starting hostapd\n", __FUNCTION__);
            if(((systemRetCode = system("/etc/init.d/hostapd start")) == 0))
            {
                sleep(3);
                reconnDebugPrint("%s: Starting udhcpd\n", __FUNCTION__);
                if((systemRetCode = system("udhcpd /etc/udhcpd.conf")) == 0)
                {
                    state = WIFI_ENABLE;
                    if((tmpFd = fopen(WIFI_ACTIVE_FILE_NAME, "w")) == 0)
                    {
                        reconnDebugPrint("%s: fopen(%s, w) failed %d (%s)\n", __FUNCTION__, 
                                WIFI_ACTIVE_FILE_NAME, errno, strerror(errno));
                    }
                    else
                    {
                        fclose(tmpFd);
                        if((retCode = wifiStartConnectionTask()) == RECONN_SUCCESS)
                        {
                            theWiFiState = WIFI_ENABLE;
                        }
                    }
                }
                else
                {
                    reconnDebugPrint("%s: system(udhcpd /etc/udhcpd.conf) failed\n", __FUNCTION__);
                    retCode = RECONN_FAILURE;
                }
            }
            else
            {
                reconnDebugPrint("%s: system(/etc/init.d/hostapd start) failed\n", __FUNCTION__);
                retCode = RECONN_FAILURE;
            }
#else
            reconnDebugPrint("%s: Enabling Wifi\n", __FUNCTION__);
            if((tmpFd = fopen(WIFI_ACTIVE_FILE_NAME, "w")) == 0)
            {
                reconnDebugPrint("%s: fopen(%s, w) failed %d (%s)\n", __FUNCTION__,
                        WIFI_ACTIVE_FILE_NAME, errno, strerror(errno));
            }
            else
            {
                fclose(tmpFd);
                theWiFiState = WIFI_ENABLE;
                wifiStartConnectionTask();
            }
#endif
        }
    }
    /*
     * We are turning WIFI OFF.
     */
    else if(newState == WIFI_DISABLE)
    {
        if(theWiFiState == WIFI_ENABLE)
        {
            disconnectAllClients(SLAVEMODE);
#ifndef __SIMULATION__
            if((systemRetCode = system("/etc/init.d/hostapd stop")) != 0)
            {
                reconnDebugPrint("%s: system(/etc/init.d/hostapd stop) failed %d\n", __FUNCTION__, systemRetCode);
                retCode = RECONN_FAILURE;
            }
            else if((systemRetCode = system("killall udhcpd")) != 0)
            {
                /*
                 * No big deal if we can't kill udhcpc 'cause we are disabling WiFi anyway.
                 */
                reconnDebugPrint("%s: system(killall udhcpd %d) failed\n", __FUNCTION__, systemRetCode);
            }

            if((systemRetCode = unlink(WIFI_ACTIVE_FILE_NAME)) != 0)
            {
                reconnDebugPrint("%s: unlink(%s) failed %d\n", __FUNCTION__, WIFI_ACTIVE_FILE_NAME, systemRetCode);
                retCode = RECONN_FAILURE;
            }
            else
            {
#else
                reconnDebugPrint("%s: bringing down WIFI\n", __FUNCTION__);
#endif
                theWiFiState = newState;
                wifiCleanUp();
#ifndef __SIMULATION__
            }
#endif
        }
    }
    else
    {
        retCode = RECONN_INVALID_PARAMETER;
    }
    return(retCode);
}

//******************************************************************************
//******************************************************************************
// FUNCTION:    wifiGetMacAddress
//
// CLASSES:     
//
// DESCRIPTION: This interface is used to retrieve the wifi devices MAC address.
//
// PARAMETERS:
//              theMacAddrBuf -    A pointer to a buffer into which this interface will
//                              place the mac address. It is up the caller to make sure the 
//                              buffer is large enough.
//
//******************************************************************************
ReconnErrCodes wifiGetMacAddress(char * theMacAddrBuf)
{
    ReconnErrCodes retCode = RECONN_SUCCESS;
    int tmpSocket, i, index;
    struct ifreq buffer;

    if((tmpSocket = socket(PF_INET, SOCK_STREAM, 0)) != -1)
    {   
        strcpy (buffer.ifr_name, "wlan0");
        ioctl(tmpSocket, SIOCGIFHWADDR, &buffer);

        for(index = 0, i = 0; i < 6; i++)
        {   
            sprintf((char *)&theMacAddrBuf[index], "%.2X", (unsigned char)buffer.ifr_hwaddr.sa_data[i]);
            if(i < 5)
            {
                strcat(theMacAddrBuf, ":");
                index += 3;
            }
        }
        close(tmpSocket);
    }
    else
    {
        reconnDebugPrint("%s: socket(PF_INET, SOCK_STREAM, 0) failed %d (%s)\n", __FUNCTION__, errno, strerror(errno));
        retCode = RECONN_FAILURE;
    }
    return (retCode);
}

//******************************************************************************
//******************************************************************************
// FUNCTION:    wifiGetSSIDorPASSWD
//
// CLASSES:     
//
// DESCRIPTION: This interface is used to retrieve the SSID from the hostap.conf
//              file. The interface will return SSID as a null terminated string
//              minus any carriage return.
//
// PARAMETERS:
//              token -     The token to search for in the hostapd.conf file.
//
//              theSSID -   A pointer to a buffer into which this interface will
//                          place the SSID. It is up the caller to make sure the 
//                          buffer is large enough.
//
//******************************************************************************
ReconnErrCodes wifiGetSSIDorPASSWD(char *token, unsigned char *theSSID)
{
    ReconnErrCodes retCode = RECONN_SUCCESS;
    int count = 0;
    size_t size;
    char *theFileString=NULL;
    char *theSSIDorPASSWD;
    FILE *theFilePtr;


    reconnDebugPrint("%s: Function entered with token = %s\n", __FUNCTION__, token);

    if(theSSID == NULL)
    {
        retCode = RECONN_INVALID_PARAMETER;
    }
    else
    {
        if((theFilePtr = fopen(WIFI_CONF_FILE, "r")) == NULL)
        {
            printf("%s: fopen(%s) failed %d (%s)\n", __FUNCTION__, WIFI_CONF_FILE, errno, strerror(errno));
            retCode = RECONN_FAILURE;
        }
        else
        {
            while((count = getline(&theFileString, &size, theFilePtr)) > 0)
            {
                if(strncmp(theFileString, token, strlen(token)) == 0)
                {
                    theSSIDorPASSWD = &(theFileString[strlen(token)]);
                    strncpy((char *)theSSID, theSSIDorPASSWD ,strlen(theSSIDorPASSWD) - 1);
                    free((void *)theFileString);
                    break;
                }
                else
                {
                    free((void*)theFileString);
                    theFileString = NULL;
                }
            }
            fclose(theFilePtr);
        }
    }
    return(retCode);
}
//******************************************************************************
//******************************************************************************
// FUNCTION:        wifiUpdateConfFile
//
// CLASSES:     
//
// DESCRIPTION: This function is called to make changes to hostapd's configuration
//              file. This function searches the configuration file for "token"
//              and replaces the text following "token" with "theNewValue"
//
// PARAMETERS:
//              token       -   a string that is searched for in the configuration file
//              theNewValue -   the replacement string.
//
//******************************************************************************
ReconnErrCodes wifiUpdateHostapdConfFile(char *token, char *theNewValue)
{
    ReconnErrCodes retCode = RECONN_SUCCESS;
    FILE *theFile, *theOutFile;
    char *theFileString, *theNewString, *theCommand;
#ifndef __SIMULATION__
    size_t size = 90;
    int count;
#endif

    theFile = theOutFile = NULL;
    theFileString = theNewString = theCommand = NULL;

    reconnDebugPrint("%s: Function entered token = %s, the Newvalue = %s\n", __FUNCTION__, token, theNewValue);
#ifndef __SIMULATION__
    if((theOutFile = fopen(WIFI_CONF_FILE_NEW, "w")) == NULL)
    {
        reconnDebugPrint("%s: fopen(%s) failed %d (%s)\n", __FUNCTION__, WIFI_CONF_FILE_NEW, errno, strerror(errno));
        retCode = RECONN_FAILURE;
    }
    else if((theFile = fopen(WIFI_CONF_FILE, "r")) == NULL)
    {
        reconnDebugPrint("%s: fopen(%s) failed %d (%s)\n", __FUNCTION__, WIFI_CONF_FILE, errno, strerror(errno));
        fclose(theOutFile);
        retCode = RECONN_FAILURE;
    }
    else
    {
        if((theFileString = (char *)malloc(size+1)))
        {
            if((theNewString = (char *)malloc(size+1)))
            {
                // get enough memory to create a command similar to " mv <sourcefile>  <destfile> NULL"
                theCommand = (char *)malloc(strlen(WIFI_CONF_FILE) + strlen(WIFI_CONF_FILE_NEW) + 5);
                while((count = getline(&theFileString, &size, theFile)) > 0)
                {
                    if(strncmp(theFileString, token, strlen(token)) == 0)
                    {
                        strcat(theNewString, token);
                        strcat(theNewString, theNewValue);
                        strcat(theNewString, "\n");
                        fwrite(theNewString, 1, strlen(theNewString), theOutFile);
                    }
                    else
                    {
                        fwrite(theFileString, 1, strlen(theFileString), theOutFile);
                    }
                    memset(theFileString, 0, size+1);
                    memset(theNewString, 0, size+1);
                    memset(theCommand, 0, strlen(WIFI_CONF_FILE) + strlen(WIFI_CONF_FILE_NEW) + 5);
                }
                if(fclose(theFile) != 0)
                {
                    reconnDebugPrint("%s: fclose(%s) failed %d (%s)\n", __FUNCTION__, WIFI_CONF_FILE, errno, strerror(errno));
                }
                if(fclose(theOutFile) != 0)
                {
                    reconnDebugPrint("%s: fclose(%s) failed %d (%s)\n", __FUNCTION__, WIFI_CONF_FILE_NEW, errno, strerror(errno));
                }
                strcat(theCommand, "mv ");
                strcat(theCommand, WIFI_CONF_FILE_NEW);
                strcat(theCommand, " ");
                strcat(theCommand, WIFI_CONF_FILE);
                reconnDebugPrint("%s: calling system(%s)\n", __FUNCTION__, theCommand);
                system(theCommand);
                unlink(WIFI_CONF_FILE_NEW);
                free(theFileString);
                free(theNewString);
            }
            else
            {
                free(theFileString);
                reconnDebugPrint("%s: malloc(theNewString) failed %d (%s)\n", __FUNCTION__, errno, strerror(errno));
                retCode = RECONN_FAILURE;
            }
        }
        else
        {
            reconnDebugPrint("%s: malloc(theFileString) failed %d (%s)\n", __FUNCTION__, errno, strerror(errno));
            retCode = RECONN_FAILURE;
        }
    }
#endif
    return(retCode);
}
//******************************************************************************
//******************************************************************************
// FUNCTION:        wifiConnectionTask
//
// CLASSES:     
//
// DESCRIPTION: This task is resposible for accepting connections to reconn via
//              the wifi interface. The task is started and stopped by commands
//              in clientApp.c
//
// PARAMETERS:
//
//******************************************************************************

/* Main While Loop */
void * wifiConnectionTask(void *args)
{
    struct sockaddr_in server_addr, clientAddr;
    static int taskState = 1;
    int optval = 1;
    unsigned int clientLen;
    short index;

    UNUSED_PARAM(args);
    reconnDebugPrint("%s: ****** started taskDone = %d wifiSocketFd = %d\n", __FUNCTION__, taskDone, wifiSocketFd);
    /* Create the incoming (server) socket */
    if((wifiSocketFd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        reconnDebugPrint("%s: Failed to initialize the incoming socket %d(%s)\n", __FUNCTION__, errno, strerror(errno));
        wifiSocketFd = -1;
        return (0);
    }
    if(setsockopt(wifiSocketFd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
    {
        reconnDebugPrint("%s: setsockopt SO_REUSEADDR failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
    }

    bzero((unsigned char *) &server_addr, sizeof(server_addr));
    /* bind the socket */
    server_addr.sin_family = AF_INET;
#ifndef __SIMULATION__
    server_addr.sin_addr.s_addr = inet_addr("192.168.0.50");
#else
    server_addr.sin_addr.s_addr = INADDR_ANY;
#endif
    server_addr.sin_port = htons(RECONN_INCOMING_PORT);

    reconnDebugPrint("%s: binding to socket %d\n", __FUNCTION__, wifiSocketFd);
    if (bind(wifiSocketFd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)
    {
        reconnDebugPrint("%s: Failed to bind the socket %d(%s)\n", __FUNCTION__, errno, strerror(errno));
        wifiSocketFd = -1;
        return (0);
    } 
    while (taskDone == FALSE)
    {
        /* pend on the incoming socket */
        if(listen(wifiSocketFd, (RECONN_MAX_NUM_CLIENTS - 1)) == 0)
        {

            clientLen = sizeof(clientAddr);

            /* sit here and wait for a new connection request */
            if((gNewSocketFd = accept(wifiSocketFd, (struct sockaddr *) &clientAddr,
                            &clientLen)) < 0)
            {
                reconnDebugPrint("%s: Failed to accept a new Client socket %d %s.\n", __FUNCTION__, errno , strerror(errno));
                /* place code here to recover from bad socket accept */
                continue;
                //return (1);
            }
            if (gNewSocketFd != 0)
            {
                index = -1;
                if(reconnGetFreeClientIndex(&index) == RECONN_SUCCESS)
                {
                    CLIENTCONTEXT *mem;
                    if((mem = malloc(sizeof(CLIENTCONTEXT))) != NULL)
                    {
                        mem->thisContext = (int *)mem;
                        mem->socketFd = gNewSocketFd;
                        mem->eqptDescriptors = &gEqptDescriptors;
                        mem->index = index;
                        mem->mode = SLAVEMODE;
                        mem->sourceIp = clientAddr;
                        mem->tmpFd = (FILE *)-1;
#ifdef DEBUG_CONNECT
                        reconnDebugPrint("%s: gNewSocketFd = %d\r\n", __FUNCTION__, gNewSocketFd);
                        reconnDebugPrint("%s: Starting reconnClient index= %u \n", __FUNCTION__, index);
#endif
                        if(pthread_create(&(reconnThreadIds[RECONN_NUM_SYS_TASKS + index]), NULL, reconnClientTask,
                                    (void *)mem) < 0)
                        {
                            reconnDebugPrint("%s: Could not start reconnClient, %d %s\n", __FUNCTION__, errno, strerror(errno));
                            continue;
                        }
                    }
                    else
                    {
                        close(gNewSocketFd);
                        reconnDebugPrint("%s: malloc failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
                    }
                }
                else
                {
                    close(gNewSocketFd);
                    reconnDebugPrint("%s: Max clients reached \n", __FUNCTION__);
                }

            }
            else
            {
                reconnDebugPrint("%s: gNewSocketFd =%d \n", __FUNCTION__, gNewSocketFd);
            }
        }
        else
        {
            if(shutdown(wifiSocketFd, 2) != 0)
            {
                reconnDebugPrint("%s: shutdown(%d, 2) failed %d (%s)\n", __FUNCTION__, wifiSocketFd, errno,
                        strerror(errno));
            }

            if(close(wifiSocketFd) != 0)
            {
                reconnDebugPrint("%s: close failed  = %d(%s) \n", __FUNCTION__, errno, strerror(errno));
            }
            wifiSocketFd = -1;
            reconnDebugPrint("%s: listen returned  = %d(%s) \n", __FUNCTION__, errno, strerror(errno));
            break;
        }
    }
    if(wifiSocketFd != -1)
    {
        if(close(wifiSocketFd) != 0)
        {
            reconnDebugPrint("%s: close returned  =%d(%s) \n", __FUNCTION__, errno, strerror(errno));
        }
        wifiSocketFd = -1;
    }
    if(reconnThreadIds[RECONN_WIFI_CONNECTION_TASK]) 
    {
        reconnThreadIds[RECONN_WIFI_CONNECTION_TASK] = -1;
    }
    reconnDebugPrint("%s: *** Task exiting\n", __FUNCTION__);
    return(&taskState);
}
#if 0
//******************************************************************************
//******************************************************************************
// FUNCTION:        wifiRdWrConfigFile
//
// CLASSES:     
//
// DESCRIPTION: This interface either reads/writes the wifi config state to/from
//              the wifi config file.
//              the wifi interface. 
//
// PARAMETERS:  theAction - either read or write the wifi config file.
//              theState - a buffer into which this interface will place the wifi state
//                      as read from the configuration file.
//******************************************************************************
ReconnErrCodes wifiRdWrConfigFile(WIFIENUM theAction, WIFIENUM *theState)
{
    FILE *theFile;
    short amount;
    WIFIENUM theConfigState = -1;
    ReconnErrCodes retCode = RECONN_SUCCESS;

    if(theAction == WIFI_READ)
    {
        if((theFile = fopen(WIFI_CONFIG_FILE, "r")) != NULL)
        {
            if((amount = fread(&theConfigState, 1, sizeof(theConfigState), theFile)) != sizeof(theConfigState))
            {
                reconnDebugPrint("%s: %s possibly corrupt\n", __FUNCTION__, WIFI_CONFIG_FILE);
                retCode =  RECONN_FILE_NOT_FOUND;
            }
            else
            {
                *theState = theConfigState;
            }
            fclose(theFile);
        }
        else
        {
            reconnDebugPrint("%s: failed to open %s %d(%s)\n", __FUNCTION__, WIFI_CONFIG_FILE, errno, strerror(errno));
            retCode = RECONN_FILE_NOT_FOUND;
        }
    }
    else if(theAction == WIFI_WRITE)
    {
        if((theFile = fopen(WIFI_CONFIG_FILE, "w")) != NULL)
        {
            if((amount = fwrite(theState, 1, sizeof(theConfigState), theFile)) != sizeof(theConfigState))
            
            { 
                reconnDebugPrint("%s:fwrite(%s, w) failed %d(%s)\n", __FUNCTION__, WIFI_CONFIG_FILE, 
                        errno, strerror(errno));
                retCode = RECONN_FAILURE;
            }
            fclose(theFile);
        }
        else
        {
            reconnDebugPrint("%s:fopen(%s, w) failed %d(%s)\n", __FUNCTION__, WIFI_CONFIG_FILE, 
                    errno, strerror(errno));
                retCode = RECONN_FAILURE;
        }
    }
    else
    {
        reconnDebugPrint("%s: invalid parameter %d\n", theAction);
        retCode = RECONN_INVALID_PARAMETER;
    }
    return(retCode);
}
#endif
