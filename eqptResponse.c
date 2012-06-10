//******************************************************************************
//****************************************************************************** //
// FILE:        eqptResponse.c
//
// CLASSES:     
//
// DESCRIPTION: This is the file which contains the thread which reads equipment
//              responses. If there is a response from a particular piece of 
//              test equipment, then this function gets the response and sends 
//              it to a queue for processing by another thread which sends the
//              response to all attached iPhone clients.
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
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <mqueue.h>
#include <errno.h>

#include "reconn.h"
#include "socket.h"
#include "eqptResponse.h"
#include "debugMenu.h"



#ifdef SOCKET_MUTEX
extern pthread_mutex_t socketMutex;
#endif
extern int libiphoned_tx(unsigned char *, unsigned int);
static struct mq_attr eqptMsgQattr;
static int socketIdList[RECONN_MAX_NUM_CLIENTS];
static char eqptMsgBuf[RECONN_RSP_PAYLOAD_SIZE];
static mqd_t EqptMsgQid;

ReconnErrCodes reconnRegisterClientApp(short theIndex, int theSocket)
{
    ReconnErrCodes retCode = RECONN_SUCCESS;

    if(theIndex < RECONN_MAX_NUM_CLIENTS)
    {
#ifdef SOCKET_MUTEX
        reconnDebugPrint("%s: Calling pthread_mutex_lock\n", __FUNCTION__);
        pthread_mutex_lock(&socketMutex);
#endif
        socketIdList[theIndex] = theSocket;
#ifdef SOCKET_MUTEX
        reconnDebugPrint("%s: Calling pthread_mutex_unlock\n", __FUNCTION__);
        pthread_mutex_unlock(&socketMutex);
#endif
    }
    else
    {
        reconnDebugPrint("%s: index out of range %d\n", __FUNCTION__, theIndex);
        retCode = RECONN_FAILURE;
    }
    return(retCode);
}

ReconnErrCodes reconnDeRegisterClientApp(short theIndex)
{
    ReconnErrCodes retCode = RECONN_SUCCESS;

    if(theIndex < RECONN_MAX_NUM_CLIENTS)
    {
#ifdef SOCKET_MUTEX
        pthread_mutex_lock(&socketMutex);
#endif
        close(socketIdList[theIndex]);
        socketIdList[theIndex] = -1;
#ifdef SOCKET_MUTEX
        pthread_mutex_lock(&socketMutex);
#endif
    }
    else
    {
        reconnDebugPrint("%s: index out of range %d\n", __FUNCTION__, theIndex);
        retCode = RECONN_FAILURE;
    }

    return(retCode);
}

int reconnClientsRegistered()
{
    int i, numClients = 0;
#ifdef SOCKET_MUTEX
    reconnDebugPrint("%s: Calling pthread_mutex_lock\n", __FUNCTION__);
    pthread_mutex_lock(&socketMutex);
#endif
    for(i = 0; i < RECONN_MAX_NUM_CLIENTS; i++)
    {
        if (socketIdList[i] != -1)
        {
            numClients++;
        }
    }
#ifdef SOCKET_MUTEX
    reconnDebugPrint("%s: Calling pthread_mutex_unlock\n", __FUNCTION__);
    pthread_mutex_unlock(&socketMutex);
#endif
    return (numClients);
}

ReconnErrCodes reconnEqptAddMsgToQ(const char *theMsgPtr, int theMsgSize)
{
    ReconnErrCodes retCode = RECONN_SUCCESS;
#ifdef DEBUG_EQPT
    reconnDebugPrint("%s: calling mq_send with theMsgSize = %d\n", __FUNCTION__, theMsgSize);
#endif
    if(theMsgPtr == NULL)
    {
        reconnDebugPrint("******** %s: theMsgPtr is NULL\n", __FUNCTION__);
        retCode = RECONN_FAILURE;
    }
    else if(theMsgSize > RECONN_RSP_PAYLOAD_SIZE + 6)
    {
        reconnDebugPrint("\n\n******** %s: theMsgSize == %d\n", __FUNCTION__, theMsgSize);
        retCode = RECONN_FAILURE;
    }
    else if(mq_send(EqptMsgQid, theMsgPtr, theMsgSize, 0) != 0)
    {
        reconnDebugPrint("\n\n******** %s: mq_send() failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
        retCode = RECONN_FAILURE;
    }
    return(retCode);
}



static void reconnEqptCleanUp()
{
    int i;

    reconnDebugPrint("%s: ***** Called\n", __FUNCTION__);
    for(i = 0; i < RECONN_MAX_NUM_CLIENTS; i++)
    {
        if(socketIdList[i] != -1)
        {
            close(socketIdList[i]);
        }
    }
    mq_close(EqptMsgQid);
    mq_unlink(EQPT_MSG_Q_NAME);
}

void *reconnEqptTask(void *args)
{
    int msgSize, i;
    static int state = 0;
    int bytesSent;

    UNUSED_PARAM(args);
    reconnDebugPrint("%s: ***** Started\n", __FUNCTION__);
    for(i = 0 ;i < RECONN_MAX_NUM_CLIENTS; i++)
    {
#ifdef SOCKET_MUTEX
        pthread_mutex_lock(&socketMutex);
#endif
        socketIdList[i]= -1;
#ifdef SOCKET_MUTEX
        pthread_mutex_lock(&socketMutex);
#endif
    }
    mq_unlink(EQPT_MSG_Q_NAME);
    memset((char*) &eqptMsgQattr, 0, sizeof(eqptMsgQattr));
    eqptMsgQattr.mq_flags    = EPQT_MSG_Q_FLAGS;
    eqptMsgQattr.mq_maxmsg   = EPQT_MSG_Q_SIZE;
    eqptMsgQattr.mq_msgsize  = RECONN_RSP_PAYLOAD_SIZE;

    EqptMsgQid = mq_open(EQPT_MSG_Q_NAME, O_RDWR | O_CREAT, NULL, NULL);
    if(EqptMsgQid == (mqd_t) -1)
    {
        reconnDebugPrint("%s: mq_open() failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
    }
    else
    {
        mq_getattr(EqptMsgQid, &eqptMsgQattr);
        while(1)
        {
            if((msgSize = mq_receive(EqptMsgQid, (char *)&eqptMsgBuf, eqptMsgQattr.mq_msgsize , NULL)) == -1)
            {
                reconnDebugPrint("%s: mq_receive failed %d (%s)\n", __FUNCTION__, errno, strerror(errno));
                break;
            }
            else
            {
#ifdef DEBUG_EQPT
                reconnDebugPrint("%s: received message of length %d \n", __FUNCTION__, msgSize);
#endif
                for(i = 0; i < RECONN_MAX_NUM_CLIENTS; i++)
                {
#ifdef SOCKET_MUTEX
                    pthread_mutex_lock(&socketMutex);
#endif
                    if(socketIdList[i] > 0)
                    {
#ifdef DEBUG_EQPT
                        reconnDebugPrint("%s: Sending message to client socket %d\n", __FUNCTION__, socketIdList[i]);
#endif
                        bytesSent = send(socketIdList[i], &eqptMsgBuf, msgSize, 0);
#ifdef DEBUG_EQPT
                        reconnDebugPrint("%s: done sending %d bytes\n", __FUNCTION__, bytesSent);
#endif
                    
                    } 
#ifdef SOCKET_MUTEX
                    pthread_mutex_lock(&socketMutex);
#endif
                }
#ifdef DEBUG_EQPT
                reconnDebugPrint("%s: Done sending message to clients\n", __FUNCTION__);
#endif
            }
        }
    }
    reconnDebugPrint("%s: ********** returning. Should not happen\n", __FUNCTION__);
    return &state;
}

void reconnGetEqptResponse(int theEqptFd, int theMsgId, int mySocketFd, ReconnMasterClientMode mode)
{
    fd_set theFileDescriptor;
    int retCode, length, payloadIndex;
#ifdef DEBUG_EQPT
    int debugIndex;
    char *debugPtr;
#endif
    struct timeval waitTime;
    struct stat FdStatus;
    ReconnResponsePacket thePacket;
    ReconnResponsePacket *thePacketPtr =&thePacket;


    FD_ZERO(&theFileDescriptor);
    FD_SET(theEqptFd, &theFileDescriptor);

    waitTime.tv_sec = 0;
    waitTime.tv_usec = 200000;

#ifdef DEBUG_EQPT
    reconnDebugPrint("%s: Calling select for Fd %d \n",__FUNCTION__, theEqptFd);
#endif

    ADD_RSPID_TO_PACKET(GENERIC_RESPONSE, thePacketPtr);
    ADD_MSGID_TO_PACKET(theMsgId, thePacketPtr);
    while(1)
    {
        if((retCode = select(theEqptFd+1, &theFileDescriptor, NULL, NULL, &waitTime)) < 0)
        {
            if(retCode < 0)
            {
                sendReconnResponse (mySocketFd, thePacket.messageId.Byte[0],
                        thePacket.messageId.Byte[1], RECONN_INVALID_MESSAGE, mode);
                reconnDebugPrint("%s: select failed %d(%s)\n",__FUNCTION__, errno, strerror(errno));
            }
        }
        else if(retCode == 0)
        {
#ifdef DEBUG_EQPT
            reconnDebugPrint("%s: select timedout\n",__FUNCTION__);
#endif
            // see if it was a timeout. If it is then the equipment did not need to respond so
            // simply return.
            break;
        }
        else if (retCode != 0)
        {
#ifdef DEBUG_EQPT
            reconnDebugPrint("%s:\n\n********************* select returned\n",__FUNCTION__);
#endif
            payloadIndex = 0;
            memset((char *)&(thePacket.dataPayload[0]), 0 , sizeof(RECONN_RSP_PAYLOAD_SIZE));

            if(FD_ISSET(theEqptFd,  &theFileDescriptor))
            {
#ifdef DEBUG_EQPT
                reconnDebugPrint("%s: Equipment has responded fd = %d \n", __FUNCTION__, theEqptFd);
#endif
                if (fstat(theEqptFd, &FdStatus) < 0)
                {
                    reconnDebugPrint("%s: fstat Failed %d (%s)\n", __FUNCTION__, errno, strerror(errno));
                    sendReconnResponse (mySocketFd, thePacket.messageId.Byte[0],
                            thePacket.messageId.Byte[1], RECONN_INVALID_MESSAGE, mode);
                }
                else if(S_ISCHR(FdStatus.st_mode))
                {
#ifdef DEBUG_EQPT
                    reconnDebugPrint("%s: is a character device\n", __FUNCTION__);
#endif
                    while((length = read(theEqptFd, 
                                    (unsigned char *)&(thePacket.dataPayload[payloadIndex]), 
                                    RECONN_RSP_PAYLOAD_SIZE)) > 0)
                    {
                        if(length < 0)
                        {
                            reconnDebugPrint("%s: Read Failed %d (%s)\n", __FUNCTION__, errno, strerror(errno));
                            sendReconnResponse (mySocketFd, thePacket.messageId.Byte[0],
                                    thePacket.messageId.Byte[1], RECONN_INVALID_MESSAGE, mode);
                            return;
                        }
#ifdef DEBUG_EQPT
                        reconnDebugPrint("%s: read %d bytes\n", __FUNCTION__, length);
#endif
                        payloadIndex += length;
                    }
#ifdef DEBUG_EQPT
                    debugPtr = (char *)&(thePacket.dataPayload[0]);
                    for(debugIndex = 0; debugIndex < payloadIndex; debugIndex++, debugPtr++)
                    {
                        reconnDebugPrint("0x%x ", *debugPtr);
                    }
                    reconnDebugPrint("\n");
#endif
                    ADD_DATA_LENGTH_TO_PACKET(payloadIndex, thePacketPtr);
                     //send(mySocketFd, (char *)thePacketPtr, payloadIndex + 6, 0);
                   if(mode == INSERTEDMASTERMODE) 
                   {
#ifdef DEBUG_EQPT
                       reconnDebugPrint("%s: Calling libiphoned_tx %d \n", __FUNCTION__,  payloadIndex + 6);
#endif
                       libiphoned_tx((unsigned char *)thePacketPtr, payloadIndex + 6);
                   }
#ifdef DEBUG_EQPT
                    reconnDebugPrint("%s: Calling reconnEqptAddMsgToQ %d \n", __FUNCTION__,  payloadIndex + 6);
#endif
                   reconnEqptAddMsgToQ((char *)thePacketPtr, payloadIndex + 6);
                }
                else
                {
                    receive_packet_data(theEqptFd, (unsigned char *)&(thePacket.dataPayload), &length);
                    ADD_DATA_LENGTH_TO_PACKET(length, thePacketPtr);
                    if(mode == INSERTEDMASTERMODE) 
                    {
                        libiphoned_tx((unsigned char *)thePacketPtr, length + 6);
                    }
                }
            }
        }
    }
#ifdef DEBUG_EQPT
    reconnDebugPrint("%s: returning\n",__FUNCTION__);
#endif
}

