//******************************************************************************
//******************************************************************************
//
// FILE:        libiphoned.h
//
// DESCRIPTION: header file for libiphoned.c to access library API
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
#ifndef LIBIPHONED_H_
#define LIBIPHONED_H_

int libiphoned_isiphonepresent(void);
int libiphoned_start(void);
int libiphoned_stop(void);
int libiphoned_tx(unsigned char *buf, unsigned int len);
int libiphoned_register_rx_callback(void(*callbackfn)(unsigned char *, int), int max_buffer);
int libiphoned_register_presence_change_callback(void(*callbackfn)(void));
int libiphoned_get_max_packet_len(void);
#endif /* LIBIPHONED_H_ */
