//******************************************************************************
//******************************************************************************
//
// FILE:        powerMgmt.c
//
// CLASSES:     
//
// DESCRIPTION: Main file which contains process used to manage the RECONN power
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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "reconn.h"
#include "powerMgmt.h"
#include "gpio.h"


static int systemStandbyTime;

void *reconnPwrMgmtTask(void *argument) 
{
    systemStandbyTime = RECONN_POWER_DOWN_TIME;

    printf("%s: **** Task Started\n", __FUNCTION__);
    while (1) 
    {
        if(systemStandbyTime == RECONN_POWER_CONSERVATION_TIME)
        {
            printf("%s: Going into power conservation mode\n", __FUNCTION__);
        }
        else if(!systemStandbyTime)
        {
            printf("%s: Powering Down the system\n", __FUNCTION__);

            // TODO this system call needs to be replaced with code that gracefully 
            // shutsdown all equipment and subsystems. Including saving the subsystem
            // states.

            // TODO: Add code to shutdown all of the internal msg queues, power down the
            // eqpt close of FD then set GPIO156 to 0
        }
        sleep(60);
        systemStandbyTime --;
    }
}

void *reconnPwrButtonTask(void *argument)
{
    FILE *powerButtonFd;
    int theButtonValue;
    int initialPowerUp = TRUE;
    static char retCode = '1';

    printf("%s: **** Task Started\n", __FUNCTION__);
    while(1)
    {
#ifndef __SIMULATION__
        if((powerButtonFd = fopen(RECONN_POWER_BUTTON_GPIO_FILENAME, "r")) == NULL)
        {
            printf("%s: fopen(RECONN_POWER_BUTTON_GPIO_FILENAME, r) failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
        }
        else if((theButtonValue = fgetc(powerButtonFd)) == EOF)
        {
            printf("%s: fgetc(RECONN_POWER_BUTTON_GPIO_FILENAME) == EOF %d(%s)\n", __FUNCTION__, errno, strerror(errno));
        }
        else
        {
            // Upon initial power up, if the power button is depressed we keep checking until it is no 
            // longer depressed. Then we monitor it to be pressed again at which time we turn the box off.
            if((initialPowerUp == TRUE) && (atoi((char *)&theButtonValue) == POWER_BUTTON_PRESSED))
            {
                usleep(RECONN_CHECK_POWER_SWITCH);
                printf(".");
                fclose(powerButtonFd);
                continue;
            }
            initialPowerUp = FALSE;
            if (atoi((char *)&theButtonValue) == POWER_BUTTON_PRESSED)
            {
                // Need to shutdown all power 
                reconnGpioAction(GPIO_140, DISABLE);
                reconnGpioAction(GPIO_141, DISABLE);
                reconnGpioAction(GPIO_157, DISABLE);
                reconnGpioAction(GPIO_161, DISABLE);
                reconnGpioAction(GPIO_157, DISABLE);
                // Turn off the power LED
                reconnGpioAction(GPIO_172, DISABLE);
                reconnGpioAction(GPIO_156, DISABLE); // releases the 5V power latch to the board.
            }
        }
        if(powerButtonFd)
        {
            fclose(powerButtonFd);
        }
        usleep(RECONN_CHECK_POWER_SWITCH);
#else
        printf("%s: **** Task not running due to simulation\n", __FUNCTION__);
        sleep(20000);
#endif
    }
    return &retCode;
}


void resetPowerStandbyCounter()
{
    systemStandbyTime = RECONN_POWER_DOWN_TIME;
}
