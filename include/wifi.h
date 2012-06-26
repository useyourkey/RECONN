//******************************************************************************
//******************************************************************************
//
// FILE:        wifi.h
//
// CLASSES:     
//
// DESCRIPTION: Header file for the wifi functionality
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
#ifndef __WIFI_H
#define __WIFI_H

// remove this line for target installation

#define WIFI_CONF_FILE_ORIG "/etc/hostapd.conf"
#define WIFI_CONF_FILE_NEW "/etc/hostapd.new"

#define WIFI_SSID_MAX_LENGTH    33
#define WIFI_PASSWD_MAX_LENGTH  63


#define WIFI_SSID_TOKEN "ssid="
#define WIFI_PASSWD_TOKEN "wpa_passphrase="

ReconnErrCodes wifiUpdateConfFile(char *token, char *theNewValue);


#endif // __WIFI_H
