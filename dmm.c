//******************************************************************************
//****************************************************************************** //
// FILE:        dmm.c
//
// FUNCTIONS:     
//              dmmOpen()
//              dmmInit()
//              dmmWrite()
//              dmmRead()
//              dmmDiags()
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
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>

#include "reconn.h"
#include "dmm.h"
#include "gpio.h"
#include "debugMenu.h"

static int      myDmmFd = -1;
static struct   termios dmm_serial;
static int      dmmPortInit = FALSE;
typedef enum 
{
    STATUS_COMMAND,
    MILLI_VOLT_COMMAND,
    VOLT_COMMAND,
    MILLI_AMP_COMMAND,
    AMP_COMMAND,
    OHM_COMMAND,
    DC_MODE_COMMAND,
    AC_MODE_COMMAND,
}DMM_COMMAND;


static char *dmmCommands[] = {
    "s", // status
    "v", // milli volt
    "V", // volt
    "i", // milli amp
    "I", // amp
    "o", // ohm
    "d",  // DC
    "a"  // AC
};

typedef enum 
{
    MILLI_VOLT_DC,
    VOLT_DC,
    MILLI_AMP_DC,
    AMP_DC,
    OHM_DC,

    MILLI_VOLT_AC,
    VOLT_AC,
    MILLI_AMP_AC,
    AMP_AC,
    OHM_AC,
}DMM_STATUS_RESULT;

static char *dmmStatusResult[] = {
    "vda",
    "Vda",
    "ida",
    "Ida",
    "oda",

    "vaa",
    "Vaa",
    "iaa",
    "Iaa",
    "oaa",
};


//*************************************************************************************
//*************************************************************************************
// FUNCTION:        diagsGetResponse
//
// DESCRIPTION: This function waits on the DMM filedescriptor for a response.
//
// Parameters:
//              buffer -    A buffer into which this routine will store the read data.
//              length -    the max length of the data to be read.
//
//*************************************************************************************
static ReconnErrCodes diagsGetResponse(char *buffer, int length)
{
    fd_set theFileDescriptor;
    struct timeval waitTime;
    ReconnErrCodes retCode = RECONN_SUCCESS;
    int err, amountRead, count = 0;

    FD_ZERO(&theFileDescriptor);
    FD_SET(myDmmFd, &theFileDescriptor);

    waitTime.tv_sec = 0;
    waitTime.tv_usec = 100000;

    do
    {
        if((err = select(myDmmFd+1, &theFileDescriptor, NULL, NULL, &waitTime)) == -1)
        {
            reconnDebugPrint("%s: select failed %d(%s)\n",__FUNCTION__, errno, strerror(errno));
            retCode = RECONN_FAILURE;
            break;
        }
        else if(err == 0)
        {
            reconnDebugPrint("%s: select timed out. Count = %d\n",__FUNCTION__, count);
            retCode = (count) ? RECONN_SUCCESS: RECONN_FAILURE;
            break;
        }
        else
        {
#ifdef DEBUG_EQPT
            reconnDebugPrint("%s:\n\n********************* select returned\n",__FUNCTION__);
#endif
            if(FD_ISSET(myDmmFd,  &theFileDescriptor))
            {
#ifdef DEBUG_EQPT
                reconnDebugPrint("%s: DMM has responded\n", __FUNCTION__);
#endif
                amountRead = read(myDmmFd, &(buffer[count]), length);
                if(amountRead < 0)
                {
                    reconnDebugPrint("%s: Read Failed %d (%s)\n", __FUNCTION__, errno, strerror(errno));
                    retCode = RECONN_FAILURE;
                    break;
                }
                count += amountRead;
            }
        }
    }while(count < length);
#ifdef DEBUG_EQPT
    reconnDebugPrint("%s:returning %d\n",__FUNCTION__, retCode);
#endif
    return (retCode);
}
//*************************************************************************************
//*************************************************************************************
// FUNCTION:        dmmOpen
//
// DESCRIPTION: This function is called to open communication to the digital Multimeter
//              and to set its communication parameters.
//
// Parameters:
//              fileDescriptor -    a pointer that this function initializes with either
//                                  NULL or the DMM's file descriptor.
//
//*************************************************************************************
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

//*************************************************************************************
//*************************************************************************************
// FUNCTION:        dmmInit
//
// DESCRIPTION: This function is called to initialize the digital multimeter. The function
//              enables power to the meter by setting its assigned GPIO.
// Parameters:
//              fileDescriptor  -   a pointer that this function initializes with either
//                                  NULL or the DMM's file descriptor.
//
//*************************************************************************************
ReconnErrCodes dmmInit(int *fileDescriptor)
{
    int ret_status = 0;
    ReconnErrCodes retcode = RECONN_SUCCESS;
    char dmmResponse[DMM_MAX_RESPONSE];
    *fileDescriptor = -1;

    if(reconnGpioAction(DMM_POWER_GPIO, ENABLE) == RECONN_FAILURE)
    {
        reconnDebugPrint("%s: reconnGpioAction(DMM_POWER_GPIO, ENABLE) failed. \n", __FUNCTION__);
        retcode = RECONN_FAILURE;
    }
    else
    {
        if((ret_status = dmmOpen(fileDescriptor)) == RECONN_SUCCESS)
        {
            // Make sure the DMM is responding

            reconnDebugPrint("%s: Getting status \n", __FUNCTION__);

            dmmWrite((unsigned char *)dmmCommands[STATUS_COMMAND], strlen(dmmCommands[STATUS_COMMAND]));
            if(diagsGetResponse((char *)&dmmResponse, DMM_MAX_RESPONSE) == RECONN_FAILURE)
            {
                reconnDebugPrint("%s: no Response disable GPIO %d \n", __FUNCTION__, DMM_POWER_GPIO);
                //power meter might be in shutdown mode. Wake it up.
                if(reconnGpioAction(DMM_POWER_GPIO, DISABLE) == RECONN_FAILURE)
                {
                    reconnDebugPrint("%s: reconnGpioAction(DMM_POWER_GPIO, DISABLE) failed. \n", __FUNCTION__);
                    retcode = RECONN_FAILURE;
                }
                usleep(50000);
                reconnDebugPrint("%s: ENABLE GPIO %d \n", __FUNCTION__, DMM_POWER_GPIO);
                if(reconnGpioAction(DMM_POWER_GPIO, ENABLE) == RECONN_FAILURE)
                {
                    reconnDebugPrint("%s: reconnGpioAction(DMM_POWER_GPIO, ENABLE) failed. \n", __FUNCTION__);
                    retcode = RECONN_FAILURE;
                }
                sleep(6);
            }

            // Clear out any queued messages
            memset(&dmmResponse, 0 , DMM_MAX_RESPONSE);
            while(diagsGetResponse((char *)&dmmResponse, DMM_MAX_RESPONSE) == RECONN_SUCCESS)
            {
                reconnDebugPrint("%s", dmmResponse);
                memset(&dmmResponse, 0 , DMM_MAX_RESPONSE);
            }
            reconnDebugPrint("\n");

            // Now set the DMM shutdown timeout value to max
            dmmWrite((unsigned char *)DMM_MAX_SHUTDOWN, strlen(DMM_MAX_SHUTDOWN));
            memset(&dmmResponse, 0 , DMM_MAX_RESPONSE);
            diagsGetResponse((char *)&dmmResponse, DMM_MAX_RESPONSE);
            if(strncmp((char *)&dmmResponse, DMM_SHUTDOWN_RESP, DMM_MAX_RESPONSE))
            {
                reconnDebugPrint("%s: setting timeout value failed. Value set was %sand value read was%s value should be %s\n", __FUNCTION__, DMM_MAX_SHUTDOWN, dmmResponse, DMM_SHUTDOWN_RESP);
            }

            dmmPortInit = TRUE;
        }
        else
        {
            retcode = RECONN_FAILURE;
        }
    }
    return (retcode);
}

//*************************************************************************************
//*************************************************************************************
// FUNCTION:        dmmWrite
//
// DESCRIPTION: This function is called to write data to the DMM
// Parameters:
//              buffer -    The data to be written
//              length -    The data length 
//
//*************************************************************************************
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

//*************************************************************************************
//*************************************************************************************
// FUNCTION:        dmmRead
//
// DESCRIPTION: This function is called to initialize the digital multimeter. The function
//              enables power to the meter by setting its assigned GPIO.
// Parameters:
//              buffer -    A buffer into which this routines places the read data.
//              length -    The data length 
//
//*************************************************************************************
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


//*************************************************************************************
//*************************************************************************************
// FUNCTION:        dmmDiags
//
// DESCRIPTION: This function does not really diagnose the hardware. It will run the 
//              dmm through each possible software configurations and compare do a comparison
//              verifying that the dmm is configured as expected.
// Parameters:
//
//*************************************************************************************
ReconnErrCodes dmmDiags()
{
    ReconnErrCodes retCode = RECONN_SUCCESS;
    char dmmResponse[DMM_MAX_RESPONSE];
    int dummyFd;
    int j, i;

    if(myDmmFd == -1)
    {
        if(dmmInit(&dummyFd) != RECONN_SUCCESS)
        {
            reconnDebugPrint("%s: dmmInit() failed\n", __FUNCTION__);
            retCode = RECONN_FAILURE;
        }
    }

    if(retCode == RECONN_SUCCESS)
    {
        // 
        // Since myDmmFd is set, that means that dmmInit() has been called.
        // Check to see if we can communicate with the dmm. If not, either the DMM 
        // hardware has died or it has gone into idle timeout and has power itself off.
        // So, power it back up and try to talk to the device.
        //

        for(j = DC_MODE_COMMAND; j <= AC_MODE_COMMAND; j++)
        {
#ifdef DEBUG_EQPT
            reconnDebugPrint("%s: sending %s\n", __FUNCTION__, dmmCommands[j]);
#endif
            if((retCode = dmmWrite((unsigned char *)dmmCommands[j], strlen((char *)dmmCommands[j]))) != RECONN_SUCCESS)
            {
                reconnDebugPrint("%s: dmmWrite(%s, %d) failed\n", __FUNCTION__, dmmCommands[i], strlen(dmmCommands[i]));
            }
            else
            {
                memset(&dmmResponse, 0 , DMM_MAX_RESPONSE);
                diagsGetResponse((char *)&dmmResponse, DMM_MAX_RESPONSE);
#ifdef DEBUG_EQPT
                reconnDebugPrint("%s: received %s\n", __FUNCTION__, dmmResponse);
#endif
                for(i = MILLI_VOLT_COMMAND; i <= OHM_COMMAND; i++)
                {
#ifdef DEBUG_EQPT
                    reconnDebugPrint("%s: sending %s\n", __FUNCTION__, dmmCommands[i]);
#endif
                    if((retCode = dmmWrite((unsigned char *)dmmCommands[i], strlen(dmmCommands[i]))) != RECONN_SUCCESS)
                    {
                        reconnDebugPrint("%s: dmmWrite(%s, %d) failed\n", __FUNCTION__, dmmCommands[i], strlen(dmmCommands[i]));
                        break;
                    }
                    else
                    {
                        memset(&dmmResponse, 0 , DMM_MAX_RESPONSE);
                        diagsGetResponse((char *)&dmmResponse, DMM_MAX_RESPONSE);
#ifdef DEBUG_EQPT
                        reconnDebugPrint("%s: received %s\n", __FUNCTION__, dmmResponse);
                        reconnDebugPrint("%s: sending %s\n", __FUNCTION__, dmmCommands[STATUS_COMMAND]);
#endif
                        if((retCode = dmmWrite((unsigned char *)dmmCommands[STATUS_COMMAND], strlen((char *)dmmCommands[STATUS_COMMAND]))) != RECONN_SUCCESS)
                        {
                            reconnDebugPrint("%s: dmmWrite(%s, %d) failed\n", __FUNCTION__, dmmCommands[STATUS_COMMAND], strlen(dmmCommands[STATUS_COMMAND]));
                        }
                        else
                        {
                            memset(&dmmResponse, 0 , DMM_MAX_RESPONSE);
                            if((retCode = diagsGetResponse((char *)&dmmResponse, DMM_MAX_RESPONSE)) != RECONN_SUCCESS)
                            {
                                reconnDebugPrint("%s: dmmRead() failed\n", __FUNCTION__);
                            }
                            else
                            {
                                if(strncmp((char *)&dmmResponse, dmmStatusResult[i-1], 3))
                                {
                                    reconnDebugPrint("%s: expected = %s VS response = %s Failed\n", __FUNCTION__, dmmStatusResult[i-1], dmmResponse);
                                    retCode = RECONN_FAILURE;
                                    i = OHM_COMMAND+1;
                                    j = AC_MODE_COMMAND+1; 
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        } // END for(j = DC_MODE_COMMAND; j <= AC_MODE_COMMAND; j++)
    }
#ifdef DEBUG_EQPT
    reconnDebugPrint("%s: returning %d\n", __FUNCTION__, retCode);
#endif
    return(retCode);
}

