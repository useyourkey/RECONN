//******************************************************************************
//******************************************************************************
//
// FILE:        dmm.h
//
// CLASSES:     
//
// DESCRIPTION: Header file for the Digital Mulimeter module
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
#ifndef __DMM_H
#define __DMM_H

#include <semaphore.h>
#ifdef __SIMULATION__
#define DMM_SERIAL_PORT         "/dev/ttyS0"
#else
#define DMM_SERIAL_PORT         "/dev/ttyO0"
#endif
#define DMM_BAUD_RATE           B9600
#define DMM_DATABITS            CS8
#define DMM_STOPBITS            0
#define DMM_PARITYON            0
#define DMM_PARITY              0
#define DMM_MAX_RESPONSE        6
#define DMM_WAIT_TIME           150000
#define DMM_MAX_SHUTDOWN        "t9999\n"
#define DMM_SHUTDOWN_RESP       "9999t "
#define DMM_SOCKET_PORT         1071
#define DMM_STATUS_RSP_LEN      5
#define DMM_CONFIG_FILE_NAME    "/home/reconn/DMMConfig"
#define DMM_NUM_CONFIG_BYTES    2

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

typedef enum
{
    SAVE_DMM_STATE,
    DMM_SAVE_COMPLETE
}DMM_POWER_BUTTON_ENUMS;

typedef struct
{
    DMM_POWER_BUTTON_ENUMS theMessage;
}DMM_POWER_BUTTON_MSG;

extern ReconnErrCodes dmmInit(int *);
extern ReconnErrCodes dmmWrite(unsigned char *buffer, int length);
extern ReconnErrCodes dmmRead(unsigned char *buffer, int *length);
extern ReconnErrCodes makeDmmOutput(unsigned char *gps_outputbuffer, int *gps_outputlength);
extern ReconnErrCodes dmmDiags(void);
extern ReconnErrCodes dmmPowerDown();
extern void *dmmSaveConfigTask(void * args);
extern void dmmLoadSavedConfig();
extern void dmmSaveConfig();
extern pthread_mutex_t dmmMutex;
extern char *dmmCommands[];
extern int gDmmDebugLevel;

#endif /* __DMM_H */
