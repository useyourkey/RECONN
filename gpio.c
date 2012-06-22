//******************************************************************************
//******************************************************************************
//
// FILE:        gpio.c
//
// DESCRIPTION: This file contains functions to enable or disable the reconn GPIOs
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
#include <string.h>
#include <errno.h>

#include "reconn.h"
#include "gpio.h"
#include "debugMenu.h"

typedef enum
{
    IN,
    OUT
}GPIO_DIRECTION;

typedef struct 
{
    char    *gpioValueFileName;
    char    *gpioDirectionFileName;
}GpioStruct;
#define GPIOROOT "/sys/class/gpio/"

// If GPIOs need to be added they must be added to gpio.h first, then to this table. The table indices
// here must match the enums in reconnGpio.h . Once those changes have been made roots/bin/autoexec.brc must
// be updated to provision the GPIO as an input or output.
char *GpioDebugNames [] = {
    "56","57","128","137","140","141","144","145","146","147","156","157","158","159","161","162","171","172","174","174"};

GpioStruct GpioTable [] = {
    {GPIOROOT"gpio56/value", GPIOROOT"gpio56/direction"},   // GPIO 56 Apple Auth Chip SPI_NSS/Mode2
    {GPIOROOT"gpio57/value", GPIOROOT"gpio57/direction"},   // GPIO 57 WiFi Enable 
    {GPIOROOT"gpio128/value", GPIOROOT"gpio128/direction"}, // GPIO 128 Enable 1.8V power for GPS ant
    {GPIOROOT"gpio137/value", GPIOROOT"gpio137/direction"}, // GPIO 137 DC Input from T5-8 Connectors
    {GPIOROOT"gpio140/value", GPIOROOT"gpio140/direction"}, // GPIO 140 Enable 3.3V power for LNB
    {GPIOROOT"gpio141/value", GPIOROOT"gpio141/direction"}, // GPIO 141 Enable 18V
    {GPIOROOT"gpio144/value", GPIOROOT"gpio144/direction"}, // GPIO 144 LNB 10MHz
    {GPIOROOT"gpio145/value", GPIOROOT"gpio145/direction"}, // GPIO 145 Charging Disable
    {GPIOROOT"gpio146/value", GPIOROOT"gpio146/direction"}, // GPIO 146 Spectrum Analyzer 10MHZ
    {GPIOROOT"gpio147/value", GPIOROOT"gpio147/direction"}, // GPIO 147 Enable 3.3V GPS power
    {GPIOROOT"gpio156/value", GPIOROOT"gpio156/direction"}, // GPIO 156 Disable 5V power
    {GPIOROOT"gpio157/value", GPIOROOT"gpio157/direction"}, // GPIO 157 Enable 12V power
    {GPIOROOT"gpio158/value", GPIOROOT"gpio158/direction"}, // GPIO 158 Battery Termo status
    {GPIOROOT"gpio159/value", GPIOROOT"gpio159/direction"}, // GPIO 159 Enable 3.3V power
    {GPIOROOT"gpio161/value", GPIOROOT"gpio161/direction"}, // GPIO 161 Enable 1.8V for Wifi Power
    {GPIOROOT"gpio162/value", GPIOROOT"gpio162/direction"}, // GPIO 162 Power Button pressed
    {GPIOROOT"gpio171/value", GPIOROOT"gpio171/direction"}, // GPIO 171 Green status LED
    {GPIOROOT"gpio172/value", GPIOROOT"gpio172/direction"}, // GPIO 172 Main power status LED
    {GPIOROOT"gpio173/value", GPIOROOT"gpio173/direction"}, // GPIO 173 Red status LED
    {GPIOROOT"gpio174/value", GPIOROOT"gpio174/direction"}, // GPIO 174 DMM Power
};

ReconnErrCodes reconnGpioAction(GpioNames theGpio, GpioAction theAction)
{
    ReconnErrCodes retCode = RECONN_SUCCESS;
#ifndef __SIMULATION__
    char *gpioPathPtr;
    int theGpioValue;
    FILE *gpioFd;
#endif

#ifdef DEBUG_GPIO
    reconnDebugPrint("%s: Function Entered with theGpio %s, theAction %s\n", __FUNCTION__, GpioDebugNames[theGpio], (theAction == DISABLE) ? "DISABLE" : "ENABLE");
#endif
    if((theAction != ENABLE) && (theAction != DISABLE))
    {
        reconnDebugPrint("%s: Invalid Action %d\n", __FUNCTION__, theAction);
        retCode = RECONN_FAILURE;
    }
    else if(theGpio > GPIO_175)
    {
        reconnDebugPrint("%s: Invalid gpio name %d\n", __FUNCTION__, theGpio);
        retCode = RECONN_FAILURE;
    }
    else
    {
#ifndef __SIMULATION__

        if((theGpio == GPIO_156) && (theAction == DISABLE))
        {
            gpioPathPtr = GPIOROOT"export";

#ifdef DEBUG_GPIO
            reconnDebugPrint("%s: Calling fopen(%s, w)\n", __FUNCTION__, gpioPathPtr);
#endif
            if((gpioFd = fopen(gpioPathPtr, "w")) != NULL)
            {
                // To kill power to the Reconn device, create the sys/class gpio for 156 and set its direction 
                // to out. The default for "value" is 0. So when the direction is set, GPIO156 is will be 0
                // this killing 5 and 3.3 V to the PCB.
#ifdef DEBUG_GPIO
                reconnDebugPrint("%s: Calling fputs(%s, %d)\n", __FUNCTION__, "156", (int)gpioFd);
#endif
                fputs("156", gpioFd);

                fclose(gpioFd);

                gpioPathPtr = GpioTable[theGpio].gpioDirectionFileName;
#ifdef DEBUG_GPIO
                reconnDebugPrint("%s: Calling fopen(%s, w+)\n", __FUNCTION__, gpioPathPtr);
#endif
                if((gpioFd = fopen(gpioPathPtr, "w+")) != NULL) 
                {
#ifdef DEBUG_GPIO
                    reconnDebugPrint("%s: Calling fputs(%s, %d)\n", __FUNCTION__, "out", (int)gpioFd);
#endif
                    fputs("out", gpioFd);
                    fclose(gpioFd);
                }
                else
                {
                    reconnDebugPrint("%s:  fopen(%s, w) failed %d(%s)]n", __FUNCTION__, gpioPathPtr, errno, strerror(errno));
                    retCode = RECONN_FAILURE;
                }
            }
            else
            {
                reconnDebugPrint("%s:  fopen(%s, w+) failed %d(%s)\n", __FUNCTION__, gpioPathPtr, errno, strerror(errno));
                retCode = RECONN_FAILURE;
            }
        }
        else
        {
            gpioPathPtr = GpioTable[theGpio].gpioValueFileName;
#ifdef DEBUG_GPIO
            reconnDebugPrint("%s: calling fopen(%s, \"w+\")\n", __FUNCTION__, gpioPathPtr);
#endif
            if((gpioFd = fopen(gpioPathPtr, "w+")) != NULL)
            {
#ifdef DEBUG_GPIO
                reconnDebugPrint("%s: Calling fputc(%c, %d)\n", __FUNCTION__, (theAction == ENABLE) ? '1':'0', (int)gpioFd);
#endif
                fputc((theAction == ENABLE) ? '1':'0', gpioFd);
                if((theGpioValue = fgetc(gpioFd)) == EOF)
                {
                    reconnDebugPrint("%s, %d: fgetc() for %s failed %d(%s)\n", __FUNCTION__, __LINE__, gpioPathPtr, errno, strerror(errno));
                    retCode = RECONN_FAILURE;
                }
                else
                {
#ifdef DEBUG_GPIO
                    reconnDebugPrint("%s, %d: fgetc() returned %d\n", __FUNCTION__, __LINE__, (char)theGpioValue);
#endif
                }
                fclose(gpioFd);
            }
            else
            {
                reconnDebugPrint("%s:  fopen(%s, w) failed %d(%s)\n", __FUNCTION__, gpioPathPtr, errno, strerror(errno));
                retCode = RECONN_FAILURE;
            }
        }
#endif
    }
#ifdef DEBUG_GPIO
    reconnDebugPrint("%s: Function returning %s \n", __FUNCTION__, (retCode == RECONN_SUCCESS) ? "SUCCESS" : "FAILURE");
#endif
    return retCode;
}
