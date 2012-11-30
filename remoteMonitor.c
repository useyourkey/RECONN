//******************************************************************************
//******************************************************************************
//
// FILE:        remoteMonitor.c
//
// CLASSES:     
//
// DESCRIPTION: This is the file which contains the remote monitoring functionality.
//              Remote Monitor is responsible for opening a TCP/IP socket connection
//              to a remove server. Once connected, remote monitor will simply send
//              the equipment data to the remote sight. The data sent to the remote 
//              site is the same data that is sent to the iPhone application.
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
// HISTORY: Created 11/08/2012 by Michael A. Carrier
// $Header:$
// $Revision: 1.3 $
// $Log:$
// 
//******************************************************************************
//******************************************************************************
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <linux/if.h>
#include <errno.h>
#include <netdb.h>

#include "reconn.h"
#include "debugMenu.h"
#include "clientApp.h"
#include "remoteMonitor.h"
#include "eqptResponse.h"
#include "socket.h"

static int remoteMonitorDone = FALSE;
int stopRemoteMonitorHb = FALSE;
static YESNO remoteMonitorActive = NO;
static int webServerSocket = -1;
static struct sockaddr_in serverAddress;
static void *remoteMonitorKeepAliveTask(void *);
static CLIENTCONTEXT *aContextPtr = NULL;
static REMOTE_MONITOR_STATES theState = RM_INIT;

//*********************************************************************************
//*********************************************************************************
//  FUNCTION:       sendRemoteMonitorConnectionStatus
//
//  DESCRIPTION:    This interface sends a message to the master client process 
//                  informing it that the remote monitoring connection status has 
//                  changed. The master client process will then format and message 
//                  and send that message to its iPhone application all connected 
//                  clients.
//  Parameters: 
//                  theState - the connection state
//
//*********************************************************************************
static void sendRemoteMonitorConnectionStatus(REMOTE_MONITOR_CONNECTION_STATES theState)
{
    unsigned char theMessage[MASTER_MSG_SIZE];

    memset(&theMessage, 0, MASTER_MSG_SIZE);
    theMessage[0] = RM_STATE_CHANGE;
    theMessage[1] = theState;
    if(masterClientMsgQid != -1)
    {
        if(mq_send(masterClientMsgQid, (const char *)&theMessage, MASTER_MSG_SIZE, 0) != 0)
        {
            reconnDebugPrint("%s: mq_send(masterClientMsgQid) failed. %d(%s)\n", __FUNCTION__, errno, strerror(errno));
        }
        else
        {
            reconnDebugPrint("%s: mq_send(masterClientMsgQid) success.\n", __FUNCTION__);
        }
    }
    else
    {
        reconnDebugPrint("%s: masterClientMsgQid == -1\n", __FUNCTION__);
    }
}

//******************************************************************************
//******************************************************************************
// FUNCTION:    remoteMonitorActivate
//
// DESCRIPTION: Interface used to activate/deactivate the remote monitor.
//
//******************************************************************************

ReconnErrCodes remoteMonitorActivate(YESNO activate, CLIENTCONTEXT *theContextPtr)
{
    ReconnErrCodes retCode = RECONN_SUCCESS;
    struct addrinfo hints, *webServiceInfo;
    int errCode, i, length;
    char *ptr;
    YESNO IPaddressIsName = NO;

    reconnDebugPrint("%s: Function Entered with activate == %s remoteMonitorActive == %s\n",
            __FUNCTION__, (activate == YES) ? "YES" : "NO", (remoteMonitorActive == YES) ? "YES" : "NO");

    if(activate == YES)
    {
        if(remoteMonitorActive == NO)
        {
            if((webServerSocket = socket(PF_INET, SOCK_STREAM, 0)) < 0)
            {
                retCode = RECONN_FAILURE;
                reconnDebugPrint("%s: socket() failed %d (%s)\n", __FUNCTION__, errno, strerror(errno));
            }
            else
            {
                /*
                 * Check to see if the IP address is a hostname or IP address
                 */
                ptr = &(theContextPtr->theWebServerHostName[0]);
                length = strlen(theContextPtr->theWebServerHostName);
                for(i = 0; i < length; i++, ptr++)
                {
                    /*
                     * If we find a letter then the IP is a hostname not an IP address
                     */
                    if(isalpha(*ptr))
                    {
                        IPaddressIsName = YES;
                        break;
                    }
                }
                if(IPaddressIsName == YES)
                {
                    memset(&hints, 0, sizeof(struct addrinfo));
                    hints.ai_family = AF_UNSPEC;
                    hints.ai_socktype = SOCK_STREAM; /* Datagram socket */
                    hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
                    hints.ai_protocol = 0;          /* Any protocol */
                    hints.ai_canonname = NULL;
                    hints.ai_addr = NULL;
                    hints.ai_next = NULL;
                    if((errCode = getaddrinfo(&(theContextPtr->theWebServerHostName[0]), NULL, &hints, &webServiceInfo)) != 0)
                    {
                        reconnDebugPrint("%s: gethostbyname() failed %d\n", __FUNCTION__, errCode);
                        retCode = RECONN_HOSTNAME_UNRESOLVED; 
                    }
                }
                if(retCode == RECONN_SUCCESS)
                {
                    if(IPaddressIsName == YES)
                    {
                        serverAddress.sin_addr.s_addr = ((struct sockaddr_in *) (webServiceInfo->ai_addr))->sin_addr.s_addr;
                    }
                    else
                    {
                        serverAddress.sin_addr.s_addr =  inet_addr(theContextPtr->theWebServerHostName);
                    }

                    serverAddress.sin_family = AF_INET;
                    serverAddress.sin_port = htons(theContextPtr->portNum);

                    if(connect(webServerSocket, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0)
                    {
                        if(errno == ECONNREFUSED)
                        {
                            retCode = RECONN_CONNECTION_REFUSED;
                        }
                        else if(errno == ENETUNREACH)
                        {
                            retCode = RECONN_NETWORK_UNREACHABLE;
                        }
                        else if(errno == ETIMEDOUT)
                        {
                            retCode = RECONN_CONNECTION_TIMEDOUT;
                        }
                        else 
                        {
                            retCode = RECONN_FAILURE;
                            reconnDebugPrint("%s: connect() failed %d (%s)\n", __FUNCTION__, errno, strerror(errno));
                        }
                    }
                    else
                    {
                        /*
                         * Now create the thread which will watch for connections on the RJ-45.
                         */
                        remoteMonitorDone = FALSE;
                        reconnDebugPrint("%s: creating remoteMonitorTask\n", __FUNCTION__);
                        if(pthread_create(&reconnThreadIds[RECONN_REMOTE_MONITOR_TASK], NULL, remoteMonitorTask, theContextPtr) < 0)
                        {
                            remoteMonitorActive = NO;
                            retCode = RECONN_FAILURE;
                            reconnDebugPrint("%s: Could not start remoteMonitorTask %d %s\n", __FUNCTION__, errno, strerror(errno));
                        }
                        else
                        {
                            remoteMonitorActive = YES;
                        }
                    }
                }
            }
        }
    }
    else if(activate == NO)
    {
        if(remoteMonitorActive == YES)
        {
            sendRemoteMonitorConnectionStatus(DISCONNECTED);
            remoteMonitorCleanup();
        }
    }
    else
    {
        reconnDebugPrint("%s: Invalid value for activate %d\n", __FUNCTION__, activate);
        retCode = RECONN_INVALID_PARAMETER;
    }
    return (retCode);
}
static YESNO remoteMonitorCleanupRunning = NO;
//******************************************************************************
//******************************************************************************
// FUNCTION:    remoteMonitorCleanup
//
// DESCRIPTION:  Function used to clean up the Remote Monitor resources.
//
//******************************************************************************
void remoteMonitorCleanup()
{
    int i;

    reconnDebugPrint("%s: Function entered, remoteMonitorCleanupRunning = %s\n", __FUNCTION__, (remoteMonitorCleanupRunning == YES) ? "YES": "NO");
    if(remoteMonitorCleanupRunning == NO)
    {
        remoteMonitorCleanupRunning = YES;
        pthread_mutex_lock(&clientListMutex);
        for(i = 0; i < (RECONN_MAX_NUM_CLIENTS); i++)
        {
            if (activeClientsList[i])
            {
                if(activeClientsList[i]->mode == REMOTEMODE)
                {
                    reconnDebugPrint("%s: DeRegistering client %u\n", __FUNCTION__, activeClientsList[i]->index);
                    reconnReturnClientIndex(activeClientsList[i]->index);

                    pthread_mutex_unlock(&clientListMutex);
                    reconnDeRegisterClientApp(activeClientsList[i]);
                    pthread_mutex_lock(&clientListMutex);

                }
            }
        }
        pthread_mutex_unlock(&clientListMutex);
        if(webServerSocket != -1)
        {
            reconnDebugPrint("%s: Calling shutdown(%u, SHUT_RDWR)\n", __FUNCTION__, webServerSocket);
            if(shutdown(webServerSocket, SHUT_RDWR))
            {
                reconnDebugPrint("%s: shutdown(%u, SHUT_RDWR) failed %d (%s)\n", __FUNCTION__, webServerSocket,
                        errno, strerror(errno));
            }
            reconnDebugPrint("%s: Calling close(%u)\n", __FUNCTION__, webServerSocket);
            if(close(webServerSocket))
            {
                reconnDebugPrint("%s: close(%u) failed %d (%s)\n", __FUNCTION__, webServerSocket, errno, 
                        strerror(errno));
            }
            webServerSocket = -1;
        }
        remoteMonitorDone = TRUE;
        remoteMonitorActive = NO;
        remoteMonitorCleanupRunning = NO;
    }
    reconnDebugPrint("%s: Function exiting\n", __FUNCTION__);
}
//******************************************************************************
//******************************************************************************
// FUNCTION:    remoteMonitorTask
//
// DESCRIPTION: This is the task responsible for processing all data coming from 
//              the web service.
//              Upon rececption of the data begin opcode this task will register 
//              itself with the reconn embedded software so that all equipment
//              data responses will be sent to the remote web service. 
//******************************************************************************
void *remoteMonitorTask(void *args)
{
    static int processState = 0;
    int bytesRead;
    //int dataLength;
    fd_set theFileDescriptor;
    struct timeval waitTime;
    short theMsgId;
    CLIENTCONTEXT *theMasterContextPtr = (CLIENTCONTEXT *)args; //pointer to the master client context
    union
    {
        char data[strlen(RECONNBEGIN)+1];
        ReconnPacket thePacket;
        ReconnResponsePacket theResponsePacket;
    }theRecvBuf;

#if 0
    ReconnPacket *thePacketPtr = &theRecvBuf.thePacket;
#endif

    if(RECONN_DEBUG_ON(RM_DEBUG))
    {
        reconnDebugPrint("%s: **** Task started\n", __FUNCTION__);
    }
    if((aContextPtr = (CLIENTCONTEXT *)malloc(sizeof(CLIENTCONTEXT))) != 0)
    {
        memset(aContextPtr, 0, sizeof(CLIENTCONTEXT));
        aContextPtr->thisContext = (int *)aContextPtr;
        aContextPtr->socketFd = webServerSocket;
        aContextPtr->eqptDescriptors = &gEqptDescriptors;
        aContextPtr->mode = REMOTEMODE;
        aContextPtr->index = -1;
        aContextPtr->tmpFd = (FILE *)-1;
        aContextPtr->sourceIp = serverAddress;
        aContextPtr->theResponsePktPtr = &aContextPtr->theResponsePkt;

        /*
         * Create the task responsible for sending keep alive messages to the remote web service.
         */
        if(pthread_create(&reconnThreadIds[RECONN_KEEPALIVE_TASK], NULL, remoteMonitorKeepAliveTask, NULL) < 0)
        {
            reconnDebugPrint("%: Could not start remoteMonitorKeepAliveTask(), %d (%s)\n", 
                    __FUNCTION__, errno , strerror(errno));
            remoteMonitorActivate(NO, theMasterContextPtr);
        }
        else
        {
            waitTime.tv_sec = 30;
            waitTime.tv_usec = 0;
            while (remoteMonitorDone == FALSE)
            {
                FD_ZERO(&theFileDescriptor);
                FD_SET(webServerSocket, &theFileDescriptor);
                if((aContextPtr->retStatus = select((webServerSocket + 1),
                                &theFileDescriptor, NULL, NULL, &waitTime)) < 0)
                {
                    reconnDebugPrint("%s: select() failed %d (%s)\n", __FUNCTION__,
                            errno, strerror(errno));
                    remoteMonitorActivate(NO, theMasterContextPtr);
                    break;
                }
                switch(theState)
                {
                    case RM_INIT:
                    {
                        if(RECONN_DEBUG_ON(RM_DEBUG))
                        {
                            reconnDebugPrint("%s: RM_INIT\n", __FUNCTION__);
                        }
                        if(aContextPtr->retStatus == 0)
                        {
                            /*
                             * The handshake reception timed out. Close the socket and terminate
                             * Remote Monitor.
                             */
                            reconnDebugPrint("%s: select() timed out\n", __FUNCTION__);
                            remoteMonitorActivate(NO, theMasterContextPtr);
                            break;
                        }
                        /*
                         * Get the data and fall through to the next state.
                         */
                        memset(&theRecvBuf, 0, sizeof(theRecvBuf));
                        if((bytesRead = recv(webServerSocket, &theRecvBuf, sizeof(theRecvBuf), 0)) <= 0)
                        {
                            reconnDebugPrint("%s: bytesRead = %d recvfrom(%d) returned %d (%s)\n", __FUNCTION__, bytesRead, webServerSocket+1, 
                                    errno, strerror(errno));
                            remoteMonitorActivate(NO, theMasterContextPtr);
                            break;
                        } 
                        theState = RM_GET_HANDSHAKE;
                    }
                    case  RM_GET_HANDSHAKE:
                    {
                        if(RECONN_DEBUG_ON(RM_DEBUG))
                        {
                            reconnDebugPrint("%s: RM_GET_HANDSHAKE\n", __FUNCTION__);
                        }
                        if(strcmp(&(theRecvBuf.data[0]), RECONNBEGIN) == 0)
                        {
                            //theState = RM_SEND_CLIENT_ACCESS;
                            theState = RM_DEVICE_ID;
                            /*
                             * Fall through to the next state
                             */
                        }
                        else
                        {
                            sendReconnResponse(webServerSocket,
                                    theRecvBuf.thePacket.messageId.Byte[0],
                                    theRecvBuf.thePacket.messageId.Byte[1],
                                    RECONN_FAILURE, aContextPtr->mode);
                            reconnDebugPrint("%s: Invalid Handshake, killing this task. \n", __FUNCTION__);
                            remoteMonitorActivate(NO, aContextPtr);
                            break;
                        }
                    }
#if 0 // Currently not used. Original design was to have the reconn send out client access request and expect a response
                    case RM_SEND_CLIENT_ACCESS:
                    {
                        if(RECONN_DEBUG_ON(RM_DEBUG))
                        {
                            reconnDebugPrint("%s: RM_SEND_CLIENT_ACCESS\n", __FUNCTION__);
                        }
                        memset(thePacketPtr, 0, sizeof(ReconnPacket));
                        ADD_MSGID_TO_PACKET(CLIENT_ACCESS_REQ, thePacketPtr);
                        dataLength = 0;
                        ADD_DATA_LENGTH_TO_PACKET(dataLength, thePacketPtr);

                        /*
                         * Send CLIENT ACCESS REQuest,  break out of this state and wait for the
                         * response.
                         */
                        if(sendSocket(webServerSocket, thePacketPtr, (RECONN_PACKET_HEADER_SIZE + dataLength), 0) < 0)
                        {
                            reconnDebugPrint("%s: sendSocket(webServerSocket) failed %d (%s)\n", __FUNCTION__, errno,
                                    strerror(errno));
                            remoteMonitorActivate(NO, theMasterContextPtr);
                        }
                        else
                        {
                            FD_ZERO(&theFileDescriptor);
                            FD_SET(webServerSocket, &theFileDescriptor);
#ifndef __SIMULATION__
                            waitTime.tv_sec = 30;
#else
                            waitTime.tv_sec = 5;
#endif
                            waitTime.tv_usec = 0;
                            theState = RM_WAIT_RESP;
                        }
                        break;
                    }
                    case RM_WAIT_RESP:
                    { 
                        if(RECONN_DEBUG_ON(RM_DEBUG)) 
                        {
                            reconnDebugPrint("%s: RM_WAIT_RESP\n", __FUNCTION__);
                        }
                        if(aContextPtr->retStatus == 0)
                        {
                            /*
                             * The CLIENT ACCESS REQuest response timed out. Close the socket and terminate
                             * Remote Monitor.
                             */
                            reconnDebugPrint("%s: select() timed out\n", __FUNCTION__);
                            remoteMonitorActivate(NO, theMasterContextPtr);
                            break;
                        }
                        /*
                         * Get the response
                         */
                        memset(&theRecvBuf, 0, sizeof(theRecvBuf));
                        if(recv(webServerSocket, &theRecvBuf, sizeof(theRecvBuf), 0) <= 0)
                        {
                            reconnDebugPrint("%s: recv(%d) failed %d (%s)\n", __FUNCTION__, webServerSocket, 
                                    errno, strerror(errno));
                            remoteMonitorActivate(NO, theMasterContextPtr);
                            break;
                        }

                        GET_RSPID_FROM_PACKET(theMsgId, theRecvBuf.theResponsePacket);
                        if((theMsgId == CLIENT_ACCESS_REQ) &&
                                (theRecvBuf.theResponsePacket.dataPayload[0] == RECONN_SUCCESS))
                        {
                            /*
                             * Set the state, break out of this case and wait for web service 
                             * to send the next request.
                             */
                            FD_ZERO(&theFileDescriptor);
                            FD_SET(webServerSocket, &theFileDescriptor);
#ifndef __SIMULATION__
                            waitTime.tv_sec = 30;
#else
                            waitTime.tv_sec = 5;
#endif
                            waitTime.tv_usec = 0;
                            theState = RM_DEVICE_ID;
                        }
                        else
                        {
                            reconnDebugPrint("%s: msgId (%u) or payload (%u) invalid, killing task\n",
                                    __FUNCTION__, theMsgId, theRecvBuf.theResponsePacket.dataPayload[0]);
                            remoteMonitorActivate(NO, theMasterContextPtr);
                        }
                        break;
                    }
#endif
                    case RM_DEVICE_ID:
                    {
                        if(RECONN_DEBUG_ON(RM_DEBUG)) 
                        {
                            reconnDebugPrint("%s: RM_DEVICE_ID\n", __FUNCTION__);
                        }
#if 0
                        if(aContextPtr->retStatus == 0)
                        {
                            /*
                             * The CLIENT ACCESS REQuest response timed out. Close the socket and terminate
                             * Remote Monitor.
                             */
                            reconnDebugPrint("%s: select() timed out\n", __FUNCTION__);
                            remoteMonitorActivate(NO, theMasterContextPtr);
                            break;
                        }
                        /*
                         * Get the response
                         */
                        memset(&theRecvBuf, 0, sizeof(theRecvBuf));
                        if(recv(webServerSocket, &theRecvBuf, sizeof(theRecvBuf), 0) <= 0)
                        {
                            reconnDebugPrint("%s: recv(%d) failed %d (%s)\n", __FUNCTION__, webServerSocket, 
                                    errno, strerror(errno));
                            remoteMonitorActivate(NO, theMasterContextPtr);
                            break;
                        } 
                        GET_MSGID_FROM_PACKET(theMsgId, theRecvBuf.thePacket);
                        if(theMsgId == RECONN_ID_REQ)
                        {
#endif
                            /*
                             * Get the Serial Number
                             */
                            if(RECONN_DEBUG_ON(RM_DEBUG))
                            {
                                reconnDebugPrint("%s: calling getDeviceId()\n", __FUNCTION__);
                            }
                            /*
                             * Get the serial and format the return packet
                             */
                            if(getDeviceId(aContextPtr) == RECONN_SUCCESS)
                            {
                                sendSocket(webServerSocket, (unsigned char *)aContextPtr->theResponsePktPtr, 
                                            (RECONN_RSPPACKET_HEADER_SIZE + aContextPtr->serialBytesRead), 0);
                            }
                            else
                            {
                                sendReconnResponse(webServerSocket,
                                        theRecvBuf.thePacket.messageId.Byte[0],
                                        theRecvBuf.thePacket.messageId.Byte[1],
                                        RECONN_FAILURE, aContextPtr->mode);

                                remoteMonitorActivate(NO, aContextPtr);
                                break;
                            }
                            /*
                             * Tell all clients we are connected.
                             */
                            sendRemoteMonitorConnectionStatus(CONNECTED);
                            theState = RM_RCV_DATA;
#if 0
                        }
                        else
                        {
                            sendReconnResponse(webServerSocket,
                                    theRecvBuf.thePacket.messageId.Byte[0],
                                    theRecvBuf.thePacket.messageId.Byte[1],
                                    RECONN_INVALID_PARAMETER, aContextPtr->mode);
                        }
#endif
                        break;
                    }
                    case RM_RCV_DATA:
                    {
                        if((bytesRead = recv(webServerSocket, &theRecvBuf, sizeof(theRecvBuf), 0)) <= 0)
                        {
                            if(bytesRead == -1)
                            {
                                reconnDebugPrint("%s: recv(%d) failed %d (%s) bytesRead = %d \n",
                                        __FUNCTION__, webServerSocket, errno, strerror(errno), bytesRead);
                            }
                            remoteMonitorActivate(NO, theMasterContextPtr);
                            break;
                        } 
                        GET_MSGID_FROM_PACKET(theMsgId, theRecvBuf.thePacket);
                        if(theMsgId == WEB_SERVICE_DATA_FLOW)
                        {
                            if(RECONN_DEBUG_ON(RM_DEBUG))
                            {
                                reconnDebugPrint("%s: Received WEB_SERVICE_DATA_FLOW -> ", __FUNCTION__);
                            }
                            if(theRecvBuf.thePacket.dataPayload[0] == RM_SEND_DATA)
                            {
                                if(RECONN_DEBUG_ON(RM_DEBUG))
                                {
                                    reconnDebugPrint(" RM_SEND_DATA\n");
                                }
                                /*
                                 * Register the remote client. This will start data 
                                 * transmission to the remote web service.
                                 */
                                if(reconnClientsRegistered(REMOTEMODE) == 0)
                                {
                                    if(RECONN_DEBUG_ON(RM_DEBUG))
                                    {
                                        reconnDebugPrint(" Calling reconnGetFreeClientIndex()\n");
                                    }
                                    if(reconnGetFreeClientIndex(&(aContextPtr->index)) == RECONN_SUCCESS)
                                    {
                                        if(RECONN_DEBUG_ON(RM_DEBUG))
                                        {
                                            reconnDebugPrint(" Calling reconnRegisterClientApp()\n");
                                        }
                                        if(reconnRegisterClientApp(aContextPtr) != RECONN_SUCCESS)
                                        {
                                            if(RECONN_DEBUG_ON(RM_DEBUG))
                                            {
                                                reconnDebugPrint("%s: reconnRegisterClientApp() index %d failed\n",
                                                        __FUNCTION__, aContextPtr->index);
                                            }
                                            remoteMonitorActivate(NO, theMasterContextPtr);
                                        }
                                    }
                                    else
                                    {
                                        reconnDebugPrint("%s: reconnGetFreeClientIndex(%u) failed\n",
                                                __FUNCTION__, aContextPtr->index);
                                        remoteMonitorActivate(NO, theMasterContextPtr);
                                    }
                                }
                                else
                                {
                                    if(RECONN_DEBUG_ON(RM_DEBUG))
                                    {
                                        reconnDebugPrint("%s: received WEB_SERVICE_DATA_FLOW when WEB_SERVICE_DATA_FLOW is already active.\n", __FUNCTION__);
                                    }
                                }
                            }
                            else if(theRecvBuf.thePacket.dataPayload[0] == RM_STOP_DATA)
                            {
                                reconnDebugPrint(" RM_STOP_DATA\n");
                                /*
                                 * De-register the remote client. This will stop data 
                                 * transmission to the remote web service.
                                 */
                                reconnDeRegisterClientApp(aContextPtr);
                            }
                            else
                            {
                                if(RECONN_DEBUG_ON(RM_DEBUG))
                                {
                                    reconnDebugPrint("%s: Invalid payload request %d\n", __FUNCTION__, 
                                            theRecvBuf.thePacket.dataPayload[0]);
                                }

                                sendReconnResponse(theMasterContextPtr->socketFd,
                                        theRecvBuf.thePacket.messageId.Byte[0],
                                        theRecvBuf.thePacket.messageId.Byte[1],
                                        RECONN_INVALID_PARAMETER, theMasterContextPtr->mode);
                            }
                            waitTime.tv_sec = 0;
                            waitTime.tv_usec = 0;
                        }
                        else if(theMsgId == CLIENT_RESIGN_REQ)
                        {
                            remoteMonitorActivate(NO, theMasterContextPtr);
                        }
                        else 
                        {
                            reconnDebugPrint("%s: Invalid msgId %d\n", __FUNCTION__, 
                                    theRecvBuf.thePacket.dataPayload[0]);
                            sendReconnResponse(theMasterContextPtr->socketFd,
                                    theRecvBuf.thePacket.messageId.Byte[0],
                                    theRecvBuf.thePacket.messageId.Byte[1],
                                    RECONN_INVALID_PARAMETER, theMasterContextPtr->mode);
                        }
                        break;
                    }
                    default:
                    {
                        reconnDebugPrint("%s: Invalid state %d\n", __FUNCTION__, theState);
                        break;
                    }
                } // switch(theState)
            } // while (remoteMonitorDone == FALSE)
        } // if(pthread_create 
    }
    else
    {
        reconnDebugPrint("%s: malloc(%u) failed %d (%s)\n", __FUNCTION__, sizeof(CLIENTCONTEXT));
        remoteMonitorActivate(NO, theMasterContextPtr);
    }
    if(aContextPtr)
    {
        free(aContextPtr);
    }
    if(RECONN_DEBUG_ON(RM_DEBUG))
    {
        reconnDebugPrint("%s: **** Task exiting\n", __FUNCTION__);
    }
    return(&processState);
}
//******************************************************************************
//******************************************************************************
// FUNCTION:    getRemoteMonitorState
//
// DESCRIPTION: Interface used to return Remote Monitor's state
//
//******************************************************************************
YESNO getRemoteMonitorState()
{
    return remoteMonitorActive;
}

//******************************************************************************
//******************************************************************************
// FUNCTION:    remoteMonitorKeepAliveTask
//
// DESCRIPTION: This task is started by remoteMonitorTask and will simply send
//              a keepalive message, every 15 seconds,  to the remote web service. 
//              If it can't send due to a socket error it will call 
//              remoteMonitorActivate(no,) to disconnect Remote Monitor.
//              
//******************************************************************************
static void *remoteMonitorKeepAliveTask(void *args)
{
    ReconnPacket theKeepAlivePacket;
    ReconnPacket *thePacketPtr = &theKeepAlivePacket;
    static int theStatus = 1;

    UNUSED_PARAM(args);
    memset(&theKeepAlivePacket, 0, sizeof(ReconnPacket));
    //ADD_MSGID_TO_PACKET(KEEPALIVE_MESSAGE, thePacketPtr);

    /*
     *  TODO this is temporary, use the above call for the final product
     */
    ADD_MSGID_TO_PACKET(0xFEFF, thePacketPtr);
    ADD_DATA_LENGTH_TO_PACKET(0, thePacketPtr);

    while((remoteMonitorDone == FALSE) && (stopRemoteMonitorHb == FALSE))
    {
        sleep(REMOTE_MONITOR_KEEPALIVE_TIME);
        if(RECONN_DEBUG_ON(RM_DEBUG))
        {
            reconnDebugPrint("%s: Sending keepalive to web service\n", __FUNCTION__);
        }
        errno = 0;
        //sendSocket(webServerSocket, (unsigned char *)&theKeepAlivePacket, RECONN_PACKET_HEADER_SIZE, 0);

        /*
         *  TODO this is temporary, use the above call for the final product
         */
        sendSocket(webServerSocket, (unsigned char *)&theKeepAlivePacket, 2, 0);

        if(errno)
        {
            reconnDebugPrint("%s:sendSocket failed %d (%s)\n", __FUNCTION__, errno, strerror(errno));
            remoteMonitorActivate(NO, NULL);
        }
    }
    reconnDebugPrint("%s: **** Task ending\n", __FUNCTION__);
    return(&theStatus);
}

//******************************************************************************
//******************************************************************************
// FUNCTION:    remoteMonitorGetDebugValues
//
// DESCRIPTION: This interface is used by debug menus to return variables that
//              are static to this file.               
//              
// Parameters:
//              theDebugPtr -   a pointer to a data structure into which this 
//                              interface will place that data.
//
//******************************************************************************
void remoteMonitorGetDebugValues(REMOTE_MONITOR_DEBUG_DATA *theDebugPtr)
{
    theDebugPtr->webServerSocket = webServerSocket;
    //memset(&(theDebugPtr->IPAddress[0]), 0, IPADDR_LEN);
    //strncpy(&(theDebugPtr->IPAddress[0]), &webServerIpAddress, IPADDR_LEN);
    //theDebugPtr->port = webServerSocket;
    theDebugPtr->remoteMonitorDone = remoteMonitorDone;
    theDebugPtr->remoteMonitorActive = remoteMonitorActive;
    theDebugPtr->theContextPtr = aContextPtr;
    theDebugPtr->theState = theState;
}
