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
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <linux/if.h>

#include "reconn.h"
#include "powerMeter.h"
#include "spectrum.h"
#include "dmm.h"
#include "powerMgmt.h"
#include "eqptResponse.h"
#include "gpio.h"
#include "debugMenu.h"
#include "upgrade.h"
#include "libiphoned.h"
#include "remoteMonitor.h"
#include "crc.h"

#define COMM_DEBUG

int gNewSocketFd; 
int fromLibToClientfd = -1;

int gpsEnabled = FALSE;
int powerMeterEnabled = FALSE;
int analyzerEnabled = FALSE;
int dmmEnabled = FALSE;

sem_t insertedMasterSemaphore;  // This semaphore is used by reconnMasterIphone() and 
                                // reconnClientTask(). reconClientTask posts, via this signal, 
                                // that it has removed the current Wifi Master.

pthread_t reconnThreadIds[RECONN_MAX_NUM_CLIENTS + RECONN_NUM_SYS_TASKS];
static int  numberOfActiveClients = 0;
ReconnEqptDescriptors gEqptDescriptors;

extern int masterClientSocketFd;
extern void insertedMasterRead();
extern int insertedMasterSocketFd;
extern void *insertedMasterTransmitTask();
extern void registerClientDebugMenu();
extern void registerFuelGaugeDebugMenu();
extern void registerSystemDebugMenu();
extern void registerSocketDebugMenu();
extern void registerRemoteMonDebugMenu();
extern void registerReconnMsgsMenu();

#ifndef __SIMULATION__
static void reconnCleanUp()
#else
void reconnCleanUp()
#endif
{
    extern int MasterTransmitListenFd;
    int i;

    reconnDebugPrint("%s:****** Function Called\n", __FUNCTION__);

    reconnEqptCleanUp();
    remoteMonitorCleanup();

    reconnDebugPrint("%s:****** closing fromLibToClientfd\n", __FUNCTION__, fromLibToClientfd);
    libiphoned_stop();
    if(fromLibToClientfd != -1)
    {
        if(close(fromLibToClientfd) != 0)
        {
            reconnDebugPrint("%s: close(MasterTransmitListenFd) failed %d (%s)\n", __FUNCTION__, errno, strerror(errno));
        }
    }

    reconnDebugPrint("%s:****** closing MasterTransmitListenFd\n", __FUNCTION__);
    if(MasterTransmitListenFd != -1)
    {
        if(shutdown(MasterTransmitListenFd, SHUT_RDWR) != 0)
        {
            reconnDebugPrint("%s: shutdown(MasterTransmitListenFd, SHUT_RDWR) failed %d (%s)\n", __FUNCTION__, errno, strerror(errno));
        }
        if(close(MasterTransmitListenFd) != 0)
        {
            reconnDebugPrint("%s: close(MasterTransmitListenFd) failed %d (%s)\n", __FUNCTION__, errno, strerror(errno));
        }
    }

    reconnDebugPrint("%s:****** destroying insertedMasterSemaphore\n", __FUNCTION__);
    sem_destroy(&insertedMasterSemaphore);
    if(gEqptDescriptors.powerMeterFd != -1)
    {
        reconnDebugPrint("%s:****** closing power Meter file descriptor %d\n", __FUNCTION__, gEqptDescriptors.powerMeterFd);
        if(close(gEqptDescriptors.powerMeterFd) != 0)
        {
            reconnDebugPrint("%s: close(gEqptDescriptors.powerMeterFd) failed %d (%s)\n", __FUNCTION__, errno, strerror(errno));
        }
    }
    if(gEqptDescriptors.analyzerFd != -1)
    {
        reconnDebugPrint("%s:****** closing analyzer file descriptor %d\n", __FUNCTION__, gEqptDescriptors.powerMeterFd);
        if(close(gEqptDescriptors.analyzerFd) != 0)
        {
            reconnDebugPrint("%s: close(gEqptDescriptors.analyzerFd) failed %d (%s)\n", __FUNCTION__, errno, strerror(errno));
        }
    }
    if(gEqptDescriptors.dmmFd != -1)
    {
        reconnDebugPrint("%s:****** closing DMM file descriptor %d\n", __FUNCTION__, gEqptDescriptors.powerMeterFd);
        if(close(gEqptDescriptors.dmmFd) != 0)
        {
            reconnDebugPrint("%s: close(gEqptDescriptors.dmmFd) failed %d (%s)\n", __FUNCTION__, errno, strerror(errno));
        }
    }
    for(i = 0; i < RECONN_MAX_NUM_CLIENTS; i++)
    {
        if (activeClientsList[i])
        { 
            reconnDebugPrint("%s:****** freeing client %d\n", __FUNCTION__, i);
            free(activeClientsList[i]->thisContext);
        }
    }
    reconnDebugPrint("\n\n%s:****** Application exiting\n", __FUNCTION__);
    exit(0);
}
//******************************************************************************
//******************************************************************************
// FUNCTION:    upgradeCheckTask
//
// DESCRIPTION: This task is started by main() when it detects the presence of
//              UPGRADE_INPROGRESS_FILE_NAME. This indicates that an upgrade
//              is in progress. This task then sleeps for UPGRADE_TASK_SLEEP_TIME.
//              If it wakes up then the upgraded reconn-service binary is running
//              and has not reset. We therefore determine that the upgraded binary
//              is probably OK. So, we move the /tmp/reconn-service binary to /usr/bin
//              which makes it the new executable. 
//******************************************************************************
static void *upgradeCheckTask(void *args)
{
    char command[FILENAME_SIZE];
    reconnDebugPrint("%s:****** Task Started\n", __FUNCTION__);
    sleep(UPGRADE_TASK_SLEEP_TIME);

    UNUSED_PARAM(args);
    memset(command, 0, FILENAME_SIZE);

    reconnDebugPrint("%s:****** Removing %s\n", __FUNCTION__, UPGRADE_INPROGRESS_FILE_NAME);
    unlink(UPGRADE_BUNDLE_NAME);
    strcat(command, "mv /tmp/reconn-service /usr/bin/");
    reconnDebugPrint("%s:****** %s)\n", __FUNCTION__, command);
    if(system(command) != 0)
    {
        reconnDebugPrint("%s: system(%s) failed.\n", __FUNCTION__, command);
    }
    return(0);
}

static void PeripheralInit() 
{
    int status = RECONN_SUCCESS;

    gEqptDescriptors.powerMeterFd = -1;
#if 0 // GPS has been removed from the reconn box
    if((status = gpsInit(&(gEqptDescriptors.gpsFd))) != RECONN_SUCCESS)
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

    if((status = SpectrumAnalyzerInit(&(gEqptDescriptors.analyzerFd))) != RECONN_SUCCESS) 
    {
        reconnDebugPrint("Spectrum Analyzer Init Failed\n");
        analyzerEnabled = FALSE;
    }
    else 
    {
        reconnDebugPrint("Spectrum Analyzer Initialized\n");
    }

    if((status = dmmInit(&(gEqptDescriptors.dmmFd))) != RECONN_SUCCESS)
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

//******************************************************************************
//******************************************************************************
// FUNCTION:    reconnGetFreeClientIndex
//
// DESCRIPTION: This function returns a free client index if one is available.
//              
//
//******************************************************************************
ReconnErrCodes reconnGetFreeClientIndex(short *theIndex)
{
    ReconnErrCodes retCode = RECONN_FAILURE;
    int i;

    if(theIndex == NULL)
    {
        reconnDebugPrint("%s: theIndex is NULL\n", __FUNCTION__);
    }
    else if(numberOfActiveClients < RECONN_MAX_NUM_CLIENTS)
    {
        /*
         * Currently only the inserted iphone master can request
         * an index and it must be 0. Any other value is overwritten
         * with the next available index.
         */
            
        if(*theIndex == 0)
        {
#ifdef DEBUG_MUTEX
            reconnDebugPrint("%s: Calling pthread_mutex_lock(&clientListMutex) %d %d %d\n", __FUNCTION__, clientListMutex.__data.__lock, clientListMutex.__data.__count, clientListMutex.__data.__owner);
#endif

            pthread_mutex_lock(&clientListMutex); 
            numberOfActiveClients++;
            activeClientsList[0] = (CLIENTCONTEXT *)1;
            retCode = RECONN_SUCCESS;
#ifdef DEBUG_MUTEX
            reconnDebugPrint("%s: Calling pthread_mutex_unlock(&clientListMutex) %d %d %d\n", __FUNCTION__, clientListMutex.__data.__lock, clientListMutex.__data.__count, clientListMutex.__data.__owner);
#endif

            pthread_mutex_unlock(&clientListMutex); 
        }
        else
        {
#ifdef DEBUG_MUTEX
            reconnDebugPrint("%s: Calling pthread_mutex_lock(&clientListMutex) %d %d %d\n", __FUNCTION__, clientListMutex.__data.__lock, clientListMutex.__data.__count, clientListMutex.__data.__owner);
#endif

            pthread_mutex_lock(&clientListMutex); 
            for (i = 1; i < RECONN_MAX_NUM_CLIENTS; i++)
            {
                if(activeClientsList[i] == 0)
                {
                    *theIndex = i;
                    /*
                     * Setting the activeClientsList location reserves it 
                     * which will be overwritten when the client registers
                     */
                    activeClientsList[i] = (CLIENTCONTEXT *)1;
                    numberOfActiveClients++;
                    retCode = RECONN_SUCCESS;
                    break;
                }
            }
#ifdef DEBUG_MUTEX
            reconnDebugPrint("%s: Calling pthread_mutex_unlock(&clientListMutex) %d %d %d\n", __FUNCTION__, clientListMutex.__data.__lock, clientListMutex.__data.__count, clientListMutex.__data.__owner);
#endif
            pthread_mutex_unlock(&clientListMutex);
        }
    }
    else
    {
        reconnDebugPrint("%s: numberOfActiveClients ==  %d\n", __FUNCTION__, numberOfActiveClients);
    }
    reconnDebugPrint("%s: returning %s\n", __FUNCTION__, (retCode == RECONN_SUCCESS) ? "RECONN_SUCCESS": "FAILURE");
    return retCode;
}

//******************************************************************************
//******************************************************************************
// FUNCTION:    reconnReturnClientIndex
//
// DESCRIPTION: This function is used to return a client's index.  The index
//              being returned was given by a call to reconnGetFreeClientIndex().
//
//******************************************************************************
void reconnReturnClientIndex(short index)
{
    if(index < RECONN_MAX_NUM_CLIENTS)
    {
        reconnThreadIds[RECONN_NUM_SYS_TASKS + index] = -1;
        numberOfActiveClients--;
        reconnDebugPrint("%s: client index %d returned numberOfActiveClients ==  %d\n", __FUNCTION__, index, numberOfActiveClients);
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
//******************************************************************************
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
    struct timespec wait_time;
    struct sockaddr_in masterTransmitAddr;
    static int iphoneInserted = FALSE;
    unsigned char theMessage[MASTER_MSG_SIZE];

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
            sem_init(&insertedMasterSemaphore, 0, 0);
            reconnDebugPrint("%s: Master WiFi client is present\n", __FUNCTION__);
            // This function will usurp mastership from the current
            // master iPhone. The physically inserted iPhone is ALWAYS
            // the master.
            
            // send current master a message telling it to give up control
            
            // Then start a clientApp() as master.
            memset(&theMessage, 0, MASTER_MSG_SIZE);
            theMessage[0] = MASTER_INSERTED;
            if(masterClientMsgQid != -1)
            {
                if(mq_send(masterClientMsgQid, (const char *)&theMessage, MASTER_MSG_SIZE, 0) != 0)
                {
                    reconnDebugPrint("%s: mq_send(%d) failed. %d(%s)\n", __FUNCTION__, errno, strerror(errno), masterClientMsgQid);
                }
                else
                {
                    reconnDebugPrint("%s: mq_send(%d) success.\n", __FUNCTION__, masterClientMsgQid);
                }
            }
            
            clock_gettime(CLOCK_REALTIME, &wait_time);
            wait_time.tv_sec += 8;
            wait_time.tv_nsec = 0;
            if(sem_timedwait(&insertedMasterSemaphore, &wait_time) != 0)
            {
                reconnDebugPrint("%s: sem_timedwait failed %d(%s).\r\n", __FUNCTION__, errno, strerror(errno));
                return;
            }
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
            masterTransmitAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
            masterTransmitAddr.sin_port = htons(RECONN_LIBTOCLIENT_PORT);

            if(connect(fromLibToClientfd, (struct sockaddr *)&masterTransmitAddr, sizeof(masterTransmitAddr)) < 0)
            {
                reconnDebugPrint("%s: connect failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
            }
        }

        CLIENTCONTEXT *mem;
        if((mem = malloc(sizeof(CLIENTCONTEXT))) != NULL)
        {
            memset(mem, 0, sizeof(CLIENTCONTEXT));
            mem->thisContext = (int *)mem;
            mem->socketFd = insertedMasterSocketFd;
            mem->eqptDescriptors = &gEqptDescriptors;
            mem->tmpFd = (FILE *)-1;
            // The inserted master is always client number 0
            mem->index = 0;
            if(reconnGetFreeClientIndex(&(mem->index)) == RECONN_SUCCESS)
            {
                mem->mode = INSERTEDMASTERMODE;
#ifdef DEBUG_CONNECT
                reconnDebugPrint("%s: insertedMasterSocketFd = %d\r\n", __FUNCTION__, insertedMasterSocketFd);
                reconnDebugPrint("%s: Starting reconnClient index= %u \n", __FUNCTION__, mem->index);
#endif
                if(pthread_create(&reconnThreadIds[RECONN_NUM_SYS_TASKS], NULL, reconnClientTask, (CLIENTCONTEXT *)mem) < 0)
                {
                    reconnDebugPrint("%s: Could not start reconnClient, %d %s\n", __FUNCTION__, errno, strerror(errno));
                }
                else
                {
                    iphoneInserted = TRUE;
                }
            }
        }
        else
        {
            reconnDebugPrint("%s: malloc failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
        }
    }
    else if(iphoneInserted == TRUE)
    {
        reconnDebugPrint("%s: Function Entered iphone EXTRACTED\n", __FUNCTION__);

        memset( &theMessage, 0, MASTER_MSG_SIZE);
        theMessage[0] = MASTER_EXTRACTED;
        printf("%s: masterClientMsgQid == %d\n", __FUNCTION__, masterClientMsgQid);
        if(masterClientMsgQid != -1)
        {
            if(mq_send(masterClientMsgQid, (const char *)&theMessage, MASTER_MSG_SIZE, 0) != 0)
            {
                reconnDebugPrint("%s: mq_send(masterClientMsgQid) failed. %d(%s)\n", __FUNCTION__, errno, strerror(errno));
            }
            else
            {
                reconnDebugPrint("%s: mq_send(masterClientMsgQid) success.\n", __FUNCTION__);
            }
        }
    }
}




int main(int argc, char **argv) 
{
    int i;
    struct stat statInfo;
    struct sigaction act;
    extern void *Usb0IpWatchTask(void *);

    UNUSED_PARAM(argc);
    UNUSED_PARAM(argv);
    memset(&gEqptDescriptors, 0, sizeof(ReconnEqptDescriptors));

#ifndef __SIMULATION__
    initReconnCrashHandlers();
#endif
    memset(&act, 0, sizeof(act));
    act.sa_flags = SA_SIGINFO;
    act.sa_sigaction = reconnCleanUp;
    signal(SIGPIPE, SIG_IGN);
    sigaction(SIGTERM, &act, NULL);

    i = sizeof(reconnThreadIds);
    for(i = 0; i < RECONN_MAX_NUM_CLIENTS + RECONN_NUM_SYS_TASKS; i++)
    {
        reconnThreadIds[i] = -1;
    }
    for(i = 0; i < (RECONN_MAX_NUM_CLIENTS); i++)
    { 
        activeClientsList[i] = 0;
    }


    // If there is an upgrade in progress then start a task that will sleep for 10 minutes then run.
    // If the task wakes up, then the upgraded application has not crashed and so the new image 
    // is probably OK.
    if(stat(UPGRADE_INPROGRESS_FILE_NAME, &statInfo) == 0)
    {
        if( pthread_create(&(reconnThreadIds[RECONN_UPGRADE_CHECK_TASK]), NULL, upgradeCheckTask, (void *)0) < 0)
        {
            reconnDebugPrint("%s: Could not start upgradeCheckTask, %d %s\n", __FUNCTION__, errno, strerror(errno));
        }
        unlink(UPGRADE_INPROGRESS_FILE_NAME);
    }

    /*
     * Initialize the AVCOM crc routine. The crc is used for spectrum analyzer upgrade and firmware 
     * version intefaces.
     */
    crcInit();

    PeripheralInit();
//#ifdef DEBUG_CONNECT
    reconnDebugPrint("      gEqptDescriptors->powerMeterFd = %d\n", gEqptDescriptors.powerMeterFd);
    reconnDebugPrint("      gEqptDescriptors->dmmFd = %d\n", gEqptDescriptors.dmmFd);
    reconnDebugPrint("      gEqptDescriptors->analyzerFd = %d\n", gEqptDescriptors.analyzerFd);
//#endif
#ifndef __SIMULATION__
    //dmmDiags();
    dmmLoadSavedConfig();
#endif
    //
    // Startup debug menus
    //
    registerClientDebugMenu();
    registerFuelGaugeDebugMenu();
    registerSystemDebugMenu();
    registerDmmDebugMenu();
    registerSocketDebugMenu();
    registerRemoteMonDebugMenu();
    registerReconnMsgsMenu();

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
        if(pthread_create(&(reconnThreadIds[RECONN_PWR_MGMT_TASK]), NULL, reconnPwrMgmtTask, (void  *)&gEqptDescriptors) < 0)
        {
            reconnDebugPrint("%s: Could not start reconnPwrMgmtTask, %d %s\n", __FUNCTION__, errno, strerror(errno));
        }
        else if(pthread_create(&(reconnThreadIds[RECONN_EQPT_RSP_TASK]), NULL, reconnGetEqptResponseTask,(void *)&gEqptDescriptors) < 0)
        {
            reconnDebugPrint("%s: Could not start reconnGetEqptResponseTask, %d %s\n", __FUNCTION__, errno, strerror(errno));
        }
        else if(pthread_create(&(reconnThreadIds[RECONN_BATTERY_MONITOR_TASK]), NULL, reconnBatteryMonTask, (void *) 0 ) < 0)
        {
            reconnDebugPrint("%s: Could not start reconnBatteryMonTask, %d %s\n", __FUNCTION__, errno, strerror(errno));
        }
        else if(pthread_create(&(reconnThreadIds[RECONN_DEBUG_MENU_TASK]), NULL, debugMenuTask, (void *) 0 ) < 0)
        {
            reconnDebugPrint("%s: Could not start debugMenuTask, %d %s\n", __FUNCTION__, errno, strerror(errno));
        }
        else if(pthread_create(&(reconnThreadIds[RECONN_POWER_METER_TASK]), NULL, powerMeterPresenceTask, (void *) &gEqptDescriptors) < 0)
        {
            reconnDebugPrint("%s: Could not start powerMeterPresenceTask, %d %s\n", __FUNCTION__, errno, strerror(errno));
        }
//#ifndef __SIMULATION__
        else if(pthread_create(&(reconnThreadIds[RECONN_DMM_SAVE_CONFIG_TASK]), NULL, dmmSaveConfigTask, (void *) 0 ) < 0)
        {
            reconnDebugPrint("%s: Could not start dmmSaveConfigTask, %d %s\n", __FUNCTION__, errno, strerror(errno));
        }
//#endif
        else
        {
#ifndef __SIMULATION__
            //
            // Need to register with libiphoned. This callback will signal when
            // the iphone has been inserted into the reconn toolkit.
            //
            libiphoned_register_presence_change_callback(reconnMasterIphone);
            libiphoned_register_rx_callback(insertedMasterRead, RECONN_RSP_PAYLOAD_SIZE);

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

            /*
             * Now that the embedded software is up and running, stop the power LED from flashing.
             */
            if(reconnGpioAction(POWER_LED_GPIO, ENABLE, NULL) != RECONN_SUCCESS)
            {
                reconnDebugPrint("%s: reconnGpioAction(POWER_LED_GPIO, ENABLE, NULL) failed.\n", __FUNCTION__);
            }
#endif
            wifiStartConnectionTask();
#ifndef __SIMULATION__
            if(pthread_create(&(reconnThreadIds[RECONN_IP_WATCH_TASK]), NULL, Usb0IpWatchTask, (void *)0) < 0)
            
            {
                reconnDebugPrint("%s: Could not start Usb0IpWatchTask, %d %s\n", __FUNCTION__, errno, strerror(errno));
            }
#endif
        }
    }

    /*
     * See if we should start the task that accepts Wifi connections.
     */ 
    pthread_join(reconnThreadIds[RECONN_POWER_METER_TASK], NULL);
    reconnDebugPrint("%s: Exiting *******************\n",__FUNCTION__);
    exit (0);
}
