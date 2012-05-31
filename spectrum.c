//******************************************************************************
//******************************************************************************
//
// FILE:        spectrum.c
//
// FUNCTIONS:   SpectrumAnalyzerInit()
//              SpectrumAnalyzerOpen()
//              SpectrumAnalyzerWrite()
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
#include <termios.h>
#include <unistd.h>
#include <errno.h>

#include "reconn.h"
#include "spectrum.h"
#include "gpio.h"

#define SPECTRUM_ANALYZER_DEV       "/dev/ttyUSB0"
#define SPECTRUM_ANALYZER_BAUD_RATE B460800
#define SPECTRUM_ANALYZER_DATABITS  CS8
#define SPECTRUM_ANALYZER_STOPBITS  0
#define SPECTRUM_ANALYZER_PARITYON  0
#define SPECTRUM_ANALYZER_PARITY    0


#define INVALID_COMMAND "Invalid SA Command\r\n\0"

int myAnalyzerFd;
int SAPortInit = FALSE;
struct termios analyzerSerial;

static ReconnErrCodes SpectrumAnalyzerOpen(int *fileDescriptor) 
{
    ReconnErrCodes retCode = RECONN_SUCCESS;

    if((myAnalyzerFd = open(SPECTRUM_ANALYZER_DEV, O_RDWR | O_NOCTTY | O_NONBLOCK)) < 0)
    {
        reconnDebugPrint("%s: Failed to open %s\n\r", __FUNCTION__, SPECTRUM_ANALYZER_DEV);
        /* failed to open the device */
        retCode = RECONN_SERIAL_PORT_OPEN_FAIL;
    }
    else
    {
        reconnDebugPrint("\n%s: serial port %s Opened\n", __FUNCTION__, SPECTRUM_ANALYZER_DEV);
        reconnDebugPrint("\n%s: Spectrum Analyer FD = %d Opened\n", __FUNCTION__, myAnalyzerFd);

        *fileDescriptor = myAnalyzerFd;
        //analyzerSerial.c_cflag = SPECTRUM_ANALYZER_BAUD_RATE | CRTSCTS | SPECTRUM_ANALYZER_DATABITS |
        analyzerSerial.c_cflag = SPECTRUM_ANALYZER_BAUD_RATE | SPECTRUM_ANALYZER_DATABITS |
            SPECTRUM_ANALYZER_STOPBITS | SPECTRUM_ANALYZER_PARITYON | SPECTRUM_ANALYZER_PARITY | 
            //SPECTRUM_ANALYZER_STOPBITS | SPECTRUM_ANALYZER_PARITYON | SPECTRUM_ANALYZER_PARITY | 
            CLOCAL | CREAD;
        analyzerSerial.c_iflag = IGNPAR;
        analyzerSerial.c_oflag = 0;
        analyzerSerial.c_lflag = 0;
        analyzerSerial.c_cc[VMIN] = 1;
        analyzerSerial.c_cc[VTIME] = 0;
        tcflush(myAnalyzerFd, TCIFLUSH);
        tcsetattr(myAnalyzerFd, TCSANOW, &analyzerSerial);

        SAPortInit = TRUE;
        /* end Power Meter Init */
        reconnDebugPrint("%s: Spectrum Analyzer Init Complete\n\n", __FUNCTION__);
        /* Now send any commands that might need to be used to initialize the Power Meter */
    }


#if 0
    myAnalyzerFd = socket(AF_INET, SOCK_STREAM, 0);
    if (myAnalyzerFd < 0)
    {
        reconnDebugPrint("%s: ERROR opening sa socket %d %s", __FUNCTION__, errno, strerror(errno));
        retCode = RECONN_FAILURE;
    }
    else
    {
        hp = gethostbyname(SPECTRUM_ANALYZER_IP);
        bcopy(hp->h_addr, &(SA_IP_addr.sin_addr.s_addr), hp->h_length);

        SA_port = SPECTRUM_ANALYZER_SOCKET;

        bzero((unsigned char *) &SA_IP_addr, sizeof(SA_IP_addr));

        SA_IP_addr.sin_family = AF_INET;

        bcopy((unsigned char *) hp->h_addr, (unsigned char *) &SA_IP_addr.sin_addr.s_addr,
                hp->h_length);

        SA_IP_addr.sin_port = htons(SA_port);

        if (connect(myAnalyzerFd, (struct sockaddr *) &SA_IP_addr, sizeof(SA_IP_addr)) < 0) 
        {
            reconnDebugPrint("%s: Spectrum Analyzer connect failed to %s\r\n", __FUNCTION__, SPECTRUM_ANALYZER_IP);
            retCode = RECONN_CONNECT_FAILED;
        }
        else
        {
            *fileDescriptor = myAnalyzerFd;
            reconnDebugPrint("%s: SA Connect Opened\r\n", __FUNCTION__);
            flags = fcntl(myAnalyzerFd, F_GETFD);
            flags = flags | O_NONBLOCK;
            fcntl(myAnalyzerFd, F_SETFD, &flags);
        }
    }
#endif
    return (retCode);
}

ReconnErrCodes SpectrumAnalyzerInit(int *fileDescriptor) 
{
    int ret_status = 0;
    ReconnErrCodes retcode = RECONN_SUCCESS;

    // Apply power to the analyzer.
    *fileDescriptor = -1;
    if(reconnGpioAction(GPIO_141, ENABLE) == RECONN_FAILURE)
    {
        reconnDebugPrint("%s: reconnGpioAction(GPIO_141, ENABLE) failed. \n", __FUNCTION__);
        retcode = RECONN_FAILURE;
    }
    else 
    {
        // Power has been applied, wait for the usb drivers to come up before attempting to opening it.
        sleep(5); 
        if((ret_status = SpectrumAnalyzerOpen(fileDescriptor)) == RECONN_SUCCESS)
        {
            SAPortInit = TRUE;
        }
        else
        {
            retcode = RECONN_FAILURE;
        }
    }
    return (retcode);
}

/* generic write to usb Power Meter */
ReconnErrCodes SpectrumAnalyzerWrite(unsigned char *buffer, int length) 
{
    ReconnErrCodes retcode = RECONN_SUCCESS;
    int debugIndex; 

    if (SAPortInit == FALSE) 
    {
        reconnDebugPrint("%s: Spectrum Analyzer port is not initialized\n", __FUNCTION__);
        retcode =  RECONN_FAILURE;
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
            reconnDebugPrint("%s: write(%d, buffer, %d) failed\n\r", __FUNCTION__, myAnalyzerFd, length);
        }
#ifdef DEBUG_SPECTRUM
        reconnDebugPrint("\n");
#endif
    }
    return (retcode);
}

/* generic read from the usb Power Meter */
ReconnErrCodes SpectrumAnalyzerRead(unsigned char *buffer, int *length) 
{
    int NumberBytes = 0;

    ReconnErrCodes retcode = RECONN_SUCCESS;

    if (SAPortInit == FALSE) 
    {
        reconnDebugPrint("%s: Spectrum Analyzer port is not initialized\n", __FUNCTION__);
        retcode = RECONN_FAILURE;
    }
    else 
    {
        *length = 0;
        if((NumberBytes = recv(myAnalyzerFd, buffer, MAX_COMMAND_INPUT, 0)) > 0)
        {
            *length = NumberBytes;
        }
    }
    return (retcode);
}

/* generic close command */
ReconnErrCodes SpectrumAnalyzerClose(void)
{
    close(myAnalyzerFd);
    return (RECONN_SUCCESS);
}
