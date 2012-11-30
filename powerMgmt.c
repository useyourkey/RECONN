//******************************************************************************
//******************************************************************************
//
// FILE:        powerMgmt.c
//
// FUNCTIONS:   reconnPwrMgmtTask()       
//              reconnBatteryMonTask()
//              resetPowerStandbyCounter()
//              getStandbyCounters()
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
#include <mqueue.h>
#include <net/if.h>

#include "reconn.h"
#include "powerMgmt.h"
#include "clientApp.h"
#include "gpio.h"
#include "fuel_gauge.h"
#include "debugMenu.h"
#include "spectrum.h"
#include "dmm.h"
#include "eqptResponse.h"

// counters to keep track of when a piece of equipment should go into power conservation 
// mode. I.E. powered down
static PowerMgmtEqptCounters eqptStbyCounters; 
char gChargerState = NOT_ATTACHED;
extern ReconnEqptDescriptors modeAndEqptDescriptors;
extern YESNO swUpgradeInProgress;

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
void *reconnPwrMgmtTask(void *args) 
{
    int i, *aEqptCounter;
    int countersReset = FALSE;
    ReconnEqptDescriptors *eqptDescriptors = (ReconnEqptDescriptors *)args;

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
        if (gChargerState == NOT_ATTACHED)
        {
            if((--eqptStbyCounters.DmmCounter) == 0)
            {
                dmmPowerDown();
                reconnDebugPrint("%s: DMM is going into Conservation Mode\n", __FUNCTION__);
            }
            if((--eqptStbyCounters.SpectrumAnalyzerCounter) == 0)
            {
                reconnGpioAction(POWER_18V_GPIO, DISABLE, NULL);
                reconnDebugPrint("%s: Spectrum Analyzer is going into Conservation Mode\n", __FUNCTION__);
                SpectrumAnalyzerClose(&(eqptDescriptors->analyzerFd));
            }

            if(!(--eqptStbyCounters.ReconnSystemCounter))
            {
                reconnDebugPrint("%s: Powering Down the system\n", __FUNCTION__);
                system("echo 0x1 > /sys/class/gpio/gpio156/value");
                system("echo 0x8 > /proc/cpld/CPLD_Tx_DIR_setbit");
            }
            countersReset = FALSE;
        }
        else if(countersReset == FALSE)
        {
            eqptStbyCounters.ReconnSystemCounter = RECONN_POWER_DOWN_TIME;

            aEqptCounter = &eqptStbyCounters.PowerMeterCounter;
            for(i = 0; i <= RESET_SPECTRUM_ANALYZER_STBY_COUNTER; i++, aEqptCounter++)
            {
                *aEqptCounter = RECONN_POWER_CONSERVATION_TIME;
            }
            countersReset = TRUE;
        }
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
uint8_t gBatteryPercentage;
float gBatteryVoltage;
char thermistorValue = TEMP_IN_RANGE;

void *reconnBatteryMonTask(void *argument)
{
    static char retCode = '1';
#ifndef __SIMULATION__
    PowerMgmtLedColors ledColor = OFF;
    FILE *dcPowerGpioFp = NULL;
    FILE *dcThermistorGpioFp;
    fuel_gauge_status_t status;
    fuel_gauge_status_t percentStatus;
    fuel_gauge_status_t voltageStatus;
    fuel_gauge_handle_t fgh;
    fuel_gauge_context_t fgh_context;
    const char *psstatus;
    int retry_count = 0;
    short chargeState = CHARGE_NO_STATE;
    unsigned char theMessage[MASTER_MSG_SIZE];
    uint16_t batteryMilliVolts;

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
        reconnDebugPrint("%s: exiting\n", __FUNCTION__);
        return &retCode;
    }
    status = fuel_gauge_open_dev(fgh, FUEL_GAUGE_DEVICE_I2C_BUS);
    if (status != FUEL_GAUGE_STATUS_SUCCESS)
    {
        fuel_gauge_status_string(status, &psstatus);
        reconnDebugPrint("%s: fuel_gauge_open() failed %d(%s)\n", __FUNCTION__, status, psstatus);
        fuel_gauge_uninit(fgh);
        reconnDebugPrint("%s: exiting\n", __FUNCTION__);
        return &retCode;
    }

    while(swUpgradeInProgress == NO)
    {
        percentStatus = fuel_gauge_get_charge_percent(fgh, &gBatteryPercentage);
        voltageStatus = fuel_gauge_get_battery_voltage(fgh, &batteryMilliVolts);

        /*
         * the MAXIM 17041 fuel gauge stores the battery voltage in 2.5Mv units.
         * So, the call to fuel_gauge_get_battery_voltage() returns the number
         * of 2.5Mv units measured across the battery.  Change the Mv to Volts.
         */
        gBatteryVoltage = batteryMilliVolts * .0025;

        if((voltageStatus != FUEL_GAUGE_STATUS_SUCCESS || (percentStatus != FUEL_GAUGE_STATUS_SUCCESS)))
        {
            if(percentStatus != FUEL_GAUGE_STATUS_SUCCESS)
            {
                fuel_gauge_status_string(percentStatus, &psstatus);
                reconnDebugPrint("%s: fuel_gauge_get_charge_percent() failed %d(%s)\n", __FUNCTION__, percentStatus, psstatus);
                /*
                 * The battery percentage failed. Set the percentage to some known value that will
                 * flash the battery status led RED this giving the user an indication that something
                 * is wrong with the battery.
                 */
                gBatteryPercentage = 7;
            }
            if(voltageStatus != FUEL_GAUGE_STATUS_SUCCESS)
            {
                fuel_gauge_status_string(voltageStatus, &psstatus);
                reconnDebugPrint("%s: fuel_gauge_get_battery_voltage() failed %d(%s)\n", __FUNCTION__, voltageStatus, psstatus);
                gBatteryVoltage = 0;
            }

            if (retry_count++ > FUEL_GAUGE_RETRY_COUNT)
            {
                reconnDebugPrint("%s: FATAL ERROR! Unable to restore communicati0on with the fuel gauge, aborting!\n", __FUNCTION__);
                reconnGpioAction(BATTERY_LED_RED_GPIO, ENABLE, NULL);
                reconnGpioAction(BATTERY_LED_GREEN_GPIO, DISABLE, NULL);
                break;
            }

            // let's try to reset the fuel gauge and recover
            status = fuel_gauge_power_on_reset(fgh);
            if (status != FUEL_GAUGE_STATUS_SUCCESS)
            {
                fuel_gauge_status_string(status, &psstatus);
                reconnDebugPrint("%s: fuel_gauge_power_on_reset() failed %d(%s)\n", __FUNCTION__, status, psstatus);
                reconnGpioAction(BATTERY_LED_RED_GPIO, ENABLE, NULL);
                reconnGpioAction(BATTERY_LED_GREEN_GPIO, DISABLE, NULL);
                break;
            }

            usleep(RECONN_BATTERY_MONITOR_SLEEP);
            continue;
        }
        else if (retry_count)
        {
            retry_count = 0;
        }

        if((dcPowerGpioFp = fopen(RECONN_DC_POWER_GPIO_FILENAME, "r")))
        {
            // The reading of the GPIO "value" file returns ascii 0 or 1
            fread(&gChargerState, 1, 1, dcPowerGpioFp);
            fclose(dcPowerGpioFp);
            dcPowerGpioFp = NULL;
        }
        else
        {
            reconnDebugPrint("%s: fopen(RECONN_DC_POWER_GPIO_FILENAME, r) failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
            gChargerState = NOT_ATTACHED;
        }
#if 0
        if((gSimulateLowBattery == YES) || ((gBatteryPercentage < 5) && (gChargerState == NOT_ATTACHED)))
        {
            // To protect the battery pack we have to power down the reconn unit
            // if the charge percentage falls below 5%
            if(masterClientMsgQid != -1)
            {
                memset( &theMessage, 0, MASTER_MSG_SIZE);
                theMessage[0] = LOW_BATTERY;
                if(mq_send(masterClientMsgQid, (const char *)&theMessage, MASTER_MSG_SIZE, 0) != 0)
                {
                    reconnDebugPrint("%s: mq_send(%d) failed. %d(%s)\n", __FUNCTION__, errno, strerror(errno), masterClientMsgQid);
                }
                else
                {
                    reconnDebugPrint("%s: mq_send(%d) success.\n", __FUNCTION__, masterClientMsgQid);
                }
            }
            dmmSaveConfig();
            /*
             * Wait for all clients to resign. If they do not resign within some reasonable amount of 
             * time probably they probably won't, so shutdown anyway.
             */
            retry_count = 20;
            while((reconnClientsRegistered(ALL) > 0) && (retry_count))
            {
                if(gSimulateLowBattery == YES) 
                {
                    reconnDebugPrint("%s: number of clients remaining %d\n", __FUNCTION__, reconnClientsRegistered(ALL));
                }
                usleep(500000);
                retry_count--;
            }

            reconnDebugPrint("%s: battery level < 5%. Killing system power.\n", __FUNCTION__);

            if(gSimulateLowBattery == YES) 
            {
                reconnDebugPrint("%s: system(echo 0x1 > /sys/class/gpio/gpio156/value)\n", __FUNCTION__);
            }
            /*
             * Set this GPIO to kill power to the box.
             */
            system("echo 0x1 > /sys/class/gpio/gpio156/value");

            if(gSimulateLowBattery == YES) 
            {
                reconnDebugPrint("%s: system(echo 0x8 > /proc/cpld/CPLD_Tx_DIR_setbit)\n", __FUNCTION__);
            }
            /*
             * Setting bit 3 in the CPLD register makes gpio156 an output and thus kills power to the box.
             */
            system("echo 0x8 > /proc/cpld/CPLD_Tx_DIR_setbit");
        }
        else if(gBatteryPercentage == 5)
#else
            if(gBatteryPercentage == 5)
#endif
        {
            if(ledColor == RED)
            {
                // we are at 5% battery charge percentage so we have to flash
                // the battery status LED Red.
                reconnGpioAction(BATTERY_LED_RED_GPIO, DISABLE, NULL);
                ledColor = OFF;
            }
            else
            {
                reconnGpioAction(BATTERY_LED_RED_GPIO, ENABLE, NULL);
                reconnGpioAction(BATTERY_LED_GREEN_GPIO, DISABLE, NULL);
                ledColor = RED;
            }
        }
        else if(gBatteryPercentage < 10)
        {
            if(ledColor != RED)
            {
                reconnGpioAction(BATTERY_LED_RED_GPIO, ENABLE, NULL);
                reconnGpioAction(BATTERY_LED_GREEN_GPIO, DISABLE, NULL);
                ledColor = RED;
            }
            else if(gChargerState == ATTACHED)
            {
                reconnGpioAction(BATTERY_LED_RED_GPIO, DISABLE, NULL);
                ledColor = OFF;
            }
        }
        else if(gBatteryPercentage > FUEL_GAUGE_MAX_PERCENTAGE) 
        {
            if(ledColor != GREEN)
            {
                reconnGpioAction(BATTERY_LED_GREEN_GPIO, ENABLE, NULL);
                reconnGpioAction(BATTERY_LED_RED_GPIO, DISABLE, NULL);
                ledColor = GREEN;
            }
        }
        else
        {
            if(ledColor != GREEN)
            {
                reconnGpioAction(BATTERY_LED_GREEN_GPIO, ENABLE, NULL);
                reconnGpioAction(BATTERY_LED_RED_GPIO, DISABLE, NULL);
                ledColor = GREEN;
            }
            else if(gChargerState == ATTACHED)
            {
                reconnGpioAction(BATTERY_LED_GREEN_GPIO, DISABLE, NULL);
                ledColor = OFF;
            }
        }
        if (gChargerState == ATTACHED)
        {
            if (gBatteryPercentage > FUEL_GAUGE_MAX_PERCENTAGE)
            {
                if(chargeState != CHARGE_DISABLED)
                {
                    /* disable charging. Enabling the GPIO shuts down charging*/
                    //reconnDebugPrint("%s: Charging circuitry disabled\n", __FUNCTION__);
                    reconnGpioAction(CHARGE_DISABLE_GPIO, ENABLE, NULL);
                    chargeState = CHARGE_DISABLED;
                }
            }
            else if (chargeState != CHARGE_ENABLED)
            {
                // Enable charging if we are within the proper temperature operating
                // range . Read the battery thermistor to make that determination. 
                //
                // NOTE:Disabling the charge GPIO enables charging. 
                if((dcThermistorGpioFp = fopen(RECONN_CHARGE_THERMISTOR_GPIO_FILENAME, "r")))
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
                    //reconnDebugPrint("%s: Charging circuitry enabled\n", __FUNCTION__);
                    reconnGpioAction(CHARGE_DISABLE_GPIO, DISABLE, NULL);
                    chargeState = CHARGE_ENABLED;
                }
                else
                {
                     reconnDebugPrint("%s:  thermistorValue(%u) is out of range\n", __FUNCTION__, thermistorValue);
                }
            }
        }
        else if (chargeState != CHARGE_DISABLED)
        {
            reconnDebugPrint("%s: Charger is not attached so disable charging circuitry\n", __FUNCTION__);
            reconnGpioAction(CHARGE_DISABLE_GPIO, ENABLE, NULL);
            chargeState = CHARGE_DISABLED;
        }

        // If the gCharger is attached then we flash LEDS at a 1 second rate otherwise 1/2 second.
        usleep((gChargerState == ATTACHED) ? RECONN_BATTERY_MONITOR_SLEEP : RECONN_BATTERY_MONITOR_SLEEP/2 );
    }

    reconnDebugPrint("%s: fuel_gauge_close_dev()\n", __FUNCTION__);
    status = fuel_gauge_close_dev(fgh);
    if (status != FUEL_GAUGE_STATUS_SUCCESS)
    {
        fuel_gauge_status_string(status, &psstatus);
        reconnDebugPrint("%s: fuel_gauge_close_dev() failed %d(%s)\n", __FUNCTION__, status, psstatus);
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
    reconnDebugPrint("%s: exiting\n", __FUNCTION__);
    return &retCode;
}


//******************************************************************************
//******************************************************************************
//
// FUNCTION:    resetPowerStandbyCounter
//
// DESCRIPTION: Interface used to reset the overall system and various equipment 
//              timers. These timer, when they expire, will shutdown their respective
//              eqpt or the system.
//
// Parameters:
//              theEqpt -  an timer enumeration
//
//******************************************************************************
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

//******************************************************************************
//******************************************************************************
//
// FUNCTION:    getStandbyCounters
//
// DESCRIPTION: Interface used by system debug menu to get a copy of the system's
//              standby counters for display.
//
// Parameters:
//              counterBuf - a data structure into which this function will write
//                           the standby values.
//
//******************************************************************************
ReconnErrCodes getStandbyCounters(PowerMgmtEqptCounters *counterBuf)
{
    ReconnErrCodes retCode = RECONN_SUCCESS;

    if(counterBuf == NULL)
    {
        reconnDebugPrint("%s: input parameter is NULL\n", __FUNCTION__);
        retCode = RECONN_INVALID_PARAMETER;
    }
    else
    {
        counterBuf->PowerMeterCounter = eqptStbyCounters.PowerMeterCounter;
        counterBuf->DmmCounter = eqptStbyCounters.DmmCounter;
        counterBuf->GpsCounter = eqptStbyCounters.GpsCounter;
        counterBuf->SpectrumAnalyzerCounter = eqptStbyCounters.SpectrumAnalyzerCounter;
        counterBuf->ReconnSystemCounter = eqptStbyCounters.ReconnSystemCounter;
    }
    return(retCode);
}
