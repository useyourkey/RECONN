//******************************************************************************
//******************************************************************************
// FILE:        dmm.c
//
// FUNCTIONS:     
//              dmmOpen()
//              dmmInit()
//              dmmWrite()
//              dmmPowerDown()
//              dmmRead()
//              dmmDiags()
//              dmmSaveConfigTask()
//              dmmSaveConfig()
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
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <arpa/inet.h>

#include "reconn.h"
#include "dmm.h"
#include "gpio.h"
#include "debugMenu.h"

static int      myDmmFd = -1;
#ifndef __SIMULATION__
static struct   termios dmm_serial;
#endif
static int      dmmPortInit = FALSE;
static FILE     *theDmmConfigFilePtr = 0;
pthread_mutex_t dmmMutex = PTHREAD_MUTEX_INITIALIZER;


char *dmmCommands[] = {
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
#ifdef DEBUG_EQPT
    int i;
    char *ptr;
#endif

    FD_ZERO(&theFileDescriptor);
    FD_SET(myDmmFd, &theFileDescriptor);

    waitTime.tv_sec = 0;
    waitTime.tv_usec = DMM_WAIT_TIME;

#ifdef DEBUG_EQPT
    reconnDebugPrint("\n%s: Function Entered length = %d\n",__FUNCTION__, length);
#endif
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
            reconnDebugPrint("%s: >>>>>>>>>>>>>>>>>>>>  select timed out. Count = %d\n",__FUNCTION__, count);
            retCode = (count) ? RECONN_SUCCESS: RECONN_FAILURE;
            break;
        }
        else
        {
#ifdef DEBUG_EQPT
            reconnDebugPrint("%s: ********************* select returned\n",__FUNCTION__);
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
#ifdef DEBUG_EQPT
                reconnDebugPrint("%s: amount read is %d\n", __FUNCTION__, amountRead);
#endif
                count += amountRead;
            }
        }
    }while(count < length);
#ifdef DEBUG_EQPT
    reconnDebugPrint("%s: buffer is ", __FUNCTION__);
    for(i = 0, ptr = buffer; i < count; i++, ptr++)
    {
        reconnDebugPrint("%c", *ptr);
    }
    reconnDebugPrint("\n");
    for(i = 0, ptr = buffer; i < count; i++, ptr++)
    {
        reconnDebugPrint("0x%x ", *ptr);
    }
    reconnDebugPrint("\n");
    reconnDebugPrint("%s:returning %s\n",__FUNCTION__, (retCode == RECONN_FAILURE) ? "FAILURE": "SUCCESS");
#endif
    return (retCode);
}
//*************************************************************************************
//*************************************************************************************
// FUNCTION:        dmmPowerDown
//
// DESCRIPTION: This function is called to power down the DMM.
//
// Parameters:
//              fileDescriptor -    a pointer that this function initializes with either
//                                  NULL or the DMM's file descriptor.
//
//*************************************************************************************
ReconnErrCodes dmmPowerDown()
{
    ReconnErrCodes retCode;

    if((retCode = reconnGpioAction(DMM_POWER_GPIO, DISABLE, NULL)) == RECONN_SUCCESS)
    {
        close(myDmmFd);
        myDmmFd = -1;
        gEqptDescriptors.dmmFd = -1;
        dmmPortInit = FALSE;
    }
    else
    {
        reconnDebugPrint("%s: reconnGpioAction(DMM_POWER_GPIO, ENABLE, NULL) failed. \n", __FUNCTION__);
    }
    return(retCode);
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
#ifndef __SIMULATION__
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
#endif

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
    ReconnErrCodes retCode = RECONN_SUCCESS;
#ifndef __SIMULATION__
    char dmmResponse[DMM_MAX_RESPONSE + 1];
    short gpioState;
#endif
    *fileDescriptor = -1;

#ifndef __SIMULATION__
    if(reconnGpioAction(DMM_POWER_GPIO, READ, &gpioState) == RECONN_SUCCESS)
    {
        if(gpioState == GPIO_IS_INACTIVE)
        {
            if(reconnGpioAction(DMM_POWER_GPIO, ENABLE, NULL) == RECONN_FAILURE)
            {
                reconnDebugPrint("%s: reconnGpioAction(DMM_POWER_GPIO, ENABLE, NULL) failed. \n", __FUNCTION__);
                retCode = RECONN_FAILURE;
            }
            else
            {
                /*
                 * Give the DMM time to boot
                 */
                sleep(5);
            }
        }
        if(retCode == RECONN_SUCCESS)
        {
            if((retCode = dmmOpen(fileDescriptor)) == RECONN_SUCCESS)
            {
                // Make sure the DMM is responding

                reconnDebugPrint("%s: Getting status \n", __FUNCTION__);

                memset(&dmmResponse, 0, DMM_MAX_RESPONSE + 1);
                dmmWrite((unsigned char *)dmmCommands[STATUS_COMMAND], strlen(dmmCommands[STATUS_COMMAND]));
                if(diagsGetResponse((char *)&dmmResponse, DMM_STATUS_RSP_LEN) == RECONN_FAILURE)
                {
                    reconnDebugPrint("%s: no Response disable GPIO %d \n", __FUNCTION__, DMM_POWER_GPIO);
                    //power meter might be in shutdown mode. Wake it up.

                    if(reconnGpioAction(DMM_POWER_GPIO, DISABLE, NULL) == RECONN_FAILURE)
                    {
                        reconnDebugPrint("%s: reconnGpioAction(DMM_POWER_GPIO, DISABLE, NULL) failed. \n", __FUNCTION__);
                        retCode = RECONN_FAILURE;
                    }
                    usleep(50000);
                    reconnDebugPrint("%s: ENABLE GPIO %d \n", __FUNCTION__, DMM_POWER_GPIO);
                    if(reconnGpioAction(DMM_POWER_GPIO, ENABLE, NULL) == RECONN_FAILURE)
                    {
                        reconnDebugPrint("%s: reconnGpioAction(DMM_POWER_GPIO, ENABLE, NULL) failed. \n", __FUNCTION__);
                        retCode = RECONN_FAILURE;
                    }
                    sleep(5);
                }
                else
                {
                    reconnDebugPrint("%s: status = %c", __FUNCTION__, dmmResponse[0]);
                    reconnDebugPrint("%c", dmmResponse[1]);
                    reconnDebugPrint("%c", dmmResponse[2]);
                    reconnDebugPrint("%c\n", dmmResponse[3]);
                }

                // Clear out any queued messages
                memset(&dmmResponse, 0 , DMM_MAX_RESPONSE);
                while(diagsGetResponse((char *)&dmmResponse, 1) == RECONN_SUCCESS)
                {
                    reconnDebugPrint("%s", dmmResponse);
                    memset(&dmmResponse, 0 , DMM_MAX_RESPONSE);
                }
                reconnDebugPrint("\n");

                // Now set the DMM shutdown timeout value to max
                dmmWrite((unsigned char *)DMM_MAX_SHUTDOWN, strlen(DMM_MAX_SHUTDOWN));

                memset(&dmmResponse, 0 , DMM_MAX_RESPONSE);
                usleep(50000);
                diagsGetResponse((char *)&dmmResponse, DMM_MAX_RESPONSE);
                if(strncmp((char *)&dmmResponse, DMM_SHUTDOWN_RESP, DMM_MAX_RESPONSE))
                {
                    reconnDebugPrint("%s: setting timeout value failed. Value set was%sand value read was%s value should be %s\n", __FUNCTION__, DMM_MAX_SHUTDOWN, dmmResponse, DMM_SHUTDOWN_RESP);
                }
                dmmLoadSavedConfig();
                dmmPortInit = TRUE;
            }
        }
    }
    else
    {
        reconnDebugPrint("%s: GPIO_174, READ) failed \n", __FUNCTION__);
        retCode = RECONN_FAILURE;
    }
#else
    dmmPortInit = TRUE;
#endif
    return (retCode);
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
    int bytesWritten;
#if 0
    int i;
    unsigned char *ptr = buffer;
#endif

    if (dmmPortInit == FALSE)
    {
        RetCode = RECONN_DMM_PORT_NOT_INITIALIZED;
    }
    else
    {
#if 0
        reconnDebugPrint("%s: writing ", __FUNCTION__);
        for(i = 0; i < length; i++, ptr++)
        {
            reconnDebugPrint("%c", *ptr);
        }
        reconnDebugPrint("\n");
#endif 
        bytesWritten = write (myDmmFd, buffer, length);
        if((bytesWritten != length) || (bytesWritten < 0))
        {
            reconnDebugPrint("%s: write failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
            RetCode = RECONN_FAILURE;
        }
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
ReconnErrCodes dmmRead (unsigned char *buffer, int *length)
{
   int       NumberBytes = 0;
   ReconnErrCodes retCode = RECONN_SUCCESS;
#if 0
       int i;
       unsigned char *ptr = buffer;
#endif


   if (dmmPortInit == FALSE)
   {
      reconnDebugPrint ("%s: DMM Port not initialized \n\r", __FUNCTION__);
      retCode = RECONN_DMM_PORT_NOT_INITIALIZED;
   }
   else
   {
       *length = 0;
       if((NumberBytes = read (myDmmFd, buffer, 2044)) > 0)
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
   return (retCode);
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
    int voltageMode, i;

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

        for(voltageMode = DC_MODE_COMMAND; voltageMode <= AC_MODE_COMMAND; voltageMode++)
        {
#ifdef DEBUG_EQPT
            reconnDebugPrint("%s: sending %s\n", __FUNCTION__, dmmCommands[voltageMode]);
#endif
            if((retCode = dmmWrite((unsigned char *)dmmCommands[voltageMode], strlen((char *)dmmCommands[voltageMode]))) != RECONN_SUCCESS)
            {
                reconnDebugPrint("%s: dmmWrite(%s, %d) failed\n", __FUNCTION__, dmmCommands[i], strlen(dmmCommands[i]));
            }
            else
            {
                memset(&dmmResponse, 0 , DMM_MAX_RESPONSE);
                /*
                 * Give the meter a chance to process the command before getting the response
                 */
                usleep(DMM_WAIT_TIME);

                /*
                 * Currently the redfish firmware on the DMM does not echo back "a" when commanded
                 * so skip looking for a response when switching to AC mode.
                 */
                if(voltageMode == DC_MODE_COMMAND)
                {
                    diagsGetResponse((char *)&dmmResponse, 1);
#ifdef DEBUG_EQPT
                    reconnDebugPrint("%s: received %s\n", __FUNCTION__, dmmResponse);
#endif
                }
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
                        diagsGetResponse((char *)&dmmResponse, 1);
#ifdef DEBUG_EQPT
                        reconnDebugPrint("%s: received %s\n", __FUNCTION__, dmmResponse);
                        reconnDebugPrint("%s: sending %s\n", __FUNCTION__, dmmCommands[STATUS_COMMAND]);
#endif
                        /*
                         * Give the meter a chance to process the previous command before getting status.
                         */
                        usleep(DMM_WAIT_TIME);
                        if((retCode = dmmWrite((unsigned char *)dmmCommands[STATUS_COMMAND], strlen((char *)dmmCommands[STATUS_COMMAND]))) != RECONN_SUCCESS)
                        {
                            reconnDebugPrint("%s: dmmWrite(%s, %d) failed\n", __FUNCTION__, dmmCommands[STATUS_COMMAND], strlen(dmmCommands[STATUS_COMMAND]));
                        }
                        else
                        {
                            memset(&dmmResponse, 0 , DMM_MAX_RESPONSE);
                            if((retCode = diagsGetResponse((char *)&dmmResponse, DMM_STATUS_RSP_LEN)) != RECONN_SUCCESS)
                            {
                                reconnDebugPrint("%s: diagsGetResponse() failed\n", __FUNCTION__);
                            }
                            else
                            {
                                if(strncmp((char *)&dmmResponse, dmmStatusResult[i-1], 3))
                                {
                                    reconnDebugPrint("%s: expected = %s VS response = %s Failed\n", __FUNCTION__, dmmStatusResult[i-1], dmmResponse);
                                    retCode = RECONN_FAILURE;
                                    i = OHM_COMMAND+1;
                                    voltageMode = AC_MODE_COMMAND+1; 
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

//*************************************************************************************
//*************************************************************************************
// FUNCTION:        dmmSaveConfigTask
//
// DESCRIPTION: This tasks sole responsibility is to connect to the reconnPowerDaemon and
//              wait for a message. When the message comes in it means the user has hit the 
//              power button and this task then gets the digital mulitimeter's current configuration
//              and stores it in a file. This task then sends a message back to the reconnPowerDaemon
//              informing it that the write is complete and it can power down the system.
//
//              Note:
//              Under normal operating conditions this task is called at power up then
//              blocks on the recv() call. When the power button is pushed this task is
//              triggered, completes it job then dies. No need to loop because power is about
//              to be removed by reconnPowerDaemon.
// Parameters:
//
//*************************************************************************************
void *dmmSaveConfigTask(void * args)
{
    static int retCode = 0;
    struct sockaddr_in server_addr;
    int bytesReceived;
    int dmmSocketFd = -1;
    extern YESNO swUpgradeInProgress;
    DMM_POWER_BUTTON_MSG dmmPowerButtonMsg;

    
    UNUSED_PARAM(args); bzero((unsigned char *) &server_addr, sizeof(server_addr));
    /* bind the socket */
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(DMM_SOCKET_PORT);

    while(swUpgradeInProgress == NO)
    {
        if(dmmSocketFd == -1)
        {
            if((dmmSocketFd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            {
                reconnDebugPrint("%s: socket failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
                break;
            }
        }
        if(connect(dmmSocketFd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
        {
            /*
             * PowerDaemon might not be running, so sleep for a while and try again.
             */
#ifndef __SIMULATION__
            reconnDebugPrint("%s: connect failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
#endif
            sleep(2);
        }
        else
        {
            reconnDebugPrint("%s: waiting for command from PowerDaemon\n", __FUNCTION__);
            if((bytesReceived = recv(dmmSocketFd, &(dmmPowerButtonMsg.theMessage), sizeof(dmmPowerButtonMsg), 0)) > 0)
            {
                reconnDebugPrint("%s: received command from PowerDaemon\n", __FUNCTION__);

                if(dmmPowerButtonMsg.theMessage == SAVE_DMM_STATE)
                {
                    dmmSaveConfig();
                    reconnDebugPrint("%s: sending reply to PowerDaemon\n", __FUNCTION__);

                    dmmPowerButtonMsg.theMessage = DMM_SAVE_COMPLETE;
                    send(dmmSocketFd, &(dmmPowerButtonMsg.theMessage), sizeof(dmmPowerButtonMsg), 0);
                }
                else
                {
                    reconnDebugPrint("%s: Invalid message received %d\n", __FUNCTION__, dmmPowerButtonMsg.theMessage);
                }
            }
            else if(bytesReceived == 0)
            {
                reconnDebugPrint("%s: PowerDaemon closed the socket.\n", __FUNCTION__);
                if(shutdown(dmmSocketFd, SHUT_RDWR) != 0)
                {
                    reconnDebugPrint("%s: shutdown(%d ,SHUT_RDWR) failed %d (%s)\n", __FUNCTION__, 
                            dmmSocketFd, errno, strerror(errno));
                }
                if(close(dmmSocketFd) != 0)
                {
                    reconnDebugPrint("%s: close(%d) failed %d (%s)\n", __FUNCTION__, dmmSocketFd, 
                            errno, strerror(errno));
                }
                dmmSocketFd = -1;
            }
            else
            {
                /*
                 * Something happened to the socket try to reconnect
                 */
                close(dmmSocketFd);
                reconnDebugPrint("%s: recv returned %d (%s) \n", __FUNCTION__, errno, strerror(errno));
                sleep(2);
            }
        }
    }
    return (&retCode);
}
//*************************************************************************************
//*************************************************************************************
// FUNCTION:        dmmLoadSavedConfig
//
// DESCRIPTION: This function is used to load the DMM with the stored configuration
//
// Parameters:
//
//*************************************************************************************
void dmmLoadSavedConfig()
{
    char dmmResponse[DMM_MAX_RESPONSE + 1];
    char theConfigString[DMM_MAX_RESPONSE + 1];
    size_t amountRead;

    /*
     * Read the config file and set the meter accordingly. If there is not config file, no big deal. 
     * One will be written when the reconn unit is powered down.
     */
    reconnDebugPrint("%s: Function Entered\n", __FUNCTION__);
    if((theDmmConfigFilePtr = fopen(DMM_CONFIG_FILE_NAME, "r")) == NULL)
    {
        if(errno != ENOENT)
        {
            reconnDebugPrint("%s: fopen(%s, r) failed %d(%s)\n", __FUNCTION__, DMM_CONFIG_FILE_NAME, errno, strerror(errno));
        }
        else
        {
            /*
             * File not found is a valid condition.
             */
        }
    }
    else
    {
        // Clear out any queued messages
        memset(&dmmResponse, 0 , DMM_MAX_RESPONSE + 1);
        while(diagsGetResponse((char *)&dmmResponse, 1) == RECONN_SUCCESS)
        {
            reconnDebugPrint("%s", dmmResponse);
            memset(&dmmResponse, 0 , DMM_MAX_RESPONSE);
        }
        reconnDebugPrint("\n");

        memset(&theConfigString, 0, DMM_MAX_RESPONSE + 1);
        /*
         * The config file only holds 2 bytes of config data.
         */
        if((amountRead = fread(&theConfigString, 1, DMM_NUM_CONFIG_BYTES, theDmmConfigFilePtr)) == DMM_NUM_CONFIG_BYTES)
        {
            if(dmmWrite((unsigned char *)&theConfigString[0], 1) == RECONN_SUCCESS)
            {
                usleep(DMM_WAIT_TIME);
                memset(&dmmResponse, 0, DMM_MAX_RESPONSE + 1);
                if(diagsGetResponse((char *)&dmmResponse, 1) == RECONN_SUCCESS)
                {
                    if(dmmWrite((unsigned char *)&theConfigString[1], 1) == RECONN_SUCCESS)
                    { 
                        usleep(DMM_WAIT_TIME);
                        memset(&dmmResponse, 0, DMM_MAX_RESPONSE + 1);
                        if(diagsGetResponse((char *)&dmmResponse, 1) == RECONN_FAILURE)
                        {
                            reconnDebugPrint("%s: diagsGetResponse() byte 1 failed\n", __FUNCTION__);
                        }
                    }
                    else
                    {
                        reconnDebugPrint("%s: dmmWrite() byte 1 failed\n", __FUNCTION__);
                    }
                }
                else
                {
                    reconnDebugPrint("%s: diagsGetResponse() byte 0 failed\n", __FUNCTION__);
                }
            }
            else
            {
                reconnDebugPrint("%s: dmmWrite() byte 0 failed\n", __FUNCTION__);
            }
        }
        else if(amountRead != DMM_NUM_CONFIG_BYTES)
        {
            reconnDebugPrint("%s: fread(%s) returned %d \n", __FUNCTION__, DMM_CONFIG_FILE_NAME, amountRead);
        }
        else
        {
            reconnDebugPrint("%s: fread(%s) failed %d(%s)\n", __FUNCTION__, DMM_CONFIG_FILE_NAME, errno ,strerror(errno));
        }
        fclose(theDmmConfigFilePtr);
    }
    reconnDebugPrint("%s: Function exiting \n", __FUNCTION__);
}
//*************************************************************************************
//*************************************************************************************
// FUNCTION:        dmmSaveConfig
//
// DESCRIPTION: This function is used to load the DMM with the stored configuration
//
// Parameters:
//
//*************************************************************************************
void dmmSaveConfig()
{
    char dmmResponse[DMM_MAX_RESPONSE];
    int bytesWritten;

    unlink(DMM_CONFIG_FILE_NAME);
    if((theDmmConfigFilePtr = fopen(DMM_CONFIG_FILE_NAME, "w")) == NULL)
    {
        reconnDebugPrint("%s: fopen(%s, w) failed %d(%s)\n", __FUNCTION__, DMM_CONFIG_FILE_NAME, errno, strerror(errno));
    } 
    else 
    { 
        memset(&dmmResponse, 0, DMM_MAX_RESPONSE + 1); 
        pthread_mutex_lock(&dmmMutex); 
        if(dmmWrite((unsigned char *)dmmCommands[STATUS_COMMAND], strlen(dmmCommands[STATUS_COMMAND])) == RECONN_SUCCESS) 
        { 
            if(diagsGetResponse((char *)&dmmResponse, DMM_STATUS_RSP_LEN) == RECONN_SUCCESS) 
            { 
                reconnDebugPrint("%s: writing ", __FUNCTION__); 
                reconnDebugPrint("%c ", dmmResponse[0]); 
                reconnDebugPrint("%c ", dmmResponse[1]); 
                reconnDebugPrint("%c ", dmmResponse[2]); 
                reconnDebugPrint("%c ", dmmResponse[3]); 
                reconnDebugPrint("%c ", dmmResponse[4]); 
                reconnDebugPrint("\n "); 
                /* 
                 * Only the first 2 bytes mean anything.  
                 * byte 1 = input mode (Voltage, Current, Resistance) 
                 * byte 2 = AC or DC 
                 * byte 3 = ranging (auto or manual) 
                 */ 
                bytesWritten = fwrite(&dmmResponse, 1, DMM_NUM_CONFIG_BYTES, theDmmConfigFilePtr); 
                if((bytesWritten == 0) || (bytesWritten != DMM_NUM_CONFIG_BYTES)) 
                { 
                    reconnDebugPrint("%s: fwrite() failed %d(%s)\n", __FUNCTION__, errno, strerror(errno)); 
                } 
            } 
            else 
            { 
                reconnDebugPrint("%s: diagsGetResponse failed\n", __FUNCTION__); 
            } 
        } 
        else 
        { 
            reconnDebugPrint("%s: dmmWrite() failed\n", __FUNCTION__); 
        } 
        if(fflush(theDmmConfigFilePtr) != 0) 
        { 
            reconnDebugPrint("%s: fflush() failed %d(%s)\n", __FUNCTION__, errno, strerror(errno)); 
        } 
        if(fsync(fileno(theDmmConfigFilePtr)) != 0){ 
            reconnDebugPrint("%s: fsync() failed %d(%s)\n", __FUNCTION__, errno, strerror(errno)); 
        } 
        if(fclose(theDmmConfigFilePtr) != 0)
        {
            reconnDebugPrint("%s: fclose() failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
        }
        pthread_mutex_unlock(&dmmMutex);
    }
}
