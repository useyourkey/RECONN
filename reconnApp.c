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
#include <mqueue.h>
#include <errno.h>
#include <linux/if.h>

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
#include "fuel_gauge.h"

#define COMM_DEBUG

int gNewSocketFd; 
int fromLibToClientfd;

int gpsEnabled = FALSE;
int powerMeterEnabled = FALSE;
int analyzerEnabled = FALSE;
int dmmEnabled = FALSE;

static pthread_t reconnThreadIds[RECONN_MAX_NUM_CLIENTS + RECONN_NUM_SYS_TASKS];
static int  numberOfActiveClients = 0;
static int in_socket_fd;
ReconnModeAndEqptDescriptors modeAndEqptDescriptors;

extern mqd_t masterClientMsgQid;
static struct mq_attr masterClientMsgQAttr;
static unsigned char theMessage[INSERTED_MASTER_MSG_SIZE];

extern int masterClientSocketFd;
extern void insertedMasterRead();
extern int insertedMasterSocketFd;
extern void *insertedMasterTransmitTask();
extern void registerClientDebugMenu();
extern void registerFuelGaugeDebugMenu();
extern void registerSystemDebugMenu();
extern void registerSocketDebugMenu();

static void reconnCleanUp()
{
    reconnDebugPrint("%s:****** Function Called\n", __FUNCTION__);
    reconnDebugPrint("%s:****** closing in_socket_fd\n", __FUNCTION__, in_socket_fd);
    close(in_socket_fd);
    reconnDebugPrint("%s:****** closing fromLibToClientfd\n", __FUNCTION__, in_socket_fd);
    close(fromLibToClientfd);
    exit(0);
}
static void *upgradeCheckTask(void *args)
{
    reconnDebugPrint("%s:****** Task Started\n", __FUNCTION__);
    sleep(20);

    UNUSED_PARAM(args);
    system("killall powerLedFlash");
    reconnGpioAction(POWER_LED_GPIO, ENABLE, NULL);
    reconnDebugPrint("%s:****** Removing %s\n", __FUNCTION__, UPGRADE_INPROGRESS_FILE_NAME);
    unlink(UPGRADE_INPROGRESS_FILE_NAME);
    unlink(UPGRADE_BUNDLE_NAME);
    return(0);
}

static void PeripheralInit(ReconnModeAndEqptDescriptors *modeEqptDescriptors) 
{
    int status = RECONN_SUCCESS;

#if 0 // GPS has been removed from the reconn box
    if((status = gpsInit(&(modeEqptDescriptors->gpsFd))) != RECONN_SUCCESS)
    {
        reconnDebugPrint("%s: GPS Init Failed\n", __FUNCTION__);
        gpsEnabled = FALSE;
    } 
    else
    {
        reconnDebugPrint("GPS Initialized\n");
    }
    /* end GPS Init - GPS now configured */
#endif

    if((status = SpectrumAnalyzerInit(&(modeEqptDescriptors->analyzerFd))) != RECONN_SUCCESS) 
    {
        reconnDebugPrint("Spectrum Analyzer Init Failed\n");
        analyzerEnabled = FALSE;
    }
    else 
    {
        reconnDebugPrint("Spectrum Analyzer Initialized\n");
    }

    /* initialize the power meter */
    if ((status = powerMeterInit(&(modeEqptDescriptors->powerMeterFd))) != RECONN_SUCCESS)
    {
        reconnDebugPrint("Power Meter Init Failed\n");
        powerMeterEnabled = FALSE;
    } 
    else 
    {
        reconnDebugPrint("Power Meter Initialized\n");
    }
    if((status = dmmInit(&(modeEqptDescriptors->dmmFd))) != RECONN_SUCCESS)
    {
        reconnDebugPrint("%s: DMM Init Failed\n", __FUNCTION__);
        dmmEnabled = FALSE;
    }
    else
    {
        reconnDebugPrint("%s: DMM Initialized\n", __FUNCTION__);
    }
    // Power up the Wifi chip
    if((status = reconnGpioAction(GPIO_57, ENABLE, NULL)) == RECONN_FAILURE)
    {
        reconnDebugPrint("%s: Could not power up WiFi device \n", __FUNCTION__);
    }
    else
    {   
        reconnDebugPrint("%s: Wifi powered\n", __FUNCTION__);
    }
}

static short reconnGetFreeClientIndex()
{
    short retCode = -1;
    int i;

    if(numberOfActiveClients < RECONN_MAX_NUM_CLIENTS)
    {
        // The 0th element is reserved for the inserted iphone master
        for (i = 1; i < RECONN_MAX_NUM_CLIENTS; i++)
        {
            if(reconnThreadIds[RECONN_NUM_SYS_TASKS + i] == (pthread_t)-1)
                break;
        }
        retCode = i;
        numberOfActiveClients++;
    }
    else
    {
        reconnDebugPrint("%s: numberOfActiveClients ==  %d\n", __FUNCTION__, numberOfActiveClients);
    }
    reconnDebugPrint("%s: returning %d\n", __FUNCTION__, retCode);
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
        reconnDebugPrint("%s: index (%d) is out of range\n", __FUNCTION__, index);
    }
}

#ifndef __SIMULATION__
extern void initReconnCrashHandlers(void);
#endif

//******************************************************************************
//****************************************************************************** //
// FUNCTION:    reconnMasterIphone
//
// DESCRIPTION: This function is registered with 
//              libiphoned_register_presence_change_callback() and is invoked
//              when the iphone library functions detect the removal or insertion 
//              of an iphone in the front panel of the reconn unit.
//
//******************************************************************************
void reconnMasterIphone()
{
    pthread_t task;
    struct timespec wait_time;
    struct sockaddr_in masterTransmitAddr;
    static int iphoneInserted = FALSE;
#ifdef __SIMULATION__
    int simulate_isiphonepresent();
#endif

    reconnDebugPrint("%s: Function Entered\n", __FUNCTION__);
#ifdef __SIMULATION__
    if(simulate_isiphonepresent() == 0) 
#else
    if(libiphoned_isiphonepresent() == 0) 
#endif
    {
        reconnDebugPrint("%s: Function Entered iphone INSERTED\n", __FUNCTION__);
        if (masterClientSocketFd != -1)
        {
            reconnDebugPrint("%s: Master WiFi client is present\n", __FUNCTION__);
            // This function will usurp mastership from the current
            // master iPhone. The physically inserted iPhone is ALWAYS
            // the master.
            
            // send current master a message telling it to give up control
            
            // Then start a clientApp() as master.
            memset( &theMessage, 0, INSERTED_MASTER_MSG_SIZE);
            theMessage[0] = MASTER_INSERTED;
            if(mq_send(masterClientMsgQid, (const char *)&theMessage, INSERTED_MASTER_MSG_SIZE, 0) != 0)
            {
                reconnDebugPrint("%s: mq_send(masterClientMsgQid) failed. %d(%s)\n", __FUNCTION__, errno, strerror(errno));
            }
            else
            {
                reconnDebugPrint("%s: mq_send(masterClientMsgQid) success.\n", __FUNCTION__);
            }
            
            clock_gettime(CLOCK_REALTIME, &wait_time);
            wait_time.tv_sec += 1;
            wait_time.tv_nsec = 0;
#if 0
            if(mq_timedreceive(masterClientMsgQid, (char *) &theMessage, sizeof(theMessage), 0, &wait_time) != 0)
            {
                reconnDebugPrint("%s: mq_timedreceive failed %d(%s).\r\n", __FUNCTION__, errno, strerror(errno));
                return(0);
            }
#endif
        }
        else
        {
            reconnDebugPrint("%s: ***NO Master WiFi client\n", __FUNCTION__);
        }

        if((fromLibToClientfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            reconnDebugPrint("%s: socket failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
        }
        else
        {
            bzero((unsigned char *) &masterTransmitAddr, sizeof(masterTransmitAddr));

            masterTransmitAddr.sin_family = AF_INET;
            masterTransmitAddr.sin_addr.s_addr = INADDR_ANY;
            masterTransmitAddr.sin_port = htons(RECONN_INCOMING_PORT+2);

            if(connect(fromLibToClientfd, (struct sockaddr *)&masterTransmitAddr, sizeof(masterTransmitAddr)) < 0)
            {
                reconnDebugPrint("%s: connect failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
            }
        }

        gNewSocketFd = insertedMasterSocketFd;
        // The inserted master is always client number 0
        modeAndEqptDescriptors.clientIndex = 0;
        modeAndEqptDescriptors.clientMode = INSERTEDMASTERMODE;
#ifdef DEBUG_CONNECT
        reconnDebugPrint("%s: gNewSocketFd = %d\r\n", __FUNCTION__, gNewSocketFd);
        reconnDebugPrint("%s: Starting reconnClient index= %u \n", __FUNCTION__, modeAndEqptDescriptors.clientIndex);
#endif
        if(pthread_create(&task, NULL, reconnClientTask, (void *)&modeAndEqptDescriptors) < 0)
        {
            reconnDebugPrint("%s: Could not start reconnClient, %d %s\n", __FUNCTION__, errno, strerror(errno));
        }
        iphoneInserted = TRUE;
    }
    else if(iphoneInserted == TRUE)
    {
        reconnDebugPrint("%s: Function Entered iphone EXTRACTED\n", __FUNCTION__);

        memset( &theMessage, 0, INSERTED_MASTER_MSG_SIZE);
        theMessage[0] = MASTER_EXTRACTED;
        printf("%s: masterClientMsgQid == %d\n", __FUNCTION__, masterClientMsgQid);
        if(mq_send(masterClientMsgQid, (const char *)&theMessage, INSERTED_MASTER_MSG_SIZE, 0) != 0)
        {
            reconnDebugPrint("%s: mq_send(masterClientMsgQid) failed. %d(%s)\n", __FUNCTION__, errno, strerror(errno));
        }
        else
        {
            reconnDebugPrint("%s: mq_send(masterClientMsgQid) success.\n", __FUNCTION__);
        }
    }
}




int main(int argc, char **argv) 
{
#if 0
    int in_socket_fd; /* Incoming socket file descriptor for socket 1068     */
#endif
    int intport = 0, i;
    ReconnClientIndex index;
    socklen_t client_len;
    struct sockaddr_in server_addr, client_addr;
    struct stat statInfo;
    struct sigaction act;
    struct ifreq ifr;
    int optval = 1;

    UNUSED_PARAM(argc);
    UNUSED_PARAM(argv);
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

    i = sizeof(reconnThreadIds);
    for(i = 0; i < RECONN_MAX_NUM_CLIENTS + RECONN_NUM_SYS_TASKS; i++)
    {
        reconnThreadIds[i] = -1;
    }

#ifndef __SIMULATION__
    //
    // Need to register with libiphoned. This callback will signal when
    // the iphone has been inserted into the reconn toolkit.
    //
    libiphoned_register_presence_change_callback(reconnMasterIphone);
    libiphoned_register_rx_callback(insertedMasterRead);

    //
    // Now start libiphoned. This is a daemon which does all of the "heavy lifting" for the 
    // front panel iPhone. The daemon does the reading, writing, insertion, and iPhone extraction.
    //
    reconnDebugPrint("%s: Calling libiphoned_start()\n", __FUNCTION__);
    if(libiphoned_start() == -1)
    {
        reconnDebugPrint("%s: libiphoned_start() failed\n", __FUNCTION__);
        exit(0);
    }
#endif
    
    /* Create the incoming (server) socket */
    if((in_socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    {
        reconnDebugPrint("%s: Server Failed to initialize the incoming socket %d(%s)\n", __FUNCTION__, errno, strerror(errno));
        exit (0);
    }

    memset(&ifr, 9, sizeof(ifr));
    snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "wlan0");
#ifndef __SIMULATION__
    if(setsockopt(in_socket_fd, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr)) < 0)
    {
        reconnDebugPrint("%s: setsockopt SO_BINDTODEVICE failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
        exit(0);
    }
#endif
    if(setsockopt(in_socket_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
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
    intport = RECONN_INCOMING_PORT;
    server_addr.sin_port = htons(intport);

    reconnDebugPrint("%s: binding to socket\n", __FUNCTION__);
    if (bind(in_socket_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) 
    {
        reconnDebugPrint("reconnApp: Server Failed to bind the socket %d(%s)\n",  errno, strerror(errno));
        exit (0);
    }

    // If there is an upgrade in progress then start a task that will sleep for 5 minutes then run.
    // If the task does run, then the upgraded application has not crashed and so the new image 
    // is OK.
    if(stat(UPGRADE_INPROGRESS_FILE_NAME, &statInfo) == 0)
    {
        if( pthread_create(&(reconnThreadIds[RECONN_UPGRADE_CHECK_TASK]), NULL, upgradeCheckTask, (void *)0) < 0)
        {
            reconnDebugPrint("%s: Could not start reconnEqptTask %d %s\n", __FUNCTION__, errno, strerror(errno));
        }
    }
    PeripheralInit(&modeAndEqptDescriptors);
#ifdef DEBUG_CONNECT
    reconnDebugPrint("%s: modeAndEqptDescriptors->GpsFd = %d\n", __FUNCTION__, modeAndEqptDescriptors.gpsFd);
    reconnDebugPrint("      modeAndEqptDescriptors->PowerMeterFd = %d\n", modeAndEqptDescriptors.powerMeterFd);
    reconnDebugPrint("      modeAndEqptDescriptors->LnbFd = %d\n", modeAndEqptDescriptors.lnbFd);
    reconnDebugPrint("      modeAndEqptDescriptors->DmmFd = %d\n", modeAndEqptDescriptors.dmmFd);
    reconnDebugPrint("      modeAndEqptDescriptors->AnalyzerFd = %d\n", modeAndEqptDescriptors.analyzerFd);
#endif
    dmmDiags();
    //
    // Startup debug menus
    //
    registerClientDebugMenu();
    registerFuelGaugeDebugMenu();
    registerSystemDebugMenu();
    registerDmmDebugMenu();
    registerSocketDebugMenu();

    if(pthread_create(&(reconnThreadIds[RECONN_MASTER_SOCKET_TASK]), NULL, insertedMasterTransmitTask, (void *)0) < 0)
    {
        reconnDebugPrint("%s: Could not start openInsertedMasterSocket %d %s\n", __FUNCTION__, errno, strerror(errno));
    }


    /*  Start up the command processing */
    if( pthread_create(&(reconnThreadIds[RECONN_EQPT_TASK]), NULL, reconnEqptTask, (void *)0) < 0)
    {
        reconnDebugPrint("%s: Could not start reconnEqptTask %d %s\n", __FUNCTION__, errno, strerror(errno));
    }
    else
    {
        if(pthread_create(&(reconnThreadIds[RECONN_PWR_MGMT_TASK]), NULL, reconnPwrMgmtTask, (void *)0) < 0)
        {
            reconnDebugPrint("%s: Could not start reconnPwrMgmtTask, %d %s\n", __FUNCTION__, errno, strerror(errno));
        }
#if 0
        else if(pthread_create(&(reconnThreadIds[RECONN_PWR_BUTTON_TASK]), NULL, reconnPwrButtonTask, 
                    (void *)0) < 0)
        {
            reconnDebugPrint("%s: Could not start reconnPwrMgmtTask, %d %s\n", __FUNCTION__, errno, strerror(errno));
        }
#endif
        else if(pthread_create(&(reconnThreadIds[RECONN_BATTERY_MONITOR_TASK]), NULL, reconnBatteryMonTask, (void *) 0 ) < 0)
        {
            reconnDebugPrint("%s: Could not start reconnBatteryMonTask, %d %s\n", __FUNCTION__, errno, strerror(errno));
        }
        else if(pthread_create(&(reconnThreadIds[RECONN_DEBUG_MENU_TASK]), NULL, debugMenuTask, (void *) 0 ) < 0)
        {
            reconnDebugPrint("%s: Could not start debugMenuTask,, %d %s\n", __FUNCTION__, errno, strerror(errno));
        }
        else if(pthread_create(&(reconnThreadIds[RECONN_POWER_METER_TASK]), NULL, powerMeterPresenceTask, (void *) 0 ) < 0)
        {
            reconnDebugPrint("%s: Could not start debugMenuTask,, %d %s\n", __FUNCTION__, errno, strerror(errno));
        }
        else
        {
            /* Main While Loop */
            while (TRUE) 
            {
                /* pend on the incoming socket */
                if(listen(in_socket_fd, (RECONN_MAX_NUM_CLIENTS - 1)) == 0)
                {

                    client_len = sizeof(client_addr);

                    /* sit here and wait for a new connection request */
                    if((gNewSocketFd = accept(in_socket_fd, (struct sockaddr *) &client_addr,
                                    &client_len)) < 0)
                    {
                        reconnDebugPrint("%s: Failed to open a new Client socket %d %s.\n", __FUNCTION__, errno , strerror(errno));
                        /* place code here to recover from bad socket accept */
                        continue;
                        //exit (1);
                    }
                    if (gNewSocketFd != 0) 
                    {
                        if((index = reconnGetFreeClientIndex()) != -1)
                        {
                            modeAndEqptDescriptors.clientIndex = index;
                            modeAndEqptDescriptors.clientMode = INITMODE;
#ifdef DEBUG_CONNECT
                            reconnDebugPrint("%s: gNewSocketFd = %d\r\n", __FUNCTION__, gNewSocketFd);
                            reconnDebugPrint("%s: Starting reconnClient index= %u \n", __FUNCTION__, index);
#endif
                            if(pthread_create(&(reconnThreadIds[RECONN_NUM_SYS_TASKS + index]), NULL, reconnClientTask, 
                                        (void *)&modeAndEqptDescriptors) < 0)
                            {
                                reconnDebugPrint("%s: Could not start reconnClient, %d %s\n", __FUNCTION__, errno, strerror(errno));
                                continue;
                            }
                        }
                    }
                    else
                    {
                        reconnDebugPrint("%s: gNewSocketFd =%d \n", __FUNCTION__, gNewSocketFd);
                    }
                }
                else
                {
                    reconnDebugPrint("%s: listen returned  =%d(%s) \n", __FUNCTION__, errno, strerror(errno));
                    //close(in_socket_fd);
                    break;
                }
            }
        }
    }
    reconnDebugPrint("%s: Exiting *******************\n",__FUNCTION__);
    exit (0);
}
