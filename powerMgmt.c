//******************************************************************************
//******************************************************************************
//
// FILE:        powerMgmt.c
//
// FUNCTIONS:   reconnPwrMgmtTask()       
//              reconnPwrButtonTask()
//              resetPowerStandbyCounter()
//
// DESCRIPTION: Main file which contains process used to manage the RECONN power
//              This includes power conservation and monitoring power button.         
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
#include "fuel_gauge.h"

// counters to keep track of when a piece of equipment should go into power conservation 
// mode. I.E. powered down
static PowerMgmtEqptCounters eqptStbyCounters; 

//******************************************************************************
//******************************************************************************
//
// FUNCTIONS:   reconnPwrMgmtTask()       
//
// DESCRIPTION: This task is responsible for monitoring system resources to determine
//              if the system should place a piece of test equipment into power conservication
//              mode. I.E. power it down. The task also monitors the entire system 
//              for lack of iPhone client activity. If there is no client activity i.e. no
//              commands being sent to the reconn box, this task will gracefully shutdown
//              the system.
//******************************************************************************
void *reconnPwrMgmtTask(void *argument) 
{
    int i, *aEqptCounter;

    (void) argument;  // quiet compiler

    eqptStbyCounters.ReconnSystemCounter = RECONN_POWER_DOWN_TIME;

    aEqptCounter = &eqptStbyCounters.PowerMeterCounter;

    for(i = 0; i < RESET_SPECTRUM_ANALYZER_STBY_COUNTER; i++, aEqptCounter++)
    {
        *aEqptCounter = RECONN_POWER_CONSERVATION_TIME;
    }

    printf("%s: **** Task Started\n", __FUNCTION__);
    while (1) 
    {
        for(i = 0; i < RESET_SPECTRUM_ANALYZER_STBY_COUNTER; i++)
        {
            eqptStbyCounters.PowerMeterCounter--;
            if((--eqptStbyCounters.PowerMeterCounter) == 0)
            {
                printf("%s: Power Meter is going into Conservation Mode\n", __FUNCTION__);
            }
            if((--eqptStbyCounters.DmmCounter) == 0)
            {
                printf("%s: Dmm is going into Conservation Mode\n", __FUNCTION__);
            }
            if((--eqptStbyCounters.GpsCounter) == 0)
            {
                printf("%s: GPS is going into Conservation Mode\n", __FUNCTION__);
            }
            if((--eqptStbyCounters.SpectrumAnalyzerCounter) == 0)
            {
                reconnGpioAction(GPIO_141, DISABLE);
                printf("%s: Spectrum Analyzer is going into Conservation Mode\n", __FUNCTION__);
            }
        }

        if(!eqptStbyCounters.ReconnSystemCounter)
        {
            printf("%s: Powering Down the system\n", __FUNCTION__);

            // TODO this system call needs to be replaced with code that gracefully 
            // shutsdown all equipment and subsystems. Including saving the subsystem
            // states.

            // TODO: Add code to shutdown all of the internal msg queues, power down the
            // eqpt close of FD then set GPIO156 to 0
        }
        sleep(60);
        eqptStbyCounters.ReconnSystemCounter --;
    }
}

void *reconnPwrButtonTask(void *argument)
{
    FILE *powerButtonFd;
    int theButtonValue;
    int initialPowerUp = TRUE;
    static char retCode = '1';

    (void) argument;  // quiet compiler

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
                break;
            }
        }
        if(powerButtonFd)
        {
            fclose(powerButtonFd);
        }
        usleep(RECONN_CHECK_POWER_SWITCH);
#else
        printf("%s: **** Task not running due to simulation\n", __FUNCTION__);
        sleep(200000);
#endif
    }
    return &retCode;
}

uint8_t batteryPercentage;
int chargerAttached = FALSE;

void *reconnBatteryMonTask(void *argument)
{
    static char retCode = '1';
    PowerMgmtLedColors ledColor = OFF;
    FILE *dcPowerGpioFp;
    fuel_gauge_status_t status;
    fuel_gauge_handle_t fgh;
    fuel_gauge_context_t fgh_context;
    const char *psstatus;
    int retry_count;

    (void) argument;  // quiet compiler

#ifndef __SIMULATION__
    if((dcPowerGpioFp = fopen(RECONN_DC_POWER_GPIO_FILENAME, "r")) == NULL)
    {
        printf("%s: fopen(RECONN_DC_POWER_GPIO_FILENAME,\"r\") failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
    }
#endif
    printf("%s: **** Task Started\n", __FUNCTION__);

    fgh = &fgh_context;
    status = fuel_gauge_init(&fgh);
    if (status != FUEL_GAUGE_STATUS_SUCCESS)
    {
        fuel_gauge_status_string(status, &psstatus);
        printf("%s: fuel_gauge_init() failed %d(%s)\n", __FUNCTION__, status, psstatus);
        return &retCode;
    }
    status = fuel_gauge_open_dev(fgh, RECONN_FUEL_GAUGE_DEVICE_I2C_BUS);
    if (status != FUEL_GAUGE_STATUS_SUCCESS)
    {
        fuel_gauge_status_string(status, &psstatus);
        printf("%s: fuel_gauge_open() failed %d(%s)\n", __FUNCTION__, status, psstatus);
        fuel_gauge_uninit(fgh);
        return &retCode;
    }

    while(1)
    {
        status = fuel_gauge_get_charge_percent(fgh, &batteryPercentage);
        if (status != FUEL_GAUGE_STATUS_SUCCESS)
        {
            fuel_gauge_status_string(status, &psstatus);
            printf("%s: fuel_gauge_get_charge_percent() failed %d(%s)\n", __FUNCTION__, status, psstatus);
            if (retry_count++ > FUEL_GAUGE_RETRY_COUNT)
            {
                printf("%s: FATAL ERROR! Unable to restore communicati0on with the fuel gauge, aborting!\n", __FUNCTION__);
                break;
            }

            // let's try to reset the fuel gauge and recover
            status = fuel_gauge_power_on_reset(fgh);
            if (status != FUEL_GAUGE_STATUS_SUCCESS)
            {
                fuel_gauge_status_string(status, &psstatus);
                printf("%s: fuel_gauge_power_on_reset() failed %d(%s)\n", __FUNCTION__, status, psstatus);
                break;
            }

            usleep(RECONN_BATTERY_MONITOR_SLEEP);
            continue;
        }
        else if (retry_count)
        {
            retry_count = 0;
        }

        if(batteryPercentage < 5)
        {
            if(ledColor == RED)
            {
                // we are less than 5% battery power so we have to flash
                // the battery status LED between Red and off at a rate
                // of 1 second.
                reconnGpioAction(BATTERY_LED_RED_GPIO, DISABLE);
                ledColor = OFF;
            }
            else
            {
                reconnGpioAction(BATTERY_LED_RED_GPIO, ENABLE);
                reconnGpioAction(BATTERY_LED_GREEN_GPIO, DISABLE);
                ledColor = RED;
            }
        }
        else if(batteryPercentage < 10)
        {
            if(ledColor != RED)
            {
                reconnGpioAction(BATTERY_LED_RED_GPIO, ENABLE);
                reconnGpioAction(BATTERY_LED_GREEN_GPIO, DISABLE);
                ledColor = RED;
            }
            else if(chargerAttached == TRUE)
            {
                reconnGpioAction(BATTERY_LED_RED_GPIO, DISABLE);
                ledColor = OFF;
            }
        }
        else if(batteryPercentage > 95)  // 5% tolerance fudge
        {
            if(ledColor != GREEN)
            {
                reconnGpioAction(BATTERY_LED_GREEN_GPIO, ENABLE);
                reconnGpioAction(BATTERY_LED_RED_GPIO, DISABLE);
                ledColor = GREEN;
            }
        }
        else
        {
            if(ledColor != GREEN)
            {
                reconnGpioAction(BATTERY_LED_GREEN_GPIO, ENABLE);
                reconnGpioAction(BATTERY_LED_RED_GPIO, DISABLE);
                ledColor = GREEN;
            }
            else if(chargerAttached == TRUE)
            {
                reconnGpioAction(BATTERY_LED_GREEN_GPIO, DISABLE);
                ledColor = OFF;
            }
        }
        // TODO need to get GPI 137 to determine if charger is attached
#ifndef __SIMULATION__
        if(dcPowerGpioFp)
            fread(&chargerAttached, 1, 1, dcPowerGpioFp);
#endif
        usleep((chargerAttached == TRUE) ? RECONN_BATTERY_MONITOR_SLEEP : RECONN_BATTERY_MONITOR_SLEEP/2 );
    }

    status = fuel_gauge_close_dev(fgh);
    if (status != FUEL_GAUGE_STATUS_SUCCESS)
    {
        fuel_gauge_status_string(status, &psstatus);
        printf("%s: fuel_gauge_close() failed %d(%s)\n", __FUNCTION__, status, psstatus);
    }
    status = fuel_gauge_uninit(fgh);
    if (status != FUEL_GAUGE_STATUS_SUCCESS)
    {
        fuel_gauge_status_string(status, &psstatus);
        printf("%s: fuel_gauge_unit() failed %d(%s)\n", __FUNCTION__, status, psstatus);
    }

    if(dcPowerGpioFp)
    {
        fclose(dcPowerGpioFp);
    }
    return &retCode;
}


void resetPowerStandbyCounter(PowerMgmtEqptType theEqpt)
{
    switch(theEqpt)
    {
        case RESET_POWER_METER_STBY_COUNTER:
        {
            eqptStbyCounters.PowerMeterCounter = RECONN_POWER_CONSERVATION_TIME;
            break;
        }
        case RESET_DMM_STBY_COUNTER:
        {
            eqptStbyCounters.DmmCounter = RECONN_POWER_CONSERVATION_TIME;
            break;
        }
        case RESET_GPS_STBY_COUNTER:
        {
            eqptStbyCounters.GpsCounter = RECONN_POWER_CONSERVATION_TIME;
            break;
        }
        case RESET_SPECTRUM_ANALYZER_STBY_COUNTER:
        {
            eqptStbyCounters.SpectrumAnalyzerCounter = RECONN_POWER_CONSERVATION_TIME;
            break;
        }
        case RESET_SYSTEM_SHUTDOWN_TIME:
        {
            break;
        }
        default:
        {
            printf("%s: Invalid eqpt type %d\n", __FUNCTION__, theEqpt);
        }
    }
    eqptStbyCounters.ReconnSystemCounter = RECONN_POWER_DOWN_TIME;
}
