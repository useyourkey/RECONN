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
#include <sys/stat.h>
#include <string.h>

#include "reconn.h"
#include "powerMeter.h"
#include "gpio.h"
#include "debugMenu.h"
#include "socket.h"

int myMeterFd;
struct termios meterSerial;

int pm_wait_flag = TRUE;
static int PowerMeterPortInit = FALSE;

ReconnErrCodes powerMeterClose(int *fileDescriptor)
{
#ifdef DEBUG_EQPT
    reconnDebugPrint("%s: Function Entered\n", __FUNCTION__);
#endif
    if(fileDescriptor != -1)
    {
        close(fileDescriptor);
        PowerMeterPortInit = FALSE;
        *fileDescriptor = -1;
        return RECONN_SUCCESS;
    }
    else
    {
        reconnDebugPrint("%s: bad fileDescriptor %d\n", __FUNCTION__, fileDescriptor);
        return RECONN_FAILURE;
    }
}
static ReconnErrCodes powerMeterOpen(int *fileDescriptor) 
{
    ReconnErrCodes retCode = RECONN_SUCCESS;
    int bytesRead, dataLength = 200;
    unsigned char meterData[dataLength];

    reconnDebugPrint("%s: Function Entered\n", __FUNCTION__);
    if((myMeterFd = open(POWER_METER_DEV, O_RDWR | O_NOCTTY | O_NONBLOCK)) < 0)
    {
        reconnDebugPrint("%s: Failed to open %s\n", __FUNCTION__, POWER_METER_DEV);
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

        memset(meterData, 0, dataLength);
        reconnDebugPrint("%s: Calling write \n", __FUNCTION__);
        write(myMeterFd, POWER_METER_COMMAND, 2);
        sleep(1);
        if((bytesRead = read(myMeterFd, &meterData, dataLength)) < 0)
        {
            reconnDebugPrint("%s: read failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
            retCode = RECONN_FAILURE;
        }
        else if(strstr((const char *)&meterData, "Power Meter"))
        {
            PowerMeterPortInit = TRUE;
            reconnDebugPrint("%s: Power Meter Init Complete\r\n", __FUNCTION__);
        }
        else
        {
            printf("%s: power meter responded bytesRead = %d (%s)\n", __FUNCTION__, bytesRead, meterData);
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
//******************************************************************************
//****************************************************************************** //
// FUNCTION:        powerMeterPresenceTask
//
// DESCRIPTION: This functions sole purpose is to detect the insertion or extraction
//              of the power meter then notify the master client of the event. 
//
//******************************************************************************
void *powerMeterPresenceTask(void *args)
{
    static int status = 0;
    struct stat theStatus;
    int insertMessageSent = FALSE;
    int extractMessageSent = FALSE;
    int meterInserted = FALSE;
    extern int masterClientSocketFd;
    extern int insertedMasterSocketFd;
    extern ReconnModeAndEqptDescriptors modeAndEqptDescriptors;

    UNUSED_PARAM(args);

    reconnDebugPrint("%s: ***** Started\n", __FUNCTION__);
    while(1)
    {
        /*
         * If there is a master client connected check the power meter status.
         */
        if((masterClientSocketFd != -1)  || (insertedMasterSocketFd != -1))
        {
            if(stat(POWER_METER_DEV, &theStatus) < 0)
            {
                if(errno == ENOENT)
                {
                    if(meterInserted == TRUE)
                    {
                        // The meter has been extracted
                        if(extractMessageSent == FALSE)
                        {
                            reconnDebugPrint("%s: Sending extract message\n", __FUNCTION__);
                            modeAndEqptDescriptors.powerMeterFd = -1;
                            powerMeterClose(&(modeAndEqptDescriptors.powerMeterFd));
                            if(insertedMasterSocketFd != -1)
                            {
                                sendReconnResponse(insertedMasterSocketFd, 
                                        (PMETER_STATUS_NOTIF & 0xff00) >> 8,
                                        (PMETER_STATUS_NOTIF & 0x00ff), 
                                        0, INSERTEDMASTERMODE);
                            }
                            else
                            {
                                sendReconnResponse(masterClientSocketFd, 
                                        (PMETER_STATUS_NOTIF & 0xff00) >> 8,
                                        (PMETER_STATUS_NOTIF & 0x00ff), 
                                        0, MASTERMODE);
                            }
                            extractMessageSent = TRUE;
                            insertMessageSent = FALSE;
                            meterInserted = FALSE;
                        }
                    }
                }
                else
                {
                    reconnDebugPrint("%s: stat(%s) failed %d (%s)\n", __FUNCTION__, POWER_METER_DEV, errno, strerror(errno));
                }
            }
            else
            {
                /*
                 * The meter is inserted. If a notification message has not 
                 * been sent, send it.
                 */
                if(insertMessageSent == FALSE)
                {
                    if(powerMeterInit(&(modeAndEqptDescriptors.powerMeterFd)) == RECONN_SUCCESS)
                    {
                        reconnDebugPrint("%s: Sending insertion message for powerMeterFd %d\n", __FUNCTION__, modeAndEqptDescriptors.powerMeterFd);
                        if(insertedMasterSocketFd != -1)
                        {
                            sendReconnResponse(insertedMasterSocketFd, 
                                    (PMETER_STATUS_NOTIF & 0xff00) >> 8,
                                    (PMETER_STATUS_NOTIF & 0x00ff), 
                                    1, INSERTEDMASTERMODE);
                        }
                        else
                        {
                            sendReconnResponse(masterClientSocketFd, 
                                    (PMETER_STATUS_NOTIF & 0xff00) >> 8,
                                    (PMETER_STATUS_NOTIF & 0x00ff), 
                                    1, MASTERMODE);
                        }
                        insertMessageSent = TRUE;
                        extractMessageSent = FALSE;
                        meterInserted = TRUE;
                    }
                    else
                    {
                        reconnDebugPrint("%s: something inserted that is not a power meter\n", __FUNCTION__);
                    }
                }
            }
        }
        else
        {
            insertMessageSent  = FALSE;
            extractMessageSent = FALSE;
            meterInserted = FALSE;
            if(modeAndEqptDescriptors.powerMeterFd != -1)
            {
                powerMeterClose(&(modeAndEqptDescriptors.powerMeterFd));
            }
        }
        sleep(POWER_METER_SCAN_SLEEP);
    }
    return(&status);
}
