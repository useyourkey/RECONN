//******************************************************************************
//******************************************************************************
//
// FILE:        gps.h.h.c
//
// CLASSES:     
//
// DESCRIPTION: Header file for the Global Positioning System module
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
#ifndef __GPS_H
#define __GPS_H

#define GPS_SERIAL_PORT          "/dev/ttyS1"
#define GPS_BAUD_RATE            B9600
#define GPS_DATABITS             CS8
#define GPS_STOPBITS             0
#define GPS_PARITYON             0
#define GPS_PARITY               0

ReconnErrCodes gpsInit(int *fileDescriptor);
ReconnErrCodes gpsWrite(unsigned char *buffer, int length);
ReconnErrCodes gpsRead(unsigned char *buffer, int *length);
#endif /* __GPS_H */
