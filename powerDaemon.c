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
#include <pthread.h>
#include <signal.h>
#include <semaphore.h>
#include <getopt.h>

#include "reconn.h"
#include "gpio.h"
#include "powerMgmt.h"

static int retCode;

int main(void) 
{
    FILE *powerButtonFd;
    int theButtonValue;
    int initialPowerUp = TRUE;

    /* Our process ID and Session ID */
    pid_t pid, sid;

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
    while(1)
    {
#ifndef __SIMULATION__
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
                continue;
            }
            initialPowerUp = FALSE;
            if (atoi((char *)&theButtonValue) == POWER_BUTTON_PRESSED)
            {
                // Need to shutdown all power 
#if 0
                reconnGpioAction(GPIO_140, DISABLE);
                reconnGpioAction(GPIO_141, DISABLE);
                reconnGpioAction(GPIO_157, DISABLE);
                reconnGpioAction(GPIO_161, DISABLE);
                reconnGpioAction(GPIO_157, DISABLE);
                // Turn off the power LED
                reconnGpioAction(GPIO_172, DISABLE);
#endif
                printf("%s: button pressed\n", __FUNCTION__);
                system("echo 0x1 > /sys/class/gpio/gpio156/value");
                system("echo 0x8 > /proc/cpld/CPLD_Tx_DIR_setbit");
                break;
            }
        }
        if(powerButtonFd)
        {
            fclose(powerButtonFd);
        }
        usleep(RECONN_CHECK_POWER_SWITCH);
#else
        printf("%s: **** Task is in simulation\n", "ReconnPowerButton");
        sleep(2);
#endif
    }
    printf("%s: **** Task is in Ending\n", "ReconnPowerButton");
    return retCode;
}

