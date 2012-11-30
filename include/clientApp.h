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
#include "wifi.h"
#include <arpa/inet.h>
#include <sys/stat.h>
#include <mqueue.h>

#define INSERTED_MASTER_MSG_Q_NAME  "/InsMstQ"  // Front panel inserted iPhone
#define RECONNBEGIN "CFS RECONN 00.00 BEGIN"
#define CLIENTSLEEPTIME 1000 //microseconds
#ifndef __SIMULATION__
#define CLIENTNODATATIME 4000000 //microseconds (4 seconds)
#else
#define CLIENTNODATATIME 4000000 //microseconds (4 seconds)
#endif
#define IPADDR_LEN 16
#define MACADDR_LEN IPADDR_LEN
#define HOSTNAME_LEN 255 

#define MASTER_MSG_SIZE 4
typedef enum
{
    MASTER_INSERTED,    // master client has been inserted into front panel
    MASTER_EXTRACTED,   // Instered master client has been removed from front panel
    MASTER_ACK,         // WiFi master client acknowledment message
    LOW_BATTERY,        // Power Management 
    METER_INSERTED,     // Power meter insertion event
    METER_EXTRACTED,    // Power meter extraction event
    RM_STATE_CHANGE,    // Remote Monitor connection state change
}InsertedClientMessage;

typedef struct { 
    short   index;
    short serialBytesRead; 
    unsigned short cmdid; 
    unsigned short responseId;
    int     *thisContext;
    int     noDataCount;
    int     socketFd; 
    int     tmpSocketFd; 
    int     connectionOpen;
    int     pktLength; 
    int     length; 
    int     responseNeeded; 
    int     retStatus;
    int     debugIndex; 
    int     packetRetCode; 
    int     flags; 
    uint16_t portNum; 
    ssize_t numBytes;
#ifdef DEBUG_CLIENT 
    int debugIndex; 
    char *debugPtr; 
#endif
    char    *debugPtr; 
    char IPAddr[IPADDR_LEN];
    char SubnetMask[MACADDR_LEN];
    char newSSID[WIFI_SSID_MAX_LENGTH + 1]; 
    char newPasswd[WIFI_PASSWD_MAX_LENGTH + 1];
    char theSerialNumberString[RECONN_SERIALNUM_MAX_LEN]; 
    char theMessage[MASTER_MSG_SIZE];
    char theWebServerHostName[HOSTNAME_LEN + 1];
    FILE *tmpFd;
    struct sockaddr_in sourceIp;
    struct stat fileStat;
    struct ifreq ifr;
    ReconnPacket thePacket;
    ReconnResponsePacket theResponsePkt;
    ReconnResponsePacket *theResponsePktPtr;
    ReconnErrCodes retCode;
    ReconnEqptDescriptors *eqptDescriptors;
    ReconnMasterClientMode mode;
}CLIENTCONTEXT;

extern CLIENTCONTEXT *activeClientsList[];
extern mqd_t masterClientMsgQid;

void * reconnClientTask(void *);
ReconnErrCodes formatReconnPacket(int, char *, int, ReconnPacket *);
void reconnGetEqptResponse(int , int , int, ReconnMasterClientMode); 
void insertedMasterRead(unsigned char *, int); 
extern void registerDebugCommand();
extern pthread_mutex_t clientListMutex;
extern int clientList(int);
extern ReconnErrCodes getDeviceId(CLIENTCONTEXT *);
#ifdef RECONN_TIMING
extern int timeval_subtract (result, x, y);
#endif

#endif
