//******************************************************************************
//******************************************************************************
//
// FILE:        upgrade.h
//
// CLASSES:     
//
// DESCRIPTION: Header file for the Reconn Upgrade Applications
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
#ifndef __UPGRADE_H
#define __UPGRADE_H

#define HEADER_VERSION_SIZE 8
#define HEADER_LENGTH_SIZE 2
#define VERSION_STRING_SIZE 10
#define PAYLOAD_LENGTH_SIZE 8
#define VERSIONS 3
#define MD5SUM_SIZE 32

#define UPGRADE_BUNDLE_NAME "/tmp/reconnBundle"
#define UPGRADE_HEADER_SUM_NAME "/tmp/headerSum"
#define UPGRADE_HEADER_DATA_NAME "/tmp/headerData"
#define UPGRADE_GZIP_NAME "/tmp/reconn-service.gz"
#define UPGRADE_RECONN_NAME "/tmp/reconn-service"

#define UPGRADE_INPROGRESS_FILE_NAME "/tmp/upgrade_inprogress"

typedef struct
{
    char headerVersion[HEADER_VERSION_SIZE];
    char headerLength[HEADER_LENGTH_SIZE];
    char headerChecksum[MD5SUM_SIZE];
    char payloadLength[PAYLOAD_LENGTH_SIZE];
    char payloadChecksum[MD5SUM_SIZE];
}UPGRADEBUNDLE;


extern ReconnErrCodes extractBundle();

#endif /* __UPGRADE_H */
