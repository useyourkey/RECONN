//*****************************************************************************
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
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>

#include <mqueue.h>
#include <errno.h>

#include "reconn.h"
#include "socket.h"
#include "eqptResponse.h"
#include "debugMenu.h"
#include "dmm.h"
#include "spectrum.h"
#include "powerMeter.h"
#include "remoteMonitor.h"

//#define DEBUG_EQPT

#define TIMEVALSUBTRACT(result, a, b) result.tv_sec = a.tv_sec - b.tv_sec;\
                                          result.tv_usec = a.tv_usec - b.tv_usec;\
if (result.tv_usec < 0)\
{\
        --(result.tv_sec);\
        result.tv_usec += 1000000;\
}

extern pthread_mutex_t socketMutex;
extern int libiphoned_tx(unsigned char *, unsigned int);
static struct mq_attr eqptMsgQattr;
int socketIdList[RECONN_MAX_NUM_CLIENTS];
//static char eqptMsgBuf[RECONN_RSP_PAYLOAD_SIZE + RECONN_RSPPACKET_HEADER_SIZE];
static ReconnResponsePacket theResponsePacket;
static mqd_t EqptMsgQid;
int gAnalyzerHigh = 0;
int gDmmHigh = 0;
int gPowerMeterHigh = 0;
int gPayloadHigh = 0;
int gDmmResponses = 0;
int gAnalyzerResponses = 0;
int gMeterResponses = 0;
static int gEqptTaskDone = FALSE;
static int gEqptGetTaskDone = FALSE;

//******************************************************************************
//******************************************************************************
// FUNCTION:    reconnRegisterClientApp
//
// CLASSES:     
//
// DESCRIPTION: Interface used to register an iPhone client with the reconn 
//              embedded software. By registerin a context to the embedded software
//              all data returned from the SA, DMM, and power meter will be sent to 
//              theContext's socket. This is accomplised by adding theContext's socket
//              descriptor to socketIdList[] array.
//              
// PARAMETERS:
//              theContext  - a pointer to the context which is to be added to socketIdList[]
//
// RETURNS:
//              RECONN_INVALID_PARAMETER    - theContext is NULL
//              RECONN_SUCCESS              - theContext has been added
//              RECONN_FAILURE              - theContext->index is out of range
//
//******************************************************************************
ReconnErrCodes reconnRegisterClientApp(CLIENTCONTEXT *theContext)
{
    ReconnErrCodes retCode = RECONN_SUCCESS;

    if(theContext == NULL)
    {
        reconnDebugPrint("%s: NULL context ptr%d\n", __FUNCTION__);
        retCode = RECONN_INVALID_PARAMETER;
    }
    else if(theContext->index < RECONN_MAX_NUM_CLIENTS)
    {
        /*
         * Index 0 is the inserted master and it does not
         * need to go on the socket list because it
         * uses a different mechanism to received its data
         */
        if(theContext->index != 0)
        {
#ifdef DEBUG_MUTEX
            reconnDebugPrint("%s: Calling pthread_mutex_lock(&socketMutex) %d %d %d\n", __FUNCTION__, socketMutex.__data.__lock, socketMutex.__data.__count, socketMutex.__data.__owner);
#endif
            pthread_mutex_lock(&socketMutex);
            socketIdList[theContext->index] = theContext->socketFd;

#ifdef DEBUG_MUTEX
            reconnDebugPrint("%s: Calling pthread_mutex_unlock(&socketMutex) %d %d %d\n", __FUNCTION__, socketMutex.__data.__lock, socketMutex.__data.__count, socketMutex.__data.__owner);
#endif
            pthread_mutex_unlock(&socketMutex);
        }

#ifdef DEBUG_MUTEX
        reconnDebugPrint("%s: Calling pthread_mutex_lock(&clientListMutex) %d %d %d\n", __FUNCTION__, clientListMutex.__data.__lock, clientListMutex.__data.__count, clientListMutex.__data.__owner);
#endif

        pthread_mutex_lock(&clientListMutex);
        activeClientsList[theContext->index] = theContext;
#ifdef DEBUG_MUTEX
        reconnDebugPrint("%s: Calling pthread_mutex_unlock(&clientListMutex) %d %d %d\n", __FUNCTION__, clientListMutex.__data.__lock, clientListMutex.__data.__count, clientListMutex.__data.__owner);
#endif
        pthread_mutex_unlock(&clientListMutex);
    }
    else
    {
        reconnDebugPrint("%s: index out of range %d\n", __FUNCTION__, theContext->index);
        retCode = RECONN_FAILURE;
    }
    return(retCode);
}

//******************************************************************************
//******************************************************************************
// FUNCTION:    reconnDeRegisterClientApp
//
// CLASSES:     
//
// DESCRIPTION: Interface used to remove theContext's socket descriptor from socketIdList[] array.
//              
// PARAMETERS:
//              theContext  - a pointer to the context which is to be removed from socketIdList[]
//
// RETURNS:
//              RECONN_INVALID_PARAMETER    - theContext is NULL
//              RECONN_SUCCESS              - theContext has been added
//              RECONN_FAILURE              - theContext->index is out of range
//
//******************************************************************************
ReconnErrCodes reconnDeRegisterClientApp(CLIENTCONTEXT *theContext)
{
    ReconnErrCodes retCode = RECONN_SUCCESS;

    if(theContext == NULL)
    {
        reconnDebugPrint("%s: NULL context ptr\n", __FUNCTION__);
        retCode = RECONN_INVALID_PARAMETER;
    }
    else if(theContext->index < RECONN_MAX_NUM_CLIENTS)
    {
        /*
         * Index 0 is the inserted master and it does not
         * need to be removed from the socket list because it
         * uses a different mechanism to received its data.
         */
        if(theContext->index != 0)
        {
#ifdef DEBUG_MUTEX
            reconnDebugPrint("%s: Calling pthread_mutex_lock(&socketMutex) %d %d %d\n", __FUNCTION__, socketMutex.__data.__lock, socketMutex.__data.__count, socketMutex.__data.__owner);
#endif
            pthread_mutex_lock(&socketMutex);

            if(socketIdList[theContext->index] != -1)
            {
                /*
                 * A remote client can be deregistered and remain connected. So do not close its
                 * socket. remoteMonitorTask will be responsible for closing the socket.
                 */
                if(theContext->mode != REMOTEMODE)
                {
                    reconnDebugPrint("%s: client %d closing socket %d\n", __FUNCTION__, theContext->index, 
                            socketIdList[theContext->index]);
                    if(close(socketIdList[theContext->index]) != 0)
                    {
                        reconnDebugPrint("%s: close(%d) failed %d(%s)\n", __FUNCTION__, theContext->index, errno, 
                                strerror(errno));
                    }
                }
                socketIdList[theContext->index] = -1;
            }

#ifdef DEBUG_MUTEX
            reconnDebugPrint("%s: Calling pthread_mutex_unlock(&socketMutex) %d %d %d\n", __FUNCTION__, socketMutex.__data.__lock, socketMutex.__data.__count, socketMutex.__data.__owner);
#endif
            pthread_mutex_unlock(&socketMutex);
        }

#ifdef DEBUG_MUTEX
        reconnDebugPrint("%s: Calling pthread_mutex_lock(&clientListMutex) %d %d %d\n", __FUNCTION__, clientListMutex.__data.__lock, clientListMutex.__data.__count, clientListMutex.__data.__owner);
#endif

        pthread_mutex_lock(&clientListMutex);
        activeClientsList[theContext->index] = 0;

#ifdef DEBUG_MUTEX
        reconnDebugPrint("%s: Calling pthread_mutex_unlock(&clientListMutex) %d %d %d\n", __FUNCTION__, clientListMutex.__data.__lock, clientListMutex.__data.__count, clientListMutex.__data.__owner);
#endif

        pthread_mutex_unlock(&clientListMutex);
    }
    else
    {
        reconnDebugPrint("%s: index out of range %d\n", __FUNCTION__, theContext->index);
        retCode = RECONN_FAILURE;
    }

    return(retCode);
}

//******************************************************************************
//******************************************************************************
// FUNCTION:    reconnClientsRegistered
//
// DESCRIPTION: This function will return the number of clients registered relative
//              to a client type or all clients.
//              
// PARAMETERS:
//              theType  -  an enumerated client type (ALL, SLAVE, REMOTE)
//
// RETURNS:     the number of clients registered
//
//******************************************************************************
int reconnClientsRegistered(REGISTRATION_TYPE_e theType)
{
    int i, numClients = 0;

#ifdef DEBUG_MUTEX
    reconnDebugPrint("%s: Calling pthread_mutex_lock(&socketMutex) %d %d %d\n", __FUNCTION__, socketMutex.__data.__lock, socketMutex.__data.__count, socketMutex.__data.__owner);
#endif
    pthread_mutex_lock(&socketMutex);
#ifdef DEBUG_MUTEX
    reconnDebugPrint("%s: Calling pthread_mutex_lock(&clientListMutex) %d %d %d\n", __FUNCTION__, clientListMutex.__data.__lock, clientListMutex.__data.__count, clientListMutex.__data.__owner);
#endif

    pthread_mutex_lock(&clientListMutex); 
    for(i = 0; i < RECONN_MAX_NUM_CLIENTS; i++)
    {
        if (activeClientsList[i])
        {
            if((theType == ALL) || 
                    ((theType == REMOTE) && (activeClientsList[i]->mode == REMOTEMODE)) || 
                    ((theType == SLAVE) && (activeClientsList[i]->mode == SLAVEMODE)))
            {
                //reconnDebugPrint("%s: mode %s found\n", __FUNCTION__, (activeClientsList[i]->mode == REMOTEMODE) ? "REMOTE" : (activeClientsList[i]->mode == SLAVEMODE) ? "SLAVE": (activeClientsList[i]->mode == INSERTEDMASTERMODE) ? "INSERTED MASTER": "WIFI MASTER");
                numClients++;
            }
        }
    }
#ifdef DEBUG_MUTEX
    reconnDebugPrint("%s: Calling pthread_mutex_unlock(&socketMutex) %d %d %d\n", __FUNCTION__, socketMutex.__data.__lock, socketMutex.__data.__count, socketMutex.__data.__owner);
#endif
    pthread_mutex_unlock(&socketMutex);

#ifdef DEBUG_MUTEX
    reconnDebugPrint("%s: Calling pthread_mutex_unlock(&clientListMutex) %d %d %d\n", __FUNCTION__, clientListMutex.__data.__lock, clientListMutex.__data.__count, clientListMutex.__data.__owner);
#endif

    pthread_mutex_unlock(&clientListMutex);
    return (numClients);
}

//******************************************************************************
//******************************************************************************
// FUNCTION:    reconnEqptAddMsgToQ
//
// CLASSES:     
//
// DESCRIPTION: Interface used to add a message to the equipment message queue. 
//              The queue is used to buffer the equipment which is sent to all 
//              registered clients.
//              
// PARAMETERS:
//              theMsgPtr   - a pointer to the message.
//              theMsgSize  - the message's length.
//
// RETURNS:
//              RECONN_SUCCESS              - The message was successfully added to the queue
//              RECONN_FAILURE              - The message could not be added
//
//******************************************************************************
ReconnErrCodes reconnEqptAddMsgToQ(const char *theMsgPtr, int theMsgSize)
{
    ReconnErrCodes retCode = RECONN_SUCCESS;
#ifdef DEBUG_EQPT
    reconnDebugPrint("%s: calling mq_send with theMsgSize = %d EqptMsgQid = %d\n", __FUNCTION__, theMsgSize, EqptMsgQid);
#endif
    if(theMsgPtr == NULL)
    {
        reconnDebugPrint("******** %s: theMsgPtr is NULL\n", __FUNCTION__);
        retCode = RECONN_FAILURE;
    }
    else if(theMsgSize > RECONN_RSP_PAYLOAD_SIZE + RECONN_RSPPACKET_HEADER_SIZE)
    {
        reconnDebugPrint("\n\n******** %s: theMsgSize == %d\n", __FUNCTION__, theMsgSize);
        retCode = RECONN_FAILURE;
    }
    else if(mq_send(EqptMsgQid, theMsgPtr, theMsgSize, 0) != 0)
    {
        reconnDebugPrint("\n\n******** %s: mq_send() failed %d(%s) EqptMsgQid = %d\n", __FUNCTION__, errno, strerror(errno), EqptMsgQid);
        retCode = RECONN_FAILURE;
    }
#ifdef DEBUG_EQPT
    reconnDebugPrint("%s: returning %d \n", __FUNCTION__, retCode);
#endif
    return(retCode);
}



//******************************************************************************
//******************************************************************************
// FUNCTION:    reconnEqptCleanUp
//
// CLASSES:     
//
// DESCRIPTION: Interface used to clean up all equipment related system resources
//              
// PARAMETERS:
//
// RETURNS:
//
//******************************************************************************
void reconnEqptCleanUp()
{
    int i;

    reconnDebugPrint("%s: ***** Called\n", __FUNCTION__);

    if(gEqptTaskDone == FALSE)
    {
        gEqptTaskDone = TRUE;
        gEqptGetTaskDone = TRUE;
#ifdef DEBUG_MUTEX
        reconnDebugPrint("%s: Calling pthread_mutex_lock(&socketMutex) %d %d %d\n", __FUNCTION__, socketMutex.__data.__lock, socketMutex.__data.__count, socketMutex.__data.__owner);
#endif

        pthread_mutex_lock(&socketMutex);
        for(i = 0; i < RECONN_MAX_NUM_CLIENTS; i++)
        {
            if(socketIdList[i] != -1)
            {
                shutdown(socketIdList[i], 2);
                if(close(socketIdList[i]) != 0)
                {
                    reconnDebugPrint("%s: Close(%d) failed %d (%s)\n", __FUNCTION__, errno, strerror(errno));
                }
            }
        }
#ifdef DEBUG_MUTEX
        reconnDebugPrint("%s: Calling pthread_mutex_unlock(&socketMutex) %d %d %d\n", __FUNCTION__, socketMutex.__data.__lock, socketMutex.__data.__count, socketMutex.__data.__owner);
#endif

        pthread_mutex_unlock(&socketMutex);
        mq_close(EqptMsgQid);
        mq_unlink(EQPT_MSG_Q_NAME);
    }
}

//*************************************************************************************
//*************************************************************************************
// FUNCTION:    reconnEqptTask
//
// DESCRIPTION: This is the task responsible for sending any equipment data to the
//              clients. It reads a queue that is filled by reconnGetEqptResponseTask().
//              This task then sends that queued data to all registered clients.
//
// Parameters:
//
//*************************************************************************************
void *reconnEqptTask(void *args)
{
    int msgSize, i;
    int msgId;
    static int state = 0;

    UNUSED_PARAM(args);
    atexit(reconnEqptCleanUp);
#ifdef DEBUG_MUTEX
    reconnDebugPrint("%s: Calling pthread_mutex_lock(&socketMutex) %d %d %d\n", __FUNCTION__, socketMutex.__data.__lock, socketMutex.__data.__count, socketMutex.__data.__owner);
#endif

    pthread_mutex_lock(&socketMutex);
    for(i = 0 ;i < RECONN_MAX_NUM_CLIENTS; i++)
    {
        socketIdList[i]= -1;
    }
#ifdef DEBUG_MUTEX
    reconnDebugPrint("%s: Calling pthread_mutex_unlock(&socketMutex) %d %d %d\n", __FUNCTION__, socketMutex.__data.__lock, socketMutex.__data.__count, socketMutex.__data.__owner);
#endif

    pthread_mutex_unlock(&socketMutex);
    mq_unlink(EQPT_MSG_Q_NAME);
    memset((char*) &eqptMsgQattr, 0, sizeof(eqptMsgQattr));
    eqptMsgQattr.mq_flags    = EQPT_MSG_Q_FLAGS;
    eqptMsgQattr.mq_maxmsg   = EQPT_MSG_Q_SIZE;
    eqptMsgQattr.mq_msgsize  = RECONN_RSP_PAYLOAD_SIZE + RECONN_RSPPACKET_HEADER_SIZE;

#ifndef __SIMULATION__
    EqptMsgQid = mq_open(EQPT_MSG_Q_NAME, O_RDWR | O_CREAT, NULL, &eqptMsgQattr);
#else
    EqptMsgQid = mq_open(EQPT_MSG_Q_NAME, O_RDWR | O_CREAT, NULL, NULL);
#endif
    if(EqptMsgQid == (mqd_t) -1)
    {
        reconnDebugPrint("%s: mq_open() failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
    }
    else
    {
        reconnDebugPrint("%s: ***** Started EqptMsgQid = %d\n", __FUNCTION__, EqptMsgQid);
        mq_getattr(EqptMsgQid, &eqptMsgQattr);
        while(gEqptTaskDone == FALSE)
        {
            memset(&theResponsePacket, 0, sizeof (ReconnResponsePacket));
            if((msgSize = mq_receive(EqptMsgQid, (char *)&theResponsePacket, eqptMsgQattr.mq_msgsize , NULL)) == -1)
            {
                reconnDebugPrint("%s: mq_receive failed %d (%s), EqptMsgQid %d\n", __FUNCTION__, errno, strerror(errno), EqptMsgQid);
                break;
            }
            else
            {
                if(RECONN_DEBUG_ON(EQPT_TASK))
                {
                    reconnDebugPrint("%s: received message of length %d \n", __FUNCTION__, msgSize);
                    if(msgSize > RECONN_RSP_PAYLOAD_SIZE + RECONN_RSPPACKET_HEADER_SIZE)
                    {
                        reconnDebugPrint("\n\n<<<<<<<<<<<<<<<<<<<< %s: received message of length %d \n", __FUNCTION__, msgSize);
                        continue;
                    }
                }
                if(gDebugTimingEnabled == YES)
                {
                    GET_MSGID_FROM_PACKET(msgId, theResponsePacket);
                    if(msgId == SPECANA_PKT_RCVD_NOTIF)
                    {
                        gettimeofday(&queueStopTime, NULL);
                        TIMEVALSUBTRACT(queueResult, queueStopTime, queueStartTime);
                        reconnDebugPrint("QUEUE: result.tv_sec = %lu result.tv_usec = %lu \n", 
                                queueResult.tv_sec, queueResult.tv_usec);

                        gettimeofday(&stopTime, NULL);
                        TIMEVALSUBTRACT(result, stopTime, startTime);
                        reconnDebugPrint("TOTAL: result.tv_sec = %lu result.tv_usec = %lu \n", 
                                result.tv_sec, result.tv_usec);
                        reconnDebugPrint("********************************\n");
                    }   
                }
                for(i = 0; i < RECONN_MAX_NUM_CLIENTS; i++)
                {
                    pthread_mutex_lock(&socketMutex);
                    if(socketIdList[i] > 0)
                    {
                        if(RECONN_DEBUG_ON(EQPT_TASK)) 
                        {
                            reconnDebugPrint("%s: Sending message to client socket %d\n", __FUNCTION__, socketIdList[i]);
                        }
                        sendSocket(socketIdList[i], (unsigned char *)&theResponsePacket, msgSize, 0);
                    } 
                    pthread_mutex_unlock(&socketMutex);
                }
#ifdef DEBUG_EQPT
                reconnDebugPrint("%s: Done sending message to clients\n", __FUNCTION__);
#endif
            }
        }
    }
    reconnDebugPrint("%s: ********** returning.\n", __FUNCTION__);
    reconnEqptCleanUp();
    return &state;
}

//*************************************************************************************
//*************************************************************************************
// FUNCTION:    disconnectAllClients
//
// DESCRIPTION: This interface is used to disconnect all attached clients. 
//
// Parameters: mode - the client mode to disconnect
//
//*************************************************************************************
void disconnectAllClients(ReconnMasterClientMode mode)
{
    int i;
    ReconnPacket thePacket;
    ReconnPacket *thePacketPtr = &thePacket;

    memset(thePacketPtr, 0, sizeof(ReconnPacket));
    //ADD_RSPID_TO_PACKET(GENERIC_RESPONSE, thePacketPtr);
    ADD_MSGID_TO_PACKET(RECONN_QUITING_NOTIF, thePacketPtr);
    ADD_DATA_LENGTH_TO_PACKET(0, thePacketPtr);

#ifdef DEBUG_MUTEX
    reconnDebugPrint("%s: Calling pthread_mutex_lock(&socketMutex) %d %d %d\n", __FUNCTION__, socketMutex.__data.__lock, socketMutex.__data.__count, socketMutex.__data.__owner);
#endif
    pthread_mutex_lock(&socketMutex);
#ifdef DEBUG_MUTEX
    reconnDebugPrint("%s: Calling pthread_mutex_lock(&clientListMutex) %d %d %d\n", __FUNCTION__, clientListMutex.__data.__lock, clientListMutex.__data.__count, clientListMutex.__data.__owner);
#endif

    pthread_mutex_lock(&clientListMutex);
    for(i = 0; i < RECONN_MAX_NUM_CLIENTS; i++)
    {
        if ((activeClientsList[i]) && (activeClientsList[i]->mode == mode))
        {
            reconnDebugPrint("%s: Sending disconnect command to socket %d\n", __FUNCTION__, activeClientsList[i]->socketFd);
            sendSocket(activeClientsList[i]->socketFd, (unsigned char *)thePacketPtr, RECONN_PACKET_HEADER_SIZE, 0);
            shutdown(activeClientsList[i]->socketFd, SHUT_WR);
            usleep(500000);
            shutdown(activeClientsList[i]->socketFd, SHUT_RD);
        }
    }

#ifdef DEBUG_MUTEX
    reconnDebugPrint("%s: Calling pthread_mutex_unlock(&socketMutex) %d %d %d\n", __FUNCTION__, socketMutex.__data.__lock, socketMutex.__data.__count, socketMutex.__data.__owner);
#endif
    pthread_mutex_unlock(&socketMutex);

#ifdef DEBUG_MUTEX
    reconnDebugPrint("%s: Calling pthread_mutex_unlock(&clientListMutex) %d %d %d\n", __FUNCTION__, clientListMutex.__data.__lock, clientListMutex.__data.__count, clientListMutex.__data.__owner);
#endif

    pthread_mutex_unlock(&clientListMutex);

}

//*************************************************************************************
//*************************************************************************************
// FUNCTION:    reconnGetEqptResponseTask
//
// DESCRIPTION: This is the task responsible for reading the test equipment. If there is
//              data from any of them, this task reads the data formats a response packet
//              and places the packet on the eqptMsgQid for the eqpt task to read and send
//              to all connected clients.
// Parameters:
//              eqptDescriptors - a pointer to the test equipment descriptors
//
//*************************************************************************************
void *reconnGetEqptResponseTask(void *args)
{
    fd_set theFileDescriptor;
    static int state = 0;
    int retCode, length = 0, payloadIndex; 
    int maxFd;
    int msgId;
    extern int insertedMasterSocketFd;
    ReconnEqptDescriptors *eqptDescriptors = (ReconnEqptDescriptors *) args;

    ReconnErrCodes readStatus = RECONN_SUCCESS;
    int debugIndex;
    char *debugPtr;
    struct timeval waitTime;
    ReconnResponsePacket thePacket;
    ReconnResponsePacket *thePacketPtr =&thePacket;


#ifdef DEBUG_EQPT
    reconnDebugPrint("%s: Function Entered \n",__FUNCTION__);
#endif

    reconnDebugPrint("%s: ***** Started\n", __FUNCTION__);
    while(gEqptGetTaskDone == FALSE)
    {
        waitTime.tv_sec = 0;
        waitTime.tv_usec = 200000;
        maxFd = 0;
        FD_ZERO(&theFileDescriptor);
        if(eqptDescriptors->analyzerFd != -1)
        {
            FD_SET(eqptDescriptors->analyzerFd, &theFileDescriptor);
            maxFd = (eqptDescriptors->analyzerFd > maxFd) ? eqptDescriptors->analyzerFd : maxFd;
        }
        if(eqptDescriptors->powerMeterFd != -1)
        {
            FD_SET(eqptDescriptors->powerMeterFd, &theFileDescriptor);
            maxFd = (eqptDescriptors->powerMeterFd > maxFd) ? eqptDescriptors->powerMeterFd : maxFd;
        }
        if(eqptDescriptors->dmmFd != -1)
        {
            FD_SET(eqptDescriptors->dmmFd, &theFileDescriptor);
            maxFd = (eqptDescriptors->dmmFd > maxFd) ? eqptDescriptors->dmmFd : maxFd;
        }

        //if((retCode = select(theEqptFd+1, &theFileDescriptor, NULL, NULL, &waitTime)) < 0)
        if((retCode = select(maxFd+1, &theFileDescriptor, NULL, NULL, &waitTime)) < 0)
        {
            reconnDebugPrint("%s: select failed %d(%s)\n",__FUNCTION__, errno, strerror(errno));
            /*
             * Give some time for other process to clean up the bad file descriptor before continuing.
             * Problems with file descriptors typically occurs during software upgrade or crashHandler
             * shutting things down.
             */
            usleep(500000);
            continue;
        }
        else if(retCode == 0)
        {
//#ifdef DEBUG_EQPT
//         reconnDebugPrint("%s: select timedout\n",__FUNCTION__);
//#endif
            continue;
        }
        else if (retCode != 0)
        {

#ifdef DEBUG_EQPT
            reconnDebugPrint("%s:\n\n********************* select returned\n",__FUNCTION__);
#endif
            payloadIndex = 0;
            memset((char *)&(thePacket.dataPayload[0]), 0 , RECONN_RSP_PAYLOAD_SIZE);

            if(FD_ISSET(eqptDescriptors->dmmFd,  &theFileDescriptor))
            {
                ADD_RSPID_TO_PACKET(GENERIC_RESPONSE, thePacketPtr);
                ADD_MSGID_TO_PACKET(DMM_PKT_RCVD_NOTIF, thePacketPtr);
                pthread_mutex_lock(&dmmMutex);
                
                if(RECONN_DEBUG_ON(DMM_DEBUG_EQPT))
                {
                    reconnDebugPrint("%s: DMM has responded \n", __FUNCTION__);
                }
                while((readStatus = dmmRead((unsigned char *)&(thePacket.dataPayload[payloadIndex]), &length)) == RECONN_SUCCESS)
                {
                    gDmmResponses++;
                    if(length > gDmmHigh)
                        gDmmHigh = length;

                    payloadIndex += length;
                    if(payloadIndex >= RECONN_RSP_PAYLOAD_SIZE)
                    {
                        if(payloadIndex == RECONN_RSP_PAYLOAD_SIZE)
                        {
                            break;
                        }
                        else
                        {
                            reconnDebugPrint("%s:>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> payloadIndex %d\n", __FUNCTION__, payloadIndex);
                            payloadIndex = RECONN_RSP_PAYLOAD_SIZE;
                        }
                    }
                } 
                pthread_mutex_unlock(&dmmMutex);
            }
            else if(FD_ISSET(eqptDescriptors->analyzerFd, &theFileDescriptor))
            {
                ADD_RSPID_TO_PACKET(GENERIC_RESPONSE, thePacketPtr);
                ADD_MSGID_TO_PACKET(SPECANA_PKT_RCVD_NOTIF, thePacketPtr);
                if(RECONN_DEBUG_ON(SPECANA_DEBUG_EQPT))
                {
                    reconnDebugPrint("%s: Analyzer has responded \n", __FUNCTION__);
                }
                length = 0;
                while((readStatus = SpectrumAnalyzerRead((unsigned char *)&(thePacket.dataPayload[payloadIndex]), &length)) == RECONN_SUCCESS)
                {
                    gAnalyzerResponses++;
                    if(length < 0)
                    {
                        reconnDebugPrint("%s: Read Failed %d (%s)\n", __FUNCTION__, errno, strerror(errno));
                        continue;
                    }
                    if(length > gAnalyzerHigh)
                        gAnalyzerHigh = length;

                    payloadIndex += length;
                    if(payloadIndex >= RECONN_RSP_PAYLOAD_SIZE)
                    {
                        if(payloadIndex == RECONN_RSP_PAYLOAD_SIZE)
                        {
                            break;
                        }
                        else
                        {
                            reconnDebugPrint("%s:>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> payloadIndex %d\n", __FUNCTION__, payloadIndex);
                            payloadIndex = RECONN_RSP_PAYLOAD_SIZE;
                        }
                        break;
                    }
                } 
                if(gDebugTimingEnabled == YES)
                {
                    deviceStopTime.tv_sec = deviceStopTime.tv_usec = 0;
                    gettimeofday(&deviceStopTime, NULL);
                    TIMEVALSUBTRACT(deviceResult, deviceStopTime, deviceStartTime);
                    reconnDebugPrint("********************************\nDEVICE: result.tv_sec = %lu result.tv_usec = %lu  PAYLOAD LENGTH %lu\n", deviceResult.tv_sec, deviceResult.tv_usec, payloadIndex);
                    gettimeofday(&queueStartTime, NULL);
                }
            }
            else if(FD_ISSET(eqptDescriptors->powerMeterFd, &theFileDescriptor))
            {
                ADD_RSPID_TO_PACKET(GENERIC_RESPONSE, thePacketPtr);
                ADD_MSGID_TO_PACKET(PMETER_PKT_RCVD_NOTIF, thePacketPtr);

                if(RECONN_DEBUG_ON(DMM_DEBUG_EQPT))
                {
                    reconnDebugPrint("%s: Power Meter has responded \n", __FUNCTION__);
                }
                while((readStatus = powerMeterRead((unsigned char *)&(thePacket.dataPayload[payloadIndex]), &length)) == RECONN_SUCCESS)
                {
                    gMeterResponses++;
                    //reconnDebugPrint("%s: Power Meter read %d bytes\n", __FUNCTION__, length);
                    if(length < 0)
                    {
                        reconnDebugPrint("%s: Read Failed %d (%s)\n", __FUNCTION__, errno, strerror(errno));
                        continue;
                    }

                    if(length > gPowerMeterHigh)
                        gPowerMeterHigh = length;

                    payloadIndex += length;
                    //reconnDebugPrint("%s: Power Meter payloadIndex = %d\n", __FUNCTION__, payloadIndex);
                    if(payloadIndex >= RECONN_RSP_PAYLOAD_SIZE)
                    {
                        if(payloadIndex == RECONN_RSP_PAYLOAD_SIZE)
                        {
                            break;
                        }
                        else
                        {
                            reconnDebugPrint("%s:>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> payloadIndex %d\n", __FUNCTION__, payloadIndex);
                            payloadIndex = RECONN_RSP_PAYLOAD_SIZE;
                        }
                        break;
                    }
                    if(RECONN_DEBUG_ON(DMM_DEBUG_EQPT))
                    {
                        reconnDebugPrint("%s: Power Meter sending %d bytes\n", __FUNCTION__, payloadIndex);
                    }
                }
                if(readStatus == RECONN_PM_PORT_NOT_INITIALIZED)
                {
                    continue;
                }
            }
            else
            {
#ifdef DEBUG_EQPT
                reconnDebugPrint("%s: >>>>>>>>   unknown FD is set <<<<<<<< \n", __FUNCTION__);
#endif
                continue;
            }
            if(payloadIndex)
            {
                GET_MSGID_FROM_PACKET(msgId, thePacket);
                /*
                 * Debug messaging on?
                 */
                if(((RECONN_DEBUG_ON(DMM_DEBUG_SND)) && (msgId == DMM_PKT_RCVD_NOTIF)) ||
                        ((RECONN_DEBUG_ON(SPECANA_DEBUG_SND)) && (msgId == SPECANA_PKT_RCVD_NOTIF)) || 
                        ((RECONN_DEBUG_ON(PMETER_DEBUG_SND)) && (msgId == PMETER_PKT_RCVD_NOTIF)))
                {
                    debugPtr = (char *)&(thePacket.dataPayload[0]);
                    for(debugIndex = 0; debugIndex < payloadIndex; debugIndex++, debugPtr++)
                    {
                        reconnDebugPrint("0x%x ", *debugPtr);
                    }
                    reconnDebugPrint("\n");
                }

                if(payloadIndex > gPayloadHigh)
                    gPayloadHigh = payloadIndex;

                ADD_DATA_LENGTH_TO_PACKET(payloadIndex, thePacketPtr);
                if(insertedMasterSocketFd != -1) 
                {
#ifdef DEBUG_EQPT
                    reconnDebugPrint("%s: Calling libiphoned_tx %d \n", __FUNCTION__,  payloadIndex + RECONN_RSPPACKET_HEADER_SIZE);
#endif
                    libiphoned_tx((unsigned char *)thePacketPtr, payloadIndex + RECONN_RSPPACKET_HEADER_SIZE);
                }
#ifdef DEBUG_EQPT
                reconnDebugPrint("%s: Calling reconnEqptAddMsgToQ %d \n", __FUNCTION__,  payloadIndex + RECONN_RSPPACKET_HEADER_SIZE);
#endif
                reconnEqptAddMsgToQ((char *)thePacketPtr, payloadIndex + RECONN_RSPPACKET_HEADER_SIZE);
            }
            else
            {
#ifdef DEBUG_EQPT
                reconnDebugPrint("%s: no bytes to be read\n", __FUNCTION__);
#endif
                continue;
            }
        }
    }
    reconnDebugPrint("%s: returning\n",__FUNCTION__);
    return (&state);
}
