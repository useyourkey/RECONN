//******************************************************************************
//******************************************************************************
//
// FILE:        reconnApp.c
//
// CLASSES:     
//
// DESCRIPTION: This is the file which contains the main function for the 
//              reconn application
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
//******************************************************************************

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sched.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <errno.h>

#include "reconn.h"
#include "gps.h"
#include "powerMeter.h"
#include "spectrum.h"
#include "dmm.h"
#include "socket.h"
#include "clientApp.h"
#include "powerMgmt.h"
#include "eqptResponse.h"
#include "gpio.h"
#include "debugMenu.h"
#include "upgrade.h"

#define COMM_DEBUG

int gNewSocketFd; 

int gpsEnabled = FALSE;
int powerMeterEnabled = FALSE;
int analyzerEnabled = FALSE;
int dmmEnabled = FALSE;

// Reconn System tasks
typedef enum
{
    RECONN_EQPT_TASK,
    RECONN_PWR_MGMT_TASK,
    RECONN_PWR_BUTTON_TASK,
    RECONN_BATTERY_MONITOR_TASK,
    RECONN_DEBUG_MENU_TASK,
    RECONN_UPGRADE_CHECK_TASK,
    RECONN_NUM_SYS_TASKS
}RECONN_TASKS;
static pthread_t reconnThreadIds[RECONN_MAX_NUM_CLIENTS + RECONN_NUM_SYS_TASKS];
static int  numberOfActiveClients = 0;
static int in_socket_fd;



static void reconnCleanUp()
{
    printf("%s:****** Task Called\n", __FUNCTION__);
    printf("%s:****** closing socket\n", __FUNCTION__);
    close(in_socket_fd);
    exit(0);
}
static void *upgradeCheckTask(void *args)
{
    printf("%s:****** Task Started\n", __FUNCTION__);
    sleep(20);

    system("killall powerLedFlash");
    reconnGpioAction(POWER_LED_GPIO, ENABLE);
    printf("%s:****** Removing %s\n", __FUNCTION__, UPGRADE_INPROGRESS_FILE_NAME);
    unlink(UPGRADE_INPROGRESS_FILE_NAME);
    unlink(UPGRADE_BUNDLE_NAME);
    return(0);
}

static void PeripheralInit(ReconnModeAndEqptDescriptors *modeEqptDescriptors) 
{
    int status = RECONN_SUCCESS;

    /* initialize the GPS - (GPS_SERIAL_PORT) */
    if((status = gpsInit(&(modeEqptDescriptors->gpsFd))) != RECONN_SUCCESS)
    {
        printf("%s: GPS Init Failed\n", __FUNCTION__);
        gpsEnabled = FALSE;
    } 
    else
    {
        printf("GPS Initialized\n");
    }
    /* end GPS Init - GPS now configured */

#if 0
    if((status = SpectrumAnalyzerInit(&(modeEqptDescriptors->analyzerFd))) != RECONN_SUCCESS) 
    {
        printf("Spectrum Analyzer Init Failed\n");
        analyzerEnabled = FALSE;
    }
    else 
    {
        printf("Spectrum Analyzer Initialized\n");
    }

#endif
    /* initialize the power meter */
    if ((status = powerMeterInit(&(modeEqptDescriptors->powerMeterFd))) != RECONN_SUCCESS)
    {
        printf("Power Meter Init Failed\n");
        powerMeterEnabled = FALSE;
    } 
    else 
    {
        printf("Power Meter Initialized\n");
    }
    if((status = dmmInit(&(modeEqptDescriptors->dmmFd))) != RECONN_SUCCESS)
    {
        printf("%s: DMM Init Failed\n", __FUNCTION__);
        dmmEnabled = FALSE;
    }
    else
    {
        printf("%s: DMM Initialized\n", __FUNCTION__);
    }
    // Power up the Wifi chip
    if((status = reconnGpioAction(GPIO_57, ENABLE)) == RECONN_FAILURE)
    {
        printf("%s: Could not power up WiFi device \n", __FUNCTION__);
    }
    else
    {   
        printf("%s: Wifi powered\n", __FUNCTION__);
    }
}

static short reconnGetFreeClientIndex()
{
    short retCode = -1;

    if(numberOfActiveClients < RECONN_MAX_NUM_CLIENTS)
    {
        retCode = numberOfActiveClients;
        numberOfActiveClients++;
    }
    printf("%s: returning %d\n", __FUNCTION__, retCode);
    return retCode;
}

void reconnReturnClientIndex(short index)
{
    if((index >= 0) && (index < RECONN_MAX_NUM_CLIENTS))
    {
        reconnThreadIds[RECONN_NUM_SYS_TASKS + index] = -1;
        numberOfActiveClients--;
    }
    else
    {
        printf("%s: index (%d) is out of range\n", __FUNCTION__, index);
    }
}

#ifndef __SIMULATION__
extern void initReconnCrashHandlers(void);
#endif

int main(int argc, char **argv) 
{
#if 0
    int in_socket_fd; /* Incoming socket file descriptor for socket 1068     */
#endif
    int intport = 0;
    ReconnClientIndex index;
    socklen_t client_len;
    struct sockaddr_in server_addr, client_addr;
    ReconnModeAndEqptDescriptors modeAndEqptDescriptors;
    struct stat statInfo;
    struct sigaction act;
    pthread_attr_t attr;
    int optval = 1;

    memset(&modeAndEqptDescriptors, 0, sizeof(ReconnModeAndEqptDescriptors));

#ifndef __SIMULATION__
    initReconnCrashHandlers();
#endif
    memset(&act, 0, sizeof(act));
    act.sa_flags = SA_SIGINFO;
    act.sa_sigaction = reconnCleanUp;
    sigaction(SIGTERM, &act, NULL);


    // TODO remove this assignent when LNB is ready.
    modeAndEqptDescriptors.lnbFd = -1;

    PeripheralInit(&modeAndEqptDescriptors);

#ifdef DEBUG_CONNECT
    printf("%s: modeAndEqptDescriptors->GpsFd = %d\n", __FUNCTION__, modeAndEqptDescriptors.gpsFd);
    printf("      modeAndEqptDescriptors->PowerMeterFd = %d\n", modeAndEqptDescriptors.powerMeterFd);
    printf("      modeAndEqptDescriptors->LnbFd = %d\n", modeAndEqptDescriptors.lnbFd);
    printf("      modeAndEqptDescriptors->DmmFd = %d\n", modeAndEqptDescriptors.dmmFd);
    printf("      modeAndEqptDescriptors->AnalyzerFd = %d\n", modeAndEqptDescriptors.analyzerFd);
#endif

    /* Create the incoming (server) socket */
    if((in_socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    {
        printf("%s: Server Failed to initialize the incoming socket %d(%s)\n", __FUNCTION__, errno, strerror(errno));
        exit (0);
    }

    if(setsockopt(in_socket_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
    {
        printf("%s: setsockopt failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
    }

    bzero((unsigned char *) &server_addr, sizeof(server_addr));
    /* bind the socket */
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    intport = RECONN_INCOMING_PORT;
    server_addr.sin_port = htons(intport);

    printf("%s: binding to socket\n", __FUNCTION__);
    if (bind(in_socket_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) 
    {
        printf("%s: Server Failed to bind the socket %d(%s)\n", __FUNCTION__, errno, strerror(errno));
        exit (0);
    }
    // If there is an upgrade in progress then start a task that will sleep for 5 minutes then run.
    // If the task does run, then the upgraded application has not crashed and do the new image is OK.
    if(stat(UPGRADE_INPROGRESS_FILE_NAME, &statInfo) == 0)
    {
        if( pthread_create(&(reconnThreadIds[RECONN_UPGRADE_CHECK_TASK]), NULL, upgradeCheckTask, (void *)0) < 0)
        {
            printf("%s: Could not start reconnEqptTask %d %s\n", __FUNCTION__, errno, strerror(errno));
        }
    }

    /*  Start up the command processing */
    if( pthread_create(&(reconnThreadIds[RECONN_EQPT_TASK]), NULL, reconnEqptTask, (void *)0) < 0)
    {
        printf("%s: Could not start reconnEqptTask %d %s\n", __FUNCTION__, errno, strerror(errno));
    }
    else
    {
        if(pthread_create(&(reconnThreadIds[RECONN_PWR_MGMT_TASK]), NULL, reconnPwrMgmtTask, (void *)0) < 0)
        {
            printf("%s: Could not start reconnPwrMgmtTask, %d %s\n", __FUNCTION__, errno, strerror(errno));
        }
        else if(pthread_create(&(reconnThreadIds[RECONN_PWR_BUTTON_TASK]), NULL, reconnPwrButtonTask, 
                    (void *)0) < 0)
        {
            printf("%s: Could not start reconnPwrMgmtTask, %d %s\n", __FUNCTION__, errno, strerror(errno));
        }
        else if(pthread_create(&(reconnThreadIds[RECONN_BATTERY_MONITOR_TASK]), NULL, reconnBatteryMonTask, (void *) 0 ) < 0)
        {
            printf("%s: Could not start reconnBatteryMonTask, %d %s\n", __FUNCTION__, errno, strerror(errno));
        }
        else if(pthread_create(&(reconnThreadIds[RECONN_DEBUG_MENU_TASK]), NULL, debugMenuTask, (void *) 0 ) < 0)
        {
            printf("%s: Could not start debugMenuTask,, %d %s\n", __FUNCTION__, errno, strerror(errno));
        }
        else
        {
            /* Main While Loop */
            while (TRUE) 
            {
                /* pend on the incoming socket */
                if(listen(in_socket_fd, 5) == 0)
                {

                    client_len = sizeof(client_addr);

                    /* sit here and wait for a new connection request */
                    if((gNewSocketFd = accept(in_socket_fd, (struct sockaddr *) &client_addr,
                                    &client_len)) < 0)
                    {
                        printf("%s: Failed to open a new Client socket %d %s.\n", __FUNCTION__, errno , strerror(errno));
                        /* place code here to recover from bad socket accept */
                        continue;
                        //exit (1);
                    }
                    if (gNewSocketFd != 0) 
                    {
                        index = reconnGetFreeClientIndex();
                        modeAndEqptDescriptors.clientIndex = index;
                        modeAndEqptDescriptors.clientMode = (index == MASTERMODE) ? MASTERMODE : CLIENTMODE;
#ifdef DEBUG_CONNECT
                        printf("%s: gNewSocketFd = %d\r\n", __FUNCTION__, gNewSocketFd);
                        printf("%s: Starting reconnClient index= %u \n", __FUNCTION__, index);
#endif
                        if(pthread_create(&(reconnThreadIds[RECONN_NUM_SYS_TASKS + index]), NULL, reconnClientTask, 
                                    (void *)&modeAndEqptDescriptors) < 0)
                        {
                            printf("%s: Could not start reconnClient, %d %s\n", __FUNCTION__, errno, strerror(errno));
                            continue;
                        }
                    }
                    else
                    {
                        printf("%s: gNewSocketFd =%d \n", __FUNCTION__, gNewSocketFd);
                    }
                }
                else
                {
                    printf("%s: listen returned  =%d(%s) \n", __FUNCTION__, errno, strerror(errno));
                    //close(in_socket_fd);
                    break;
                }
            }
        }
    }
    printf("%s: Exiting *******************\n",__FUNCTION__);
    exit (0);
}
