//******************************************************************************
//******************************************************************************
//
// FILE:        spectrum.h
//
// CLASSES:     
//
// DESCRIPTION: Header file for the Spectrum Analizer module
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
#ifndef __SPECTRUM_H
#define __SPECTRUM_H

#define SPECTRUM_ANALYZER_DEV       "/dev/AvcomAnalyzer"
#define SPECTRUM_ANALYZER_BAUD_RATE B460800
#define SPECTRUM_ANALYZER_DATABITS  CS8
#define SPECTRUM_ANALYZER_STOPBITS  0
#define SPECTRUM_ANALYZER_PARITYON  0
#define SPECTRUM_ANALYZER_PARITY    0
#define INVALID_COMMAND "Invalid SA Command\r\n\0"


extern ReconnErrCodes SpectrumAnalyzerInit(int *);
extern ReconnErrCodes SpectrumAnalyzerWrite(unsigned char *, int );
extern ReconnErrCodes SpectrumAnalyzerRead(unsigned char *, int *);
extern ReconnErrCodes SpectrumAnalyzerClose(int *);
extern ReconnErrCodes makeSpectrumAnalyzerOutput(unsigned char *, int *);
extern ReconnErrCodes SpectrumAnalyzerSelectRead(unsigned char *, int *, struct timeval *);
extern ReconnErrCodes SpectrumAnalyzerUpgrade();
#endif /* __SPECTRUM_H */

