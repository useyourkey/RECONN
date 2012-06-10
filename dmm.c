//******************************************************************************
//****************************************************************************** //
// FILE:        dmm.c
//
// CLASSES:     
//
// DESCRIPTION: This is the file which contains functions to read, write and control
//              the Digital Multi Meter
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
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

#include "reconn.h"
#include "dmm.h"
#include "gpio.h"
#include "debugMenu.h"

int myDmmFd;
struct termios dmm_serial;
int       dmm_wait_flag = TRUE;
int       dmmPortInit = FALSE;


static ReconnErrCodes dmmOpen (int *fileDescriptor)
{
    ReconnErrCodes RetCode = RECONN_SUCCESS;

    if((myDmmFd = open(DMM_SERIAL_PORT, O_RDWR | O_NOCTTY | O_NONBLOCK)) < 0)
    {
        /* failed to open the serial port */
        reconnDebugPrint ("%s: Failed to open serial port %s\n\r", __FUNCTION__, DMM_SERIAL_PORT);
        RetCode = RECONN_DMM_SERIAL_PORT_OPEN_FAIL;
        *fileDescriptor = -1;
    }
    else
    {
        reconnDebugPrint ("%s: serial port %s open.\r\n", __FUNCTION__, DMM_SERIAL_PORT);
        *fileDescriptor = myDmmFd;
        dmm_serial.c_cflag =
            DMM_BAUD_RATE | DMM_DATABITS | DMM_STOPBITS | DMM_PARITYON | DMM_PARITY | CLOCAL | CREAD;
        dmm_serial.c_iflag = IGNPAR;
        dmm_serial.c_oflag = 0;
        dmm_serial.c_lflag = 0;
        dmm_serial.c_cc[VMIN] = 1;
        dmm_serial.c_cc[VTIME] = 0;
        tcflush (myDmmFd, TCIFLUSH);
        tcsetattr (myDmmFd, TCSANOW, &dmm_serial);

        dmmPortInit = TRUE;
    }
    return (RetCode);
}

ReconnErrCodes dmmInit(int *fileDescriptor)
{
    int ret_status = 0;
    ReconnErrCodes retcode = RECONN_SUCCESS;
    *fileDescriptor = -1;

    if(reconnGpioAction(GPIO_174, ENABLE) == RECONN_FAILURE)
    {
        reconnDebugPrint("%s: reconnGpioAction(GPIO_174, ENABLE) failed. \n", __FUNCTION__);
        retcode = RECONN_FAILURE;
    }
    else
    {
        if((ret_status = dmmOpen(fileDescriptor)) == RECONN_SUCCESS)
        {
            dmmPortInit = TRUE;
        }
        else
        {
            retcode = RECONN_FAILURE;
        }
    }
    return (retcode);
}

/* generic write to DMM device command */
ReconnErrCodes dmmWrite (unsigned char *buffer, int length)
{
    ReconnErrCodes RetCode = RECONN_SUCCESS;
   if (dmmPortInit == FALSE)
   {
      RetCode = RECONN_DMM_PORT_NOT_INITIALIZED;
   }
   else
   {
       write (myDmmFd, buffer, length);
   }
   return (RetCode);
}

/* generic read from DMM device command */
ReconnErrCodes dmmRead (unsigned  char *buffer, int *length)
{
   int       NumberBytes = 0;
   ReconnErrCodes RetCode = RECONN_SUCCESS;

   if (dmmPortInit == FALSE)
   {
      reconnDebugPrint ("%s: DMM Port not initialized \n\r", __FUNCTION__);
      RetCode = RECONN_FAILURE;
   }
   else
   {
       *length = 0;
       if((NumberBytes = read (myDmmFd, buffer, 2044)) > 0)
       {
           *length = NumberBytes;
       }
   }
   return (RetCode);
}

#if 0
ReconnErrCodes makeDmmOutput (char *dmm_outputbuffer, int *dmm_outputlength)
{
   int  count = 0;
   ReconnPacket *thePacket = (ReconnPacket *)&global_dmm_output;

   for (count = 0; count < *dmm_outputlength; count++)
   {
      thePacket->dataPayload[count] = dmm_outputbuffer[count];
   }
   ADD_MSGID_TO_PACKET(DMM_PKT_RCVD_NOTIFICATION, thePacket); 
   ADD_DATA_LENGTH_TO_PACKET(count, thePacket);

   /*
    * Add 4 to account for the MSGID and Data length 
    */
   global_dmm_outputlength = count + 4;
   return (RECONN_PACKET_READY);
}
#endif
