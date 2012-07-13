//******************************************************************************
//******************************************************************************
//
// FILE:        clientApp.h.c
//
// CLASSES:     
//
// DESCRIPTION: This is the file which contains the main function for the 
//              reconn application
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
#ifndef __CLIENTAPP_H
#define __CLIENTAPP_H

#define INSERTED_MASTER_MSG_Q_NAME  "/InsMstQ"  // Front panel inserted iPhone
#define RECONNBEGIN "CFS RECONN 00.00 BEGIN"

#define INSERTED_MASTER_MSG_SIZE 4
typedef enum
{
    MASTER_INSERTED,    // master client has been inserted into front panel
    MASTER_EXTRACTED,   // Instered master client has been removed from front panel
    MASTER_ACK          // WiFi master client acknowledment message
}InsertedClientMessage;

void * reconnClientTask(void *);
<<<<<<< HEAD
ReconnErrCodes receive_packet_data(int, unsigned char *, int *);
=======
int receive_packet_data(int, unsigned char *, int *);
>>>>>>> e8268624e0e87864409446d6a84c51cb33eb182b
ReconnErrCodes formatReconnPacket(int, char *, int, ReconnPacket *);
void reconnGetEqptResponse(int , int , int, ReconnMasterClientMode);
void insertedMasterRead(unsigned char *, int);

extern void registerDebugCommand();

#endif
