//******************************************************************************
//******************************************************************************
//
// FILE:       powermeter.h
//
// CLASSES:     
//
// DESCRIPTION: Header file for the Power Meter module
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
#ifndef __POWERMETER_H
#define __POWERMETER_H

#define POWER_METER_DEV      "/dev/AvcomMeter"
#define POWER_METER_BAUD        B115200
#define POWER_METER_DATABITS    CS8
#define POWER_METER_STOPBITS    0
#define POWER_METER_BAUD_RATE   B115200
#define POWER_METER_DATABITS    CS8
#define POWER_METER_STOPBITS    0
#define POWER_METER_PARITYON    0
#define POWER_METER_PARITY      0
#define POWER_METER_PARITYON    0
#define POWER_METER_PARITY      0
#define POWER_METER_COMMAND     "H\r"

#define POWER_METER_SCAN_SLEEP 1

ReconnErrCodes powerMeterInit(int *);
ReconnErrCodes powerMeterWrite(unsigned char *buffer, int length);
ReconnErrCodes powerMeterRead(unsigned char *buffer, int *length);
int pm_command_processing(unsigned char *command, int length, unsigned char *buffer,
		int *outlength);
int makePowerMeterOutput(unsigned char *pm_outputbuffer, int *pm_outputlength);
extern void *powerMeterPresenceTask(void *args);
#endif /* __POWERMETER_H */
