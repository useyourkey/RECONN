//******************************************************************************
//******************************************************************************
// FILE:        clientApp.c
//
// CLASSES:     
//
// DESCRIPTION: This is the file which contains the main function for the 
//              client processing.
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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <mqueue.h>

#include "reconn.h"
#include "clientApp.h"
#include "socket.h"
#include "spectrum.h"
#include "gps.h"
#include "powerMeter.h"
#include "dmm.h"
#include "powerMgmt.h"
#include "eqptResponse.h"
#include "gpio.h"
#include "version.h"
#include "upgrade.h"
#include "debugMenu.h"
#include "wifi.h"
#include "remoteMonitor.h"
#include "fuel_gauge.h"
#include "libiphoned.h"
#include "aov_protocol.h"
#include "aov_errno.h"

struct timeval startTime;
struct timeval stopTime;
struct timeval result;

struct timeval deviceStartTime;
struct timeval deviceStopTime;
struct timeval deviceResult;

struct timeval queueStartTime;
struct timeval queueStopTime;
struct timeval queueResult;

// only used by a master client process
mqd_t masterClientMsgQid = -1;
static char  masterClientMsgBuf[4];
static struct mq_attr masterClientMsgQAttr;
static pthread_mutex_t clientMutex = PTHREAD_MUTEX_INITIALIZER;
//
extern int libiphoned_tx(unsigned char *, unsigned int);
extern sem_t insertedMasterSemaphore;
extern ReconnEqptDescriptors gEqptDescriptors;

int masterClientSocketFd = -1;
int insertedMasterSocketFd = -1; 
int MasterTransmitListenFd = -1;
/*
 * This pointer is only used during the process of switching the current master 
 * client to a slave and the correspoding slave to a master.
 */
static CLIENTCONTEXT  * theSlaveContextPtr = NULL; 

YESNO swUpgradeInProgress = NO;

//******************************************************************************
//******************************************************************************
// FUNCTION:        receive_packet_data
//
// CLASSES:     
//
// DESCRIPTION: Interface used, by functions within this file, to receive data from
//              from the socket. The data is coming from a wifi client.
//              
// PARAMETERS:
//              socket - the socket file descriptor
//              buffer - Pointer a buffer into which this interface will place the data
//              length - The data's length
//
//******************************************************************************
static int receive_packet_data(int socket, unsigned char *buffer, int *length) 
{
    int count;
    int loop;
    int len = 0;
    unsigned char *ptr;
#ifdef COMM_DEBUG
    int i;
#endif

    /*
     * The packet length must be at least RECONN_PACKET_HEADER_SIZE bytes
     * which includes the message ID (2 bytes) and the data length (2 bytes)
     */
    if((len = recv(socket, buffer, RECONN_PACKET_HEADER_SIZE, 0)) == 0)
    {
        *length = RECONN_CLIENT_SOCKET_CLOSED;
        return(-1);
    }
    else if((len == -1) && ((errno == EAGAIN) || ((errno == EWOULDBLOCK))))
    {
        *length = RECONN_EAGAINOREWOULDBLOCK;
        return(-1);
    }
    else if(len == -1)
    {
        *length = -1;
        return(-1);
    }
    else if (len != RECONN_PACKET_HEADER_SIZE) 
    {
        reconnDebugPrint("%s: recv is less than %d  len = %d\n", __FUNCTION__, RECONN_PACKET_HEADER_SIZE, len);
        *length = -1;
        return (-1);
    }
    else
    {

#ifdef COMM_DEBUG
        reconnDebugPrint("%s %d: ", __FUNCTION__, __LINE__);
        ptr = buffer;
        for (i = 0; i < RECONN_PACKET_HEADER_SIZE; ++i, ptr++) 
        {
            reconnDebugPrint("%x ", (unsigned int) *ptr);
        }
#endif

        count = buffer[2] << 8;
        count = count + buffer[3];
        if(count > RECONN_PAYLOAD_SIZE)
        {
            reconnDebugPrint("%s: received payload of %u is larger than %u\n", __FUNCTION__, count, RECONN_PAYLOAD_SIZE);
        }
        ptr = (unsigned char *)&buffer[RECONN_PACKET_HEADER_SIZE];
        for (loop = 0; loop < count; loop++, ptr++) 
        {
            recv(socket, ptr,  1, 0);
#ifdef COMM_DEBUG
            reconnDebugPrint("%x ", *ptr);
#endif
        }
#ifdef COMM_DEBUG
        reconnDebugPrint("\n");
#endif
    *length = count + RECONN_PACKET_HEADER_SIZE;
    return (count + RECONN_PACKET_HEADER_SIZE);
    }
}
//******************************************************************************
//******************************************************************************
// FUNCTION:        reconnClientTask
//
// CLASSES:     
//
// DESCRIPTION: This file is the main work horse of the reconn embedded software.
//              Any reconn iPhone application (client) the connects to the toolkit, 
//              whether the connection is via WiFi or the 30 pin front panel connector,
//              will have its own reconnClientTask() thread. The differences are 
//              
//              1. Slave clients have a limited number of executable opcodes that 
//                 the thread will process.
//              2. Master clients process all opcodes and create a message queue used
//                 to receives messages about the front panel connection state.
//              3. Inserted master clients process all opcodes and do NOT create a 
//                 message queue.
//              4. All client types except the inserted master use sockets with which
//                 to communicate with the iPhone application. The inserted master uses
//                 libiphoned() library calls.
//
//******************************************************************************
void *reconnClientTask(void *args) 
{
    CLIENTCONTEXT  *contextPtr = (CLIENTCONTEXT *)args;
    contextPtr->retStatus = 1;
    contextPtr->theResponsePktPtr = &(contextPtr->theResponsePkt);
    contextPtr->retCode = RECONN_SUCCESS;
    contextPtr->connectionOpen = TRUE;

    reconnDebugPrint("%s: Sending %s to client %d\n", __FUNCTION__, RECONNBEGIN, contextPtr->index);

    pthread_mutex_lock(&clientMutex);
    if(contextPtr->mode == INSERTEDMASTERMODE)
    {
        /*
         * Give the MFI a chance to settle down before
         * sending the handshake between this app and the iphone app.
         */
        sleep(1);
        reconnDebugPrint("%s: sending %s \n", __FUNCTION__, RECONNBEGIN);
        // Send response out the 30 pin USB 
        libiphoned_tx((unsigned char *)RECONNBEGIN, strlen(RECONNBEGIN));
        reconnDebugPrint("%s: registering client %d with eqptTask\n", __FUNCTION__, contextPtr->index);
        if(reconnRegisterClientApp(contextPtr) != RECONN_SUCCESS)
        {
            reconnDebugPrint("%s: Inserted Master reconnRegisterClientApp() index %u failed\n", __FUNCTION__, contextPtr->index);
        }
        //
        // An iPhone that is inserted into the front panel 
    }
    else
    {
        sendSocket(contextPtr->socketFd, (unsigned char *)RECONNBEGIN, strlen(RECONNBEGIN), 0);
    }

    reconnDebugPrint("%s: reconnClientTask index %d\n", __FUNCTION__, contextPtr->index);
    reconnDebugPrint("%s:                  mode == %s\n", __FUNCTION__, (contextPtr->mode == MASTERMODE) ? "Master": (contextPtr->mode == SLAVEMODE) ? "Slave" : (contextPtr->mode ==  REMOTEMODE) ? "Remote" : "Inserted Master");
    reconnDebugPrint("%s:                  socketFd %d\n", __FUNCTION__, contextPtr->socketFd);
    //reconnDebugPrint("\n%s:                  gpsFd %d\n", __FUNCTION__, contextPtr->eqptDescriptors->gpsFd);
    reconnDebugPrint("%s:                  powerMeterFd %d\n", __FUNCTION__, contextPtr->eqptDescriptors->powerMeterFd);
    //reconnDebugPrint("%s:                  lnbFd %d\n", __FUNCTION__, contextPtr->eqptDescriptors->lnbFd);
    reconnDebugPrint("%s:                  dmmFd %d\n", __FUNCTION__, contextPtr->eqptDescriptors->dmmFd);
    reconnDebugPrint("%s:                  analyzerFd %d\n", __FUNCTION__, contextPtr->eqptDescriptors->analyzerFd);

    pthread_mutex_unlock(&clientMutex);
    while (contextPtr->connectionOpen == TRUE) 
    {
        contextPtr->responseNeeded = FALSE;
        memset((unsigned char *) &(contextPtr->thePacket), 0, sizeof(ReconnPacket));
        /* receive the command from the client */

#if 0
        reconnDebugPrint("%s: client with index %d waiting for command\n", __FUNCTION__, contextPtr->index);
#endif
        contextPtr->packetRetCode = receive_packet_data(contextPtr->socketFd, (unsigned char *)&(contextPtr->thePacket), &contextPtr->length);

        if((contextPtr->packetRetCode == -1) && (contextPtr->length == RECONN_CLIENT_SOCKET_CLOSED))
        {
            reconnDebugPrint("%s: Socket closed by Client %d mode = %s\n", __FUNCTION__, contextPtr->index, (contextPtr->mode == MASTERMODE) ? "Master": (contextPtr->mode == SLAVEMODE) ? "Slave" : (contextPtr->mode ==  REMOTEMODE) ? "Remote" : "Inserted Master");
            contextPtr->connectionOpen = FALSE;
            reconnReturnClientIndex(contextPtr->index);
            reconnDeRegisterClientApp(contextPtr);
            if(contextPtr->mode == MASTERMODE)
            {
                if((masterClientMsgQid != -1) && (mq_close(masterClientMsgQid) == -1))
                {
                    reconnDebugPrint("%s: mq_close() failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
                }
                dmmSaveConfig();
                masterClientMsgQid = -1;
                masterClientSocketFd = -1;
            }
            contextPtr->connectionOpen = FALSE;
            continue;
        }
        else if((contextPtr->packetRetCode == -1) && (contextPtr->length == -1))
        {
            reconnDebugPrint("%s: Error reading from socket\n", __FUNCTION__);
            reconnDeRegisterClientApp(contextPtr);
            reconnReturnClientIndex(contextPtr->index);
            if(contextPtr->mode != SLAVEMODE)
            {
                masterClientSocketFd = -1;
            }
            contextPtr->connectionOpen = FALSE;
            continue;
        }
        else
        {
            /*
             * Something has returned from receive_packet_data(). If we are a master device then
             * some data has returned OR there was a recv() timeout within receive_packet_data(). In either
             * case, see if there is anything on the masterClientMsgQid informing this task of an iphone
             * front panel insertion/extraction. 
             *
             * The master device, whether Wifi or MFI inserted, should
             * never stop sending data with one exception:
             * when an iPhone is a WiFi master and is then inserted into the toolkit's front panel it will stop 
             * sending data for a short amount of time and receive_packet_data() will timeout. If the amount
             * of time the master device (MFI or WIFI) is longer than 2 seconds something bad has happend so 
             * remove mastership and kill the thread. This prevents an unpredictable event from holding 
             * mastership forever.
             *
             * Note: Slave clients block inside receive_packet_data() and will never return 
             * RECONN_EAGAINOREWOULDBLOCK.
             */
            if((masterClientMsgQid != -1) && (mq_getattr(masterClientMsgQid, &masterClientMsgQAttr) == 0))
            {
                if(masterClientMsgQAttr.mq_curmsgs)
                {
                    if((contextPtr->numBytes = mq_receive(masterClientMsgQid, (char *)&masterClientMsgBuf, masterClientMsgQAttr.mq_msgsize, NULL)) == -1)
                    {
                        if(errno != EAGAIN)
                        {
                            reconnDebugPrint("%s: mq_receive failed %d (%s)\n", __FUNCTION__, errno, strerror(errno));
                        }
                        continue;
                    }
                    else
                    {
                        reconnDebugPrint("%s: Client %d message on mq_receive \n", __FUNCTION__, contextPtr->index);
                        if(masterClientMsgBuf[0] == MASTER_INSERTED)
                        {
                            // send message to the iPhone client telling it 
                            // that mastership has been removed because an 
                            // iPhone has been inserted into the toolkit's 
                            // front panel.
                            reconnDebugPrint("%s: Client %d Received MASTER_INSERTED \n", __FUNCTION__, contextPtr->index);
                            sendReconnResponse (contextPtr->socketFd, 
                                    ((MASTER_MODE_REMOVED_NOTIF & 0xff00) >> 8),
                                    MASTER_MODE_REMOVED_NOTIF & 0x00ff, RECONN_ERROR_UNKNOWN, contextPtr->mode);

                            contextPtr->mode = SLAVEMODE;
                            masterClientSocketFd = -1;
                            contextPtr->flags = fcntl(contextPtr->socketFd, F_GETFL, NULL);
                            contextPtr->flags &= (~O_NONBLOCK);
                            fcntl(contextPtr->socketFd, F_SETFL, contextPtr->flags);
                            if(mq_close(masterClientMsgQid) == -1)
                            {
                                reconnDebugPrint("%s: mq_close() failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
                            }
                            masterClientMsgQid = -1;
                            sem_post(&insertedMasterSemaphore);
                            usleep(CLIENTSLEEPTIME);
                            continue;
                        }
                        else if(masterClientMsgBuf[0] == MASTER_EXTRACTED)
                        {
                            reconnDebugPrint("%s: Client %d Received MASTER_EXTRACTED \n", __FUNCTION__, contextPtr->index);
                            reconnReturnClientIndex(contextPtr->index);
                            reconnDeRegisterClientApp(contextPtr);
                            close(insertedMasterSocketFd);
                            masterClientSocketFd = insertedMasterSocketFd = -1;
                            if(mq_close(masterClientMsgQid) == -1)
                            {
                                reconnDebugPrint("%s: mq_close() failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
                            }
                            masterClientMsgQid = -1;
                            contextPtr->connectionOpen = FALSE;
                            continue;
                        }
                        else if(masterClientMsgBuf[0] == LOW_BATTERY)
                        {
                            reconnDebugPrint("%s: Client %d Received LOW_BATTERY indication from power management\n", __FUNCTION__, contextPtr->index);
                            memset(contextPtr->theResponsePktPtr, 0, sizeof(ReconnResponsePacket));
                            ADD_RSPID_TO_PACKET(GENERIC_RESPONSE, contextPtr->theResponsePktPtr);
                            ADD_MSGID_TO_PACKET(RECONN_POWER_DOWN_NOTIF, contextPtr->theResponsePktPtr);
                            ADD_DATA_LENGTH_TO_PACKET(0, contextPtr->theResponsePktPtr);
                            if(contextPtr->mode == INSERTEDMASTERMODE)
                            {
                                // Send response out the 30 pin USB 
                                libiphoned_tx((unsigned char *)contextPtr->theResponsePktPtr, RECONN_RSPPACKET_HEADER_SIZE);
                            }
                            reconnEqptAddMsgToQ((char *)contextPtr->theResponsePktPtr, RECONN_RSPPACKET_HEADER_SIZE);
                            continue;
                        }
                        else if(masterClientMsgBuf[0] == METER_INSERTED)
                        {
                            reconnDebugPrint("%s: Client %d Received METER_INSERTED indication\n", __FUNCTION__, contextPtr->index);
                            memset(contextPtr->theResponsePktPtr, 0, sizeof(ReconnResponsePacket));
                            ADD_RSPID_TO_PACKET(GENERIC_RESPONSE, contextPtr->theResponsePktPtr);
                            ADD_MSGID_TO_PACKET(PMETER_STATUS_NOTIF, contextPtr->theResponsePktPtr);
                            ADD_DATA_LENGTH_TO_PACKET(1, contextPtr->theResponsePktPtr);
                            contextPtr->theResponsePktPtr->dataPayload[0] = INSERTED;
                            if(insertedMasterSocketFd != -1)
                            {
                                libiphoned_tx((unsigned char *)contextPtr->theResponsePktPtr, RECONN_RSPPACKET_HEADER_SIZE + 1);
                            }
                            /*
                             * Tell all Slave devices
                             */
                            reconnEqptAddMsgToQ((char *)contextPtr->theResponsePktPtr, RECONN_RSPPACKET_HEADER_SIZE + 1);
                            continue;
                        }
                        else if(masterClientMsgBuf[0] == METER_EXTRACTED)
                        {
                            reconnDebugPrint("%s: Client %d Received METER_EXTRACTED indication from power management\n", __FUNCTION__, contextPtr->index);
                            memset(contextPtr->theResponsePktPtr, 0, sizeof(ReconnResponsePacket));
                            ADD_RSPID_TO_PACKET(GENERIC_RESPONSE, contextPtr->theResponsePktPtr);
                            ADD_MSGID_TO_PACKET(PMETER_STATUS_NOTIF, contextPtr->theResponsePktPtr);
                            ADD_DATA_LENGTH_TO_PACKET(1, contextPtr->theResponsePktPtr);
                            contextPtr->theResponsePktPtr->dataPayload[0] = EXTRACTED;
                            if(insertedMasterSocketFd != -1)
                            {
                                libiphoned_tx((unsigned char *)contextPtr->theResponsePktPtr, RECONN_RSPPACKET_HEADER_SIZE + 1);
                            }
                            /*
                             * Tell all Slave devices
                             */
                            reconnEqptAddMsgToQ((char *)contextPtr->theResponsePktPtr, RECONN_RSPPACKET_HEADER_SIZE + 1);
                            continue;
                        }
                        else if(masterClientMsgBuf[0] == RM_STATE_CHANGE)
                        {
                            reconnDebugPrint("%s: Client %d Received RM_STATE_CHANGE (%s) masterClientMsgBuf[1] == %d\n",
                                    __FUNCTION__, contextPtr->index, 
                                    (masterClientMsgBuf[1] == CONNECTED) ? "Connected" : 
                                    (masterClientMsgBuf[1] == DISCONNECTED) ? "Disconnected": "unknown",
                                    masterClientMsgBuf[1]);

                            ADD_RSPID_TO_PACKET(GENERIC_RESPONSE, contextPtr->theResponsePktPtr);
                            ADD_MSGID_TO_PACKET(WEB_SERVICE_STATE, contextPtr->theResponsePktPtr);
                            ADD_DATA_LENGTH_TO_PACKET(1, contextPtr->theResponsePktPtr);
                            contextPtr->theResponsePktPtr->dataPayload[0] = masterClientMsgBuf[1];
                            if(insertedMasterSocketFd != -1)
                            {
                                libiphoned_tx((unsigned char *)&(contextPtr->theResponsePktPtr), RECONN_RSPPACKET_HEADER_SIZE + 1);
                            }
                            /*
                             * Tell all Slave devices
                             */
                            reconnEqptAddMsgToQ((char *)contextPtr->theResponsePktPtr, RECONN_RSPPACKET_HEADER_SIZE + 1);
                            continue;
                        }
                        else
                        {
                            reconnDebugPrint("%s: invalid msg type %d\n", __FUNCTION__, masterClientMsgBuf[0]);
                            continue;
                        }
                    }
                }
                else
                {
                    if((contextPtr->packetRetCode == -1 ) && (contextPtr->length == RECONN_EAGAINOREWOULDBLOCK))
                    {
                        usleep(CLIENTSLEEPTIME);
                        /*
                         * This counter keeps track of the amount of time the master has not been transmitting
                         * data. If the "no data" time is too long then something bad has happened. Disconnect
                         * this master session.
                         */
                        if(contextPtr->mode == MASTERMODE)
                        {
                            contextPtr->noDataCount += CLIENTSLEEPTIME;
#ifndef __SIMULATION__
                            if(contextPtr->noDataCount >= CLIENTNODATATIME)
                            {
                                reconnDebugPrint("%s: Client %d being disconnected to due inactivity timeout (%d microseconds) \n", __FUNCTION__, contextPtr->index, contextPtr->noDataCount );
                                contextPtr->connectionOpen = FALSE;
                                reconnReturnClientIndex(contextPtr->index);
                                reconnDeRegisterClientApp(contextPtr);
                                if((masterClientMsgQid != -1) && (mq_close(masterClientMsgQid) == -1))
                                {
                                    reconnDebugPrint("%s: mq_close() failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
                                }
                                dmmSaveConfig();
                                masterClientMsgQid = -1;
                                masterClientSocketFd = -1;
                            }
#endif
                        }
                        continue;
                    }
                }
            }
#if 0
            else 
            {
                if((contextPtr->packetRetCode == -1 ) && (contextPtr->length == RECONN_EAGAINOREWOULDBLOCK))
                {
                    continue;
                }
            }
#endif
        }
#ifdef DEBUG_CLIENT

        reconnDebugPrint("%s %d: Packet received from client %d with length %d\n", __FUNCTION__, __LINE__, contextPtr->index, contextPtr->length);
        debugPtr = (char *)&(contextPtr->thePacket);
        for(debugIndex = 0; debugIndex < contextPtr->length + 4; debugIndex++)
        {
            reconnDebugPrint("0x%x ", debugPtr[debugIndex]);
        }
        reconnDebugPrint("\n");
#endif
        /* everyone needs to know the packet length */
        GET_DATA_LENGTH_FROM_PACKET(contextPtr->pktLength, contextPtr->thePacket);
        //reconnDebugPrint("%s %d: pktLength = %d\n", __FUNCTION__, __LINE__, contextPtr->pktLength);

        GET_MSGID_FROM_PACKET(contextPtr->cmdid, contextPtr->thePacket);
        //reconnDebugPrint("%s %d: cmdid = 0x%x\n", __FUNCTION__, __LINE__, cmdid);
        if((contextPtr->mode == SLAVEMODE) && 
                ((contextPtr->cmdid != KEEPALIVE_MESSAGE) &&
                 (contextPtr->cmdid != CLIENT_RESIGN_REQ) &&
                 (contextPtr->cmdid != CLIENT_ACCESS_REQ) && 
                 (contextPtr->cmdid != MASTER_MODE_RESIGN_REQ) && 
                 (contextPtr->cmdid != MASTER_MODE_REQ)))
        {
            // a command came in from the client that we do not like, so
            // do nothing and the client will deal with the lack of
            // response.
        }
        else if((contextPtr->mode == REMOTEMODE) &&
                ((contextPtr->cmdid != KEEPALIVE_MESSAGE) &&
                 (contextPtr->cmdid != CLIENT_RESIGN_REQ) &&
                 (contextPtr->cmdid != CLIENT_ACCESS_REQ) &&
                 (contextPtr->cmdid != MASTER_MODE_REQ)))
        {
            // a command came in from the remote client that we do not like so
            // do nothing, and the client will deal with the lack of
            // response.
        }
        else
        {
            contextPtr->noDataCount = 0;
            switch (contextPtr->cmdid) 
            {
                case KEEPALIVE_MESSAGE:
                {
                    //reconnDebugPrint("%s: Client %d Received KEEPALIVE_MESSAGE\n", __FUNCTION__, contextPtr->index);
                    sendReconnResponse(contextPtr->socketFd, 
                            ((KEEPALIVE_MESSAGE  & 0xff00) >> 8), 
                            (KEEPALIVE_MESSAGE & 0x00ff),
                            RECONN_SUCCESS, contextPtr->mode);
                    resetPowerStandbyCounter(RESET_SYSTEM_SHUTDOWN_TIME);
                    break;
                }
                case RECONN_CONNECTION_STATUS_REQ:
                {
                    reconnDebugPrint("%s: Client %d Received RECONN_CONNECTION_STATUS_REQ\n", __FUNCTION__, contextPtr->index);
                    sendReconnResponse(contextPtr->socketFd, 
                            ((RECONN_CONNECTION_STATUS_REQ & 0xff00) >> 8), 
                            (RECONN_CONNECTION_STATUS_REQ & 0x00ff), 
                            RECONN_SUCCESS, contextPtr->mode);
                    resetPowerStandbyCounter(RESET_SYSTEM_SHUTDOWN_TIME);
                    break;
                }
                case RECONN_ID_REQ:
                {
                    reconnDebugPrint("%s: Client %d Received RECONN_ID_REQ\n", __FUNCTION__, contextPtr->index);
                    if(getDeviceId(contextPtr) == RECONN_SUCCESS)
                    {
                        /*
                         * Send the data to the master then to all connected clients
                         */
                        if (contextPtr->mode == INSERTEDMASTERMODE)
                        {
                            libiphoned_tx((unsigned char *)contextPtr->theResponsePktPtr, RECONN_RSPPACKET_HEADER_SIZE + contextPtr->serialBytesRead);
                        }
                        reconnEqptAddMsgToQ((char *)contextPtr->theResponsePktPtr, RECONN_RSPPACKET_HEADER_SIZE + contextPtr->serialBytesRead);
                    }
                    else
                    {
                        reconnDebugPrint("%s: getDeviceId() failed.\n", __FUNCTION__);
                        sendReconnResponse(contextPtr->socketFd, contextPtr->thePacket.messageId.Byte[0],
                                contextPtr->thePacket.messageId.Byte[1], RECONN_FAILURE, contextPtr->mode);
                    }
                    break;
                }
                case RECONN_INIT_STATE_SET_REQ:
                {
                    char tmpSSID[WIFI_SSID_MAX_LENGTH+1];

                    reconnDebugPrint("%s: Client %d Received RECONN_INIT_STATE_SET_REQ %d\n", __FUNCTION__, contextPtr->index, contextPtr->thePacket.dataPayload[0]);
                    contextPtr->retCode = RECONN_SUCCESS;

                    if ((contextPtr->thePacket.dataPayload[0] == NON_DEFAULT) || 
                            (contextPtr->thePacket.dataPayload[0] == DEFAULT))
                    {
                        if(contextPtr->thePacket.dataPayload[0] == NON_DEFAULT)
                        {
                            unlink(RECONN_DEFAULT_FILE_NAME);
                            system("/sbin/siconfig usb0 down");
                            system("udhcpc --interface usb0 --host reconn --background --release");
                            system("iptables -X");
                            system("iptables -t nat -X");
                            system("iptables -t nat -A POSTROUTING -o usb0 -j MASQUERADE");
                            system("iptables -A FORWARD -i usb0 -j ACCEPT");
                            system("echo 1 > /proc/sys/net/ipv4/ip_forward");
                        }
                        else
                        {
                            if((contextPtr->tmpFd = fopen(RECONN_DEFAULT_FILE_NAME, "w")) == 0)
                            {
                                reconnDebugPrint("%s: fopen(%s,w) failed %d(%s)\n", __FUNCTION__,
                                        RECONN_DEFAULT_FILE_NAME);
                                contextPtr->retCode = RECONN_FAILURE;
                            }
                            else
                            {
                                fclose(contextPtr->tmpFd);
                                contextPtr->tmpFd = (FILE *)-1;
                                if((contextPtr->retCode = wifiUpdateHostapdConfFile(WIFI_PASSWD_TOKEN, "coolfire1")) == RECONN_SUCCESS)
                                {
                                    memset(&tmpSSID, 0, WIFI_SSID_MAX_LENGTH+1);
                                    strcat((char *)&tmpSSID, "reconn");

                                    if((contextPtr->tmpFd = fopen(RECONN_SERIALNUM_FILE_NAME, "r")) != 0)
                                    {
                                        if(fread(&(contextPtr->newSSID), 1, WIFI_SSID_MAX_LENGTH, contextPtr->tmpFd) <= 0)
                                        {
                                            /*
                                             * inform the developer of the failure. At least "reconn" 
                                             * will be written to the config file.
                                             */
                                            reconnDebugPrint("%s: fopen(%s, r) failed %d (%s)\n", 
                                                    __FUNCTION__, RECONN_SERIALNUM_FILE_NAME, errno,
                                                    strerror(errno));
                                        }
                                        else
                                        {
                                            strcat((char *)&tmpSSID, (char *)&(contextPtr->newSSID[1]));
                                        }
                                        fclose(contextPtr->tmpFd);
                                        contextPtr->tmpFd = (FILE *)-1;
                                    }
                                    else
                                    {
                                        /*
                                         * inform the developer of the failure. At least "reconn" will be
                                         * written to the config file.
                                         */
                                        reconnDebugPrint("%s: fopen(%s, r) failed %d (%s)\n", __FUNCTION__, RECONN_SERIALNUM_FILE_NAME, errno, strerror(errno));
                                    }
                                    contextPtr->retCode = wifiUpdateHostapdConfFile(WIFI_SSID_TOKEN, (char *)&tmpSSID);
                                }
                                else
                                {
                                     reconnDebugPrint("%s: wifiUpdateHostapdConfFile(WIFI_PASSWD_TOKEN, coolfire1) failed\n", __FUNCTION__);
                                     contextPtr->retCode = RECONN_FAILURE;
                                }
                            }
                        }
                    }
                    else
                    {
                        reconnDebugPrint("%s: Client %d invalid state %d\n", __FUNCTION__, contextPtr->thePacket.dataPayload[0]);
                        contextPtr->retCode = RECONN_INVALID_MESSAGE;
                    }
                    sendReconnResponse (contextPtr->socketFd,
                            contextPtr->thePacket.messageId.Byte[0],
                            contextPtr->thePacket.messageId.Byte[1],
                            contextPtr->retCode, contextPtr->mode);
                    resetPowerStandbyCounter(RESET_SYSTEM_SHUTDOWN_TIME);

                    break;
                }
                case CLIENT_RESIGN_REQ:
                {
                    reconnDebugPrint("%s: Client %d Received CLIENT_RESIGN_REQ\n", __FUNCTION__, contextPtr->index);
                    pthread_mutex_lock(&clientMutex);
                    if(contextPtr->mode == INSERTEDMASTERMODE)
                    {
                        sendReconnResponse(contextPtr->socketFd, contextPtr->thePacket.messageId.Byte[0], 
                                contextPtr->thePacket.messageId.Byte[1], RECONN_SUCCESS, contextPtr->mode);
                    }
                    else
                    {
                        if(contextPtr->mode == MASTERMODE)
                        {
                            if((masterClientMsgQid != -1) && (mq_close(masterClientMsgQid) == -1))
                            {
                                reconnDebugPrint("%s: mq_close() failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
                            }
                            dmmSaveConfig();
                            masterClientMsgQid = -1;
                            masterClientSocketFd = -1;
                        }
                        /* The client has requested to be disconnected */
                        sendReconnResponse(contextPtr->socketFd, contextPtr->thePacket.messageId.Byte[0],
                                contextPtr->thePacket.messageId.Byte[1], RECONN_SUCCESS, contextPtr->mode);
                        //usleep(CLIENTSLEEPTIME);
                        reconnDeRegisterClientApp(contextPtr);
                        reconnReturnClientIndex(contextPtr->index);
                        contextPtr->connectionOpen = FALSE;
                        resetPowerStandbyCounter(RESET_SYSTEM_SHUTDOWN_TIME);
                    }
                    pthread_mutex_unlock(&clientMutex);
                    break;
                }
                case CLIENT_ACCESS_REQ:
                {
                    reconnDebugPrint("%s: Client %d Received CLIENT_ACCESS_REQ\n", __FUNCTION__, contextPtr->index);
                    sendReconnResponse(contextPtr->socketFd, 
                            contextPtr->thePacket.messageId.Byte[0], 
                            contextPtr->thePacket.messageId.Byte[1], 
                            RECONN_SUCCESS, contextPtr->mode);
                    resetPowerStandbyCounter(RESET_SYSTEM_SHUTDOWN_TIME);
                    break;
                }
                case MASTER_MODE_REQ:
                {
                    reconnDebugPrint("%s: Client %d Received MASTER_MODE_REQ\n", __FUNCTION__, contextPtr->index);
                    if (masterClientSocketFd == contextPtr->socketFd)
                    {
                        // This process is already the master client
                        sendReconnResponse(contextPtr->socketFd, 
                                contextPtr->thePacket.messageId.Byte[0], 
                                contextPtr->thePacket.messageId.Byte[1], RECONN_SUCCESS, contextPtr->mode);
                        reconnDebugPrint("%s %d: Client already is Master. Sending Success \n", __FUNCTION__, __LINE__);
                        resetPowerStandbyCounter(RESET_SYSTEM_SHUTDOWN_TIME);
                        break;
                    }
                    else if (masterClientSocketFd == -1)
                    {
                        /*
                         * Open the queue which is used to communicate between 
                         * reconnMasterIphone(), powerMeterPresenceTask() and the master client
                         * application. When an iphone is inserted into the toolkit's front panel a 
                         * message will be sent here via masterClientMsgQid. The queue is then
                         * read to determine what needs to be done.
                         */
                        if(mq_unlink(INSERTED_MASTER_MSG_Q_NAME) == -1)
                        {
                            reconnDebugPrint("%s: mq_unlink() failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
                        }
                        masterClientMsgQAttr.mq_flags    = 0;
                        masterClientMsgQAttr.mq_maxmsg   = 5;
                        masterClientMsgQAttr.mq_msgsize  = 10;

                        if((masterClientMsgQid = 
                                    mq_open(INSERTED_MASTER_MSG_Q_NAME, 
                                        (O_RDWR | O_CREAT | 
                                         O_NONBLOCK), 0, &masterClientMsgQAttr)) == (mqd_t) -1)
                        {
                            reconnDebugPrint("%s: mq_open() failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
                            contextPtr->connectionOpen = FALSE;
                            free((void *)contextPtr->thisContext);
                            return(&contextPtr->retStatus);
                        }
                        reconnDebugPrint("%s: masterClientMsgQid = %d\n", __FUNCTION__, masterClientMsgQid);
                        contextPtr->flags = fcntl(contextPtr->socketFd, F_GETFL, NULL);
                        /*
                         * Change the Master's socketFd from Blocking to non blocking. This allows
                         * the master task to check the masterClientMsgQid for front panel insertion
                         * events. It also allows this task to make sure the master device has not
                         * stopped communicating. If it does stop communicating for some length of time
                         * mastership is removed and the thread is killed.
                         */
                        fcntl(contextPtr->socketFd, F_SETFL, contextPtr->flags |= O_NONBLOCK);
                        contextPtr->mode = (contextPtr->mode == SLAVEMODE) ? MASTERMODE : contextPtr->mode;

                        // This process becomes the master client
                        sendReconnResponse(contextPtr->socketFd, 
                                contextPtr->thePacket.messageId.Byte[0],
                                contextPtr->thePacket.messageId.Byte[1], 
                                RECONN_SUCCESS, contextPtr->mode);
                        /*
                         * If the box is defaulted then we have to send this OPCODE. The opcode
                         * causes the iphone application to display a setup wizard to the user.
                         */
                        if(stat(RECONN_DEFAULT_FILE_NAME, &(contextPtr->fileStat)) == 0)
                        {
                            sendReconnResponse(contextPtr->socketFd, 
                                ((RECONN_INIT_STATE_REQ & 0xff00) >> 8),
                                (RECONN_INIT_STATE_REQ & 0x00ff),
                                RECONN_ERROR_UNKNOWN, contextPtr->mode);
                        }
                        else if(errno != ENOENT)
                        {
                            reconnDebugPrint("%s: fstat(%s) failed %d(%s)\n", __FUNCTION__, 
                                    RECONN_DEFAULT_FILE_NAME, errno, strerror(errno));
                        }

                        masterClientSocketFd = contextPtr->socketFd;
                        /*
                         * Check to see if the power meter is present. If it is tell the master.
                         */
                        if(isPowerMeterPresent() == RECONN_SUCCESS)
                        {
                            reconnDebugPrint("%s: Sending insertion message for powerMeterFd %d\n", __FUNCTION__, gEqptDescriptors.powerMeterFd);
                            memset(contextPtr->theResponsePktPtr, 0, sizeof(ReconnResponsePacket));
                            ADD_RSPID_TO_PACKET(GENERIC_RESPONSE, contextPtr->theResponsePktPtr);
                            ADD_MSGID_TO_PACKET(PMETER_STATUS_NOTIF, contextPtr->theResponsePktPtr);
                            ADD_DATA_LENGTH_TO_PACKET(1, contextPtr->theResponsePktPtr);
                            contextPtr->theResponsePktPtr->dataPayload[0] = INSERTED;
                            if(insertedMasterSocketFd != -1)
                            {
                                libiphoned_tx((unsigned char *)contextPtr->theResponsePktPtr, RECONN_RSPPACKET_HEADER_SIZE + 1);
                            }
                            else
                            {
                                sendSocket(masterClientSocketFd, (unsigned char *)contextPtr->theResponsePktPtr, RECONN_RSPPACKET_HEADER_SIZE + 1, 0);
                            }
                        }

                        reconnDebugPrint("%s %d: Client %d is now the Master. Sending Success \n", __FUNCTION__, __LINE__, contextPtr->index);
                    }
                    else
                    {
                        sendReconnResponse(contextPtr->socketFd,
                                contextPtr->thePacket.messageId.Byte[0], 
                                contextPtr->thePacket.messageId.Byte[1], 
                                RECONN_INVALID_MESSAGE, contextPtr->mode);

                        reconnDebugPrint("%s %d: Sending Failure because there is already a master (%d)\n", __FUNCTION__, __LINE__, masterClientSocketFd);
                        /*
                         * Notify the master that this slave client has attached.
                         */
                        ADD_RSPID_TO_PACKET(GENERIC_RESPONSE, contextPtr->theResponsePktPtr);
                        ADD_MSGID_TO_PACKET(RECONN_SLAVE_ATTACHED, contextPtr->theResponsePktPtr);
                        ADD_DATA_LENGTH_TO_PACKET(0, contextPtr->theResponsePktPtr);
                        if (insertedMasterSocketFd != -1)
                        {
                            libiphoned_tx((unsigned char *)contextPtr->theResponsePktPtr, RECONN_RSPPACKET_HEADER_SIZE);
                        }
                        else if(masterClientSocketFd != -1)
                        {
                            sendSocket(masterClientSocketFd, (unsigned char *)contextPtr->theResponsePktPtr, RECONN_RSPPACKET_HEADER_SIZE, 0);
                        }
                        /*
                         * Check to see if the power meter is present. If it is tell the slave device.
                         */
                        if(isPowerMeterPresent() == RECONN_SUCCESS)
                        {
                            reconnDebugPrint("%s: Sending insertion message for powerMeterFd %d\n", __FUNCTION__, gEqptDescriptors.powerMeterFd);
                            memset(contextPtr->theResponsePktPtr, 0, sizeof(ReconnResponsePacket));
                            ADD_RSPID_TO_PACKET(GENERIC_RESPONSE, contextPtr->theResponsePktPtr);
                            ADD_MSGID_TO_PACKET(PMETER_STATUS_NOTIF, contextPtr->theResponsePktPtr);
                            ADD_DATA_LENGTH_TO_PACKET(1, contextPtr->theResponsePktPtr);
                            contextPtr->theResponsePktPtr->dataPayload[0] = INSERTED;
                            sendSocket(contextPtr->socketFd, (unsigned char *)contextPtr->theResponsePktPtr, RECONN_RSPPACKET_HEADER_SIZE + 1, 0);
                        }
                    }
                    reconnDebugPrint("%s: registering client with eqptTask\n", __FUNCTION__);
                    if(reconnRegisterClientApp(contextPtr) != RECONN_SUCCESS)
                    {   
                        reconnDebugPrint("%s: reconnRegisterClientApp() index %d failed\n", __FUNCTION__, 
                                contextPtr->index);
                        if(contextPtr->socketFd >= 0)
                        {
                            close(contextPtr->socketFd);
                            free((void *)contextPtr->thisContext);
                            return &contextPtr->retStatus;
                        }
                    }
                    resetPowerStandbyCounter(RESET_SYSTEM_SHUTDOWN_TIME);
                    break;
                }
                /*
                 * Should only be sent by a slave client
                 */
                case MASTER_MODE_RESIGN_REQ:
                {
                    reconnDebugPrint("%s: Client %d Received MASTER_MODE_RESIGN_REQ\n", __FUNCTION__, contextPtr->index);
                    if (masterClientSocketFd == contextPtr->socketFd)
                    {
                        sendReconnResponse(contextPtr->socketFd, 
                                contextPtr->thePacket.messageId.Byte[0], 
                                contextPtr->thePacket.messageId.Byte[1], 
                                RECONN_INVALID_STATE, contextPtr->mode);
                    }
                    else
                    {
                        /*
                         * Any iphone inserted in the reconn front panel is ALWAYS the 
                         * master and therefore will never give up mastership.
                         */
                        if(insertedMasterSocketFd != -1)
                        {
                            sendReconnResponse(contextPtr->socketFd, 
                                    ((MASTER_MODE_RESIGN_RESP & 0xff00) >> 8),
                                    (MASTER_MODE_RESIGN_RESP & 0x00ff), 
                                    RECONN_DENIED, contextPtr->mode);
                        }
                        else
                        {
                            /* 
                             *  Ask the master, via its file descriptor to release
                             *  mastership.
                             */
                            if(formatReconnPacket(MASTER_MODE_RESIGN_QUERY, (char *)0, 0, &(contextPtr->thePacket)) == RECONN_SUCCESS)
                            {
                                sendSocket(masterClientSocketFd, (unsigned char *)&(contextPtr->thePacket), RECONN_PACKET_HEADER_SIZE, 0);
                            }
                            /*
                             * Remember the slave's socket descriptor
                             */
                            theSlaveContextPtr = (CLIENTCONTEXT *)contextPtr->thisContext;
                        }
                    }
                    resetPowerStandbyCounter(RESET_SYSTEM_SHUTDOWN_TIME);
                    break;
                }
                /*
                 * Only sent by a master client
                 */
                case MASTER_MODE_RESIGN_RESP:
                {
                    reconnDebugPrint("n%s: Client %d Received MASTER_MODE_RESIGN_RESP\n", __FUNCTION__, contextPtr->index);
                    if ((contextPtr->thePacket.dataPayload[0] == RECONN_SUCCESS) || 
                            (contextPtr->thePacket.dataPayload[0] == RECONN_DENIED))
                    {
                        if(contextPtr->thePacket.dataPayload[0] == RECONN_SUCCESS)
                        {
                            /*
                             * Tell the master iPhone to switch to a slave
                             */
                            if(formatReconnPacket(MASTER_MODE_REMOVED_NOTIF, (char *)0, 0, &(contextPtr->thePacket)) == RECONN_SUCCESS)
                            {
                                sendSocket(masterClientSocketFd, (unsigned char *)&(contextPtr->thePacket), RECONN_PACKET_HEADER_SIZE, 0);
                                /*
                                 * Switch this task to slave, change its socket from non-blocking to blocking.
                                 */
                                contextPtr->mode  = SLAVEMODE;
                                contextPtr->flags = fcntl(contextPtr->socketFd, F_GETFL, NULL);
                                contextPtr->flags &= (~O_NONBLOCK);
                                fcntl(contextPtr->socketFd, F_SETFL, contextPtr->flags);

                                /*
                                 * Now that this task is going to be master, change its socketFd from Blocking to 
                                 * non blocking. This allows the task to check the masterClientMsgQid for front panel 
                                 * insertion events. It also allows this task to make sure the master iPhone device has not
                                 * stopped communicating. If it does stop communicating for some length of time
                                 * mastership is removed and the thread is killed.
                                 */
                                fcntl(theSlaveContextPtr->socketFd, F_SETFL, theSlaveContextPtr->flags |= O_NONBLOCK);

                                /*
                                 * This slave task which started the slave to master transition becomes the master.
                                 */
                                masterClientSocketFd = theSlaveContextPtr->socketFd;
                                theSlaveContextPtr->mode = MASTERMODE;
                                if(formatReconnPacket(MASTER_MODE_SET_NOTIF, (char *)0, 0, &(theSlaveContextPtr->thePacket)) == RECONN_SUCCESS)
                                {
                                    reconnDebugPrint("%s: sending MASTER_MODE_SET_NOTIF\n", __FUNCTION__);
                                    sendSocket(theSlaveContextPtr->socketFd, (unsigned char *)&(theSlaveContextPtr->thePacket), RECONN_PACKET_HEADER_SIZE, 0);
                                }
                                /*
                                 * Notify the new master that a slave client has attached.
                                 */
                                ADD_RSPID_TO_PACKET(GENERIC_RESPONSE, contextPtr->theResponsePktPtr);
                                ADD_MSGID_TO_PACKET(RECONN_SLAVE_ATTACHED, contextPtr->theResponsePktPtr);
                                ADD_DATA_LENGTH_TO_PACKET(0, contextPtr->theResponsePktPtr);
                                sendSocket(masterClientSocketFd, (unsigned char *)contextPtr->theResponsePktPtr, RECONN_RSPPACKET_HEADER_SIZE, 0);
                                theSlaveContextPtr = NULL;
                            }
                        }
                        else
                        {
                            sendReconnResponse(theSlaveContextPtr->socketFd, 
                                    contextPtr->thePacket.messageId.Byte[0], 
                                    contextPtr->thePacket.messageId.Byte[1], 
                                    RECONN_DENIED, contextPtr->mode);
                        }
                    }
                    else
                    { 
                        reconnDebugPrint("%s: Invalid payload %d\n", __FUNCTION__, contextPtr->thePacket.dataPayload[0]);
                    }
                    break;
                }
                case RECONN_SW_VERSION_REQ:
                {
                    unsigned int length;
                    char *theSwVersionString;

                    reconnDebugPrint("n%s: Client %d Received RECONN_SW_VERSION_NOTIF\n", __FUNCTION__, contextPtr->index);

                    memset(contextPtr->theResponsePktPtr, 0, sizeof(ReconnResponsePacket));
                    ADD_RSPID_TO_PACKET(GENERIC_RESPONSE, contextPtr->theResponsePktPtr);
                    ADD_MSGID_TO_PACKET(RECONN_SW_VERSION_REQ, contextPtr->theResponsePktPtr);
                    theSwVersionString = getReconnSwVersion();
                    length = strlen(theSwVersionString);
                    ADD_DATA_LENGTH_TO_PACKET(length, contextPtr->theResponsePktPtr);
                    strncpy(&(contextPtr->theResponsePktPtr->dataPayload[0]), theSwVersionString, length);
                    reconnDebugPrint("%s: returning version %s\n", __FUNCTION__, theSwVersionString);
                    if(contextPtr->mode == INSERTEDMASTERMODE)
                    {
                        // Send response out the 30 pin USB 
                        libiphoned_tx((unsigned char *)contextPtr->theResponsePktPtr, RECONN_RSPPACKET_HEADER_SIZE + length);
                    }
                    else
                    {
                        sendSocket(masterClientSocketFd, 
                                (unsigned char *)contextPtr->theResponsePktPtr, RECONN_RSPPACKET_HEADER_SIZE + length, 0);
                    }
                    resetPowerStandbyCounter(RESET_SYSTEM_SHUTDOWN_TIME);
                    break;
                }
                case RECONN_DEVICE_DEBUG_REQ:
                {
                    sendReconnResponse(contextPtr->socketFd, 
                            contextPtr->thePacket.messageId.Byte[0], 
                            contextPtr->thePacket.messageId.Byte[1], 
                            RECONN_FAILURE, contextPtr->mode);
                    break;
                }
                case RECONN_IP_ADDRESS_REQ:
                {
                    reconnDebugPrint("n%s: Client %d Received RECONN_IP_ADDRESS_REQ\n", __FUNCTION__, contextPtr->index);

                    sendReconnResponse(contextPtr->socketFd, 
                            contextPtr->thePacket.messageId.Byte[0], 
                            contextPtr->thePacket.messageId.Byte[1], 
                            RECONN_SUCCESS, contextPtr->mode);
                    if ((contextPtr->tmpSocketFd = socket(AF_INET, SOCK_STREAM, 0)) <= 0)
                    {
                        reconnDebugPrint("%s: socket() failed %d (%s)\n", __FUNCTION__, errno, strerror(errno));
                    }
                    else
                    {

                        memset(&contextPtr->ifr, 0, sizeof(contextPtr->ifr));
#ifdef __SIMULATION__
                        strncpy(contextPtr->ifr.ifr_name, "eth0", IF_NAMESIZE);
#else
                        strncpy(contextPtr->ifr.ifr_name, "usb0", IF_NAMESIZE);
#endif
                        memset(&(contextPtr->theResponsePktPtr->dataPayload[0]), 0
                                , RECONN_RSP_PAYLOAD_SIZE);
                        memset(contextPtr->IPAddr, 0, IPADDR_LEN);
                        memset(contextPtr->SubnetMask, 0, MACADDR_LEN);
                        ADD_RSPID_TO_PACKET(GENERIC_RESPONSE, contextPtr->theResponsePktPtr);
                        ADD_MSGID_TO_PACKET(RECONN_IP_ADDRESS_NOTIF, contextPtr->theResponsePktPtr);
                        if (ioctl(contextPtr->tmpSocketFd, SIOCGIFADDR, &contextPtr->ifr) != -1)
                        {
                            strcpy(contextPtr->IPAddr, inet_ntoa(((struct sockaddr_in *)&contextPtr->ifr.ifr_addr)->sin_addr));
                            if (ioctl(contextPtr->tmpSocketFd, SIOCGIFNETMASK, &contextPtr->ifr) != -1)
                            {
                                strcpy(contextPtr->SubnetMask, inet_ntoa(((struct sockaddr_in *)&contextPtr->ifr.ifr_netmask)->sin_addr));
                                strcpy(&(contextPtr->theResponsePktPtr->dataPayload[0]), contextPtr->IPAddr);
                                contextPtr->length = strlen(contextPtr->IPAddr);
                                /*
                                 * Because the dataPayload area has been set to '0' adding one to the length
                                 * effectively adds a NUll terminator between the IPAddr and subnetmask strings.
                                 */
                                contextPtr->length++;

                                strcpy(&(contextPtr->theResponsePktPtr->dataPayload[contextPtr->length]), contextPtr->SubnetMask);
                                contextPtr->length += strlen(contextPtr->SubnetMask);
                                /*
                                 * Because the dataPayload area has been set to '0' adding one to the length
                                 * effectively adds a NUll terminator after subnetmask string.
                                 */
                                contextPtr->length++;
                                ADD_DATA_LENGTH_TO_PACKET(contextPtr->length, contextPtr->theResponsePktPtr);
                                close(contextPtr->tmpSocketFd);
                            }
                            else
                            {
                                /*
                                 * Because the dataPayload area has been set to '0' adding one to the length
                                 * effectively adds a NUll terminator between the IPAddr and subnetmask strings.
                                 */
                                contextPtr->length = 1;
                                ADD_DATA_LENGTH_TO_PACKET(contextPtr->length, contextPtr->theResponsePktPtr);
                                reconnDebugPrint("%s: ioctl(SIOCGIFNETMASK) failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
                            }
                        }
                        else
                        {
                            /*
                             * Because the dataPayload area has been set to '0' adding one to the length
                             * effectively adds a NULL terminator to the dataPayload.
                             */
                            contextPtr->length = 1;
                            ADD_DATA_LENGTH_TO_PACKET(contextPtr->length, contextPtr->theResponsePktPtr);
                            reconnDebugPrint("%s: ioctl(SIOCGIFADDR) failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
                        }
                        close(contextPtr->tmpSocketFd);
                    }
                    if (contextPtr->mode == INSERTEDMASTERMODE)
                    {
                        libiphoned_tx((unsigned char *)contextPtr->theResponsePktPtr, RECONN_RSPPACKET_HEADER_SIZE + contextPtr->length);
                    }
                    else
                    {
                        sendSocket(masterClientSocketFd,
                                (unsigned char *)contextPtr->theResponsePktPtr,
                                RECONN_RSPPACKET_HEADER_SIZE + contextPtr->length, 0);
                    }
                    break;
                }
#if 0
                case REMOTE_MONITOR_STATE_REQ:
                {
                    reconnDebugPrint("n%s: Client %d Received REMOTE_MONITOR_STATE_REQ\n", __FUNCTION__, contextPtr->index);
                    sendReconnResponse (contextPtr->socketFd, 
                            ((REMOTE_MONITOR_STATE_REQ  & 0xff00) >> 8), 
                            (REMOTE_MONITOR_STATE_REQ  & 0x00ff), 
                            getRemoteMonitorState(), contextPtr->mode); 
                    break;
                }
                case REMOTE_MONITOR_ENABLE_REQ:
                {
                    reconnDebugPrint("\n%s: Client %d Received REMOTE_MONITOR_ENABLE_REQ payload == %d\n", __FUNCTION__, contextPtr->index, contextPtr->thePacket.dataPayload[0]);

                    reconnDebugPrint("\n%s: Client %d monitorState = %d \n", __FUNCTION__, contextPtr->index, contextPtr->thePacket.dataPayload[0]);
                    contextPtr->retCode = RECONN_SUCCESS;
                    if ((contextPtr->thePacket.dataPayload[0] == MONITOR_OFF) || 
                            (contextPtr->thePacket.dataPayload[0] == MONITOR_ON))
                    {
                        if((contextPtr->retCode = remoteMonitorActivate((contextPtr->thePacket.dataPayload[0] == MONITOR_ON) ? YES : NO)) != RECONN_SUCCESS)
                        {
                            reconnDebugPrint("%s: remoteMonitorActivate(%s) failed\n", __FUNCTION__, (contextPtr->thePacket.dataPayload[0] == MONITOR_ON) ? "YES": "NO");
                        }
                    }
                    else
                    {
                        reconnDebugPrint("%s: REMOTE_MONITOR_ENABLE_REQ: Invalid payload byte 0x%x\n", __FUNCTION__, contextPtr->thePacket.dataPayload[0]);
                        contextPtr->retCode =  RECONN_INVALID_PARAMETER; 
                    }

                    sendReconnResponse (contextPtr->socketFd, 
                            ((REMOTE_MONITOR_ENABLE_REQ & 0xff00) >> 8), 
                            (REMOTE_MONITOR_ENABLE_REQ & 0x00ff), 
                            contextPtr->retCode, contextPtr->mode); 
                    break;
                }
#endif
                case RECONN_RETRIEVE_CRASHLOG:
                {
                    reconnDebugPrint("\n%s: Client %d Received RECONN_RETRIEVE_CRASHLOG \n", __FUNCTION__, contextPtr->index);
                    memset(contextPtr->theResponsePktPtr, 0, sizeof(ReconnResponsePacket));
                    ADD_RSPID_TO_PACKET(GENERIC_RESPONSE, contextPtr->theResponsePktPtr);
                    ADD_MSGID_TO_PACKET(RECONN_RETRIEVE_CRASHLOG, contextPtr->theResponsePktPtr);

                    memset(&(contextPtr->theResponsePkt.dataPayload[0]), 0 , RECONN_RSP_PAYLOAD_SIZE);
                    if(stat(RECONN_CRASHLOG_FILE_NAME, &(contextPtr->fileStat)) == 0)
                    {
                        /*
                         * Open the crash log.
                         */
                        if((contextPtr->tmpFd = fopen(RECONN_CRASHLOG_FILE_NAME, "r")))
                        {
                            contextPtr->length = 0;
                            while(fread(&(contextPtr->theResponsePkt.dataPayload[contextPtr->length]), 1, 1, contextPtr->tmpFd))
                            {
                                if(contextPtr->length == RECONN_RSP_PAYLOAD_SIZE - 1)
                                {
                                    contextPtr->length ++;
                                    ADD_DATA_LENGTH_TO_PACKET(contextPtr->length, contextPtr->theResponsePktPtr);
                                    if(contextPtr->mode == INSERTEDMASTERMODE)
                                    {
                                        // Send response out the 30 pin USB 
                                        libiphoned_tx((unsigned char *)contextPtr->theResponsePktPtr,
                                                RECONN_RSPPACKET_HEADER_SIZE + contextPtr->length);
                                    }
                                    else
                                    {
                                        sendSocket(masterClientSocketFd, 
                                                (unsigned char *)contextPtr->theResponsePktPtr,
                                                RECONN_RSPPACKET_HEADER_SIZE + contextPtr->length, 0);
                                        usleep(400000);

                                    }
                                    memset(&(contextPtr->theResponsePkt.dataPayload[0]), 0 , RECONN_RSP_PAYLOAD_SIZE);
                                    contextPtr->length = 0;
                                }
                                else
                                {
                                    contextPtr->length++;
                                }
                            }
                            reconnDebugPrint("%s: Closing file\n", __FUNCTION__);
                            fclose(contextPtr->tmpFd);
                            contextPtr->tmpFd = (FILE *)-1;
                        }
                        else
                        {
                            contextPtr->length = 1;
                            ADD_DATA_LENGTH_TO_PACKET(contextPtr->length, contextPtr->theResponsePktPtr);
                            reconnDebugPrint("%s: fopen(%s, r) failed %d (%s)\n", __FUNCTION__, 
                                    RECONN_CRASHLOG_FILE_NAME, errno, strerror(errno));
                        }
                    }
                    else
                    {
                        contextPtr->length = 1;
                        ADD_DATA_LENGTH_TO_PACKET(contextPtr->length, contextPtr->theResponsePktPtr);
                        reconnDebugPrint("%s: No crash log available\n", __FUNCTION__);
                    }
                    /*
                     * We add 1 to the data length to send the trailing NULL
                     */
                    contextPtr->length++;
                    ADD_DATA_LENGTH_TO_PACKET(contextPtr->length, contextPtr->theResponsePktPtr);
                    if(contextPtr->mode == INSERTEDMASTERMODE)
                    {
                        // Send response out the 30 pin USB 
                        libiphoned_tx((unsigned char *)contextPtr->theResponsePktPtr,
                                RECONN_RSPPACKET_HEADER_SIZE + contextPtr->length);
                    }
                    else
                    {
                        sendSocket(masterClientSocketFd, 
                                (unsigned char *)contextPtr->theResponsePktPtr,
                                RECONN_RSPPACKET_HEADER_SIZE + contextPtr->length, 0);
                    }
                    resetPowerStandbyCounter(RESET_SYSTEM_SHUTDOWN_TIME);
                    break;
                }
                case BATTERY_LEVEL_REQ:
                {
                    extern uint8_t gBatteryPercentage;

                    if(RECONN_DEBUG_ON(BATTERY_DEBUG_RCV)) 
                    {
                        reconnDebugPrint("%s: Client %d Received BATTERY_LEVEL_REQ\n", __FUNCTION__, contextPtr->index);
                    }

                    memset(contextPtr->theResponsePktPtr, 0, sizeof(ReconnResponsePacket));
                    ADD_RSPID_TO_PACKET(GENERIC_RESPONSE, contextPtr->theResponsePktPtr);
                    ADD_MSGID_TO_PACKET(BATTERY_LEVEL_RSP, contextPtr->theResponsePktPtr);
                    ADD_DATA_LENGTH_TO_PACKET(1, contextPtr->theResponsePktPtr);
                    /*
                     * We do not fully charge or discharge the battery because it can effect the battery 
                     * shelf life. So we only allow the battery to charge to FUEL_GAUGE_MAX_PERCENTAGE.
                     */
                    contextPtr->theResponsePktPtr->dataPayload[0] = (gBatteryPercentage >= FUEL_GAUGE_MAX_PERCENTAGE) ? 100 : gBatteryPercentage;
                    if (contextPtr->mode == INSERTEDMASTERMODE)
                    {
                        libiphoned_tx((unsigned char *)contextPtr->theResponsePktPtr, RECONN_RSPPACKET_HEADER_SIZE + 1);
                    }
                    reconnEqptAddMsgToQ((char *)contextPtr->theResponsePktPtr, RECONN_RSPPACKET_HEADER_SIZE + 1);
                    resetPowerStandbyCounter(RESET_SYSTEM_SHUTDOWN_TIME);
                    break;
                }
                case BATTERY_CHARGE_STATE_REQ:
                {
                    extern char gChargerState;

                    if(RECONN_DEBUG_ON(BATTERY_DEBUG_RCV)) 
                    {
                        reconnDebugPrint("%s: Client %d Received BATTERY_CHARGE_STATE_REQ\n", __FUNCTION__, contextPtr->index);
                    }

                    memset(contextPtr->theResponsePktPtr, 0, sizeof(ReconnResponsePacket));
                    ADD_RSPID_TO_PACKET(GENERIC_RESPONSE, contextPtr->theResponsePktPtr);
                    ADD_MSGID_TO_PACKET(BATTERY_CHARGE_STATE_RSP, contextPtr->theResponsePktPtr);
                    ADD_DATA_LENGTH_TO_PACKET(1, contextPtr->theResponsePktPtr);
                    contextPtr->theResponsePktPtr->dataPayload[0] = gChargerState;
                    if (contextPtr->mode == INSERTEDMASTERMODE)
                    {
                        libiphoned_tx((unsigned char *)contextPtr->theResponsePktPtr, RECONN_RSPPACKET_HEADER_SIZE + 1);
                    }
                    reconnEqptAddMsgToQ((char *)contextPtr->theResponsePktPtr, RECONN_RSPPACKET_HEADER_SIZE + 1);
                    resetPowerStandbyCounter(RESET_SYSTEM_SHUTDOWN_TIME);
                    break;
                }
                case WIFI_STATUS_REQ:
                {
                    reconnDebugPrint("%s: Client %d Received WIFI_STATUS_REQ\n", __FUNCTION__, contextPtr->index);
                    sendReconnResponse (contextPtr->socketFd, 
                            contextPtr->thePacket.messageId.Byte[0],
                            contextPtr->thePacket.messageId.Byte[1], 
                            wifiGetState(), contextPtr->mode);
                    resetPowerStandbyCounter(RESET_SYSTEM_SHUTDOWN_TIME);
                    break;
                }
                case WIFI_SET_POWER_REQ:
                {
                    reconnDebugPrint("%s: Client %d Received WIFI_SET_POWER_REQ\n", __FUNCTION__, contextPtr->index);
                    if ((contextPtr->thePacket.dataPayload[0] == WIFI_ENABLE) || 
                            (contextPtr->thePacket.dataPayload[0] == WIFI_DISABLE))
                    {
                        contextPtr->retCode = wifiSetState(contextPtr->thePacket.dataPayload[0]);
                    }
                    else
                    {
                        contextPtr->retCode = RECONN_INVALID_PARAMETER;
                    }
                    sendReconnResponse (contextPtr->socketFd, 
                            contextPtr->thePacket.messageId.Byte[0],
                            contextPtr->thePacket.messageId.Byte[1], 
                            contextPtr->retCode, contextPtr->mode);
                    resetPowerStandbyCounter(RESET_SYSTEM_SHUTDOWN_TIME);
                    break;
                }
                case WIFI_CHANGE_PASSWORD_REQ:
                {
                    reconnDebugPrint("%s: Client %d Received WIFI_CHANGE_PASSWORD_REQ\n", __FUNCTION__, contextPtr->index);
                    // passphrase must be between 8 and 63 characters
                    if((contextPtr->pktLength < 8) || (contextPtr->pktLength > WIFI_PASSWD_MAX_LENGTH))
                    {
                        sendReconnResponse (contextPtr->socketFd, 
                                contextPtr->thePacket.messageId.Byte[0], 
                                contextPtr->thePacket.messageId.Byte[1],
                                RECONN_INVALID_PARAMETER, contextPtr->mode); 
                    }
                    else
                    {
                        disconnectAllClients(SLAVEMODE);
                        memset(&(contextPtr->newPasswd), 0, WIFI_PASSWD_MAX_LENGTH+1);
                        strncat(&(contextPtr->newPasswd[0]), &(contextPtr->thePacket.dataPayload[0]),
                                contextPtr->pktLength);
                        contextPtr->retCode = wifiUpdateHostapdConfFile(WIFI_PASSWD_TOKEN, &(contextPtr->newPasswd[0]));
                        sendReconnResponse (contextPtr->socketFd, 
                                contextPtr->thePacket.messageId.Byte[0], contextPtr->thePacket.messageId.Byte[1], contextPtr->retCode , contextPtr->mode); 
                        if(contextPtr->retCode == RECONN_SUCCESS)
                        { 
                            system("/etc/init.d/hostapd reload");
                        }
                    }
                    resetPowerStandbyCounter(RESET_SYSTEM_SHUTDOWN_TIME);
                    break;
                }
                case WIFI_CHANGE_SSID_REQ:
                {
                    reconnDebugPrint("%s: Client %d Received WIFI_CHANGE_SSID_REQ\n", __FUNCTION__, contextPtr->index);
                    if(contextPtr->pktLength > WIFI_SSID_MAX_LENGTH)
                    {
                        sendReconnResponse (contextPtr->socketFd, 
                                contextPtr->thePacket.messageId.Byte[0],
                                contextPtr->thePacket.messageId.Byte[1],
                                RECONN_INVALID_PARAMETER, contextPtr->mode); 
                    }
                    else
                    {
                        disconnectAllClients(SLAVEMODE);
                        memset(&(contextPtr->newSSID), 0, WIFI_SSID_MAX_LENGTH+1);
                        strncat(contextPtr->newSSID, &(contextPtr->thePacket.dataPayload[0]), contextPtr->pktLength);
                        contextPtr->retCode = wifiUpdateHostapdConfFile(WIFI_SSID_TOKEN, &(contextPtr->newSSID[0]));
                        sendReconnResponse (contextPtr->socketFd, 
                                contextPtr->thePacket.messageId.Byte[0], 
                                contextPtr->thePacket.messageId.Byte[1], 
                                contextPtr->retCode, contextPtr->mode); 
                        if(contextPtr->retCode == RECONN_SUCCESS)
                        {
                            system("/etc/init.d/hostapd reload");
                        }
                    }
                    resetPowerStandbyCounter(RESET_SYSTEM_SHUTDOWN_TIME);
                    break;
                }
                case WIFI_MAC_ADDRESS_REQ:
                {
                    reconnDebugPrint("%s: Client %d Received WIFI_MAC_ADDRESS_REQ\n", __FUNCTION__, contextPtr->index); 
                    memset(contextPtr->theResponsePktPtr, 0, sizeof(ReconnResponsePacket));
                    ADD_RSPID_TO_PACKET(GENERIC_RESPONSE, contextPtr->theResponsePktPtr);
                    ADD_MSGID_TO_PACKET(contextPtr->cmdid , contextPtr->theResponsePktPtr);
                    if(wifiGetMacAddress(&(contextPtr->theResponsePktPtr->dataPayload[0])) == RECONN_SUCCESS)
                    {
                        ADD_DATA_LENGTH_TO_PACKET(RECONN_MACADDR_LEN, contextPtr->theResponsePktPtr);
                    }
                    else
                    {
                        ADD_DATA_LENGTH_TO_PACKET(1, contextPtr->theResponsePktPtr);
                        contextPtr->theResponsePktPtr->dataPayload[0] = RECONN_FAILURE;
                    }

                    if(contextPtr->mode == INSERTEDMASTERMODE)
                    {
                        // Send response out the 30 pin USB 
                        libiphoned_tx((unsigned char *)contextPtr->theResponsePktPtr, RECONN_RSPPACKET_HEADER_SIZE + RECONN_MACADDR_LEN);
                    }
                    else
                    {
                        sendSocket(masterClientSocketFd, 
                                (unsigned char *)contextPtr->theResponsePktPtr, RECONN_RSPPACKET_HEADER_SIZE + RECONN_MACADDR_LEN, 0);
                    }
                    resetPowerStandbyCounter(RESET_SYSTEM_SHUTDOWN_TIME);
                    break;
                }
                case WIFI_SSID_RETRIEVE:
                case WIFI_PASSWORD_RETRIEVE:
                {
                    int length;

                    reconnDebugPrint("%s: Client %d Received %s\n", __FUNCTION__, contextPtr->index, (contextPtr->cmdid == WIFI_SSID_RETRIEVE) ? "WIFI_SSID_RETRIEVE": "WIFI_PASSWORD_RETRIEVE");

                    memset(contextPtr->theResponsePktPtr, 0, sizeof(ReconnResponsePacket));
                    ADD_RSPID_TO_PACKET(GENERIC_RESPONSE, contextPtr->theResponsePktPtr);
                    ADD_MSGID_TO_PACKET(contextPtr->cmdid , contextPtr->theResponsePktPtr);
                    if(wifiGetSSIDorPASSWD((contextPtr->cmdid == WIFI_SSID_RETRIEVE) ? WIFI_SSID_TOKEN : WIFI_PASSWD_TOKEN , (unsigned char *)&(contextPtr->theResponsePkt.dataPayload[0])) == RECONN_SUCCESS)
                    {
                        length = strlen(contextPtr->theResponsePkt.dataPayload);
                        ADD_DATA_LENGTH_TO_PACKET(length, contextPtr->theResponsePktPtr);
                    }
                    else
                    {
                        length = 1;
                        ADD_DATA_LENGTH_TO_PACKET(length, contextPtr->theResponsePktPtr);
                        contextPtr->theResponsePkt.dataPayload[0] = RECONN_FAILURE;
                    }
                    if(contextPtr->mode == INSERTEDMASTERMODE)
                    {
                        // Send response out the 30 pin USB 
                        libiphoned_tx((unsigned char *)contextPtr->theResponsePktPtr, RECONN_RSPPACKET_HEADER_SIZE + length);
                    }
                    else
                    {
                        sendSocket(masterClientSocketFd, 
                                (unsigned char *)contextPtr->theResponsePktPtr, RECONN_RSPPACKET_HEADER_SIZE + length, 0);
                    }
                    resetPowerStandbyCounter(RESET_SYSTEM_SHUTDOWN_TIME);
                    break;
                }
                case SPECANA_POWER_SET_REQ:
                {
                    if(RECONN_DEBUG_ON(SPECANA_DEBUG_RCV)) 
                    {
                        reconnDebugPrint("%s: Client %d Received SPECANA_POWER_SET_REQ\n", __FUNCTION__, contextPtr->index);
                    }
                    if ((contextPtr->thePacket.dataPayload[0] == POWER_ON) || 
                            (contextPtr->thePacket.dataPayload[0] == POWER_OFF))
                    {
                        if(reconnGpioAction(POWER_18V_GPIO, (contextPtr->thePacket.dataPayload[0] == POWER_ON) ? ENABLE : DISABLE, NULL) == RECONN_FAILURE)
                        {                           
                            reconnDebugPrint("%s: reconnGpioAction(GPIO_141, ENABLE/DISABLE, NULL) failed. \n", __FUNCTION__);                          
                            sendReconnResponse (contextPtr->socketFd, 
                                    contextPtr->thePacket.messageId.Byte[0],
                                    contextPtr->thePacket.messageId.Byte[1], 
                                    RECONN_INVALID_MESSAGE, contextPtr->mode); 
                        }
                        else
                        {
                            sendReconnResponse (contextPtr->socketFd, 
                                    contextPtr->thePacket.messageId.Byte[0], 
                                    contextPtr->thePacket.messageId.Byte[1],
                                    RECONN_SUCCESS, contextPtr->mode); 
                        }
                    }
                    resetPowerStandbyCounter(RESET_SPECTRUM_ANALYZER_STBY_COUNTER);
                    break;
                }
                case SPECANA_IDLE_CFG_REQ:
                {
                    if(RECONN_DEBUG_ON(SPECANA_DEBUG_RCV)) 
                    {
                        reconnDebugPrint("%s: Client %d Received SPECANA_IDLE_CFG_REQ\n", __FUNCTION__, contextPtr->index);
                    }
                    resetPowerStandbyCounter(RESET_SPECTRUM_ANALYZER_STBY_COUNTER);
                    break;
                }
                case SPECANA_PKT_SEND_REQ:
                {
                    if(RECONN_DEBUG_ON(SPECANA_DEBUG_RCV)) 
                    {
                        reconnDebugPrint("%s: Client %d Received  SPECANA_PKT_SEND_REQ\n", __FUNCTION__, contextPtr->index);
                    }

                    if(gDebugTimingEnabled == YES)
                    {
                        gettimeofday(&startTime, NULL);
                    }
                    if (masterClientSocketFd == contextPtr->socketFd) 
                    {
                        if(RECONN_DEBUG_ON(SPECANA_DEBUG_RCV)) 
                        {
                            reconnDebugPrint("%s: Calling SpectrumAnalyzerWrite pktLength = %d \n", 
                                    __FUNCTION__, contextPtr->pktLength);
                        }

                        if(SpectrumAnalyzerWrite((unsigned char *)&(contextPtr->thePacket.dataPayload), 
                                    contextPtr->pktLength) == RECONN_SA_PORT_NOT_INITIALIZED)
                        {
                            if(SpectrumAnalyzerInit(&(contextPtr->eqptDescriptors->analyzerFd)) == RECONN_FAILURE)
                            {
                                sendReconnResponse (contextPtr->socketFd, 
                                        ((SPECANA_HARDWARE_FAIL_NOTIF & 0xff00) >> 8), 
                                        SPECANA_HARDWARE_FAIL_NOTIF  & 0x00ff, 
                                        RECONN_ERROR_UNKNOWN, contextPtr->mode); 
                                break;
                            }
                            SpectrumAnalyzerWrite((unsigned char *)&(contextPtr->thePacket.dataPayload), contextPtr->pktLength);
                        }
                        sendReconnResponse (contextPtr->socketFd, 
                                contextPtr->thePacket.messageId.Byte[0], 
                                contextPtr->thePacket.messageId.Byte[1],
                                RECONN_SUCCESS, contextPtr->mode); 
                    }
                    resetPowerStandbyCounter(RESET_SPECTRUM_ANALYZER_STBY_COUNTER);
                    break;
                }
                case PMETER_POWER_SET_REQ:
                case PMETER_IDLE_CFG_REQ:
                {
                    if(RECONN_DEBUG_ON(PMETER_DEBUG_RCV)) 
                    {
                        reconnDebugPrint("%s: Client %d Received %s:\n", __FUNCTION__, contextPtr->index, 
                                (contextPtr->cmdid== PMETER_POWER_SET_REQ) ? 
                                "PMETER_POWER_SET_REQ" : "PMETER_IDLE_CFG_REQ");
                    }
                    sendReconnResponse(contextPtr->socketFd,
                            contextPtr->thePacket.messageId.Byte[0], 
                            contextPtr->thePacket.messageId.Byte[1], 
                            RECONN_INVALID_MESSAGE, contextPtr->mode);
                    resetPowerStandbyCounter(RESET_POWER_METER_STBY_COUNTER);
                    break;
                }
                case PMETER_PKT_SEND_REQ:
                {
                    if(RECONN_DEBUG_ON(PMETER_DEBUG_RCV)) 
                    {
                        reconnDebugPrint("%s: Client %d Received  PMETER_PKT_SEND_REQ:\n", __FUNCTION__, contextPtr->index);
                    }
                    if (masterClientSocketFd == contextPtr->socketFd) 
                    {
                        if(RECONN_DEBUG_ON(PMETER_DEBUG_RCV)) 
                        {
                            reconnDebugPrint("%s: Sending %c of length %d to power meter\n", __FUNCTION__, contextPtr->thePacket.dataPayload[0], contextPtr->pktLength);
                        }

                        if(powerMeterWrite((unsigned char *)&(contextPtr->thePacket.dataPayload[0]), contextPtr->pktLength) == RECONN_PM_PORT_NOT_INITIALIZED)
                        {
                            sendReconnResponse(contextPtr->socketFd, 
                                    contextPtr->thePacket.messageId.Byte[0], 
                                    contextPtr->thePacket.messageId.Byte[1], 
                                    RECONN_INVALID_STATE, contextPtr->mode);
                            break;
                        }
                    }
                    resetPowerStandbyCounter(RESET_POWER_METER_STBY_COUNTER);
                    break;
                }
                case DMM_POWER_SET_REQ:
                {
                    short theValue;

                    if(RECONN_DEBUG_ON(DMM_DEBUG_RCV))
                    {
                        reconnDebugPrint("%s: DMM_POWER_SET_REQ \n", __FUNCTION__, contextPtr->index);
                    }
                    if ((contextPtr->thePacket.dataPayload[0] == POWER_ON) || 
                            (contextPtr->thePacket.dataPayload[0] == POWER_OFF))
                    {
                        pthread_mutex_lock(&dmmMutex);
                        if(contextPtr->thePacket.dataPayload[0] == POWER_OFF)
                        {
                            if((contextPtr->retCode = dmmPowerDown()) == RECONN_FAILURE)
                            {                       
                                reconnDebugPrint("%s: reconnGpioAction(DMM_POWER_GPIO, DISABLE, NULL) failed. \n", __FUNCTION__);
                            }  
                            sendReconnResponse(contextPtr->socketFd, 
                                    contextPtr->thePacket.messageId.Byte[0], 
                                    contextPtr->thePacket.messageId.Byte[1], 
                                    contextPtr->retCode, contextPtr->mode);
                        }
                        else if(((contextPtr->retCode = reconnGpioAction(DMM_POWER_GPIO, READ, &theValue)) == RECONN_SUCCESS) && 
                                (theValue == GPIO_IS_INACTIVE))
                        {
                            contextPtr->retCode = dmmInit(&(contextPtr->eqptDescriptors->analyzerFd));
                        }
                        sendReconnResponse(contextPtr->socketFd, 
                                contextPtr->thePacket.messageId.Byte[0], 
                                contextPtr->thePacket.messageId.Byte[1], 
                                contextPtr->retCode, contextPtr->mode);
                        resetPowerStandbyCounter(RESET_DMM_STBY_COUNTER);
                        pthread_mutex_unlock(&dmmMutex);
                        break;
                    }
                    sendReconnResponse (contextPtr->socketFd, 
                            contextPtr->thePacket.messageId.Byte[0],
                            contextPtr->thePacket.messageId.Byte[1],
                            RECONN_INVALID_MESSAGE, contextPtr->mode);
                    break;
                }
                case DMM_IDLE_CFG_REQ:
                {
                    if(RECONN_DEBUG_ON(DMM_DEBUG_RCV)) 
                    {
                        reconnDebugPrint("%s: Client %d Received DMM_IDLE_CFG_REQ\n", __FUNCTION__, 
                                contextPtr->index);
                    }

                    // communicate with the device. So, send it a status command and get the response.
                    sendReconnResponse (contextPtr->socketFd, 
                            contextPtr->thePacket.messageId.Byte[0], 
                            contextPtr->thePacket.messageId.Byte[1], 
                            RECONN_SUCCESS, contextPtr->mode); 
                    resetPowerStandbyCounter(RESET_DMM_STBY_COUNTER);
                    break;
                }
                case DMM_PKT_SEND_REQ:
                {
                    if(RECONN_DEBUG_ON(DMM_DEBUG_RCV)) 
                    {
                        reconnDebugPrint("%s: Client %d Received DMM_PKT_SEND_REQ\n", __FUNCTION__, 
                                contextPtr->index);
                        reconnDebugPrint("%s: Sending %c to meter\n", __FUNCTION__, 
                                contextPtr->thePacket.dataPayload[0]);
                    }
                    pthread_mutex_lock(&dmmMutex);
                    if((contextPtr->retCode = dmmWrite((unsigned char *)&(contextPtr->thePacket.dataPayload), contextPtr->pktLength)) == RECONN_DMM_PORT_NOT_INITIALIZED)
                    {
                        if((contextPtr->retCode = dmmInit(&(gEqptDescriptors.dmmFd))) == RECONN_SUCCESS)
                        {
                            contextPtr->retCode = dmmWrite((unsigned char *)&(contextPtr->thePacket.dataPayload), contextPtr->pktLength);
                        }
                        else
                        {
                            reconnDebugPrint("%s: dmmInit() failed.\n", __FUNCTION__);
                        }
                    }
                    pthread_mutex_unlock(&dmmMutex);
                    if(contextPtr->retCode != RECONN_SUCCESS)
                    {
                        sendReconnResponse(contextPtr->socketFd,
                                contextPtr->thePacket.messageId.Byte[0],
                                contextPtr->thePacket.messageId.Byte[1], 
                                RECONN_FAILURE, contextPtr->mode);
                        if(RECONN_DEBUG_ON(DMM_DEBUG_RCV)) 
                        {
                            reconnDebugPrint("%s: Failure Sent back to client\n", __FUNCTION__);
                        }
                    }
                    resetPowerStandbyCounter(RESET_DMM_STBY_COUNTER);
                    break;
                }
                case DMM_BUILTINTEST_REQ:
                {
                    if(RECONN_DEBUG_ON(DMM_DEBUG_RCV)) 
                    {
                        reconnDebugPrint("%s: Client %d Received DMM_BUILTINTEST_REQ\n", __FUNCTION__, contextPtr->index);
                    }
                    pthread_mutex_lock(&dmmMutex);
                    sendReconnResponse (contextPtr->socketFd, 
                            contextPtr->thePacket.messageId.Byte[0], 
                            contextPtr->thePacket.messageId.Byte[1], 
                            dmmDiags(), contextPtr->mode);

                    pthread_mutex_unlock(&dmmMutex);
                    resetPowerStandbyCounter(RESET_DMM_STBY_COUNTER);
                    break;
                }
                case LNB_POWER_SET_REQ:
                {
                    if(RECONN_DEBUG_ON(LNB_DEBUG_RCV)) 
                    {
                        reconnDebugPrint("%s: Client %d Received LNB_POWER_SET_REQ\n", __FUNCTION__, contextPtr->index);
                    }
                    if ((contextPtr->thePacket.dataPayload[0] == POWER_ON) || 
                            (contextPtr->thePacket.dataPayload[0] == POWER_OFF))
                    {
                        if(reconnGpioAction(LNB_ENABLE_GPIO, 
                                    (contextPtr->thePacket.dataPayload[0] == POWER_ON)
                                    ? ENABLE : DISABLE, NULL) == RECONN_FAILURE)
                        {                           
                            reconnDebugPrint("%s: reconnGpioAction(LNB_10MHZ_GPIO, ENABLE/DISABLE, NULL) failed. \n", __FUNCTION__);
                            sendReconnResponse(contextPtr->socketFd, 
                                    contextPtr->thePacket.messageId.Byte[0], 
                                    contextPtr->thePacket.messageId.Byte[1],
                                    RECONN_FAILURE, contextPtr->mode);
                        }
                        else
                        {
                            sendReconnResponse(contextPtr->socketFd, contextPtr->thePacket.messageId.Byte[0], 
                                    contextPtr->thePacket.messageId.Byte[1], RECONN_SUCCESS, contextPtr->mode);
                        }
                        resetPowerStandbyCounter(RESET_SYSTEM_SHUTDOWN_TIME);
                    }
                    else
                    {
                        sendReconnResponse (contextPtr->socketFd, 
                                contextPtr->thePacket.messageId.Byte[0], 
                                contextPtr->thePacket.messageId.Byte[1], 
                                RECONN_INVALID_MESSAGE, contextPtr->mode); 
                    }
                    break;
                }
                case LNB_SA_10MHZ:
                {
                    if(RECONN_DEBUG_ON(LNB_DEBUG_RCV)) 
                    {
                        reconnDebugPrint("%s: Client %d Received LNB_SA_10MHZ %d\n", __FUNCTION__, 
                            contextPtr->index, contextPtr->thePacket.dataPayload[0]); 
                    }

                    if ((contextPtr->thePacket.dataPayload[0] == POWER_ON) || 
                            (contextPtr->thePacket.dataPayload[0] == POWER_OFF))
                    {
                        if(reconnGpioAction(SA_10MHZ_GPIO, (contextPtr->thePacket.dataPayload[0] == POWER_ON) ? ENABLE : DISABLE, NULL) == RECONN_FAILURE)
                        {                           
                            reconnDebugPrint("%s: reconnGpioAction(GPIO_%d, %d, NULL) failed.  \n", __FUNCTION__, SA_10MHZ_GPIO, contextPtr->thePacket.dataPayload[0]);

                            sendReconnResponse(contextPtr->socketFd, 
                                    contextPtr->thePacket.messageId.Byte[0], 
                                    contextPtr->thePacket.messageId.Byte[1],
                                    RECONN_FAILURE, contextPtr->mode);
                        }
                        else
                        {
                            sendReconnResponse(contextPtr->socketFd, 
                                    contextPtr->thePacket.messageId.Byte[0], 
                                    contextPtr->thePacket.messageId.Byte[1], 
                                    RECONN_SUCCESS, contextPtr->mode);
                        }
                        resetPowerStandbyCounter(RESET_SYSTEM_SHUTDOWN_TIME);
                    }
                    else
                    {
                        sendReconnResponse (contextPtr->socketFd, 
                                contextPtr->thePacket.messageId.Byte[0], 
                                contextPtr->thePacket.messageId.Byte[1],
                                RECONN_INVALID_MESSAGE, contextPtr->mode); 
                    }
                    break;
                }
                case LNB_10MHZ:
                {
                    if(RECONN_DEBUG_ON(LNB_DEBUG_RCV)) 
                    {
                        reconnDebugPrint("%s: Client %d Received LNB_10MHZ %d\n", __FUNCTION__, 
                            contextPtr->index, contextPtr->thePacket.dataPayload[0] );
                    }

                    if ((contextPtr->thePacket.dataPayload[0] == POWER_ON) || 
                            (contextPtr->thePacket.dataPayload[0] == POWER_OFF))
                    {
                        if(reconnGpioAction(LNB_10MHZ_GPIO, (contextPtr->thePacket.dataPayload[0] == POWER_ON) ? ENABLE : DISABLE, NULL) == RECONN_FAILURE)
                        {                           
                            reconnDebugPrint("%s: reconnGpioAction(GPIO_%d, %d, NULL) failed. \n", __FUNCTION__, LNB_10MHZ_GPIO, contextPtr->thePacket.dataPayload[0]);

                            sendReconnResponse(contextPtr->socketFd, 
                                    contextPtr->thePacket.messageId.Byte[0], 
                                    contextPtr->thePacket.messageId.Byte[1],
                                    RECONN_FAILURE, contextPtr->mode);
                        }
                        else
                        {
                            sendReconnResponse(contextPtr->socketFd, 
                                    contextPtr->thePacket.messageId.Byte[0], 
                                    contextPtr->thePacket.messageId.Byte[1],
                                    RECONN_SUCCESS, contextPtr->mode);
                        }
                        resetPowerStandbyCounter(RESET_SYSTEM_SHUTDOWN_TIME);
                    }
                    else
                    {
                        sendReconnResponse (contextPtr->socketFd, 
                                contextPtr->thePacket.messageId.Byte[0], 
                                contextPtr->thePacket.messageId.Byte[1],
                                RECONN_INVALID_MESSAGE, contextPtr->mode); 
                    }
                    break;
                }
                case LNB_18VDC_BIAS:
                {
                    if(RECONN_DEBUG_ON(LNB_DEBUG_RCV)) 
                    {
                        reconnDebugPrint("%s: Client %d Received LNB_18VDC_BIAS payload = 0x%x\n", 
                                __FUNCTION__, contextPtr->index , contextPtr->thePacket.dataPayload[0]);
                    }
                    if ((contextPtr->thePacket.dataPayload[0] == POWER_ON) || 
                            (contextPtr->thePacket.dataPayload[0] == POWER_OFF))
                    {
                        if(reconnGpioAction(LNB_18VDC_GPIO, (contextPtr->thePacket.dataPayload[0] == POWER_ON) ? ENABLE : DISABLE, NULL) == RECONN_FAILURE)
                        {
                            reconnDebugPrint("%s: reconnGpioAction(GPIO_%d, %d, NULL) failed. \n", __FUNCTION__, LNB_18VDC_GPIO, contextPtr->thePacket.dataPayload[0]);

                            sendReconnResponse(contextPtr->socketFd,
                                    contextPtr->thePacket.messageId.Byte[0],
                                    contextPtr->thePacket.messageId.Byte[1],
                                    RECONN_FAILURE, contextPtr->mode);
                        }
                        else
                        {
                            sendReconnResponse(contextPtr->socketFd, 
                                    contextPtr->thePacket.messageId.Byte[0],
                                    contextPtr->thePacket.messageId.Byte[1], 
                                    RECONN_SUCCESS, contextPtr->mode);
                        }
                    }
                    else
                    {
                        reconnDebugPrint("%s: Invalid payload byte 0x%x\n", __FUNCTION__, contextPtr->thePacket.dataPayload[0]);
                        sendReconnResponse (contextPtr->socketFd, 
                                contextPtr->thePacket.messageId.Byte[0], 
                                contextPtr->thePacket.messageId.Byte[1], 
                                RECONN_INVALID_MESSAGE, contextPtr->mode); 
                    }
                    resetPowerStandbyCounter(RESET_SYSTEM_SHUTDOWN_TIME);
                    break;
                }
                case LNB_GPIO_STATES:
                {
                    short theValue;
                    int theDataLength = 0;

                    if(RECONN_DEBUG_ON(LNB_DEBUG_RCV)) 
                    {
                        reconnDebugPrint("%s: Client %d Received LNB_GPIO_STATES\n", __FUNCTION__, 
                                contextPtr->index);
                    }
                    // get SA_10MHZ
                    if(reconnGpioAction(SA_10MHZ_GPIO, READ, &theValue) == RECONN_SUCCESS)
                    {                           
                        contextPtr->theResponsePktPtr->dataPayload[0] = theValue;
                        theDataLength++;

                        // get LNB_10MHZ
                        if(reconnGpioAction(LNB_10MHZ_GPIO, READ, &theValue) == RECONN_SUCCESS)
                        {                           
                            contextPtr->theResponsePktPtr->dataPayload[1] = theValue;
                            theDataLength++;

                            // get 18Vdc_BIAS
                            if(reconnGpioAction(LNB_18VDC_GPIO, READ, &theValue) == RECONN_SUCCESS)
                            {                           
                                contextPtr->theResponsePktPtr->dataPayload[2] = theValue;
                                theDataLength++;

                                ADD_RSPID_TO_PACKET(GENERIC_RESPONSE, contextPtr->theResponsePktPtr);
                                ADD_MSGID_TO_PACKET(LNB_GPIO_STATES, contextPtr->theResponsePktPtr);
                                ADD_DATA_LENGTH_TO_PACKET(theDataLength, contextPtr->theResponsePktPtr);
                                if(contextPtr->mode == INSERTEDMASTERMODE)
                                {
                                    // Send response out the 30 pin USB 
                                    libiphoned_tx((unsigned char *)contextPtr->theResponsePktPtr, RECONN_RSPPACKET_HEADER_SIZE + theDataLength);
                                }
                                else
                                {
                                    sendSocket(contextPtr->socketFd, (unsigned char *)contextPtr->theResponsePktPtr, RECONN_RSPPACKET_HEADER_SIZE + theDataLength, 0);
                                }
                                resetPowerStandbyCounter(RESET_SYSTEM_SHUTDOWN_TIME);
                                break;
                            }
                        }
                    }
                    sendReconnResponse (contextPtr->socketFd,
                            contextPtr->thePacket.messageId.Byte[0], 
                            contextPtr->thePacket.messageId.Byte[1],
                            RECONN_INVALID_MESSAGE, contextPtr->mode);

                    resetPowerStandbyCounter(RESET_SYSTEM_SHUTDOWN_TIME);
                    break;
                }
                case SW_UPGRADE_REQ:
                {
                    reconnDebugPrint("%s: Client %d Received SW_UPGRADE_REQ\n", __FUNCTION__, contextPtr->index);

                    if((contextPtr->retCode = extractBundle()) == RECONN_SUCCESS)
                    {
                        /*
                         * Now see if the upgrade bundle extraction included 
                         * executable shell script. If it did, then the script
                         * has to be run BEFORE we run the new reconn-service
                         * binary. The shell script is used to upgrade other file
                         * other than the reconn-service binary.
                         */

                        wifiCleanUp();
                        if(stat(UPGRADE_SCRIPT_NAME, &(contextPtr->fileStat)) == 0)
                        {
                            reconnDebugPrint("%s: Client %d Executing %s \n", __FUNCTION__, contextPtr->index, UPGRADE_SCRIPT_NAME);
                            system("chmod ugo+x " UPGRADE_SCRIPT_NAME);
                            contextPtr->retCode = system(UPGRADE_SCRIPT_NAME);
                            contextPtr->retCode = WEXITSTATUS(contextPtr->retCode);
                            reconnDebugPrint("%s: Client %d %s returned %d \n", __FUNCTION__, contextPtr->index, UPGRADE_SCRIPT_NAME,  contextPtr->retCode);
                            unlink(UPGRADE_SCRIPT_NAME);
                        }
                        else
                        {
                            reconnDebugPrint("%s: Client %d %s Not found\n", __FUNCTION__, contextPtr->index, UPGRADE_SCRIPT_NAME);
                        }
                        if((contextPtr->retCode == RECONN_SUCCESS) ||
                                (contextPtr->retCode == RECONN_NEEDS_REBOOT))
                        {
                            swUpgradeInProgress = YES;
                            disconnectAllClients(SLAVEMODE);
                            disconnectAllClients(REMOTEMODE);
                            /*
                             * Give the clients a chance to disconnect
                             */
                            sleep(2);
                            remoteMonitorActivate(NO, contextPtr);
                            reconnDebugPrint("%s: ********    extractBundle succeeded. Reseting the system.\n", __FUNCTION__);
                            // The bundle has been extracted

                            /*
                             * flash the power button and turn the battery status LED orange to indicate 
                             * software upgrade is in progress.
                             */
                            if(reconnGpioAction(POWER_LED_GPIO, DISABLE, NULL) != RECONN_SUCCESS)
                            {
                                reconnDebugPrint("%s: reconnGpioAction(POWER_LED_GPIO, DISABLE, NULL) failed.\n", __FUNCTION__);
                            }
                            if(reconnGpioAction(BATTERY_LED_RED_GPIO, ENABLE, NULL) != RECONN_SUCCESS)
                            {
                                reconnDebugPrint("%s: reconnGpioAction(BATTERY_LED_RED_GPIO, ENABLE, NULL) failed.\n", __FUNCTION__);
                            }
                            if(reconnGpioAction(BATTERY_LED_GREEN_GPIO, ENABLE, NULL) != RECONN_SUCCESS)
                            {
                                reconnDebugPrint("%s: reconnGpioAction(BATTERY_LED_GREEN_GPIO, ENABLE, NULL) failed.\n", __FUNCTION__);
                            }
                            sendReconnResponse(contextPtr->socketFd,
                                    contextPtr->thePacket.messageId.Byte[0],
                                    contextPtr->thePacket.messageId.Byte[1],
                                    contextPtr->retCode, contextPtr->mode);
                            libiphoned_stop();
                            sleep(2);
                            if(contextPtr->retCode == RECONN_NEEDS_REBOOT)
                            {
#ifndef __SIMULATION__
                                system("reboot");
#else
                                reconnDebugPrint("%s: system is rebooting\n", __FUNCTION__);
#endif
                            }
#ifndef __SIMULATION__
                            raise(SIGTERM);
#else
                            reconnCleanUp();
#endif
                        }
                        sendReconnResponse(contextPtr->socketFd, 
                                contextPtr->thePacket.messageId.Byte[0],
                                contextPtr->thePacket.messageId.Byte[1], 
                                contextPtr->retCode, contextPtr->mode);
                    }
                    break;
                }
                case FW_UPGRADE_REQ:
                {
                    reconnDebugPrint("%s: Client %d Received FW_UPGRADE_REQ\n", __FUNCTION__, contextPtr->index);
                    sendReconnResponse(contextPtr->socketFd,
                            contextPtr->thePacket.messageId.Byte[0],
                            contextPtr->thePacket.messageId.Byte[1],
                            SpectrumAnalyzerUpgrade(), contextPtr->mode);
                    break;
                }
                case SLAVES_SEND_MSG:
                {
                    if(RECONN_DEBUG_ON(SLAVE_DEBUG_RCV)) 
                    {
                        reconnDebugPrint("%s: Client %d Received SLAVES_SEND_MSG\n", __FUNCTION__, contextPtr->index);
                    }

                    if(reconnClientsRegistered(ALL) > 1)
                    {
                        reconnEqptAddMsgToQ((char *)&(contextPtr->thePacket), contextPtr->pktLength + RECONN_PACKET_HEADER_SIZE);
                    }
                    resetPowerStandbyCounter(RESET_SYSTEM_SHUTDOWN_TIME);
                    break;
                }
                case WEB_SERVICE_IP_AND_PORT:
                {
                    if(RECONN_DEBUG_ON(RM_DEBUG))
                    {
                        reconnDebugPrint("%s: Client %d Received WEB_SERVICE_IP_AND_PORT\n", __FUNCTION__, contextPtr->index);
                    }
                    /*
                     * Extract the hostname and port from payload.
                     */
                    strcpy(&(contextPtr->theWebServerHostName[0]), (char *)&(contextPtr->thePacket.dataPayload[0]));

                    if(RECONN_DEBUG_ON(RM_DEBUG))
                    {
                        reconnDebugPrint("%s: Client %d theWebServerHostName = %s \n", __FUNCTION__, contextPtr->index, contextPtr->theWebServerHostName);
                    }

                    contextPtr->portNum = atol(&(contextPtr->thePacket.dataPayload[strlen(contextPtr->theWebServerHostName)+1]));
                    if(RECONN_DEBUG_ON(RM_DEBUG))
                    {
                        reconnDebugPrint("%s: Client %d portNum = %u \n", __FUNCTION__, contextPtr->index, contextPtr->portNum);
                    }

                    /*
                     *  Check to see if the USB is up an running.
                     */
                    if ((contextPtr->tmpSocketFd = socket(AF_INET, SOCK_STREAM, 0)) <= 0)
                    {
                        reconnDebugPrint("%s: socket() failed %d (%s)\n", __FUNCTION__, errno, strerror(errno));
                        sendReconnResponse(contextPtr->socketFd,
                                contextPtr->thePacket.messageId.Byte[0],
                                contextPtr->thePacket.messageId.Byte[1],
                                RECONN_FAILURE, contextPtr->mode);
                    }
                    else
                    {

                        memset(&contextPtr->ifr, 0, sizeof(contextPtr->ifr));
#ifdef __SIMULATION__
                        strncpy(contextPtr->ifr.ifr_name, "eth0", IF_NAMESIZE);
#else
                        strncpy(contextPtr->ifr.ifr_name, "usb0", IF_NAMESIZE);
#endif
                        memset(&(contextPtr->theResponsePktPtr->dataPayload[0]), 0,
                                RECONN_RSP_PAYLOAD_SIZE);
                        if ((ioctl(contextPtr->tmpSocketFd, SIOCGIFFLAGS, &contextPtr->ifr) != -1) ||
                                (ioctl(contextPtr->tmpSocketFd, SIOCGIFADDR, &contextPtr->ifr) != -1))
                        {
                            /*
                             * Is usb0 up 
                             */
                            if(!(contextPtr->ifr.ifr_flags &= IFF_UP))
                            {
                                sendReconnResponse(contextPtr->socketFd,
                                        contextPtr->thePacket.messageId.Byte[0],
                                        contextPtr->thePacket.messageId.Byte[1],
                                        RECONN_LINK_DOWN, contextPtr->mode);
                            }
                            else
                            {
                                /*
                                 * Open socket to the remote server
                                 */
                                if(RECONN_DEBUG_ON(RM_DEBUG))
                                {
                                    reconnDebugPrint("%s: WEB_SERVICE_IP_AND_PORT Calling remoteMonitorActivate(YES)\n", __FUNCTION__);
                                }
                                sendReconnResponse(contextPtr->socketFd,
                                        contextPtr->thePacket.messageId.Byte[0],
                                        contextPtr->thePacket.messageId.Byte[1], 
                                        remoteMonitorActivate(YES, contextPtr), contextPtr->mode);
                            } 
                        }
                        else
                        {
                            reconnDebugPrint("%s: ioctl() failed %d (%s)\n", __FUNCTION__, errno,
                                    strerror(errno));

                            sendReconnResponse(contextPtr->socketFd,
                                    contextPtr->thePacket.messageId.Byte[0],
                                    contextPtr->thePacket.messageId.Byte[1],
                                    RECONN_FAILURE, contextPtr->mode);
                        }
                        close(contextPtr->tmpSocketFd);
                        contextPtr->tmpSocketFd = -1;
                    }
                    break;
                }
                case WEB_SERVICE_STATE:
                {
                    if(RECONN_DEBUG_ON(RM_DEBUG))
                    {
                        reconnDebugPrint("n%s: Client %d Received WEB_SERVICE_STATE\n", __FUNCTION__, contextPtr->index);
                    }
                    sendReconnResponse (contextPtr->socketFd, ((WEB_SERVICE_STATE  & 0xff00) >> 8), 
                            (WEB_SERVICE_STATE  & 0x00ff), getRemoteMonitorState(), contextPtr->mode); 
                    break;
                }
                default:
                {
                    reconnDebugPrint("%s:  Client %d Received Invalid cmdid 0x%x\n", __FUNCTION__, contextPtr->index, contextPtr->cmdid);
                    sendReconnResponse (contextPtr->socketFd, 
                            contextPtr->thePacket.messageId.Byte[0], 
                            contextPtr->thePacket.messageId.Byte[1],
                            RECONN_INVALID_MESSAGE, contextPtr->mode); 
                    resetPowerStandbyCounter(RESET_SYSTEM_SHUTDOWN_TIME);
                    break;
                }
            } // switch (cmdid)
        }
        /*
         * Give other things a chance to run.
         */
        usleep(CLIENTSLEEPTIME);
    }// end while (contextPtr->connectionOpen == TRUE)
    reconnDebugPrint("%s: Client %d exiting contextPtr->mode = %s\n", __FUNCTION__, contextPtr->index, (contextPtr->mode == MASTERMODE) ? "Master": (contextPtr->mode == SLAVEMODE) ? "Slave" : (contextPtr->mode ==  REMOTEMODE) ? "Remote" : "Inserted Master");
    free((void *)contextPtr->thisContext);
    return &contextPtr->retStatus;
}


//******************************************************************************
//******************************************************************************
// FUNCTION:    formatReconnPacket
//
// DESCRIPTION: This function formats a message destined for the iphone application.
//              This function  should be be used for general response messages.
//
//******************************************************************************
ReconnErrCodes formatReconnPacket(int theMsgId, char *theData, int theDataLength, ReconnPacket *thePacket)
{
    int i;
    ReconnErrCodes retCode = RECONN_SUCCESS;

    if(theDataLength < MAX_COMMAND_INPUT)
    {
        ADD_MSGID_TO_PACKET(theMsgId, thePacket);
        ADD_DATA_LENGTH_TO_PACKET(theDataLength, thePacket);
        for(i = 0; i < theDataLength; i++)
        {
            thePacket->dataPayload[i] = *(theData)++;
        }
    }
    else
    {
        reconnDebugPrint("%s: data length (%d) is larger than %d\n" , __FUNCTION__, theDataLength, MAX_COMMAND_INPUT);
        retCode = RECONN_FAILURE;
    }
    return retCode;
}


//******************************************************************************
//******************************************************************************
// FUNCTION:    insertedMasterRead
//
// DESCRIPTION: This function is registered with the iPhone library via 
//              libiphoned_register_rx_callback().  The iPhone library uses 
//              insertedMasterRead() to send data from the iPhone to the embedded
//              software. insertedMasterRead() then sends the data to inserted master's
//              reconnClientTask().  This is a one way data direction from the iPhone
//              to the embedded software. Data from the embedded software to the iPhone
//              is handled via libiphoned_tx().
//
//******************************************************************************
void insertedMasterRead(unsigned char *dataPtr, int dataLength)
{
    extern int fromLibToClientfd;

    if(insertedMasterSocketFd != -1)
    {
        sendSocket(fromLibToClientfd, dataPtr, dataLength, 0);
    }
}

//******************************************************************************
//******************************************************************************
// FUNCTION:    insertedMasterTransmitTask
//
// DESCRIPTION: This task is created by main(). This task waits
//              for reconnMasterIphone() to connect on internal port RECONN_LIBTOCLIENT_PORT. 
//              Once connected, this task simply accepts the connection and sets
//              insertedMasterSocketFd which is used by insertedMasterRead() to move
//              data from the inserted master iphone, via iphoned, to the client's 
//              reconnClientTask().
//
//******************************************************************************
void *insertedMasterTransmitTask()
{
    int inPort;
    int optval = 1;
    static int state = 1;
    struct sockaddr_in server_addr;
    if((MasterTransmitListenFd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        reconnDebugPrint("%s: Failed to initialize the incoming socket %d(%s)\n", __FUNCTION__, errno, strerror(errno));
        return (0);
    }

    bzero((unsigned char *) &server_addr, sizeof(server_addr));
    /* bind the socket */
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    inPort = RECONN_LIBTOCLIENT_PORT;
    server_addr.sin_port = htons(inPort);
    if(setsockopt(MasterTransmitListenFd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
    {
        reconnDebugPrint("%s: setsockopt SO_REUSEADDR failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
    }
    if (bind(MasterTransmitListenFd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)
    {
        reconnDebugPrint("%s: Failed to bind the socket %d(%s)\n", __FUNCTION__, errno, strerror(errno));
        return (0);
    }
    while(1)
    {
        if(listen(MasterTransmitListenFd, 1) == 0)
        {
            if((insertedMasterSocketFd = accept(MasterTransmitListenFd, (struct sockaddr *) NULL, NULL)) < 0)
            {
                reconnDebugPrint("%s: Failed to open a new Client socket %d %s.\n", __FUNCTION__, errno , strerror(errno));
                break;
            }
        }
    }
    return(&state);
}

//******************************************************************************
//******************************************************************************
// FUNCTION:    getDeviceId
//
// DESCRIPTION: This function reads the reconn's serial number from a file. If 
//              successful, a response packet is formatted.
//
// Parameters: theContextPtr -  a pointer to a context into which this function
//                              places the serial number and response packet.
// RETURNS:
//          RECONN_SUCCESS - the serial number was read from RECONN_SERIALNUM_FILE_NAME
//          RECONN_FAILURE - Something bad happened
//
//******************************************************************************
ReconnErrCodes getDeviceId(CLIENTCONTEXT *theContextPtr)
{
    ReconnErrCodes retCode = RECONN_SUCCESS;

    if(theContextPtr == NULL)
    {
        reconnDebugPrint("%s: theContextPtr is NULL\n", __FUNCTION__);
        retCode = RECONN_FAILURE;
    }
    else if((theContextPtr->tmpFd = fopen(RECONN_SERIALNUM_FILE_NAME, "r")))
    {
        memset(&(theContextPtr->theSerialNumberString), 0, RECONN_SERIALNUM_MAX_LEN);
        theContextPtr->serialBytesRead = fread(&(theContextPtr->theSerialNumberString), 1, (RECONN_SERIALNUM_MAX_LEN - 1), theContextPtr->tmpFd);
        if((theContextPtr->serialBytesRead != 0) && (feof(theContextPtr->tmpFd)))
        {
            theContextPtr->serialBytesRead = strlen((char *)&(theContextPtr->theSerialNumberString));

            reconnDebugPrint("%s: serialBytesRead = %d\n", __FUNCTION__, theContextPtr->serialBytesRead);

            theContextPtr->theSerialNumberString[theContextPtr->serialBytesRead + 1] = 0;
            theContextPtr->serialBytesRead++;
            /*
             * Format a response packet.
             */
            ADD_RSPID_TO_PACKET(GENERIC_RESPONSE,
                    theContextPtr->theResponsePktPtr);
            ADD_MSGID_TO_PACKET(RECONN_ID_NOTIF,
                    theContextPtr->theResponsePktPtr);
            ADD_DATA_LENGTH_TO_PACKET(theContextPtr->serialBytesRead,
                    theContextPtr->theResponsePktPtr);
            strncpy(&(theContextPtr->theResponsePktPtr->dataPayload[0]),
                    theContextPtr->theSerialNumberString, theContextPtr->serialBytesRead);
            reconnDebugPrint("%s: returning ID %s\n", __FUNCTION__, theContextPtr->theSerialNumberString);
        }
        else
        {
            retCode = RECONN_FAILURE;
            reconnDebugPrint("%s: fread () failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
        }
        if(theContextPtr->tmpFd)
        {
            if(fclose(theContextPtr->tmpFd) != 0)
            {
                reconnDebugPrint("%s: fclose() failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
            }
            theContextPtr->tmpFd = (FILE *)-1;
        }
    }
    else
    {
        reconnDebugPrint("%s: fopen () failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
        retCode = RECONN_FAILURE;
    }
    return(retCode);
}
