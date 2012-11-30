//******************************************************************************
//
// FILE:        powerDaemon.c
//
// FUNCTIONS:   reconnPwrMgmtTask()       
//
// DESCRIPTION: This file contains the power button Daemon code. It is responsible
//              for monitoring the power button GPIO. When depressed, this task
//              will kill the reconn unit.
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
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <getopt.h>

#include "reconn.h"
#include "gpio.h"
#include "powerMgmt.h"
#include "dmm.h"

static int retCode = 0;
static FILE *powerButtonFd = NULL;
static int notDone = 1;

/*
 * This socket is used to communcation with reconn-service's dmm task. When a user hits the 
 * power button we have to signal the reconn-service app so that it saves the DMM's configuration.
 */
static int dmmSocketFd = -1;
static int dmmListenFd = -1;

void powerDaemonCleanUp()
{
    printf("%s: Function entered\n",__FUNCTION__);
    if(powerButtonFd)
    {
        printf("%s: closing powerButtonFd\n",__FUNCTION__);
        if(fclose(powerButtonFd) != 0)
        {
            printf("%s: fclose(powerButtonFd) failed %d (%s)\n", __FUNCTION__, errno, strerror(errno));
        }
        powerButtonFd = NULL;
    }
    if(dmmListenFd != -1)
    {
        printf("%s: closing dmmListenFd \n",__FUNCTION__);
        if(close(dmmListenFd) != 0)
        {
            printf("%s: close(dmmListenFd) failed %d (%s)\n", __FUNCTION__, errno, strerror(errno));
        }
        dmmListenFd = -1;
    }
    if(dmmSocketFd != -1)
    {
        printf("%s: closing dmmSocketFd entered\n",__FUNCTION__);
        if(close(dmmSocketFd) !=0)
        {
            printf("%s: close(dmmSocketFd) failed %d (%s)\n", __FUNCTION__, errno, strerror(errno));
        }
        dmmSocketFd = -1;
    }
    notDone = 0;
}

//*************************************************************************************
//*************************************************************************************
// FUNCTION:    powerDaemonSocketTask
//
// DESCRIPTION: This tasks is responsible for listening to a local socket waiting
//              for the reconn-service's dmmSaveConfigTask() task to connect. Once 
//              the connection occurs this task sets the dmmListenFD and listens again.
//
// Parameters:
//
//*************************************************************************************
static void *powerDaemonSocketTask(void *argument)
{
    socklen_t client_len;
    struct sockaddr_in server_addr, client_addr;
    static int retStatus = 1;
    int optval = 1;

    if((dmmListenFd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("%s: Failed to initialize the incoming socket %d(%s)\n", __FUNCTION__, errno, strerror(errno));
    } 
    else
    {
        if(setsockopt(dmmListenFd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
        {
            printf("%s: setsockopt SO_REUSEADDR failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
        }
        else
        {
            bzero((unsigned char *) &server_addr, sizeof(server_addr));
            /* bind the socket */
            server_addr.sin_family = AF_INET;
            server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
            server_addr.sin_port = htons(DMM_SOCKET_PORT);

            if (bind(dmmListenFd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)
            {
                printf("%s: Failed to bind the socket %d(%s)\n",  __FUNCTION__, errno, strerror(errno));
            }
            else
            {
                while(notDone)
                {
                    /*
                     * Only allow one connection to be active at any time.
                     */
                    if(listen(dmmListenFd, 1) == 0)
                    {
                        if((dmmSocketFd = accept(dmmListenFd, (struct sockaddr *) &client_addr,
                                        &client_len)) < 0) 
                        {
                            printf("%s: Failed to open DMM socket\n", __FUNCTION__, errno, strerror(errno));
                            continue;
                        }
                        printf("%s: DMM is connected\n", __FUNCTION__);
                    }
                    else
                    {
                        printf("%s: listen() failed %d(%s)\n",  __FUNCTION__, errno, strerror(errno));
                        sleep(5);
                    }
                }
            }
        }
    }
    return(&retStatus);
}

//*************************************************************************************
//*************************************************************************************
// FUNCTION:    main
//
// DESCRIPTION: This main work-horse of this process. The process simply reads the power
//              button GPIO waiting for it to go high which indicates the user has pressed
//              the reconn unit's power button. Then this occurs this process will send
//              a message to the reconn-service's dmmSaveConfigTask() indicating that the
//              DMM configuration needs to be saved. This process will wait for a predetermined
//              amount of time for a response.  If the wait time triggers or a response is 
//              received this process will set the main power GPIO thus turning OFF the reconn
//              unit.
//
// Parameters:
//
//*************************************************************************************
int main(void) 
{
    int theButtonValue;
    int initialPowerUp = TRUE;
    pthread_t task;
    DMM_POWER_BUTTON_MSG dmmPowerButtonMsg;
    struct sigaction act;
    struct timeval waitTime;
    fd_set selectFdSet;

    /* Our process ID and Session ID */
    pid_t pid, sid;

#ifndef __SIMULATION__
    /* Fork off the parent process */
    pid = fork();
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }
    /* If we got a good PID, then
       we can exit the parent process. */
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    /* Change the file mode mask */
    umask(0);

    /* Open any logs here */       

    /* Create a new SID for the child process */
    sid = setsid();
    if (sid < 0) {
        /* Log the failure */
        exit(EXIT_FAILURE);
    }
#endif

    memset(&act, 0, sizeof(act));
    act.sa_flags = SA_SIGINFO;
    act.sa_sigaction = powerDaemonCleanUp;
    sigaction(SIGTERM, &act, NULL);

    /*
     * Start the thread which listens for a socket connection from the reconn-service's DMM. 
     */
    if(pthread_create(&task, NULL, powerDaemonSocketTask, (void* )0) < 0)
    {
        printf("%s: Could not start powerDaemonSocketTask() %d(%s)\n", "ReconnPowerButton", errno, strerror(errno));
    }

    while(notDone)
    {
        if((powerButtonFd = fopen(RECONN_POWER_BUTTON_GPIO_FILENAME, "r")) == NULL)
        {
            printf("%s: fopen(RECONN_POWER_BUTTON_GPIO_FILENAME, r) failed %d(%s)\n", "ReconnPowerButton", errno, strerror(errno));
        }
        else if((theButtonValue = fgetc(powerButtonFd)) == EOF)
        {
            printf("%s: fgetc(RECONN_POWER_BUTTON_GPIO_FILENAME) == EOF %d(%s)\n", "ReconnPowerButton", errno, strerror(errno));
        }
        else
        {
            // Upon initial power up, if the power button is depressed we keep checking until it is no 
            // longer depressed. Then we monitor it to be pressed again at which time we turn the box off.
            if((initialPowerUp == TRUE) && (atoi((char *)&theButtonValue) == POWER_BUTTON_PRESSED))
            {
                usleep(RECONN_CHECK_POWER_SWITCH);
                fclose(powerButtonFd);
                powerButtonFd = NULL;
                continue;
            }
            initialPowerUp = FALSE;
            if (atoi((char *)&theButtonValue) == POWER_BUTTON_PRESSED)
            {
                printf("ReconnPowerButton: Power Button pressed\n");
                if(dmmSocketFd != -1)
                {
                    FD_ZERO(&selectFdSet);
                    FD_SET(dmmSocketFd, &selectFdSet);
                    waitTime.tv_sec = 1;
                    waitTime.tv_usec = 0;

                    dmmPowerButtonMsg.theMessage = SAVE_DMM_STATE;
                    send(dmmSocketFd, &(dmmPowerButtonMsg.theMessage), sizeof(dmmPowerButtonMsg), 0);
                    if((retCode = select(dmmSocketFd + 1, &selectFdSet, NULL, NULL, &waitTime)) >= 0) 
                    {
                        /*
                         * we are only checking the return code from recv() for development purposes. Because 
                         * the user has depressed the power button we have to shut down the reconn unit, so it
                         * really does not matter, post development, what recv() returns.
                         */
                        if(recv(dmmSocketFd, &(dmmPowerButtonMsg.theMessage), sizeof(dmmPowerButtonMsg), 0) > 0)
                        {
                            if(dmmPowerButtonMsg.theMessage == DMM_SAVE_COMPLETE)
                            {
                                printf("ReconnPowerButton: received response from dmmSaveConfigTask()\n");
                            }
                            else
                            {
                                printf("ReconnPowerButton: received invalid response (%d) from dmmSaveConfigTask()\n", dmmPowerButtonMsg.theMessage);
                            }
                        }
                        else
                        {
                            printf("ReconnPowerButton: recv() failed %d(%s)\n", errno, strerror(errno));
                        }
                    }
                    else
                    {
                        printf("ReconnPowerButton: listen() failed %d(%s)\n", errno, strerror(errno));
                    }
                }
                /*
                 * Setting this GPIO kills power to the box.
                 */
                system("echo 0x1 > /sys/class/gpio/gpio156/value");
                /*
                 * Setting bit 3 in the CPLD register makes the GPIO an output.
                 */
                system("echo 0x8 > /proc/cpld/CPLD_Tx_DIR_setbit");
                break;
            }
            else
            {
                if(dmmSocketFd != -1)
                {
                    FD_ZERO(&selectFdSet);
                    FD_SET(dmmSocketFd, &selectFdSet);
                    waitTime.tv_sec = 0;
                    waitTime.tv_usec = 20;
                    if((retCode = select(dmmSocketFd + 1, &selectFdSet, NULL, NULL, &waitTime)) == -1)
                    {
                        printf("%s: select() failed %d (%s)\n", "PowerDaemon", errno, strerror(errno));
                    }
                    else if (retCode > 0)
                    {
                        /*
                         * If we get here then the reconn-service application closed the socket
                         */
                        printf("%s: dmmSocketFd was closed by peer.\n", "PowerDaemon");
                        if(close(dmmSocketFd) != 0)
                        {
                            printf("%s: close(dmmSocketFd) failed %d (%s)\n", "PowerDaemon", errno, strerror(errno));
                            
                        }
                        dmmSocketFd = -1;
                    }
                }
            }
        }
        if(powerButtonFd)
        {
            fclose(powerButtonFd);
            powerButtonFd = NULL;
        }
        usleep(RECONN_CHECK_POWER_SWITCH);
    }
    printf("%s: **** Task is Ending\n", "ReconnPowerButton");
    return retCode;
}
