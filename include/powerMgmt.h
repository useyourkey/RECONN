//******************************************************************************
//******************************************************************************
//
// FILE:        powerMgmt.h
//
// CLASSES:     
//
// DESCRIPTION: Header file for power Management module
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
#ifndef __POWERMGMT_H
#define __POWERMGMT_H

#define RECONN_POWER_BUTTON_GPIO_FILENAME "/sys/class/gpio/gpio162/value"
#define RECONN_POWER_DOWN_TIME 40 // 40 minutes
#define RECONN_POWER_CONSERVATION_TIME 20 // minutes
#define RECONN_CHECK_POWER_SWITCH 2000 // microseconds (200 milliseconds)
#define RECONN_BATTERY_MONITOR_SLEEP 1000000 // microseconds (1 second)

#define RECONN_DC_POWER_GPIO_FILENAME "/sys/class/gpio/gpio137/value"

typedef enum
{
    POWER_BUTTON_NOT_PRESSED,
    POWER_BUTTON_PRESSED,
}PowerMgmtButtonState;

typedef enum
{
    OFF,
    RED,
    GREEN,
    FLASHING_RED,
}PowerMgmtLedColors;

typedef enum
{
    RESET_POWER_METER_STBY_COUNTER,
    RESET_DMM_STBY_COUNTER,
    RESET_GPS_STBY_COUNTER,
    RESET_SPECTRUM_ANALYZER_STBY_COUNTER,
    RESET_SYSTEM_SHUTDOWN_TIME
}PowerMgmtEqptType;

typedef struct
{
    int PowerMeterCounter;
    int DmmCounter;
    int GpsCounter;
    int SpectrumAnalyzerCounter;
    int ReconnSystemCounter;
}PowerMgmtEqptCounters;

void *reconnPwrMgmtTask(void *);
void *reconnPwrButtonTask(void *);
void resetPowerStandbyCounter(PowerMgmtEqptType);
void *reconnBatteryMonTask(void *argument);
#endif 
