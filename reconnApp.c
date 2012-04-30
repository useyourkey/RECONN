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
int fromLibToClientfd;

int gpsEnabled = FALSE;
int powerMeterEnabled = FALSE;
int analyzerEnabled = FALSE;
int dmmEnabled = FALSE;

static pthread_t reconnThreadIds[RECONN_MAX_NUM_CLIENTS + RECONN_NUM_SYS_TASKS];
static int  numberOfActiveClients = 0;
static int in_socket_fd;
static ReconnModeAndEqptDescriptors modeAndEqptDescriptors;

extern mqd_t masterClientMsgQid;
static struct mq_attr masterClientMsgQAttr;
static unsigned char theMessage[4];

extern int masterClientSocketFd;
extern void insertedMasterRead();
extern int insertedMasterSocketFd;
extern void *insertedMasterTransmitTask();

static void reconnCleanUp()
{
    printf("%s:****** Task Called\n", __FUNCTION__);
    printf("%s:****** closing in_socket_fd\n", __FUNCTION__, in_socket_fd);
    close(in_socket_fd);
    printf("%s:****** closing fromLibToClientfd\n", __FUNCTION__, in_socket_fd);
    close(fromLibToClientfd);
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

    if((status = SpectrumAnalyzerInit(&(modeEqptDescriptors->analyzerFd))) != RECONN_SUCCESS) 
    {
        printf("Spectrum Analyzer Init Failed\n");
        analyzerEnabled = FALSE;
    }
    else 
    {
        printf("Spectrum Analyzer Initialized\n");
    }

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

void reconnMasterIphone()
{
    ReconnClientIndex index;
    pthread_t task;
    struct timespec wait_time;
    struct sockaddr_in masterTransmitAddr;

    printf("%s: Function Entered\n", __FUNCTION__);
#ifdef __SIMULATION__
    if(simulate_isiphonepresent() == 0) 
#else
    if(libiphoned_isiphonepresent() == 0) 
#endif
    {
        printf("%s: Function Entered iphone INSERTED\n", __FUNCTION__);
        if (masterClientSocketFd != -1)
        {
            printf("%s: Master WiFi client is present\n", __FUNCTION__);
            // This function will usurp mastership from the current
            // master iPhone. The physically inserted iPhone is ALWAYS
            // the master.
            
            // send current master a message telling it to give up control
            
            // Then start a clientApp() as master.
            memset( &theMessage, 0, 4);
            theMessage[0] = MASTER_INSERTED;
            if(mq_send(masterClientMsgQid, (const char *)&theMessage, sizeof(theMessage), 0) != 0)
            {
                printf("%s: mq_send(masterClientMsgQid) failed. %d(%s)\n", __FUNCTION__, errno, strerror(errno));
            }
            else
            {
                printf("%s: mq_send(masterClientMsgQid) success.\n", __FUNCTION__);
            }
            
            clock_gettime(CLOCK_REALTIME, &wait_time);
            wait_time.tv_sec += 1;
            wait_time.tv_nsec = 0;
#if 0
            if(mq_timedreceive(masterClientMsgQid, (char *) &theMessage, sizeof(theMessage), 0, &wait_time) != 0)
            {
                printf("%s: mq_timedreceive failed %d(%s).\r\n", __FUNCTION__, errno, strerror(errno));
                return(0);
            }
#endif
        }
        else
        {
            printf("%s: ***NO Master WiFi client\n", __FUNCTION__);
        }

        if((fromLibToClientfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            printf("%s: socket failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
        }
        else
        {
            bzero((unsigned char *) &masterTransmitAddr, sizeof(masterTransmitAddr));

            masterTransmitAddr.sin_family = AF_INET;
            masterTransmitAddr.sin_addr.s_addr = INADDR_ANY;
            masterTransmitAddr.sin_port = htons(RECONN_INCOMING_PORT+2);

            if(connect(fromLibToClientfd, (struct sockaddr *)&masterTransmitAddr, sizeof(masterTransmitAddr)) < 0)
            {
                printf("%s: connect failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
            }
        }

        gNewSocketFd = insertedMasterSocketFd;
        index = reconnGetFreeClientIndex();
        modeAndEqptDescriptors.clientIndex = index;
        modeAndEqptDescriptors.clientMode = INSERTEDMASTERMODE;
#ifdef DEBUG_CONNECT
        printf("%s: gNewSocketFd = %d\r\n", __FUNCTION__, gNewSocketFd);
        printf("%s: Starting reconnClient index= %u \n", __FUNCTION__, index);
#endif
        if(pthread_create(&task, NULL, reconnClientTask, (void *)&modeAndEqptDescriptors) < 0)
        {
            printf("%s: Could not start reconnClient, %d %s\n", __FUNCTION__, errno, strerror(errno));
        }
    }
    else
    {
        printf("%s: Function Entered iphone EXTRACTED\n", __FUNCTION__);

        memset( &theMessage, 0xff, 4);
        theMessage[0] = MASTER_EXTRACTED;
        if(mq_send(masterClientMsgQid, (const char *)&theMessage, sizeof(theMessage), 0) != 0)
        {
            printf("%s: mq_send(masterClientMsgQid) failed. %d(%s)\n", __FUNCTION__, errno, strerror(errno));
        }
        else
        {
            printf("%s: mq_send(masterClientMsgQid) success.\n", __FUNCTION__);
        }
    }
}




int main(int argc, char **argv) 
{
#if 0
    int in_socket_fd; /* Incoming socket file descriptor for socket 1068     */
#endif
    int intport = 0;
    ReconnClientIndex index;
    socklen_t client_len;
    struct sockaddr_in server_addr, client_addr;
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
    printf("%s: Calling libiphoned_start()\n", __FUNCTION__);
    if(libiphoned_start() == -1)
    {
        printf("%s: libiphoned_start() failed\n", __FUNCTION__);
        exit(0);
    }
#endif
    
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
    //
    // Create the message queue used to tell the WiFi connected master client that an iPhone 
    // has been inserted into the toolkit's front panel. The front panel iPhone is ALWAYS
    // the master.
    //
    mq_unlink(INSERTED_MASTER_MSG_Q_NAME);
    masterClientMsgQAttr.mq_flags   = 0;
    masterClientMsgQAttr.mq_maxmsg   = 200;
    masterClientMsgQAttr.mq_msgsize  = 10;
    if((masterClientMsgQid = mq_open(INSERTED_MASTER_MSG_Q_NAME, 
                    (O_RDWR | O_CREAT | O_NONBLOCK), 0, NULL)) == (mqd_t) -1)
    {
        printf("%s: mq_open() failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
        exit(0);
    }

    // If there is an upgrade in progress then start a task that will sleep for 5 minutes then run.
    // If the task does run, then the upgraded application has not crashed and so the new image 
    // is OK.
    if(stat(UPGRADE_INPROGRESS_FILE_NAME, &statInfo) == 0)
    {
        if( pthread_create(&(reconnThreadIds[RECONN_UPGRADE_CHECK_TASK]), NULL, upgradeCheckTask, (void *)0) < 0)
        {
            printf("%s: Could not start reconnEqptTask %d %s\n", __FUNCTION__, errno, strerror(errno));
        }
    }
    //
    // Startup debug menus
    //
    registerClientDebugMenu();

    if(pthread_create(&(reconnThreadIds[RECONN_MASTER_SOCKET_TASK]), NULL, insertedMasterTransmitTask, (void *)0) < 0)
    {
        printf("%s: Could not start openInsertedMasterSocket %d %s\n", __FUNCTION__, errno, strerror(errno));
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
