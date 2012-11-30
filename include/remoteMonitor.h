//******************************************************************************
//******************************************************************************
//
// FILE:        remoteMonitor.h
//
// CLASSES:     
//
// DESCRIPTION: Header file for the Reconn Remote Monitor 
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
#ifndef __REMOTE_H
#define __REMOTE_H

#define REMOTE_MONITOR_PORT 1068
#define REMOTE_MONITOR_MAX_CLIENTS 1
#define REMOTE_MONITOR_KEEPALIVE_TIME 15 // seconds

typedef enum{
    RM_SEND_DATA = 1,
    RM_STOP_DATA
}REMOTE_MONITOR_DATA_STATES;

typedef enum{
    CONNECTED = 1,
    DISCONNECTED 
}REMOTE_MONITOR_CONNECTION_STATES;

typedef enum
{
    RM_INIT,
    RM_GET_HANDSHAKE,
    RM_SEND_CLIENT_ACCESS,
    RM_WAIT_RESP,
    RM_DEVICE_ID,
    RM_RCV_DATA
}REMOTE_MONITOR_STATES;

typedef struct
{
    int webServerSocket;
    //char IPAddress[RECONN_IPADDR_LEN + 1]; // account for null terminator
    //int port;
    int remoteMonitorDone;
    YESNO remoteMonitorActive;
    CLIENTCONTEXT *theContextPtr;
    REMOTE_MONITOR_STATES theState;
}REMOTE_MONITOR_DEBUG_DATA;

extern int stopRemoteMonitorHb;
extern void *remoteMonitorTask(void *args);
extern void remoteMonitorCleanup();
extern ReconnErrCodes remoteMonitorActivate(YESNO, CLIENTCONTEXT *);
extern YESNO getRemoteMonitorState();
extern void remoteMonitorGetDebugValues(REMOTE_MONITOR_DEBUG_DATA *);
#endif /* __REMOTE_H */
