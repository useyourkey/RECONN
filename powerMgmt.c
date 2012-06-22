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
#include "debugMenu.h"

// counters to keep track of when a piece of equipment should go into power conservation 
// mode. I.E. powered down
static PowerMgmtEqptCounters eqptStbyCounters; 

//******************************************************************************
//******************************************************************************
//
// FUNCTION:reconnPwrMgmtTask()       
//
// DESCRIPTION: This task is responsible for monitoring system resources to determine
//              if the system should place a piece of test equipment into power conservation
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

    for(i = 0; i <= RESET_SPECTRUM_ANALYZER_STBY_COUNTER; i++, aEqptCounter++)
    {
        *aEqptCounter = RECONN_POWER_CONSERVATION_TIME;
    }

    reconnDebugPrint("%s: **** Task Started\n", __FUNCTION__);
    while (1) 
    {
        sleep(60);
#if 0   // Currently eqpt power conservation is not part of the reconn feature set.
        if((--eqptStbyCounters.PowerMeterCounter) == 0)
        {
            reconnDebugPrint("%s: Power Meter is going into Conservation Mode\n", __FUNCTION__);
        }
        if((--eqptStbyCounters.DmmCounter) == 0)
        {
            reconnGpioAction(DMM_POWER_GPIO, DISABLE);
            reconnDebugPrint("%s: Dmm is going into Conservation Mode\n", __FUNCTION__);
        }
        if((--eqptStbyCounters.GpsCounter) == 0)
        {
            reconnGpioAction(GPS_ANTANNAE_GPIO, DISABLE);
            reconnGpioAction(GPS_ENABLE_GPIO, DISABLE);
            reconnDebugPrint("%s: GPS is going into Conservation Mode\n", __FUNCTION__);
        }
        if((--eqptStbyCounters.SpectrumAnalyzerCounter) == 0)
        {
            reconnGpioAction(POWER_18V_GPIO, DISABLE);
            reconnDebugPrint("%s: Spectrum Analyzer is going into Conservation Mode\n", __FUNCTION__);
        }
#endif

        if(!eqptStbyCounters.ReconnSystemCounter)
        {
            reconnDebugPrint("%s: Powering Down the system\n", __FUNCTION__);
            reconnGpioAction(POWER_5V_GPIO, DISABLE);
        }
        eqptStbyCounters.ReconnSystemCounter --;
    }
}

//******************************************************************************
//******************************************************************************
//
// FUNCTION:    reconnBatteryMonTask()       
//
// DESCRIPTION: This task is responsible for monitoring the system's battery and
//              12V charge line. This task is responsible for changing the color
//              and flash rate of the battery status LED. This task will also detect
//              the presence of a valid 12V charge voltage and when appropriate
//              enable or disable charging of the battery.
//
//******************************************************************************
uint8_t batteryPercentage;
char chargerAttached = NOT_ATTACHED;
char thermistorValue = TEMP_IN_RANGE;

void *reconnBatteryMonTask(void *argument)
{
    static char retCode = '1';
#ifndef __SIMULATION__
    PowerMgmtLedColors ledColor = OFF;
    FILE *dcPowerGpioFp;
    FILE *dcThermistorGpioFp;
    fuel_gauge_status_t status;
    fuel_gauge_handle_t fgh;
    fuel_gauge_context_t fgh_context;
    const char *psstatus;
    int retry_count;
    int chargeEnable = FALSE;

#endif
    (void) argument;  // quiet compiler

#ifndef __SIMULATION__
    reconnDebugPrint("%s: **** Task Started\n", __FUNCTION__);

    fgh = &fgh_context;
    status = fuel_gauge_init(&fgh);
    if (status != FUEL_GAUGE_STATUS_SUCCESS)
    {
        fuel_gauge_status_string(status, &psstatus);
        reconnDebugPrint("%s: fuel_gauge_init() failed %d(%s)\n", __FUNCTION__, status, psstatus);
        return &retCode;
    }
    status = fuel_gauge_open_dev(fgh, RECONN_FUEL_GAUGE_DEVICE_I2C_BUS);
    if (status != FUEL_GAUGE_STATUS_SUCCESS)
    {
        fuel_gauge_status_string(status, &psstatus);
        reconnDebugPrint("%s: fuel_gauge_open() failed %d(%s)\n", __FUNCTION__, status, psstatus);
        fuel_gauge_uninit(fgh);
        return &retCode;
    }

    while(1)
    {
        status = fuel_gauge_get_charge_percent(fgh, &batteryPercentage);
        if (status != FUEL_GAUGE_STATUS_SUCCESS)
        {
            fuel_gauge_status_string(status, &psstatus);
            reconnDebugPrint("%s: fuel_gauge_get_charge_percent() failed %d(%s)\n", __FUNCTION__, status, psstatus);
            if (retry_count++ > FUEL_GAUGE_RETRY_COUNT)
            {
                reconnDebugPrint("%s: FATAL ERROR! Unable to restore communicati0on with the fuel gauge, aborting!\n", __FUNCTION__);
                reconnGpioAction(BATTERY_LED_RED_GPIO, ENABLE);
                reconnGpioAction(BATTERY_LED_GREEN_GPIO, DISABLE);
                break;
            }

            // let's try to reset the fuel gauge and recover
            status = fuel_gauge_power_on_reset(fgh);
            if (status != FUEL_GAUGE_STATUS_SUCCESS)
            {
                fuel_gauge_status_string(status, &psstatus);
                reconnDebugPrint("%s: fuel_gauge_power_on_reset() failed %d(%s)\n", __FUNCTION__, status, psstatus);
                reconnGpioAction(BATTERY_LED_RED_GPIO, ENABLE);
                reconnGpioAction(BATTERY_LED_GREEN_GPIO, DISABLE);
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
            // To protect the battery pack we have to power down the reconn unit
            // if the charge percentage falls below 5%
            reconnGpioAction(POWER_5V_GPIO, ENABLE);
        }
        else if(batteryPercentage == 5)
        {
            if(ledColor == RED)
            {
                // we are at 5% battery charge percentage so we have to flash
                // the battery status LED Red and off.
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
            else if(chargerAttached == ATTACHED)
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
            else if(chargerAttached == ATTACHED)
            {
                reconnGpioAction(BATTERY_LED_GREEN_GPIO, DISABLE);
                ledColor = OFF;
            }
        }
        // TODO need to get GPIO 137 to determine if charger is attached
        if(dcPowerGpioFp = fopen(RECONN_DC_POWER_GPIO_FILENAME, "r"))
        {
            // The reading of the GPIO "value" file returns ascii 0 or 1
            fread(&chargerAttached, 1, 1, dcPowerGpioFp);
            fclose(dcPowerGpioFp);
        }
        else
        {
            //reconnDebugPrint("%s: fopen(RECONN_DC_POWER_GPIO_FILENAME, r) failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
            chargerAttached = NOT_ATTACHED;
        }

        if (chargerAttached == ATTACHED)
        {
            // 5% tolerance fudge
            if (batteryPercentage > 95)
            {
                if(chargeEnable)
                {
                    /* disable charging. Enabling the GPIO shuts down charging*/
                    reconnGpioAction(CHARGE_DISABLE_GPIO, ENABLE);
                    chargeEnable = FALSE;
                }
            }
            else if (!chargeEnable)
            {
                // Enable charging if we are within the proper temperature operating
                // range . Read the battery thermistor to make that determination. 
                //
                // NOTE:Disabling the charge GPIO enables charging. 
                if(dcThermistorGpioFp = fopen(RECONN_CHARGE_THERMISTOR_GPIO_FILENAME, "r"))
                {
                    // The reading of the GPIO "value" file returns ascii 0 or 1
                    if(fread(&thermistorValue, 1, 1, dcThermistorGpioFp) == 0)
                    {
                        reconnDebugPrint("%s: fread(&thermistorValue, 1, 1, dcThermistorGpioFp) failed \n", __FUNCTION__, errno, strerror(errno));
                    }
                    fclose(dcThermistorGpioFp);
                }
                else
                {
                    reconnDebugPrint("%s: fopen(RECONN_CHARGE_THERMISTOR_GPIO_FILENAME, r) failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
                }
                if(thermistorValue == TEMP_IN_RANGE)
                {
                    reconnGpioAction(CHARGE_DISABLE_GPIO, DISABLE);
                    chargeEnable = TRUE;
                }
            }
        }
        else if (chargeEnable)
        {
            reconnGpioAction(CHARGE_DISABLE_GPIO, ENABLE);
            chargeEnable = FALSE;
        }

        // If the charger is attached then we flash LEDS at a 1 second rate otherwise 1/2 second.
        usleep((chargerAttached == ATTACHED) ? RECONN_BATTERY_MONITOR_SLEEP : RECONN_BATTERY_MONITOR_SLEEP/2 );
    }

    status = fuel_gauge_close_dev(fgh);
    if (status != FUEL_GAUGE_STATUS_SUCCESS)
    {
        fuel_gauge_status_string(status, &psstatus);
        reconnDebugPrint("%s: fuel_gauge_close() failed %d(%s)\n", __FUNCTION__, status, psstatus);
    }
    status = fuel_gauge_uninit(fgh);
    if (status != FUEL_GAUGE_STATUS_SUCCESS)
    {
        fuel_gauge_status_string(status, &psstatus);
        reconnDebugPrint("%s: fuel_gauge_unit() failed %d(%s)\n", __FUNCTION__, status, psstatus);
    }

    if(dcPowerGpioFp)
    {
        fclose(dcPowerGpioFp);
    }
#else
    reconnDebugPrint("%s: **** Task does not run during simulation\n", __FUNCTION__);
#endif
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
            reconnDebugPrint("%s: Invalid eqpt type %d\n", __FUNCTION__, theEqpt);
        }
    }
    eqptStbyCounters.ReconnSystemCounter = RECONN_POWER_DOWN_TIME;
}
