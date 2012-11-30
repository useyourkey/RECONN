//******************************************************************************
//******************************************************************************
//
// FILE:        spectrum.c
//
// FUNCTIONS:   SpectrumAnalyzerInit()
//              SpectrumAnalyzerOpen()
//              SpectrumAnalyzerWrite()
//              SpectrumAnalyzerSelectRead()
//              SpectrumAnalyzerRead()
//              SpectrumAnalyzerClose()
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>

#include <termios.h>
#include <unistd.h>
#include <errno.h>

#include "reconn.h"
#include "spectrum.h"
#include "gpio.h"
#include "debugMenu.h"
#include "upgrade.h"
#include "aov_core.h"
#include "aov_errno.h"
#include "aov_sa.h"
#include "aov_eng.h"

int myAnalyzerFd = -1;
int SAPortOpen = FALSE;
struct termios analyzerSerial;
struct aov_version  gSpecanaFirmwareVersion;

//*************************************************************************************
//*************************************************************************************
// FUNCTION:    SpectrumAnalyzerOpen
//
// DESCRIPTION: A static interface, only visible to routines withing this file, called 
//              to open a connection to the spectrum analyzer. 
//
// Parameters:
//
//*************************************************************************************
static ReconnErrCodes SpectrumAnalyzerOpen() 
{
    ReconnErrCodes retCode = RECONN_SUCCESS;

     reconnDebugPrint("%s: Function Entered myAnalyzerFd %d\n", __FUNCTION__, myAnalyzerFd);
#ifndef __SIMULATION__
    if((myAnalyzerFd = open(SPECTRUM_ANALYZER_DEV, O_RDWR | O_NOCTTY | O_NONBLOCK)) < 0)
    {
        reconnDebugPrint("%s: Failed to open %s %d (%s)\n\r", __FUNCTION__, SPECTRUM_ANALYZER_DEV, errno ,strerror(errno));
        /* failed to open the device */
        retCode = RECONN_SERIAL_PORT_OPEN_FAIL;
    }
    else
    {
        reconnDebugPrint("\n%s: serial port %s Opened\n", __FUNCTION__, SPECTRUM_ANALYZER_DEV);
        reconnDebugPrint("\n%s: Spectrum Analyer FD = %d Opened\n", __FUNCTION__, myAnalyzerFd);

        analyzerSerial.c_cflag = SPECTRUM_ANALYZER_BAUD_RATE | SPECTRUM_ANALYZER_DATABITS |
            SPECTRUM_ANALYZER_STOPBITS | SPECTRUM_ANALYZER_PARITYON | SPECTRUM_ANALYZER_PARITY | 
            CLOCAL | CREAD;
        analyzerSerial.c_iflag = IGNPAR;
        analyzerSerial.c_oflag = 0;
        analyzerSerial.c_lflag = 0;
        analyzerSerial.c_cc[VMIN] = 1;
        analyzerSerial.c_cc[VTIME] = 0;
        tcflush(myAnalyzerFd, TCIFLUSH);
        tcsetattr(myAnalyzerFd, TCSANOW, &analyzerSerial);

        reconnDebugPrint("%s: Spectrum Analyzer Open Complete\n\n", __FUNCTION__);
        SAPortOpen = TRUE;
    }
#endif
    return (retCode);
}

//*************************************************************************************
//*************************************************************************************
// FUNCTION:    SpectrumAnalyzerInit
//
// DESCRIPTION: An interface called to initialize the spectrum analyzer.
//
//              If fileDescriptor is set then this interface will not only retrieve 
//              the firmware version into global memory, it will also place the open 
//              file descriptor into global memory space for all to use. 
//              
//              If fileDescriptor is not set, then the calling routine wants exclusive control
//              of the spectrum analyzer so this interface will open a connection to the 
//              analyzer and populate its internal file descriptor. Since the gEqptDescript.analyzerFd
//              was not passed to this routine reconnGetEqptResponseTask() will not try to read
//              data from the analyzer.
// Parameters:
//              fileDescriptor - a pointer into which this is interface will place the spectrum
//                              analyzer file descriptor
//
//*************************************************************************************
ReconnErrCodes SpectrumAnalyzerInit(int *fileDescriptor) 
{
    short gpioState;
    ReconnErrCodes retCode = RECONN_SUCCESS;

    reconnDebugPrint("%s: Function Entered fileDescriptor = %u. \n", __FUNCTION__, fileDescriptor);
    // Apply power to the analyzer.
    if(fileDescriptor)
    {
       	*fileDescriptor = -1;
    }
    if(reconnGpioAction(GPIO_141, READ, &gpioState) == RECONN_SUCCESS)
    {
       	if(gpioState == GPIO_IS_INACTIVE)
       	{
	    if(reconnGpioAction(GPIO_141, ENABLE, NULL) == RECONN_FAILURE)
	    {
	       	reconnDebugPrint("%s: reconnGpioAction(GPIO_141, ENABLE, NULL) failed. \n", __FUNCTION__);
	       	retCode = RECONN_FAILURE;
	    }
	    else 
	    {
#ifndef __SIMULATION__
	       	/*
		 * Power has been applied, wait for the usb drivers to come up before attempting to open
		 * the analyzer.
	       	 */
	       	sleep(5); 
#endif
	    }
       	}
       	if(retCode == RECONN_SUCCESS)
       	{
	    /*
	     * See if we can read the analyzer's firmware version. If we can then the unit is operational.
	     * If not, then it is probably dead.
	     *
	     * Calling SpectrumAnalyzerOpen() with a NULL fileDescriptor argument keeps 
	     * reconnGetEqptResponseTask() from reading data. 
	     */
	    if((retCode = SpectrumAnalyzerOpen()) == RECONN_SUCCESS)
	    {
                reconnDebugPrint("%s: Calling AVCOM_SA_GetFirmwareVersion()\n", __FUNCTION__);
	       	if((retCode = AVCOM_SA_GetFirmwareVersion (NULL, &gSpecanaFirmwareVersion)) == AOV_NO_ERROR)
	       	{
		    reconnDebugPrint("%s: major = %d\n", __FUNCTION__, gSpecanaFirmwareVersion.major);
		    reconnDebugPrint("minor = %d\n", gSpecanaFirmwareVersion.minor);
		    reconnDebugPrint("build = %d\n", gSpecanaFirmwareVersion.build);
		    reconnDebugPrint("stage = %d\n", gSpecanaFirmwareVersion.stage);
		    reconnDebugPrint("string = %s\n", gSpecanaFirmwareVersion.string);
		    retCode = RECONN_SUCCESS;

                    if(fileDescriptor)
                    {
                        *fileDescriptor = myAnalyzerFd;
                    }
	       	}
	       	else
	       	{
		    reconnDebugPrint("%s: AVCOM_SA_GetFirmwareVersion() failed %d \n", __FUNCTION__, retCode);
		    retCode = RECONN_FAILURE;
	       	}
            }
        }
    }
    else
    {
        reconnDebugPrint("%s: GPIO_141, READ) failed \n", __FUNCTION__);
        retCode = RECONN_FAILURE;
    }
    reconnDebugPrint("%s: Function Exiting. \n", __FUNCTION__);
    return (retCode);
}

//*************************************************************************************
//*************************************************************************************
// FUNCTION:    SpectrumAnalyzerWrite
//
// DESCRIPTION: An interface called to write data to the spectrum analyzer.
//
// Parameters:
//              buffer - a pointer to the data to be written.
//              length - the length of the data to be written.
//*************************************************************************************
ReconnErrCodes SpectrumAnalyzerWrite(unsigned char *buffer, int length) 
{
    ReconnErrCodes retcode = RECONN_SUCCESS;
#ifdef DEBUG_SPECTRUM
    int debugIndex; 
#endif

    if ((SAPortOpen == FALSE)  || (myAnalyzerFd == -1))
    {
        reconnDebugPrint("%s: Spectrum Analyzer port is not initialized\n", __FUNCTION__);
        retcode =  RECONN_SA_PORT_NOT_INITIALIZED;
    }
    else
    {
#ifdef DEBUG_SPECTRUM
        reconnDebugPrint("%s: writing data to the analyzer length = %d\n", __FUNCTION__, length);
        for(debugIndex = 0; debugIndex < length; debugIndex++)
        {
            reconnDebugPrint("0x%x ", buffer[debugIndex]);
        }
#endif

        if (write(myAnalyzerFd, buffer, length) < 0)
        {
            reconnDebugPrint("%s: write(%d, buffer, %d) failed %d %(%s)\n", __FUNCTION__, myAnalyzerFd, 
                    length, errno, strerror(errno));
        }
        if(gDebugTimingEnabled == YES)
        {   
            deviceStartTime.tv_sec = deviceStartTime.tv_usec = 0;
            gettimeofday(&deviceStartTime, NULL);
        }
#ifdef DEBUG_SPECTRUM
        reconnDebugPrint("\n");
#endif
    }
    return (retcode);
}
//*************************************************************************************
//*************************************************************************************
// FUNCTION:    SpectrumAnalyzerSelectRead
//
// DESCRIPTION: An interface called when a need arises to read from the spectrum analyzer
//              This interface uses select() to wait on the analyzer file descriptor for a
//              time determined by the caller.
// Parameters:
//              buffer - a buffer pointer into which this interface will place the data read.
//              length - then amount of bytes read
//              waitTime - a timeval pointer telling select how long wait for a response.
//
//*************************************************************************************
ReconnErrCodes SpectrumAnalyzerSelectRead(unsigned char *buffer, int *length, struct timeval *waitTime)
{
    fd_set theSelectFileDescriptor;
    ReconnErrCodes retCode = RECONN_SUCCESS;
    int retStatus, amountRemaining, amountToRead, amountRead = 0;

    if((buffer == NULL) || (length == NULL) || (waitTime == NULL))
    {
       	reconnDebugPrint("%s: Invalid paramter buffer =%d, length = %d, waitITime = %d\n", __FUNCTION__,
	       	buffer, length, waitTime);
       	retCode = RECONN_FAILURE;
    }
    else
    {
       	FD_ZERO(&theSelectFileDescriptor);
       	FD_SET(myAnalyzerFd, &theSelectFileDescriptor);

        amountRemaining = *length;
        do{
            if((retStatus = select(myAnalyzerFd+1, &theSelectFileDescriptor, NULL, NULL, waitTime)) < 0)
            {
                reconnDebugPrint("%s: select failed %d(%s)\n",__FUNCTION__, errno, strerror(errno));
                retCode = RECONN_FAILURE;
            }
            else if (retStatus == 0)
            {
                reconnDebugPrint("%s: select timedout\n",__FUNCTION__);
                *length = 0;
                retCode = RECONN_FAILURE;
            }
            else
            {
                if(FD_ISSET(myAnalyzerFd,  &theSelectFileDescriptor))
                {
                    amountToRead = amountRemaining;
                    //reconnDebugPrint("%s: calling SpectrumAnalyzerRead length = %d\n", __FUNCTION__, amountToRead);
                    retCode = SpectrumAnalyzerRead(&(buffer[amountRead]), &amountToRead);
                    amountRead += amountToRead;
                    amountRemaining -= amountRead;
                    //reconnDebugPrint("%s: amountRemaining = %d amountRead = %d  length = %d\n", __FUNCTION__, amountRemaining, amountRead, *length);
                }
                else
                {
                    reconnDebugPrint("%s: descriptor is not set\n", __FUNCTION__);
                    retCode = RECONN_FAILURE;
                }
            }
        }while((retCode == RECONN_SUCCESS) && (amountRead < *length));
    }
    return(retCode);
}


//*************************************************************************************
//*************************************************************************************
// FUNCTION:    SpectrumAnalyzerRead
//
// DESCRIPTION: An interface called when a need arises to read from the spectrum analyzer.
//
// Parameters:
//              buffer - a buffer pointer into which this interface will place the data read.
//              length - then amount of bytes read
//
//*************************************************************************************
ReconnErrCodes SpectrumAnalyzerRead(unsigned char *buffer, int *length) 
{
    int NumberBytes = 0;
    ReconnErrCodes retCode = RECONN_SUCCESS;

    if (SAPortOpen == FALSE) 
    {
        reconnDebugPrint("%s: Spectrum Analyzer port is not initialized\n", __FUNCTION__);
        retCode = RECONN_FAILURE;
    }
    else 
    {
        //reconnDebugPrint("%s: reading from %d \n", __FUNCTION__, myAnalyzerFd);
        //*length = 0
        if((NumberBytes = read(myAnalyzerFd, buffer, (*length == 0) ? RECONN_RSP_PAYLOAD_SIZE : *length)) > 0)
        {
            *length = NumberBytes;
            //reconnDebugPrint("%s: NumberBytes %d length = %d\n", __FUNCTION__, NumberBytes, *length);
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
// FUNCTION:    SpectrumAnalyzerClose
//
// DESCRIPTION: Interface used to close the spectrum analyzer fileDescriptor. 
//              
// Parameters:
//             fileDescriptor - a Pointer to the global analyzer file descriptor 
//
//*************************************************************************************
ReconnErrCodes SpectrumAnalyzerClose(int *fileDescriptor)
{
    ReconnErrCodes retCode = RECONN_SUCCESS;

    if(close(myAnalyzerFd) != 0)
    {
	 reconnDebugPrint("%s: close() failed %d (%s)\n", __FUNCTION__, errno, strerror(errno));
	 retCode = RECONN_FAILURE;
    }
    else
    {
       	if(fileDescriptor)
       	{
	    *fileDescriptor = -1;
       	}
       	myAnalyzerFd = -1;
    }
    SAPortOpen = FALSE;
    return (retCode);
}
//*************************************************************************************
//*************************************************************************************
// FUNCTION:    SpectrumAnalyzerUpgrade
//
// DESCRIPTION: An interface called to upgrade the spectrum analyzer firmware.
//              The interface will close the current analyzer file descriptor. Doing this
//              effectively removes the file descriptor from reconnGetEqptResponseTask() 
//              descriptor list. We do this so reconnGetEqptResponseTask() does not interfere
//              with this interface. We then re intialize the analyzer telling it not
//              to place the new descriptor onto reconnGetEqptResponseTask() descriptor list.
//              
//             
// Parameters:
//              
//
//*************************************************************************************
ReconnErrCodes SpectrumAnalyzerUpgrade()
{
    ReconnErrCodes retCode = RECONN_SUCCESS;
    struct stat fileStat;
    enum aov_errno avcomRetCode;
    int i, refLevel, bootLoaderMode;

    char *specanaUpgradeDataPtr = NULL;
    FILE *specanaUpgradeFilePtr = NULL;
    /*
     * We need to have sole access to any analyzer responses so close it, then reopen
     * it with a NULL FD. This will effectively remove the analyzer from the 
     * reconnGetEqptResponseTask().
     */
    if((retCode = SpectrumAnalyzerClose(&(gEqptDescriptors.analyzerFd))) == RECONN_SUCCESS)
    {
       	/*
	 * This call will not only opens exclusive access to the analyzer but it
	 * will also read the current software verison into gSpecanaFirmwareVersion.
       	 */
       	if((retCode = SpectrumAnalyzerInit(NULL)) == RECONN_SUCCESS)
       	{
	    /*
	     * Make sure the analyzer is not in Bootloader mode by 
	     * sending an application level command. If the analyzer
	     * responds with valid data then it is in application mode.
	     * Place it Bootloader mode.
	     */
	    avcomRetCode = AVCOM_SA_GetActiveRefLevel(NULL, &refLevel);
	    switch (avcomRetCode)
	    {
	       	case AOV_NO_ERROR:
		{
		    //We are not in Bootloader mode
		    // - AVCOM_SA_GetActiveRefLevel is a known command only in the SA application
		    reconnDebugPrint ("%s: Not in bootloader mode...\n", __FUNCTION__);
		    bootLoaderMode=0;
                    break;
		}
	       	case AOV_ERR_UNKNOWN_CMD:
		{
		    //We are most likely already in bootloader mode because...
		    // - We were able to connect
		    // - GetActiveRefLevel is not a known bootloader command.
		    reconnDebugPrint ("%s: In bootloader mode...\n", __FUNCTION__);
		    bootLoaderMode=1;
		    break;
		}
	       	default:
		{
		    reconnDebugPrint("%s: Error %i at GetActiveRefLevel()\n", __FUNCTION__, avcomRetCode);
		    retCode = RECONN_FAILURE;
		}
	    }
	    /*
	     * Is there an analyzer upgrade file?
	     */
	    if(retCode == RECONN_SUCCESS)
	    {
                if (stat(UPGRADE_FW_NAME, &fileStat) == 0)
                {
                    reconnDebugPrint("%s: file size = %d\n", __FUNCTION__, fileStat.st_size);
                    if((specanaUpgradeDataPtr = malloc(sizeof(unsigned char) * fileStat.st_size)))
                    {
                        /*
                         * Open the file and read is contents into memory.
                         */
                        if((specanaUpgradeFilePtr = fopen(UPGRADE_FW_NAME, "r")) != NULL)
                        {
                            /*
                             * Copy the file to memory
                             */
                            for(i = 0; i < fileStat.st_size; i++)
                            {
                                specanaUpgradeDataPtr[i] = fgetc(specanaUpgradeFilePtr);
                            }
                            fclose(specanaUpgradeFilePtr);
                            specanaUpgradeFilePtr = NULL;
                            if (!bootLoaderMode)
                            {
                                reconnDebugPrint("%s: Tell analyzer to reboot into bootloader mode...\n", __FUNCTION__);
                                if((avcomRetCode = AVCOM_ENG_BootBootloader(NULL)) != AOV_NO_ERROR )
                                {
                                    AVCOM_SA_Reboot(NULL);
                                    reconnDebugPrint("%s: Error %i at AVCOM_ENG_BootBootloader()\n", __FUNCTION__, avcomRetCode);
                                    retCode = RECONN_FAILURE;
                                }
                                SpectrumAnalyzerClose(NULL);
                            } 
                            if(retCode == RECONN_SUCCESS)
                            {
                                //Wait for 4 seconds to allow reboot to complete
                                sleep(4);
                                if(SpectrumAnalyzerOpen(NULL) == RECONN_SUCCESS)
                                {
                                    /*
                                     * Flash the analyzer
                                     */
                                    reconnDebugPrint("%s: calling AVCOM_SA_FlashSBS2()\n", __FUNCTION__);
                                    if((avcomRetCode = AVCOM_SA_FlashSBS2(NULL,
                                                    (unsigned char *)specanaUpgradeDataPtr, fileStat.st_size)) != AOV_NO_ERROR)
                                    {
                                        reconnDebugPrint("%s: AVCOM_SA_FlashSBS2() failed %d \n", __FUNCTION__,
                                                avcomRetCode);
                                        retCode = RECONN_FAILURE;
                                    }
                                    else
                                    {
                                        reconnDebugPrint("%s: Rebooting SA \n", __FUNCTION__);
                                        AVCOM_SA_Reboot(NULL);
                                        SpectrumAnalyzerClose(NULL);
                                        sleep(1);

                                        if(reconnGpioAction(GPIO_141, DISABLE, NULL) == RECONN_FAILURE)
                                        {
                                            reconnDebugPrint("%s: reconnGpioAction(GPIO_141, ENABLE, NULL) failed. \n", __FUNCTION__);
                                        }
                                        sleep(1);
                                        if(SpectrumAnalyzerInit(&(gEqptDescriptors.analyzerFd)) == RECONN_SUCCESS)
                                        {
                                            reconnDebugPrint("%s: Calling AVCOM_SA_GetFirmwareVersion()\n", __FUNCTION__);
                                            if((avcomRetCode = AVCOM_SA_GetFirmwareVersion(NULL, &(gSpecanaFirmwareVersion))) == AOV_NO_ERROR)
                                            {
                                                reconnDebugPrint("%s: major = %d\n", __FUNCTION__, gSpecanaFirmwareVersion.major);
                                                reconnDebugPrint("minor = %d\n", gSpecanaFirmwareVersion.minor);
                                                reconnDebugPrint("build = %d\n", gSpecanaFirmwareVersion.build);
                                                reconnDebugPrint("stage = %d\n", gSpecanaFirmwareVersion.stage);
                                                reconnDebugPrint("string = %s\n", gSpecanaFirmwareVersion.string);

                                            }
                                            else
                                            {
                                                reconnDebugPrint("%s: AVCOM_SA_GetFirmwareVersion() failed %d\n", __FUNCTION__, avcomRetCode);
                                                retCode = RECONN_FAILURE;
                                            }
                                        }
                                        else
                                        {
                                            reconnDebugPrint("%s: line %d SpectrumAnalyzerOpen(NULL) failed\n", __FUNCTION__, __LINE__);
                                            retCode = RECONN_FAILURE;
                                        }
                                    }
                                }
                                else
                                {
                                    reconnDebugPrint("%s: line %d SpectrumAnalyzerOpen(NULL) failed\n", __FUNCTION__, __LINE__);
                                    retCode = RECONN_FAILURE;
                                }
                            }
                            if(specanaUpgradeDataPtr)
                            {
                                free(specanaUpgradeDataPtr);
                            }
                        }
                        else
                        {
                            reconnDebugPrint("%s: fopen(UPGRADE_FW_NAME, r) failed %d (%s)\n", __FUNCTION__, errno, strerror(errno));
                            retCode = RECONN_FAILURE;
                        }
                    }
                    else
                    {
                        reconnDebugPrint("%s: malloc(%d) failed %d(%s)\n", __FUNCTION__, sizeof(unsigned char) * fileStat.st_size, errno, strerror(errno));
                        retCode = RECONN_FAILURE;
                    }
                }
                else
                {
                    reconnDebugPrint("%s: file %s not found.\n", __FUNCTION__, UPGRADE_FW_NAME);
                    retCode = RECONN_UPGRADE_FILE_NOT_FOUND;
                }
            }
        }
    }
    unlink(UPGRADE_FW_NAME);
    return(retCode);
}
