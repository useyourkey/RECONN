//******************************************************************************
//******************************************************************************
//
// FILE:        socket.c
//
// CLASSES:     
//
// DESCRIPTION:  General socket functions used by reconn software
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <errno.h>

#include "reconn.h"
#include "socket.h"


int numberOfOpenSocket = 0;
#ifdef SOCKET_MUTEX
pthread_mutex_t socketMutex = PTHREAD_MUTEX_INITIALIZER;
#endif
int socketIdList[RECONN_MAX_NUM_CLIENTS];

void sendSocket(int socket_fd, unsigned char * buffer_s, int length, int num)
{
    int errCode;
#ifdef COMM_DEBUG
    printf("%s: ", __FUNCTION__);
    for (i = 0; i < length; ++i) {
        printf("<%x>", (unsigned int) buffer_s[i]);
    }
    printf("\n\n\n");
#endif
    errCode = send(socket_fd, buffer_s, length, num);
    if(errCode == -1)
    {
        printf("%s: send failed %d %s\n", __FUNCTION__, errno, strerror(errno));
    }
    else if(errCode != length)
    {
        printf("%s: send failed to send %d bytes\n", __FUNCTION__, length);
    }
}

void sendToSocketList(unsigned char *msg_ptr, int size)
{
    int counter = 0;
    if (numberOfOpenSocket > 0)
    {

        for (counter = 0; counter < numberOfOpenSocket; counter++)
        {
#ifdef SOCKET_MUTEX
            pthread_mutex_lock(&socketMutex);
#endif
            sendSocket(socketIdList[counter], msg_ptr, size, 0);
#ifdef SOCKET_MUTEX
            pthread_mutex_unlock(&socketMutex);
#endif
        }
    }
}

void sendReconnResponse(int socket_fd, unsigned char c1, unsigned char c2, ReconnErrCodes ErrCode)
{
    ReconnResponsePacket buff;
    ReconnResponsePacket *thePacket = &buff;

    memset((char *)&buff, 0 , sizeof(ReconnResponsePacket));
    ADD_RSPID_TO_PACKET(GENERIC_RESPONSE, thePacket);
    ADD_DATA_LENGTH_TO_PACKET(1, thePacket);
    thePacket->messageId.Byte[LOW] = c1;
    thePacket->messageId.Byte[HIGH] = c2;
    thePacket->dataPayload[0] = ErrCode;
    sendSocket(socket_fd, (unsigned char *)thePacket, 7, 0);
}
