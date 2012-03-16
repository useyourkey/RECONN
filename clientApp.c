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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <time.h>

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

#define RECONNBEGIN "CFS RECONN 00.00 BEGIN"


void *reconnClientTask(void *args) 
{
    int mySocketFd;
    int length = 0;
#ifdef DEBUG_CLIENT
    int debugIndex;
    char *debugPtr;
#endif
    int p_length = 0;
    int connection_open = TRUE;
    ReconnPacket thePacket; /* be sure to check overflow case     */
    unsigned short cmdid = 0;
    ReconnErrCodes retCode = 0;
    short myIndex;
    int theEqptFd;
    int responseNeeded = FALSE;
    static int masterClientSocketFd = -1;
    ReconnModeAndEqptDescriptors *pModeAndEqptDescriptors;
    ReconnMasterClientMode myMode;

    mySocketFd = newSocketFd;

    pModeAndEqptDescriptors = args;
    myIndex = pModeAndEqptDescriptors->clientIndex;
    myMode = pModeAndEqptDescriptors->clientMode;

    printf("%s: Sending %s to client %d\n", __FUNCTION__, RECONNBEGIN, myIndex);
    sendSocket(mySocketFd, (unsigned char *)RECONNBEGIN, strlen(RECONNBEGIN), 0);

    printf("%s: reconnClientTask started myIndex %d\n", __FUNCTION__, myIndex);
    printf("%s: reconnClientTask started myMode == %s\n", __FUNCTION__, (myMode == MASTERMODE) ? "Master": "Client");
    printf("%s: reconnClientTask started mySocketFd %d\n", __FUNCTION__, mySocketFd);


    printf("%s: registering client with eqptTask\n", __FUNCTION__);
    if(reconnRegisterClientApp(myIndex , mySocketFd) != RECONN_SUCCESS)
    {
        printf("%s: reconnRegisterClientApp(%d, %d, %d) failed\n", __FUNCTION__, myIndex ,getpid(), mySocketFd);
        close(mySocketFd);
    }
    else
    {
        //
        // Every Client has a msgID to communicate with the master processes.
        //
        while (connection_open) 
        {
            bzero((unsigned char *) &thePacket, sizeof(ReconnPacket));
            /* receive the command from the client */

#ifdef DEBUG_CLIENT
            printf("%s: client with index %d waiting for command\n", __FUNCTION__, myIndex);
#endif
            retCode = receive_packet_data(mySocketFd, (unsigned char *)&thePacket, &length);
#ifdef DEBUG_CLIENT

            printf("%s %d: Packet received from client %d with length %d\n", __FUNCTION__, __LINE__, myIndex, length);
            debugPtr = (char *)&thePacket;
            for(debugIndex = 0; debugIndex < length + 4; debugIndex++)
            {
                printf("0x%x ", debugPtr[debugIndex]);
            }
            printf("\n");

#endif
            if(retCode == RECONN_CLIENT_SOCKET_CLOSED)
            {
                printf("%s: Socket closed by Client\n", __FUNCTION__);
                connection_open = FALSE;
                close(mySocketFd);
                reconnReturnClientIndex(myIndex);
                reconnDeRegisterClientApp(myIndex);
                if(myMode == MASTERMODE)
                {
                    masterClientSocketFd = -1;
                }
                connection_open = FALSE;
            }
            else if (length <= 0) 
            {
                printf("%s: Error reading from socket.\n", __FUNCTION__);
                /* recover from bad client read..?..?.. */
                close(mySocketFd);
                reconnDeRegisterClientApp(myIndex);
                reconnReturnClientIndex(myIndex);
                if(myMode == MASTERMODE)
                {
                    masterClientSocketFd = -1;
                }
                connection_open = FALSE;
            }
            else if ((thePacket.messageId.Byte[0] == 0x07) && (thePacket.messageId.Byte[1] == 0x07))
            {
                printf("%s: disonnect 0x7 0x7 received.\n", __FUNCTION__);
                /* exit command issued, not part of the command interface. */
                if (masterClientSocketFd == mySocketFd) 
                {
                    mySocketFd = 0;
                }
                close(mySocketFd);
                reconnDeRegisterClientApp(myIndex);
                reconnReturnClientIndex(myIndex);
                if(myMode == MASTERMODE)
                {
                    masterClientSocketFd = -1;
                }
                connection_open = FALSE;
            }
            else
            {
                /* everyone needs to know the packet length */
                GET_DATA_LENGTH_FROM_PACKET(p_length, thePacket);
                //printf("%s %d: p_length = %d\n", __FUNCTION__, __LINE__, p_length);

                GET_MSGID_FROM_PACKET(cmdid, thePacket);
                //printf("%s %d: cmdid = 0x%x\n", __FUNCTION__, __LINE__, cmdid);
                if((myMode == CLIENTMODE) && 
                        ((cmdid != KEEPALIVE_MESSAGE) &&
                         (cmdid != CLIENT_RESIGN_REQ) &&
                         (cmdid != CLIENT_ACCESS_REQ) && 
                         (cmdid != MASTER_MODE_REQ)))
                {
                    // a command came in from the client that we do not like
                    // do nothing, and the client will deal with the lack of
                    // response by closing the socket.
                }
                else
                {
                    switch (cmdid) 
                    {
                        case KEEPALIVE_MESSAGE: 
                        {
                            sendReconnCommandSuccess(mySocketFd, thePacket.messageId.Byte[0], thePacket.messageId.Byte[1]);
                            break;
                        }
                        case CLIENT_RESIGN_REQ:
                        {
                            /* The client has requested to be disconnected */
                            sendReconnCommandSuccess(mySocketFd, 
                                    thePacket.messageId.Byte[0], thePacket.messageId.Byte[1]);
                            reconnDeRegisterClientApp(myIndex);
                            reconnReturnClientIndex(myIndex);
                            close(mySocketFd);
                            if(myMode == MASTERMODE)
                            {
                                masterClientSocketFd = -1;
                            }
                            connection_open = FALSE;
                            break;
                        }
                        case CLIENT_ACCESS_REQ:
                        {
                            printf("%s: Received CLIENT_ACCESS_REQ\n", __FUNCTION__);
                            sendReconnCommandSuccess(mySocketFd,
                                    thePacket.messageId.Byte[0], thePacket.messageId.Byte[1]);
                            break;
                        }
                        case MASTER_MODE_REQ:
                        {
                            printf("%s: Received MASTER_MODE_REQ\n", __FUNCTION__);
                            if (masterClientSocketFd == mySocketFd) 
                            {
                                // This process is the master client
                                sendReconnCommandSuccess(mySocketFd, 
                                        thePacket.messageId.Byte[0], thePacket.messageId.Byte[1]);
                                printf("%s %d: Sending Success \n", __FUNCTION__, __LINE__);
                            }
                            else if (masterClientSocketFd == -1)
                            {
                                // nobody is master.  
                                // make this process the master client
                                masterClientSocketFd = mySocketFd;
                                sendReconnCommandSuccess(mySocketFd,
                                        thePacket.messageId.Byte[0], thePacket.messageId.Byte[1]); 
                                printf("%s %d: Sending Success \n", __FUNCTION__, __LINE__);
                            }
                            else
                            {
                                sendReconnCommandFailed(mySocketFd,
                                        thePacket.messageId.Byte[0], thePacket.messageId.Byte[1]); 
                                printf("%s %d: Sending Failure because there is already a master \n", __FUNCTION__, __LINE__);
                                break;
                            }
                        }
                        case MASTER_MODE_RESIGN_REQ:
                        {
                            printf("%s: Received MASTER_MODE_RESIGN_REQ\n", __FUNCTION__);
                            if (masterClientSocketFd == mySocketFd)
                            {
                                sendReconnCommandSuccess(mySocketFd,
                                        thePacket.messageId.Byte[0], thePacket.messageId.Byte[1]);
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
                                    sendReconnCommandSuccess(mySocketFd, thePacket.messageId.Byte[0], 
                                            thePacket.messageId.Byte[1]);
                                    break;
                                }
                            }
                            sendReconnCommandFailed (mySocketFd, thePacket.messageId.Byte[0],
                                    thePacket.messageId.Byte[1]);
#endif
                            break;
                        }
                        case WIFI_CHANGE_PASSWORD_REQ:
                        case WIFI_CHANGE_SSID_REQ:
                        {
                            break;
                        }
                        case SPECANA_POWER_SET_REQ:
                        {
                            if ((thePacket.dataPayload[0] == POWER_ON) || 
                                    (thePacket.dataPayload[0] == POWER_OFF))
                            {
                                if(reconnGpioAction(GPIO_141, (thePacket.dataPayload[0] == POWER_ON) ? ENABLE : DISABLE) == RECONN_FAILURE)
                                {                           
                                    printf("%s: reconnGpioAction(GPIO_141, ENABLE/DISABLE) failed. \n", __FUNCTION__);                          
                                    sendReconnCommandFailed (mySocketFd, 
                                            thePacket.messageId.Byte[0],
                                            thePacket.messageId.Byte[1]); 
                                }
                                else
                                {
                                    sendReconnCommandSuccess (mySocketFd, 
                                            thePacket.messageId.Byte[0], 
                                            thePacket.messageId.Byte[1]); 
                                }
                                break;
                            }
                            break;
                        }
                        case SPECANA_IDLE_CFG_REQ:
                        {
                            break;
                        }
                        case SPECANA_PKT_SEND_REQ:
                        {
#ifdef DEBUG_SPECTRUM
                            printf("%s: Received  SPECANA_PKT_SEND_REQ\n", __FUNCTION__);
#endif
                            if (masterClientSocketFd == mySocketFd) 
                            {
#ifdef DEBUG_SPECTRUM
                                printf("%s: Calling SpectrumAnalyzerWrite p_length = %d \n", __FUNCTION__, p_length);
#endif
                                SpectrumAnalyzerWrite((unsigned char *)&(thePacket.dataPayload), p_length);
                                cmdid = SPECANA_PKT_RCVD_NOTIFICATION;
                                theEqptFd = pModeAndEqptDescriptors->analyzerFd;
                                responseNeeded = TRUE;
                            }
                            break;

                        }
                        case GPS_POWER_SET_REQ:
                        {
                            if ((thePacket.dataPayload[0] == POWER_ON) || 
                                    (thePacket.dataPayload[0] == POWER_OFF))
                            {
                                if(gpsInit(&(pModeAndEqptDescriptors->gpsFd)) == RECONN_SUCCESS)
                                {
                                    sendReconnCommandFailed (mySocketFd, thePacket.messageId.Byte[0], 
                                            thePacket.messageId.Byte[1]); 
                                    break;
                                }
                            }
                            sendReconnCommandFailed(mySocketFd,
                                    thePacket.messageId.Byte[0], thePacket.messageId.Byte[1]);
                            break;
                        }
                        case GPS_IDLE_CFG_REQ:
                        {
                            break;
                        }
                        case GPS_PKT_SEND_REQ:
                        {
                            if (masterClientSocketFd == mySocketFd) 
                            {
                                if(gpsWrite((unsigned char *)&(thePacket.dataPayload), p_length) == RECONN_SUCCESS)
                                {
                                    sendReconnCommandSuccess(mySocketFd,
                                            thePacket.messageId.Byte[0], thePacket.messageId.Byte[1]);
                                }
                            }
                            break;
                        }
                        case PMETER_POWER_SET_REQ:
                        case PMETER_IDLE_CFG_REQ:
                        {
                            sendReconnCommandFailed(mySocketFd,
                                    thePacket.messageId.Byte[0], thePacket.messageId.Byte[1]);
                            break;
                        }
                        case PMETER_PKT_SEND_REQ:
                        {
                            printf("%s: Received  PMETER_PKT_SEND_REQ:\n", __FUNCTION__);
                            if (masterClientSocketFd == mySocketFd) 
                            {
                                printf("%s: Sending %c of length %d to power meter\n", __FUNCTION__, thePacket.dataPayload[0], p_length);
                                // The power meter is a USB device that can be plugged in at any time.
                                // If the meter is plugged in at boot time, PeripherliInit() would have
                                // opened ttyUSB1.  If this call returns RECONN_PM_PORT_NOT_INITIALIZED
                                // then try opening the power meter. If the open is not successfull
                                // then there is no power meter plugged into a USB port.
                                if(powerMeterWrite((unsigned char *)&(thePacket.dataPayload[0]), p_length) == RECONN_PM_PORT_NOT_INITIALIZED)
                                {
                                    if(powerMeterInit(pModeAndEqptDescriptors->powerMeterFd) != RECONN_SUCCESS)
                                    {
                                        sendReconnCommandFailed(mySocketFd, thePacket.messageId.Byte[0], 
                                            thePacket.messageId.Byte[1]);
                                        break;
                                    }
                                }
                                cmdid = PMETER_PKT_RCVD_NOTIFICATION;
                                theEqptFd = pModeAndEqptDescriptors->powerMeterFd;
                                responseNeeded = TRUE;
                            }
                            break;
                        }
                        case DMM_POWER_SET_REQ:
                        {
                            if ((thePacket.dataPayload[0] == POWER_ON) || 
                                    (thePacket.dataPayload[0] == POWER_OFF))
                            {
                                if(dmmInit(&(pModeAndEqptDescriptors->analyzerFd)) == RECONN_SUCCESS)
                                {
                                    sendReconnCommandSuccess(mySocketFd, thePacket.messageId.Byte[0], 
                                            thePacket.messageId.Byte[1]);
                                    break;
                                }
                            }
                            sendReconnCommandFailed (mySocketFd, thePacket.messageId.Byte[0],
                                    thePacket.messageId.Byte[1]);
                            break;
                        }
                        case DMM_IDLE_CFG_REQ:
                        case DMM_BUILTINTEST_REQ:
                        {
                            // communicate with the device. So, send it a status command and get the response.

                            sendReconnCommandSuccess (mySocketFd, thePacket.messageId.Byte[0], 
                                    thePacket.messageId.Byte[1]); 
                            break;
                        }
                        case DMM_PKT_SEND_REQ:
                        {
                            printf("%s: Received DMM_PKT_SEND_REQ\n", __FUNCTION__);
                            if (masterClientSocketFd == mySocketFd) 
                            {
                                printf("%s: Sending %c to meter\n", __FUNCTION__, thePacket.dataPayload[0]);
                                dmmWrite((unsigned char *)&(thePacket.dataPayload), p_length);
                                sendReconnCommandSuccess(mySocketFd,
                                        thePacket.messageId.Byte[0], thePacket.messageId.Byte[1]);
                                printf("%s: Success Sent back to client\n", __FUNCTION__);
                                theEqptFd = pModeAndEqptDescriptors->dmmFd;
                                responseNeeded = TRUE;
                            }
                            break;

                        }
                        default:
                        {
                            printf("%s: Invalid cmdid received %u\n", __FUNCTION__, cmdid);
                            sendReconnCommandFailed (mySocketFd, thePacket.messageId.Byte[0], 
                                    thePacket.messageId.Byte[1]); 
                            break;
                        }
                    } // switch (cmdid)
                }
                if((myMode == MASTERMODE) && (responseNeeded))
                {
#ifdef DEBUG_EQPT
                    printf("%s: Calling reconnGetEqptResponse(%d)\n", __FUNCTION__, theEqptFd);
#endif
                    reconnGetEqptResponse(theEqptFd, cmdid, mySocketFd);
                    // get data from devices 
                }
            }
            resetPowerStandbyCounter();
        }
    }
    return 1;
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
        printf("%s: recv()call failed  %d(%s)\n", __FUNCTION__, errno, strerror(errno));
    }
    else if (len != 4) 
    {
        printf("%s: recv is less than 4 len = %d\n", __FUNCTION__, len);
        return (-1);
    }
    else
    {

#ifdef COMM_DEBUG
        printf("%s %d: ", __FUNCTION__, __LINE__);
        for (i = 0; i < 4; ++i) 
        {
            printf("[%x]", (unsigned int) buffer[i]);
        }
#endif

        count = buffer[2] << 8;
        count = count + buffer[3];
        for (loop = 0; loop < count; loop++) 
        {
            recv(socket, &buffer[loop + 4], 1, 0);
#ifdef COMM_DEBUG
            printf("[%x]", (unsigned int) buffer[loop + 4]);
#endif
        }
#ifdef COMM_DEBUG
        printf("\n");
#endif
    }
    *length = count + 4;
    return (count + 4);
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
        printf("%s: data length (%d) is larger than %d\n" , __FUNCTION__, theDataLength, MAX_COMMAND_INPUT);
        retCode = RECONN_FAILURE;
    }
    return retCode;
}
