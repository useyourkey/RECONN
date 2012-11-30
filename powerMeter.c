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
#include <net/if.h>

#include "reconn.h"
#include "powerMeter.h"
#include "eqptResponse.h"
#include "gpio.h"
#include "debugMenu.h"
#include "socket.h"
#include "libiphoned.h"

int myMeterFd;
struct termios meterSerial;

int pm_wait_flag = TRUE;
static int PowerMeterPortInit = FALSE;

//******************************************************************************
//******************************************************************************
// FUNCTION:        powerMeterClose
//
// DESCRIPTION: This function is called to close the power meter file descriptor
//
//  PARAMETERS: fileDescriptor - pointer to the power meter filedescriptor.
//
//******************************************************************************
ReconnErrCodes powerMeterClose(int *fileDescriptor)
{
    ReconnErrCodes retCode = RECONN_SUCCESS;
#ifdef DEBUG_EQPT
    reconnDebugPrint("%s: Function Entered\n", __FUNCTION__);
#endif
    if(fileDescriptor == NULL)
    {
        reconnDebugPrint("%s: bad fileDescriptor %d\n", __FUNCTION__, fileDescriptor);
        retCode = RECONN_INVALID_PARAMETER;
    }
    else if(*fileDescriptor != -1)
    {
        if(close(*fileDescriptor) != 0)
        {
            reconnDebugPrint("%s: close(%d) failed %d (%s)\n", __FUNCTION__, *fileDescriptor, 
                    errno, strerror(errno));
        }
        PowerMeterPortInit = FALSE;
        *fileDescriptor = -1;
    }
    return(retCode);
}

//******************************************************************************
//******************************************************************************
// FUNCTION:        powerMeterOpen
//
// DESCRIPTION: Static function used to open communication to the power meter.
//
//  PARAMETERS: fileDescriptor - pointer to the power meter filedescriptor.
//
//******************************************************************************
#ifndef __SIMULATION__
static ReconnErrCodes powerMeterOpen(int *fileDescriptor) 
{
    ReconnErrCodes retCode = RECONN_SUCCESS;
    int bytesRead;
    unsigned char meterData[RECONN_RSP_PAYLOAD_SIZE];

    reconnDebugPrint("%s: Function Entered\n", __FUNCTION__);
    if((myMeterFd = open(POWER_METER_DEV, O_RDWR | O_NOCTTY | O_NONBLOCK)) < 0)
    {
        reconnDebugPrint("%s: Failed to open %s\n", __FUNCTION__, POWER_METER_DEV);
        retCode = RECONN_POWER_METER_OPEN_FAIL;
        *fileDescriptor = -1;
    }
    else
    {
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

        memset(meterData, 0, RECONN_RSP_PAYLOAD_SIZE);
        reconnDebugPrint("%s: writing H to meter\n", __FUNCTION__);
        write(myMeterFd, POWER_METER_COMMAND, strlen(POWER_METER_COMMAND));
        sleep(1);
        if((bytesRead = read(myMeterFd, &meterData, RECONN_RSP_PAYLOAD_SIZE)) < 0)
        {
            reconnDebugPrint("%s: read failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
            close(myMeterFd);
            retCode = RECONN_FAILURE;
        }
        else if(strstr((const char *)&meterData, "Power Meter"))
        {
            reconnDebugPrint("%s: Power Meter Init Complete\r\n", __FUNCTION__);
            *fileDescriptor = myMeterFd;
        }
        else
        {
            printf("%s: power meter responded bytesRead = %d %s\n", __FUNCTION__, bytesRead, (char *) &meterData);
            close(myMeterFd);
            retCode = RECONN_FAILURE;
        }
    }
    return (retCode);
}
#endif

//******************************************************************************
//******************************************************************************
// FUNCTION:        powerMeterInit
//
// DESCRIPTION: Function used to open initialize the power meter. This function
//              utilizes powerMeterOpen() to do the heavy lifting.
//
//  PARAMETERS: fileDescriptor - pointer to the power meter filedescriptor.
//
//******************************************************************************
ReconnErrCodes powerMeterInit(int *fileDescriptor)
{
#ifndef __SIMULATION__
    int ret_status = 0;
#endif
    ReconnErrCodes retcode = RECONN_SUCCESS;
    *fileDescriptor = -1;

#ifndef __SIMULATION__
    if((ret_status = powerMeterOpen(fileDescriptor)) == RECONN_SUCCESS)
    {
        PowerMeterPortInit = TRUE;
    }
    else
    {
        retcode = RECONN_FAILURE;
    }
#endif
    return (retcode);
}

//******************************************************************************
//******************************************************************************
// FUNCTION:        powerMeterWrite
//
// DESCRIPTION: Interface called to write date to the power meter. First checks to 
//              see if the meter is initialized before writing the data.
//
//  PARAMETERS: buffer - The data to be written
//              length - The data's length
//
//******************************************************************************
ReconnErrCodes powerMeterWrite(unsigned char *buffer, int length) 
{
    ReconnErrCodes retCode = RECONN_SUCCESS;
#if 0
    int i;
    unsigned char *ptr = buffer;
#endif

    if (PowerMeterPortInit == FALSE) 
    {
        retCode = RECONN_PM_PORT_NOT_INITIALIZED;
    }
    else
    {
#if 0
        reconnDebugPrint("%s: writing %d bytes ", __FUNCTION__, length);
        for(i = 0; i < length; i++, ptr++)
        {        
            reconnDebugPrint("%c", *ptr);
        }            
        reconnDebugPrint("\n");
#endif
        write(myMeterFd, buffer, length);
    }
    return (retCode);
}

//******************************************************************************
//******************************************************************************
// FUNCTION:        powerMeterRead
//
// DESCRIPTION: Interface called to read date from the power meter. First checks to 
//              see if the meter is initialized before reading the data.
//
//  PARAMETERS: buffer - a character buffer into which this interface writes the data.
//              length - The data's length
//
//******************************************************************************
ReconnErrCodes powerMeterRead(unsigned char *buffer, int *length) 
{
    int NumberBytes = 0;
    ReconnErrCodes retCode = RECONN_SUCCESS;
#if 0
    int i;
    unsigned char *ptr = buffer;

#endif

    //reconnDebugPrint("\n%s: Function Entered\n", __FUNCTION__);
    if (PowerMeterPortInit == FALSE) 
    {
        retCode = RECONN_PM_PORT_NOT_INITIALIZED;
    }
    else
    {
        *length = 0;
        if((NumberBytes = read(myMeterFd, buffer, MAX_COMMAND_INPUT)) > 0)
        {
            *length = NumberBytes;
#if 0
            reconnDebugPrint("%s: read %d bytes ", __FUNCTION__, NumberBytes);
            for(i = 0; i < NumberBytes; i++, ptr++)
            {        
                reconnDebugPrint("%c", *ptr);
            }            
            reconnDebugPrint("\n");
#endif

        }
        else
        {
            retCode = RECONN_FAILURE;
        }
    }
    //reconnDebugPrint("\n%s: Function returning %d\n", __FUNCTION__, retCode);
    return (retCode);
}
//******************************************************************************
//******************************************************************************
// FUNCTION:        powerMeterPresenceTask
//
// DESCRIPTION: This function's sole purpose is to detect the insertion and extraction
//              of the power meter then notify the master client of the event via 
//              masterClientMsgQid. 
//
//******************************************************************************
void *powerMeterPresenceTask(void *args)
{
    static int status = 0;
    int insertMessageSent = FALSE;
    int extractMessageSent = FALSE;
    int meterInserted = FALSE;
    //ReconnResponsePacket theResponsePkt;
    //ReconnResponsePacket *theResponsePktPtr = &theResponsePkt;
    ReconnEqptDescriptors *eqptDescriptors = (ReconnEqptDescriptors *)args;
    ReconnErrCodes retCode = RECONN_SUCCESS;
    //extern int masterClientSocketFd;
    //extern int insertedMasterSocketFd;
    unsigned char theMessage[MASTER_MSG_SIZE];

    reconnDebugPrint("%s: ***** Started\n", __FUNCTION__);
    while(1)
    {
        /*
         * If there is a master client connected check the power meter status.
         */
        if((retCode = isPowerMeterPresent()) == RECONN_FILE_NOT_FOUND)
        {
            if(meterInserted == TRUE)
            {
                // The meter has been extracted
                if(extractMessageSent == FALSE)
                {
                    reconnDebugPrint("%s: Sending extract message\n", __FUNCTION__);

                    memset( &theMessage, 0, MASTER_MSG_SIZE);
                    theMessage[0] = METER_EXTRACTED;
                    if(masterClientMsgQid != -1)
                    {
                        if(mq_send(masterClientMsgQid, (const char *)&theMessage, MASTER_MSG_SIZE, 0) != 0)
                        {
                            reconnDebugPrint("%s: mq_send(%d) failed. %d(%s)\n", __FUNCTION__, errno, strerror(errno), masterClientMsgQid);
                        }
                        else
                        {
                            reconnDebugPrint("%s: mq_send(%d) success.\n", __FUNCTION__, masterClientMsgQid);
                        }
                    }
                    powerMeterClose(&(eqptDescriptors->powerMeterFd));
                    extractMessageSent = TRUE;
                    insertMessageSent = FALSE;
                    meterInserted = FALSE;
                }
            }
        }
        else if(retCode == RECONN_SUCCESS)
        {
            /*
             * The meter is inserted. If a notification message has not 
             * been sent, send it.
             */
            if(insertMessageSent == FALSE)
            {
                if(meterInserted == FALSE)
                {
                    reconnDebugPrint("%s: Calling powerMeterInit()\n", __FUNCTION__);
                    retCode = powerMeterInit(&(eqptDescriptors->powerMeterFd));
                }
                if(retCode == RECONN_SUCCESS)
                {
                    reconnDebugPrint("%s: Sending insertion message for powerMeterFd %d\n", __FUNCTION__, eqptDescriptors->powerMeterFd);
                    memset( &theMessage, 0, MASTER_MSG_SIZE);
                    theMessage[0] = METER_INSERTED;
                    if(masterClientMsgQid != -1)
                    {
                        if(mq_send(masterClientMsgQid, (const char *)&theMessage, MASTER_MSG_SIZE, 0) != 0)
                        {
                            reconnDebugPrint("%s: mq_send(%d) failed. %d(%s)\n", __FUNCTION__, errno, strerror(errno), masterClientMsgQid);
                        }
                        else
                        {
                            reconnDebugPrint("%s: mq_send(%d) success.\n", __FUNCTION__, masterClientMsgQid);
                        }
                    }
                    insertMessageSent = TRUE;
                    extractMessageSent = FALSE;
                    meterInserted = TRUE;
                }
            }
        }
        usleep(POWER_METER_SCAN_SLEEP);
    }
    reconnDebugPrint("%s: ***** exiting\n", __FUNCTION__);
    return(&status);
}

//******************************************************************************
//******************************************************************************
// FUNCTION:        isPowerMeterPresent
//
// DESCRIPTION: An interface used to determine if the power meter has been inserted.
//              The embedded software uses udev to determine when a meter has been
//              inserted or extracted. There are rules in /etc/dev/rules.d/40-reconn-usb.rules
//              that create a symbolic link to /dev/AvcomMeter when an Avcom Power
//              meter is inserted into the top USB connector of the reconn back panel.
//
// RETURNS:     RECONN_SUCCESS          -   The meter has been inserted
//              RECONN_FILE_NOT_FOUND   -   The meter has not been inserted.
//              RECONN_FAILURE          -   An internal error has occured.
//******************************************************************************
ReconnErrCodes isPowerMeterPresent()
{
    ReconnErrCodes retCode = RECONN_SUCCESS;
    struct stat theStatus;

    if(stat(POWER_METER_DEV, &theStatus) < 0)
    {
        if(errno == ENOENT)
        {
            retCode = RECONN_FILE_NOT_FOUND;
        }
        else
        {
            reconnDebugPrint("%s: stat(%s) failed %d (%s)\n", __FUNCTION__, POWER_METER_DEV, errno, strerror(errno));
            retCode = RECONN_FAILURE;
        }
    }
    return (retCode);
}
