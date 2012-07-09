//******************************************************************************
//****************************************************************************** //
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
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include <fcntl.h>
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

// only used by a master client process
mqd_t masterClientMsgQid = -1;
static char  masterClientMsgBuf[4];
static struct mq_attr masterClientMsgQAttr;
//
extern int libiphoned_tx(unsigned char *, unsigned int);

int masterClientSocketFd = -1;
int insertedMasterSocketFd = -1; 

static void clientCleanUp()
{
    reconnDebugPrint("%s: **** Called\n", __FUNCTION__);
}

//******************************************************************************
//****************************************************************************** //
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
    static int retStatus = 1;
    int mySocketFd;
    int length = 0;
    int p_length = 0;
    int connection_open = TRUE;
    ReconnPacket thePacket; /* be sure to check overflow case     */
    ReconnResponsePacket theResponsePkt;
    ReconnResponsePacket *theResponsePktPtr = &theResponsePkt;
    unsigned short cmdid = 0;
    unsigned short responseId = 0;
    ReconnErrCodes retCode = RECONN_SUCCESS;
    short myIndex;
    int theEqptFd, numBytes;
    int responseNeeded;
    ReconnModeAndEqptDescriptors *pModeAndEqptDescriptors;
    ReconnMasterClientMode myMode;
    char newSSID[WIFI_SSID_MAX_LENGTH + 1];
    char newPasswd[WIFI_PASSWD_MAX_LENGTH + 1];
#ifdef DEBUG_CLIENT
    int debugIndex;
    char *debugPtr;
#endif

    //atexit(clientCleanUp);
    mySocketFd = gNewSocketFd;

    pModeAndEqptDescriptors = args;
    myIndex = pModeAndEqptDescriptors->clientIndex;
    myMode = pModeAndEqptDescriptors->clientMode;

    reconnDebugPrint("%s: Sending %s to client %d\n", __FUNCTION__, RECONNBEGIN, myIndex);


    if(myMode == INSERTEDMASTERMODE)
    {
        /*
         * Give the MFI a chance to settle down before
         * sending the handshake between this app and the iphone app.
         */
        sleep(1);
        printf("%s: sending %s \n", __FUNCTION__, RECONNBEGIN);
        // Send response out the 30 pin USB 
        libiphoned_tx((unsigned char *)RECONNBEGIN, strlen(RECONNBEGIN));

        //
        // An iPhone that is inserted into the front panel 
    }
    else
    {
        sendSocket(mySocketFd, (unsigned char *)RECONNBEGIN, strlen(RECONNBEGIN), 0);
        reconnDebugPrint("%s: registering client with eqptTask\n", __FUNCTION__);
        if(reconnRegisterClientApp(myIndex, mySocketFd) != RECONN_SUCCESS)
        {   
            reconnDebugPrint("%s: reconnRegisterClientApp(%d, %d, %d) failed\n", __FUNCTION__, myIndex ,getpid(), mySocketFd);
            if(mySocketFd >= 0)
            {
                close(mySocketFd);
                return &retStatus;
            }
        }
    }

    reconnDebugPrint("%s: reconnClientTask myIndex %d\n", __FUNCTION__, myIndex);
    reconnDebugPrint("%s:                  myMode == %s\n", __FUNCTION__, (myMode == MASTERMODE) ? "Master": (myMode == INITMODE) ? "Init" : "Inserted Master");
    reconnDebugPrint("%s:                  mySocketFd %d\n", __FUNCTION__, mySocketFd);
    reconnDebugPrint("\n%s:                  gpsFd %d\n", __FUNCTION__, pModeAndEqptDescriptors->gpsFd);
    reconnDebugPrint("%s:                  powerMeterFd %d\n", __FUNCTION__, pModeAndEqptDescriptors->powerMeterFd);
    reconnDebugPrint("%s:                  lnbFd %d\n", __FUNCTION__, pModeAndEqptDescriptors->lnbFd);
    reconnDebugPrint("%s:                  dmmFd %d\n", __FUNCTION__, pModeAndEqptDescriptors->dmmFd);
    reconnDebugPrint("%s:                  analyzerFd %d\n", __FUNCTION__, pModeAndEqptDescriptors->analyzerFd);


    while (connection_open == TRUE) 
    {
        responseNeeded = FALSE;
        bzero((unsigned char *) &thePacket, sizeof(ReconnPacket));
        /* receive the command from the client */

#if 0
        reconnDebugPrint("%s: client with index %d waiting for command\n", __FUNCTION__, myIndex);
#endif
        if((retCode = receive_packet_data(mySocketFd, (unsigned char *)&thePacket, &length)) == RECONN_CLIENT_SOCKET_CLOSED)
        {
            reconnDebugPrint("%s: Socket closed by Client %d myMode = %d\n", __FUNCTION__, myIndex, myMode);
            connection_open = FALSE;
            reconnReturnClientIndex(myIndex);
            reconnDeRegisterClientApp(myIndex);
            if(myMode == MASTERMODE)
            {
                if(mq_close(masterClientMsgQid) == -1)
                {
                    reconnDebugPrint("%s: mq_close() failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
                }
                masterClientMsgQid = -1;
                masterClientSocketFd = -1;
            }
            connection_open = FALSE;
        }
        else if(retCode == RECONN_ERROR_UNKNOWN)
        {
            if(((errno == EAGAIN) || (errno == EWOULDBLOCK)) && 
                    (myMode != CLIENTMODE))
            {
                if(mq_getattr(masterClientMsgQid, &masterClientMsgQAttr) == 0)
                {
                    if(masterClientMsgQAttr.mq_curmsgs)
                    {
                        if((numBytes = mq_receive(masterClientMsgQid, (char *)&masterClientMsgBuf, masterClientMsgQAttr.mq_msgsize, NULL)) == -1)
                        {
                            if(errno != EAGAIN)
                            {
                                reconnDebugPrint("%s: mq_receive failed %d (%s)\n", __FUNCTION__, errno, strerror(errno));
                            }
                            continue;
                        }
                        else
                        {
                            reconnDebugPrint("%s: message on mq_receive \n", __FUNCTION__);
                            if(masterClientMsgBuf[0] == MASTER_INSERTED)
                            {
                                int result;
                                // send message to the iPhone client telling it 
                                // that mastership has been removed because an 
                                // iPhone has been inserted into the toolkit's 
                                // front panel.
                                reconnDebugPrint("%s: received MASTER_INSERTED \n", __FUNCTION__);
                                memset(theResponsePktPtr, 0, sizeof(theResponsePkt));

                                sendReconnResponse (mySocketFd, 
                                        ((MASTER_MODE_REMOVED_NOTIFICATION & 0xff00) >> 8),
                                        thePacket.messageId.Byte[1], RECONN_ERROR_UNKNOWN, myMode);

                                myMode = CLIENTMODE;
                                fcntl(mySocketFd, F_GETFL, NULL);
                                result &= (~O_NONBLOCK);
                                fcntl(mySocketFd, F_SETFL, result);

                                //send message to insertedMasterClient process
                                continue;
                            }
                            else if(masterClientMsgBuf[0] == MASTER_EXTRACTED)
                            {
                                reconnDebugPrint("%s: received MASTER_EXTRACTED \n", __FUNCTION__);
                                reconnReturnClientIndex(myIndex);
                                reconnDeRegisterClientApp(myIndex);
                                masterClientSocketFd = -1;
                                close(insertedMasterSocketFd);
                                if(mq_close(masterClientMsgQid) == -1)
                                {
                                    reconnDebugPrint("%s: mq_close() failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
                                }
                                masterClientMsgQid = -1;
                                connection_open = FALSE;
                            }
                        }
                    }
                }
            }
            else
            {
                reconnDebugPrint("%s: Error reading from socket\n", __FUNCTION__);
                reconnDeRegisterClientApp(myIndex);
                reconnReturnClientIndex(myIndex);
                if(myMode != CLIENTMODE)
                {
                    masterClientSocketFd = -1;
                }
                connection_open = FALSE;
            }
        }
        else
        {
#ifdef DEBUG_CLIENT

            reconnDebugPrint("%s %d: Packet received from client %d with length %d\n", __FUNCTION__, __LINE__, myIndex, length);
            debugPtr = (char *)&thePacket;
            for(debugIndex = 0; debugIndex < length + 4; debugIndex++)
            {
                reconnDebugPrint("0x%x ", debugPtr[debugIndex]);
            }
            reconnDebugPrint("\n");
#endif
            /* everyone needs to know the packet length */
            GET_DATA_LENGTH_FROM_PACKET(p_length, thePacket);
            //reconnDebugPrint("%s %d: p_length = %d\n", __FUNCTION__, __LINE__, p_length);

            GET_MSGID_FROM_PACKET(cmdid, thePacket);
            //reconnDebugPrint("%s %d: cmdid = 0x%x\n", __FUNCTION__, __LINE__, cmdid);
            if((myMode == CLIENTMODE) && 
                    ((cmdid != KEEPALIVE_MESSAGE) &&
                     (cmdid != CLIENT_RESIGN_REQ) &&
                     (cmdid != CLIENT_ACCESS_REQ) && 
                     (cmdid != MASTER_MODE_REQ)))
            {
                // a command came in from the client that we do not like so
                // do nothing, and the client will deal with the lack of
                // response.
            }
            else
            {
                switch (cmdid) 
                {
                    case KEEPALIVE_MESSAGE: 
                    {
                        sendReconnResponse(mySocketFd, thePacket.messageId.Byte[0], thePacket.messageId.Byte[1], RECONN_SUCCESS, myMode);
                        resetPowerStandbyCounter(RESET_SYSTEM_SHUTDOWN_TIME);
                        break;
                    }
                    case CLIENT_RESIGN_REQ:
                    {
                        reconnDebugPrint("%s: Received CLIENT_RESIGN_REQ\n", __FUNCTION__);
                        if(myMode == INSERTEDMASTERMODE)
                        {
                            sendReconnResponse(mySocketFd, thePacket.messageId.Byte[0], 
                                    thePacket.messageId.Byte[1], RECONN_FAILURE, myMode);
                        }
                        else
                        {
                            /* The client has requested to be disconnected */
                            sendReconnResponse(mySocketFd, thePacket.messageId.Byte[0],
                                    thePacket.messageId.Byte[1], RECONN_SUCCESS, myMode);
                            reconnDeRegisterClientApp(myIndex);
                            reconnReturnClientIndex(myIndex);
                            if(myMode == MASTERMODE)
                            {
                                if(mq_close(masterClientMsgQid) == -1)
                                {
                                    reconnDebugPrint("%s: mq_close() failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
                                }
                                masterClientMsgQid = -1;
                                masterClientSocketFd = -1;
                            }
                            connection_open = FALSE;
                            resetPowerStandbyCounter(RESET_SYSTEM_SHUTDOWN_TIME);
                        }
                        break;
                    }
                    case CLIENT_ACCESS_REQ:
                    {
                        reconnDebugPrint("%s: Received CLIENT_ACCESS_REQ\n", __FUNCTION__);
                        sendReconnResponse(mySocketFd,
                                thePacket.messageId.Byte[0], thePacket.messageId.Byte[1], RECONN_SUCCESS, myMode);
                        resetPowerStandbyCounter(RESET_SYSTEM_SHUTDOWN_TIME);
                        break;
                    }
                    case MASTER_MODE_REQ:
                    {
                        reconnDebugPrint("%s: Received MASTER_MODE_REQ\n", __FUNCTION__);
                        if (masterClientSocketFd == mySocketFd)
                        {
                            // This process is already the master client
                            sendReconnResponse(mySocketFd, 
                                    thePacket.messageId.Byte[0], 
                                    thePacket.messageId.Byte[1], RECONN_SUCCESS, myMode);
                            reconnDebugPrint("%s %d: Client already is Master. Sending Success \n", __FUNCTION__, __LINE__);
                        }
                        else if (masterClientSocketFd == -1)
                        {
                            //
                            // Open the queue which is used to communicate between the iphone insertion
                            // tasks and the client application tasks.
                            //
                            // When an iphone is inserted into the toolkit's front panel a 
                            // message will be sent to the masterClientMsgQid. The queue is then
                            // read to determine what to do. 
                            //
                            mq_unlink(INSERTED_MASTER_MSG_Q_NAME);
                            masterClientMsgQAttr.mq_flags    = 0;
                            masterClientMsgQAttr.mq_maxmsg   = 200;
                            masterClientMsgQAttr.mq_msgsize  = 10;

                            if((masterClientMsgQid = 
                                        mq_open(INSERTED_MASTER_MSG_Q_NAME, 
                                            (O_RDWR | O_CREAT | 
                                             O_NONBLOCK), 0, NULL)) == (mqd_t) -1)
                            {
                                reconnDebugPrint("%s: mq_open() failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
                                reconnDeRegisterClientApp(myIndex);
                                connection_open = FALSE;
                                return(&retStatus);
                            }
                            fcntl(mySocketFd, F_SETFL, O_NONBLOCK);
                            myMode = (myMode == INITMODE) ? MASTERMODE : myMode;

                            // This process becomes the master client
                            masterClientSocketFd = mySocketFd;
                            sendReconnResponse(mySocketFd, thePacket.messageId.Byte[0],
                                    thePacket.messageId.Byte[1], RECONN_SUCCESS, myMode);

                            reconnDebugPrint("%s %d: Client %d is now the Master. Sending Success \n", __FUNCTION__, __LINE__, myIndex);
                        }
                        else
                        {
                            sendReconnResponse(mySocketFd,
                                    thePacket.messageId.Byte[0], 
                                    thePacket.messageId.Byte[1], 
                                    RECONN_INVALID_MESSAGE, myMode); 

                            reconnDebugPrint("%s %d: Sending Failure because there is already a master (%d)\n", __FUNCTION__, __LINE__, masterClientSocketFd);
                            myMode = CLIENTMODE;
                            break;
                        }
                        resetPowerStandbyCounter(RESET_SYSTEM_SHUTDOWN_TIME);
                        break;
                    }
                    case MASTER_MODE_RESIGN_REQ:
                    {
                        reconnDebugPrint("%s: Received MASTER_MODE_RESIGN_REQ\n", __FUNCTION__);
                        if (masterClientSocketFd == mySocketFd)
                        {
                            sendReconnResponse(mySocketFd,
                                    thePacket.messageId.Byte[0], thePacket.messageId.Byte[1], RECONN_SUCCESS, myMode);
                        }
                        else
                        {
                            //
                            // Ask the master, via its file descriptor to release
                            // mastership.
                            if(formatReconnPacket(MASTER_MODE_RESIGN_QUERY, (char *)0, 0, &thePacket) == RECONN_SUCCESS)
                            {
                                sendSocket(masterClientSocketFd, (unsigned char *)&thePacket, 4, 0);
                            }
                        }
                        resetPowerStandbyCounter(RESET_SYSTEM_SHUTDOWN_TIME);
                        break;
                    }
                    case RECONN_SW_VERSION_REQ:
                    {
                        unsigned int length;
                        char *theSwVersionString;

                        reconnDebugPrint("n%s: Received RECONN_SW_VERSION_NOTIF\n", __FUNCTION__);

                        memset(theResponsePktPtr, 0, sizeof(theResponsePkt));
                        ADD_RSPID_TO_PACKET(GENERIC_RESPONSE, theResponsePktPtr);
                        ADD_MSGID_TO_PACKET(RECONN_SW_VERSION_REQ, theResponsePktPtr);
                        theSwVersionString = getReconnSwVersion();
                        length = strlen(theSwVersionString);
                        ADD_DATA_LENGTH_TO_PACKET(length, theResponsePktPtr);
                        strcat(&(theResponsePktPtr->dataPayload[0]), theSwVersionString);
                        if(myMode == INSERTEDMASTERMODE)
                        {
                            // Send response out the 30 pin USB 
                            libiphoned_tx((unsigned char *)theResponsePktPtr, length + 6);
                        }
                        else
                        {
                            sendSocket(masterClientSocketFd, 
                                    (unsigned char *)theResponsePktPtr, length + 6, 0);
                        }
                        resetPowerStandbyCounter(RESET_SYSTEM_SHUTDOWN_TIME);
                        break;
                    }
                    case BATTERY_LEVEL_REQ:
                    {
                        extern uint8_t batteryPercentage;

                        reconnDebugPrint("%s: Received BATTERY_LEVEL_REQ\n", __FUNCTION__);

                        sendReconnResponse (mySocketFd, (BATTERY_LEVEL_RSP & 0xff00) >> 8,
                                (BATTERY_LEVEL_RSP & 0x00ff), batteryPercentage, myMode);

                        break;
                    }
                    case BATTERY_CHARGE_STATE_REQ:
                    {
                        extern char chargerAttached;

                        reconnDebugPrint("%s: Received BATTERY_CHARGE_STATE_REQ\n",
                                __FUNCTION__);

                        sendReconnResponse (mySocketFd, 
                                (BATTERY_CHARGE_STATE_RSP & 0xff00) >> 8,
                                (BATTERY_CHARGE_STATE_RSP & 0x00ff), chargerAttached,
                                myMode);

                        break;
                    }
                    case WIFI_STATUS_REQ:
                    case WIFI_SET_POWER_REQ:
                    {
#if 0 
                        if ((thePacket.dataPayload[0] == POWER_ON) || 
                                (thePacket.dataPayload[0] == POWER_OFF))
                        {
                            if( WiFiInit(pModeAndEqptDescriptors->WiFiFd) == RECONN_SUCCESS)
                            {
                                sendReconnResponse(mySocketFd, thePacket.messageId.Byte[0], 
                                        thePacket.messageId.Byte[1], RECONN_SUCCESS, myMode);
                                break;
                            }
                        }
#endif
                        sendReconnResponse (mySocketFd, thePacket.messageId.Byte[0],
                                thePacket.messageId.Byte[1], RECONN_INVALID_MESSAGE, myMode);
                        resetPowerStandbyCounter(RESET_SYSTEM_SHUTDOWN_TIME);
                        break;
                    }
                    case WIFI_CHANGE_PASSWORD_REQ:
                    {
                        reconnDebugPrint("%s: Received WIFI_CHANGE_PASSWORD_REQ\n", __FUNCTION__);
                        // passphrase must be between 8 and 63 characters
                        if((p_length < 8) || (p_length > WIFI_PASSWD_MAX_LENGTH))
                        {
                            sendReconnResponse (mySocketFd, thePacket.messageId.Byte[0], 
                                    thePacket.messageId.Byte[1], RECONN_INVALID_PARAMETER, myMode); 
                        }
                        else
                        {
                            memset(&newPasswd, 0, WIFI_PASSWD_MAX_LENGTH+1);
                            strncat(&newPasswd[0], &thePacket.dataPayload[0], p_length);
                            retCode = wifiUpdateConfFile(WIFI_PASSWD_TOKEN, &newPasswd[0]);
                            sendReconnResponse (mySocketFd, 
                                    thePacket.messageId.Byte[0], thePacket.messageId.Byte[1], retCode , myMode); 
                            if(retCode == RECONN_SUCCESS)
                            { 
                                system("/etc/init.d/hostapd reload");
                            }
                        }
                        resetPowerStandbyCounter(RESET_SYSTEM_SHUTDOWN_TIME);
                        break;
                    }
                    case WIFI_CHANGE_SSID_REQ:
                    {
                        reconnDebugPrint("%s: Received WIFI_CHANGE_SSID_REQ\n", __FUNCTION__);
                        if(p_length > WIFI_SSID_MAX_LENGTH)
                        {
                            sendReconnResponse (mySocketFd, thePacket.messageId.Byte[0],
                                    thePacket.messageId.Byte[1], RECONN_INVALID_PARAMETER, myMode); 
                        }
                        else
                        {
                            memset(&newSSID, 0, WIFI_SSID_MAX_LENGTH+1);
                            strncat(newSSID, &thePacket.dataPayload[0], p_length);
                            retCode = wifiUpdateConfFile(WIFI_SSID_TOKEN, &newSSID[0]);
                            sendReconnResponse (mySocketFd, 
                                    thePacket.messageId.Byte[0], thePacket.messageId.Byte[1], retCode, myMode); 
                            if(retCode == RECONN_SUCCESS)
                            {
                                system("/etc/init.d/hostapd reload");
                            }
                        }
                        resetPowerStandbyCounter(RESET_SYSTEM_SHUTDOWN_TIME);
                        break;
                    }
                    case SPECANA_POWER_SET_REQ:
                    {
#ifdef DEBUG_EQPT
                        reconnDebugPrint("%s: Received SPECANA_POWER_SET_REQ\n", __FUNCTION__);
#endif
                        if ((thePacket.dataPayload[0] == POWER_ON) || 
                                (thePacket.dataPayload[0] == POWER_OFF))
                        {
                            if(reconnGpioAction(POWER_18V_GPIO, (thePacket.dataPayload[0] == POWER_ON) ? ENABLE : DISABLE, NULL) == RECONN_FAILURE)
                            {                           
                                reconnDebugPrint("%s: reconnGpioAction(GPIO_141, ENABLE/DISABLE, NULL) failed. \n", __FUNCTION__);                          
                                sendReconnResponse (mySocketFd, 
                                        thePacket.messageId.Byte[0],
                                        thePacket.messageId.Byte[1], RECONN_INVALID_MESSAGE, myMode); 
                            }
                            else
                            {
                                sendReconnResponse (mySocketFd, 
                                        thePacket.messageId.Byte[0], 
                                        thePacket.messageId.Byte[1], RECONN_SUCCESS, myMode); 
                            }
                        }
                        resetPowerStandbyCounter(RESET_SPECTRUM_ANALYZER_STBY_COUNTER);
                        break;
                    }
                    case SPECANA_IDLE_CFG_REQ:
                    {
                        resetPowerStandbyCounter(RESET_SPECTRUM_ANALYZER_STBY_COUNTER);
                        break;
                    }
                    case SPECANA_PKT_SEND_REQ:
                    {
#ifdef DEBUG_SPECTRUM
                        reconnDebugPrint("%s: Received  SPECANA_PKT_SEND_REQ\n", __FUNCTION__);
#endif
                        if (masterClientSocketFd == mySocketFd) 
                        {
#ifdef DEBUG_SPECTRUM
                            reconnDebugPrint("%s: Calling SpectrumAnalyzerWrite p_length = %d \n", __FUNCTION__, p_length);
#endif
                            if(SpectrumAnalyzerWrite((unsigned char *)&(thePacket.dataPayload), p_length) == RECONN_SA_PORT_NOT_INITIALIZED)
                            {
                                if(SpectrumAnalyzerInit(&(pModeAndEqptDescriptors->analyzerFd)) == RECONN_FAILURE)
                                {
                                    sendReconnResponse (mySocketFd, 
                                            thePacket.messageId.Byte[0], 
                                            thePacket.messageId.Byte[1], RECONN_FAILURE, myMode); 
                                    break;
                                }
                                SpectrumAnalyzerWrite((unsigned char *)&(thePacket.dataPayload), p_length);
                            }
                            responseId = SPECANA_PKT_RCVD_NOTIFICATION;
                            theEqptFd = pModeAndEqptDescriptors->analyzerFd;
                            responseNeeded = TRUE;
                        }
                        resetPowerStandbyCounter(RESET_SPECTRUM_ANALYZER_STBY_COUNTER);
                        break;
                    }
                    case GPS_POWER_SET_REQ:
                    {
                        if ((thePacket.dataPayload[0] == POWER_ON) || 
                                (thePacket.dataPayload[0] == POWER_OFF))
                        {
                            if(gpsInit(&(pModeAndEqptDescriptors->gpsFd)) != RECONN_SUCCESS)
                            {
                                sendReconnResponse (mySocketFd, thePacket.messageId.Byte[0], 
                                        thePacket.messageId.Byte[1], RECONN_FAILURE, myMode); 
                            }
                            resetPowerStandbyCounter(RESET_GPS_STBY_COUNTER);
                        }
                        else
                        {
                            sendReconnResponse(mySocketFd, thePacket.messageId.Byte[0], 
                                    thePacket.messageId.Byte[1], RECONN_INVALID_MESSAGE, myMode);
                        }
                        break;
                    }
                    case GPS_IDLE_CFG_REQ:
                    {
                        resetPowerStandbyCounter(RESET_GPS_STBY_COUNTER);
                        break;
                    }
                    case GPS_PKT_SEND_REQ:
                    {
                        if (masterClientSocketFd == mySocketFd) 
                        {
                            if(gpsWrite((unsigned char *)&(thePacket.dataPayload), p_length) == RECONN_SUCCESS)
                            {
                                sendReconnResponse(mySocketFd, thePacket.messageId.Byte[0], 
                                        thePacket.messageId.Byte[1], RECONN_SUCCESS, myMode);
                            }
                        }
                        resetPowerStandbyCounter(RESET_GPS_STBY_COUNTER);
                        break;
                    }
                    case PMETER_POWER_SET_REQ:
                    case PMETER_IDLE_CFG_REQ:
                    {
                        sendReconnResponse(mySocketFd,
                                thePacket.messageId.Byte[0], thePacket.messageId.Byte[1], 
                                RECONN_INVALID_MESSAGE, myMode);
                        resetPowerStandbyCounter(RESET_POWER_METER_STBY_COUNTER);
                        break;
                    }
                    case PMETER_PKT_SEND_REQ:
                    {
#ifdef DEBUG_EQPT
                        reconnDebugPrint("%s: Received  PMETER_PKT_SEND_REQ:\n", __FUNCTION__);
#endif
                        if (masterClientSocketFd == mySocketFd) 
                        {
#ifdef DEBUG_EQPT
                            reconnDebugPrint("%s: Sending %c of length %d to power meter\n", __FUNCTION__, thePacket.dataPayload[0], p_length);
#endif 
                            // The power meter is a USB device that can be plugged in at any time.
                            // If the meter is plugged in at boot time, PeripherliInit() would have
                            // opened ttyUSB1.  If this call returns RECONN_PM_PORT_NOT_INITIALIZED
                            // then try opening the power meter. If the open is not successfull
                            // then there is no power meter plugged into a USB port.
                            if(powerMeterWrite((unsigned char *)&(thePacket.dataPayload[0]), p_length) == RECONN_PM_PORT_NOT_INITIALIZED)
                            {
                                sendReconnResponse(mySocketFd, thePacket.messageId.Byte[0], 
                                        thePacket.messageId.Byte[1], RECONN_INVALID_STATE, myMode);
                                break;
                            }
                            responseId = PMETER_PKT_RCVD_NOTIFICATION;
                            theEqptFd = pModeAndEqptDescriptors->powerMeterFd;
                            responseNeeded = TRUE;
                        }
                        resetPowerStandbyCounter(RESET_POWER_METER_STBY_COUNTER);
                        break;
                    }
                    case DMM_POWER_SET_REQ:
                    {
#ifdef DEBUG_EQPT
                        reconnDebugPrint("%s: DMM_POWER_SET_REQ \n", __FUNCTION__);
#endif
                        if ((thePacket.dataPayload[0] == POWER_ON) || 
                                (thePacket.dataPayload[0] == POWER_OFF))
                        {
                            if(dmmInit(&(pModeAndEqptDescriptors->analyzerFd)) == RECONN_SUCCESS)
                            {
                                resetPowerStandbyCounter(RESET_DMM_STBY_COUNTER);
                                sendReconnResponse(mySocketFd, thePacket.messageId.Byte[0], 
                                        thePacket.messageId.Byte[1], RECONN_SUCCESS, myMode);
                                break;
                            }
                        }
                        sendReconnResponse (mySocketFd, thePacket.messageId.Byte[0],
                                thePacket.messageId.Byte[1], RECONN_INVALID_MESSAGE, myMode);
                        break;
                    }
                    case DMM_IDLE_CFG_REQ:
                    {
                        // communicate with the device. So, send it a status command and get the response.

                        sendReconnResponse (mySocketFd, thePacket.messageId.Byte[0], 
                                thePacket.messageId.Byte[1], RECONN_SUCCESS, myMode); 
                        resetPowerStandbyCounter(RESET_DMM_STBY_COUNTER);
                        break;
                    }
                    case DMM_PKT_SEND_REQ:
                    {
#ifdef DEBUG_EQPT
                        reconnDebugPrint("%s: Received DMM_PKT_SEND_REQ\n", __FUNCTION__);
#endif
                        if (masterClientSocketFd == mySocketFd) 
                        {
#ifdef DEBUG_EQPT
                            reconnDebugPrint("%s: Sending %c to meter\n", __FUNCTION__, thePacket.dataPayload[0]);
#endif
                            dmmWrite((unsigned char *)&(thePacket.dataPayload), p_length);
                            sendReconnResponse(mySocketFd,
                                    thePacket.messageId.Byte[0], thePacket.messageId.Byte[1], RECONN_SUCCESS, myMode);
#ifdef DEBUG_EQPT
                            reconnDebugPrint("%s: Success Sent back to client\n", __FUNCTION__);
#endif
                            responseId = DMM_PKT_RCVD_NOTIFICATION;
                            theEqptFd = pModeAndEqptDescriptors->dmmFd;
                            responseNeeded = TRUE;
                        }
                        resetPowerStandbyCounter(RESET_DMM_STBY_COUNTER);
                        break;
                    }
                    case DMM_BUILTINTEST_REQ:
                    {
#ifdef DEBUG_EQPT
                        reconnDebugPrint("%s: Received DMM_BUILTINTEST_REQ\n", __FUNCTION__);
#endif
                        sendReconnResponse (mySocketFd, thePacket.messageId.Byte[0], 
                                thePacket.messageId.Byte[1], dmmDiags(), myMode);

                        resetPowerStandbyCounter(RESET_DMM_STBY_COUNTER);
                        break;
                    }
                    case LNB_POWER_SET_REQ:
                    {
#ifdef DEBUG_EQPT
                        reconnDebugPrint("%s: Received LNB_POWER_SET_REQ\n", __FUNCTION__);
#endif
                        if ((thePacket.dataPayload[0] == POWER_ON) || 
                                (thePacket.dataPayload[0] == POWER_OFF))
                        {
                            if(reconnGpioAction(LNB_ENABLE_GPIO, 
                                        (thePacket.dataPayload[0] == POWER_ON)
                                        ? ENABLE : DISABLE, NULL) == RECONN_FAILURE)
                            {                           
                                reconnDebugPrint("%s: reconnGpioAction(LNB_10MHZ_GPIO, ENABLE/DISABLE, NULL) failed. \n", __FUNCTION__);
                                sendReconnResponse(mySocketFd, 
                                        thePacket.messageId.Byte[0], 
                                        thePacket.messageId.Byte[1],
                                        RECONN_FAILURE, myMode);
                            }
                            else
                            {
                                sendReconnResponse(mySocketFd, thePacket.messageId.Byte[0], 
                                        thePacket.messageId.Byte[1], RECONN_SUCCESS, myMode);
                            }
                            resetPowerStandbyCounter(RESET_SYSTEM_SHUTDOWN_TIME);
                        }
                        else
                        {
                            sendReconnResponse (mySocketFd, thePacket.messageId.Byte[0], 
                                    thePacket.messageId.Byte[1], RECONN_INVALID_MESSAGE, myMode); 
                        }
                        break;
                    }
                    case LNB_SA_10MHZ:
                    {
#ifdef DEBUG_EQPT
                        reconnDebugPrint("%s: Received LNB_SA_10MHZ %d\n", __FUNCTION__, 
                                thePacket.dataPayload[0]); 
#endif

                        if ((thePacket.dataPayload[0] == POWER_ON) || 
                                (thePacket.dataPayload[0] == POWER_OFF))
                        {
                            if(reconnGpioAction(SA_10MHZ_GPIO, (thePacket.dataPayload[0] == POWER_ON) ? ENABLE : DISABLE, NULL) == RECONN_FAILURE)
                            {                           
                                reconnDebugPrint("%s: reconnGpioAction(GPIO_%d, %d, NULL) failed.  \n", __FUNCTION__, SA_10MHZ_GPIO, thePacket.dataPayload[0]);

                                sendReconnResponse(mySocketFd, thePacket.messageId.Byte[0], 
                                        thePacket.messageId.Byte[1], RECONN_FAILURE, myMode);
                            }
                            else
                            {
                                sendReconnResponse(mySocketFd, thePacket.messageId.Byte[0], 
                                        thePacket.messageId.Byte[1], RECONN_SUCCESS, myMode);
                            }
                            resetPowerStandbyCounter(RESET_SYSTEM_SHUTDOWN_TIME);
                        }
                        else
                        {
                            sendReconnResponse (mySocketFd, thePacket.messageId.Byte[0], 
                                    thePacket.messageId.Byte[1], RECONN_INVALID_MESSAGE, myMode); 
                        }
                        break;
                    }
                    case LNB_10MHZ:
                    {
#ifdef DEBUG_EQPT
                        reconnDebugPrint("%s: Received LNB_10MHZ %d\n", __FUNCTION__, 
                                thePacket.dataPayload[0] );
#endif
                        if ((thePacket.dataPayload[0] == POWER_ON) || 
                                (thePacket.dataPayload[0] == POWER_OFF))
                        {
                            if(reconnGpioAction(LNB_10MHZ_GPIO, (thePacket.dataPayload[0] == POWER_ON) ? ENABLE : DISABLE, NULL) == RECONN_FAILURE)
                            {                           
                                reconnDebugPrint("%s: reconnGpioAction(GPIO_%d, %d, NULL) failed. \n", __FUNCTION__, LNB_10MHZ_GPIO, thePacket.dataPayload[0]);

                                sendReconnResponse(mySocketFd, thePacket.messageId.Byte[0], 
                                        thePacket.messageId.Byte[1], RECONN_FAILURE, myMode);
                            }
                            else
                            {
                                sendReconnResponse(mySocketFd, thePacket.messageId.Byte[0], 
                                        thePacket.messageId.Byte[1], RECONN_SUCCESS, myMode);
                            }
                            resetPowerStandbyCounter(RESET_SYSTEM_SHUTDOWN_TIME);
                        }
                        else
                        {
                            sendReconnResponse (mySocketFd, thePacket.messageId.Byte[0], 
                                    thePacket.messageId.Byte[1], RECONN_INVALID_MESSAGE, myMode); 
                        }
                        break;
                    }
                    case LNB_18VDC_BIAS:
                    {
                        reconnDebugPrint("%s: Received LNB_18VDC_BIAS payload = 0x%x\n", __FUNCTION__, thePacket.dataPayload[0]);
                        if ((thePacket.dataPayload[0] == POWER_ON) || 
                                (thePacket.dataPayload[0] == POWER_OFF))
                        {
                            if(reconnGpioAction(LNB_18VDC_GPIO, (thePacket.dataPayload[0] == POWER_ON) ? ENABLE : DISABLE, NULL) == RECONN_FAILURE)
                            {
                                reconnDebugPrint("%s: reconnGpioAction(GPIO_%d, %d, NULL) failed. \n", __FUNCTION__, LNB_18VDC_GPIO, thePacket.dataPayload[0]);

                                sendReconnResponse(mySocketFd, thePacket.messageId.Byte[0],
                                        thePacket.messageId.Byte[1], RECONN_FAILURE, myMode);
                            }
                            else
                            {
                                sendReconnResponse(mySocketFd, thePacket.messageId.Byte[0],
                                        thePacket.messageId.Byte[1], RECONN_SUCCESS, myMode);
                            }
                        }
                        else
                        {
                            reconnDebugPrint("%s: Invalid payload byte 0x%x\n", __FUNCTION__, thePacket.dataPayload[0]);
                            sendReconnResponse (mySocketFd, thePacket.messageId.Byte[0], 
                                    thePacket.messageId.Byte[1], RECONN_INVALID_MESSAGE, myMode); 
                        }
                        resetPowerStandbyCounter(RESET_SYSTEM_SHUTDOWN_TIME);
                        break;
                    }
                    case LNB_GPIO_STATES:
                    {
                        short theValue;
                        int theDataLength = 0;

                        reconnDebugPrint("%s: Received LNB_GPIO_STATES\n", __FUNCTION__);
                        // get SA_10MHZ
                        if(reconnGpioAction(SA_10MHZ_GPIO, READ, &theValue) == RECONN_SUCCESS)
                        {                           
                            theResponsePktPtr->dataPayload[0] = theValue;
                            theDataLength++;

                            // get LNB_10MHZ
                            if(reconnGpioAction(LNB_10MHZ_GPIO, READ, &theValue) == RECONN_SUCCESS)
                            {                           
                                theResponsePktPtr->dataPayload[1] = theValue;
                                theDataLength++;

                                // get 18Vdc_BIAS
                                if(reconnGpioAction(LNB_18VDC_GPIO, READ, &theValue) == RECONN_SUCCESS)
                                {                           
                                    theResponsePktPtr->dataPayload[2] = theValue;
                                    theDataLength++;

                                    ADD_RSPID_TO_PACKET(GENERIC_RESPONSE, theResponsePktPtr);
                                    ADD_MSGID_TO_PACKET(LNB_GPIO_STATES, theResponsePktPtr);
                                    ADD_DATA_LENGTH_TO_PACKET(theDataLength, theResponsePktPtr);
                                    if(myMode == INSERTEDMASTERMODE)
                                    {
                                        // Send response out the 30 pin USB 
                                        libiphoned_tx((unsigned char *)theResponsePktPtr, RECONN_RSPPACKET_HEADER_SIZE + theDataLength);
                                    }
                                    else
                                    {
                                        sendSocket(mySocketFd, (unsigned char *)theResponsePktPtr, RECONN_RSPPACKET_HEADER_SIZE + theDataLength, 0);
                                    }
                                    resetPowerStandbyCounter(RESET_SYSTEM_SHUTDOWN_TIME);
                                    break;
                                }
                            }
                        }
                        sendReconnResponse (mySocketFd, thePacket.messageId.Byte[0], 
                                thePacket.messageId.Byte[1], RECONN_INVALID_MESSAGE, myMode);

                        resetPowerStandbyCounter(RESET_SYSTEM_SHUTDOWN_TIME);
                        break;
                    }
                    case SW_UPGRADE_REQ:
                    {
                        reconnDebugPrint("%s: Received SW_UPGRADE_REQ\n", __FUNCTION__);
                        if(reconnClientsRegistered() > 1)
                        {
                            retCode = RECONN_UPGRADE_CLIENT_CONNECTED;
                        }
                        else 
                        {
                            retCode = extractBundle();
                            sendReconnResponse(mySocketFd, thePacket.messageId.Byte[0],
                                thePacket.messageId.Byte[1], retCode, myMode);
                            if(retCode == RECONN_SUCCESS)
                            {
                                reconnDebugPrint("%s: ********    extractBundle succeeded. Reseting the system.\n", __FUNCTION__);
                                // The bundle has been extracted
                                system("killall iphoned");
                                raise(SIGTERM);
                            }
                        }
                        break;
                    }
                    case SLAVES_SEND_MSG:
                    {
                        reconnDebugPrint("%s: Received SLAVES_SEND_MSG\n", __FUNCTION__);
                        reconnEqptAddMsgToQ((char *)&(thePacket), p_length + 4);
                        resetPowerStandbyCounter(RESET_SYSTEM_SHUTDOWN_TIME);
                        break;
                    }

                    default:
                    {
                        reconnDebugPrint("%s: Invalid cmdid 0x%xreceived %u\n", __FUNCTION__, cmdid);
                        sendReconnResponse (mySocketFd, thePacket.messageId.Byte[0], 
                                thePacket.messageId.Byte[1], RECONN_INVALID_MESSAGE, myMode); 
                        resetPowerStandbyCounter(RESET_SYSTEM_SHUTDOWN_TIME);
                        break;
                    }
                } // switch (cmdid)
            }
            if(((myMode == MASTERMODE) || myMode == INSERTEDMASTERMODE)
                    && (responseNeeded))
            {
#ifdef DEBUG_EQPT
                reconnDebugPrint("%s: Calling reconnGetEqptResponse(%d)\n", __FUNCTION__, theEqptFd);
#endif
                reconnGetEqptResponse(theEqptFd, responseId, mySocketFd, myMode);
                // get data from devices 
            }
        }
    }
    reconnDebugPrint("%s: Client %d exiting myMode = %d\n", __FUNCTION__, myIndex, myMode);
    return &retStatus;
}

ReconnErrCodes receive_packet_data(int socket, unsigned char *buffer, int *length) 
{
    int count;
    int loop;
    int len = 0;
#ifdef COMM_DEBUG
    int i;
#endif

    /*
     * The packet length must be at least 4 bytes
     * which includes the message ID (2 bytes) and the data length (2 bytes)
     */
    if((len = recv(socket, buffer, 4, 0)) == 0)
    {
        return(RECONN_CLIENT_SOCKET_CLOSED);
    }
    else if(len == -1)
    {
        return (RECONN_ERROR_UNKNOWN);
    }
    else if (len != 4) 
    {
        reconnDebugPrint("%s: recv is less than 4 len = %d\n", __FUNCTION__, len);
        *length = -1;
        return (-1);
    }
    else
    {

#ifdef COMM_DEBUG
        reconnDebugPrint("%s %d: ", __FUNCTION__, __LINE__);
        for (i = 0; i < 4; ++i) 
        {
            reconnDebugPrint("[%x]", (unsigned int) buffer[i]);
        }
#endif

        count = buffer[2] << 8;
        count = count + buffer[3];
        for (loop = 0; loop < count; loop++) 
        {
            recv(socket, &buffer[loop + 4], 1, 0);
#ifdef COMM_DEBUG
            reconnDebugPrint("[%x]", (unsigned int) buffer[loop + 4]);
#endif
        }
#ifdef COMM_DEBUG
        reconnDebugPrint("\n");
#endif
    *length = count + 4;
    return (count + 4);
    }
}


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
//****************************************************************************** //
// FUNCTION:    insertedMasterRead
//
// DESCRIPTION: This function is registered with the iPhone library via 
//              libiphoned_register_rx_callback().  The iPhone library uses this
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

void *insertedMasterTransmitTask()
{
    int in_socket_fd, inPort;
    struct sockaddr_in server_addr;
    if((in_socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        reconnDebugPrint("%s: Server Failed to initialize the incoming socket %d(%s)\n", __FUNCTION__, errno, strerror(errno));
        return (0);
    }

    bzero((unsigned char *) &server_addr, sizeof(server_addr));
    /* bind the socket */
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    inPort = RECONN_INCOMING_PORT+2;
    server_addr.sin_port = htons(inPort);
    if (bind(in_socket_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)
    {
        reconnDebugPrint("%s: Server Failed to bind the socket %d(%s)\n", __FUNCTION__, errno, strerror(errno));
        exit (0);
    }
    while(1)
    {
        if(listen(in_socket_fd, 1) == 0)
        {
            if((insertedMasterSocketFd = accept(in_socket_fd, (struct sockaddr *) NULL, NULL)) < 0)
            {
                reconnDebugPrint("%s: Failed to open a new Client socket %d %s.\n", __FUNCTION__, errno , strerror(errno));
            }
        }
    }
}
