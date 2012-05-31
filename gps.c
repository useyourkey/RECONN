//******************************************************************************
//******************************************************************************
//
// FILE:       gps.c
//
// CLASSES:
//
// DESCRIPTION: <DESCRIPTION>
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

#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <sys/types.h>

#include "reconn.h"
#include "gpio.h"
#include "gps.h"

int myGpsFd;
struct termios gpsSerial;
struct sigaction saio; /* definition of signal action       */
int gps_wait_flag = TRUE;
int gpsPortInit = FALSE;

static ReconnErrCodes gpsOpen( int *fileDescriptor) 
{
    ReconnErrCodes retCode = RECONN_SUCCESS;

    if((myGpsFd = open(GPS_SERIAL_PORT, O_RDWR | O_NOCTTY | O_NONBLOCK)) < 0)
    {
        /* failed to open the serial port */
        reconnDebugPrint("%s: Failed to open %s\n\r", __FUNCTION__, GPS_SERIAL_PORT);
        retCode = RECONN_SERIAL_PORT_OPEN_FAIL;
    }
    else
    {
        *fileDescriptor = myGpsFd;
        reconnDebugPrint("%s: serial port %s open.\r\n", __FUNCTION__, GPS_SERIAL_PORT);
        gpsSerial.c_cflag = GPS_BAUD_RATE | CRTSCTS | GPS_DATABITS | GPS_STOPBITS
            | GPS_PARITYON | GPS_PARITY | CLOCAL | CREAD;
        gpsSerial.c_iflag = IGNPAR;
        gpsSerial.c_oflag = 0;
        gpsSerial.c_lflag = 0;
        gpsSerial.c_cc[VMIN] = 1;
        gpsSerial.c_cc[VTIME] = 0;
        tcflush(myGpsFd, TCIFLUSH);
        tcsetattr(myGpsFd, TCSANOW, &gpsSerial);
        gpsPortInit = TRUE;
        /* end GPS Init - GPS serial port is now configured */
    }
    return (retCode);
}

ReconnErrCodes gpsInit(int *fileDescriptor)
{
    int ret_status = 0;
    ReconnErrCodes retcode = RECONN_SUCCESS;
    *fileDescriptor = -1;

    if(reconnGpioAction(GPIO_147, ENABLE) == RECONN_FAILURE)
    {
        reconnDebugPrint("%s: reconnGpioAction(GPIO_147, ENABLE) failed. \n", __FUNCTION__);
        retcode = RECONN_FAILURE;
    }
    else
    {
        if((ret_status = gpsOpen(fileDescriptor)) != RECONN_CONNECT_FAILED)
        {
            gpsPortInit = TRUE;
        }
        else
        {
            retcode = RECONN_FAILURE;
        }
    }
    return (retcode);
}

/* generic write to GPS device command */
ReconnErrCodes gpsWrite(unsigned char *buffer, int length) 
{
    ReconnErrCodes RetCode = RECONN_SUCCESS;

    if (gpsPortInit == FALSE) 
    {
        reconnDebugPrint("%s: GPS port is not initialized\n", __FUNCTION__);
        RetCode = RECONN_FAILURE;
        //retCode = GPS_PORT_NOT_INITIALIZED;
    }
    else
    {
        write(myGpsFd, buffer, length);
    }
    return (RetCode);
}

/* generic read from GPS device command */
ReconnErrCodes gpsRead(unsigned char *buffer, int *length) 
{
    int NumberBytes = 0;
    ReconnErrCodes RetCode = RECONN_SUCCESS;

    if (gpsPortInit == FALSE) 
    {
        reconnDebugPrint("%s: GPS port is not initialized\n", __FUNCTION__);
        RetCode = RECONN_FAILURE;
    }
    else
    {
        *length = 0;
        if ((NumberBytes = read(myGpsFd, buffer, 1)) > 0)
        {
            *length = NumberBytes;
        }
    }
    return (RetCode);
}
