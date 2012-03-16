//******************************************************************************
//****************************************************************************** //
// FILE:        powerMeter.c
//
// CLASSES:     
//
// DESCRIPTION: This is the file which contains functions to read, write and control
//              the power meterd
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
#include <termios.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <string.h>

#include "reconn.h"
#include "powerMeter.h"
#include "gpio.h"

int myMeterFd;
struct termios meterSerial;

int pm_wait_flag = TRUE;
int PowerMeterPortInit = FALSE;

static ReconnErrCodes powerMeterOpen(int *fileDescriptor) 
{
    ReconnErrCodes retCode = RECONN_SUCCESS;
    int bytesRead, dataLength = 200;
    unsigned char meterData[dataLength];

    printf("%s: Function Entered\n", __FUNCTION__);
    if((myMeterFd = open(POWER_METER_DEV, O_RDWR | O_NOCTTY | O_NONBLOCK)) < 0)
    {
        printf("%s: Failed to open %s\n", __FUNCTION__, POWER_METER_DEV);
        retCode = RECONN_POWER_METER_OPEN_FAIL;
        *fileDescriptor = -1;
    }
    else
    {
        *fileDescriptor = myMeterFd;
        meterSerial.c_cflag = POWER_METER_BAUD_RATE | CRTSCTS | POWER_METER_DATABITS | POWER_METER_STOPBITS
            | POWER_METER_PARITYON | POWER_METER_PARITY | CLOCAL | CREAD;
        meterSerial.c_iflag = IGNPAR;
        meterSerial.c_oflag = 0;
        meterSerial.c_lflag = 0;
        meterSerial.c_cc[VMIN] = 1;
        meterSerial.c_cc[VTIME] = 0;
        tcflush(myMeterFd, TCIFLUSH);
        tcsetattr(myMeterFd, TCSANOW, &meterSerial);

        // send "H" to the serial port and parse what is returned. If we find "Power Meter" in the
        // text returned then the device at ttyUSB1 is infact a power meter and therefore we can
        // say initializationis complete.

        printf("%s: Calling send \n", __FUNCTION__);
        memset(meterData, 0, dataLength);
        write(myMeterFd, POWER_METER_COMMAND, 2);
        sleep(1);
        if((bytesRead = read(myMeterFd, &meterData, dataLength)) < 0)
        {
            printf("%s: read failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
            retCode = RECONN_FAILURE;
        }
        else if(strstr((const char *)&meterData, "Power Meter"))
        {
            PowerMeterPortInit = TRUE;
            printf("%s: Power Meter Init Complete\r\n", __FUNCTION__);
        }
        else
        {
            retCode = RECONN_FAILURE;
        }
    }
    return (retCode);
}

ReconnErrCodes powerMeterInit(int *fileDescriptor)
{
    int ret_status = 0;
    ReconnErrCodes retcode = RECONN_SUCCESS;
    *fileDescriptor = -1;

    if((ret_status = powerMeterOpen(fileDescriptor)) == RECONN_SUCCESS)
    {
        PowerMeterPortInit = TRUE;
    }
    else
    {
        retcode = RECONN_FAILURE;
    }
    return (retcode);
}
/* generic write to usb Power Meter */
ReconnErrCodes powerMeterWrite(unsigned char *buffer, int length) 
{
    ReconnErrCodes retCode = RECONN_SUCCESS;

    if (PowerMeterPortInit == FALSE) 
    {
        retCode = RECONN_PM_PORT_NOT_INITIALIZED;
    }
    else
    {
        write(myMeterFd, buffer, length);
    }
    return (retCode);
}

/* generic read from the usb Power Meter */
ReconnErrCodes powerMeterRead(unsigned char *buffer, int *length) 
{
    int NumberBytes = 0;
    ReconnErrCodes retCode = RECONN_SUCCESS;

    if (PowerMeterPortInit == FALSE) {
        retCode = RECONN_PM_PORT_NOT_INITIALIZED;
    }
    else
    {
        *length = 0;
        if((NumberBytes = read(myMeterFd, buffer, MAX_COMMAND_INPUT)) > 0)
        {
            *length = NumberBytes;
        }
    }
    return (retCode);
}
