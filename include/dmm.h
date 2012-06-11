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
#define DMM_MAX_RESPONSE        5
#define DMM_MAX_SHUTDOWN        "t9999\n"
#define DMM_SHUTDOWN_RESP       "9999t"

ReconnErrCodes dmmInit(int *);
ReconnErrCodes dmmWrite(unsigned char *buffer, int length);
ReconnErrCodes dmmRead(unsigned char *buffer, int *length);
ReconnErrCodes makeDmmOutput(unsigned char *gps_outputbuffer, int *gps_outputlength);
ReconnErrCodes dmmDiags(void);
#endif /* __DMM_H */
