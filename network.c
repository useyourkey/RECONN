//******************************************************************************
//******************************************************************************
//
// FILE:        network.c
//
// CLASSES:     
//
// DESCRIPTION: This is the file which contains reconn networking functions.
//              At present, there is only the task that watches for IP changes
//              to USB0
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
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <net/if.h>
#include <arpa/inet.h>

#include "reconn.h"
#include "debugMenu.h"
#include "network.h"
#include "socket.h"
#include "libiphoned.h"

extern int masterClientSocketFd;
extern int insertedMasterSocketFd;
//******************************************************************************
//******************************************************************************
// FUNCTION:    Usb0IpWatchTask
//
// DESCRIPTION: This task is responsible for receiving messages from the kernel
//              relative to the reconn network interfaces. The task will also 
//              check to see if USB0's link is up or down.
//              At present we are only concerned about USB0 events. 
//              As such, this task only pays attention to USB0 events and in 
//              particular; IP address changes. When an IP
//              address change event is received and the event is for a new IP 
//              address a message is sent to the iPhone application informing it
//              of the new IP. At which point the iPhone application sends an IP
//              address request message to the embedded software.
//
//******************************************************************************
void *Usb0IpWatchTask(void *args)
{
    FILE *theLinkStatusFd;
    struct sockaddr_nl addr;
    struct timeval waitTime;
    struct nlmsghdr *netLinkHeader;
    static int state = 1;
    fd_set theFileDescriptor;
    unsigned int len;
    int sock;
    int messageSent = FALSE;
    int retCode;
    uint32_t ipaddr;
    char headerBuf[1024];
    char name[IFNAMSIZ];
    LINKSTATUS_e theValue = LINK_DOWN;
    LINKSTATUS_e oldLinkState = LINK_DOWN;
    ReconnResponsePacket theResponsePkt;
    ReconnResponsePacket *theResponsePktPtr = &theResponsePkt;


    UNUSED_PARAM(args);

    reconnDebugPrint("%s: ****** Task started\n", __FUNCTION__);
    if ((sock = socket(PF_NETLINK, SOCK_RAW, NETLINK_ROUTE)) == -1) 
    {
        reconnDebugPrint("%s: socket(PF_NETLINK, SOCK_RAW, NETLINK_ROUTE) failed %d (%s)", __FUNCTION__, errno, strerror(errno));
    }
    else
    {

        memset(&addr, 0, sizeof(addr));
        addr.nl_family = AF_NETLINK;
        addr.nl_groups = RTMGRP_IPV4_IFADDR;

        if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1) 
        {
            reconnDebugPrint("%s: couldn't bind %d (%s)\n", __FUNCTION__, errno , strerror(errno));
        }
        else
        {
            netLinkHeader = (struct nlmsghdr *)headerBuf;
            while(1)
            {
                /*
                 * If there is a master client connected check USB link status
                 */
                if((masterClientSocketFd != -1)  || (insertedMasterSocketFd != -1))
                {
                    FD_ZERO(&theFileDescriptor);
                    FD_SET(sock, &theFileDescriptor);
                    waitTime.tv_sec = 0;
                    waitTime.tv_usec = IPWATCHSLEEPTIME;
                    if((retCode = select(sock + 1, &theFileDescriptor, NULL, NULL, &waitTime)) < 0)
                    {
                        reconnDebugPrint("%s: select failed %d(%s)\n",__FUNCTION__, errno, strerror(errno));
                        break;
                    }
                    else if(retCode == 0)
                    {
                        /*
                         * We have timed out so check to see if USB0 link is up or down.
                         */
                        if((theLinkStatusFd = fopen(USB_CARRIER_FILENAME, "r")) == NULL)
                        {
                            reconnDebugPrint("%s: fopen(%s) failed %d (%s)\n", __FUNCTION__, USB_CARRIER_FILENAME, 
                                    errno, strerror(errno));
                        }
                        else
                        {
                            /*
                             * If USB0 is brought down via ifconfig usb0 down, USB_CARRIER_FILENAME can be opened
                             * but any read of that file results in EINVAL. Treat EINVAL as link down.
                             */
                            if(((fread(&theValue, 1, 1, theLinkStatusFd)) != 1) && (errno != EINVAL))
                            {
                                reconnDebugPrint("%s: fread() failed %d (%s)\n", __FUNCTION__, errno, strerror(errno));
                            }
                            else
                            {
                                if(errno == EINVAL)
                                {
                                    theValue = LINK_DOWN;
                                }
                                errno = 0;
#ifdef DEBUG_NETWORK
                                reconnDebugPrint("%s: oldLinkState  = %d theValue = %d messageSent = %d\n", __FUNCTION__, oldLinkState, theValue, messageSent);  
#endif
                                if((oldLinkState != theValue) || (messageSent == FALSE))
                                {
                                    oldLinkState = theValue;
                                    /*
                                     * Send message to the master client informing it of the link transition.
                                     */
                                    memset(theResponsePktPtr, 0, sizeof(theResponsePkt));
                                    ADD_RSPID_TO_PACKET(GENERIC_RESPONSE, theResponsePktPtr);
                                    ADD_MSGID_TO_PACKET(RECONN_LINK_STATUS_NOTIF, theResponsePktPtr);
                                    ADD_DATA_LENGTH_TO_PACKET(1, theResponsePktPtr);
                                    theResponsePktPtr->dataPayload[0] = (theValue == '1') ? LINK_UP : LINK_DOWN;
                                    if(insertedMasterSocketFd != -1)
                                    {
                                        libiphoned_tx((unsigned char *)theResponsePktPtr, RECONN_RSPPACKET_HEADER_SIZE + 1);
                                    }
                                    else
                                    {
                                        sendSocket(masterClientSocketFd, (unsigned char *)theResponsePktPtr, RECONN_RSPPACKET_HEADER_SIZE + 1, 0);
                                    }
                                    messageSent = TRUE;
                                }
                            }
                            if(fclose(theLinkStatusFd) != 0)
                            {
#ifdef DEBUG_NETWORK
                                reconnDebugPrint("%s: fclose() failed %d (%s)\n", __FUNCTION__, 
                                        errno, strerror(errno));
#endif
                            }
                        }
                    }
                    else
                    {
                        if(FD_ISSET(sock, &theFileDescriptor))
                        {
                            if ((len = recv(sock, netLinkHeader, 1024, 0)) > 0) 
                            {
#ifdef DEBUG_NETWORK
                                reconnDebugPrint("%s: Message received len = %d netLinkHeader->nlmsg_type = %d\n", __FUNCTION__, len, netLinkHeader->nlmsg_type);
#endif
                                while ((NLMSG_OK(netLinkHeader, len)) && ((netLinkHeader->nlmsg_type) != NLMSG_DONE)) 
                                {
                                    if (netLinkHeader->nlmsg_type == RTM_NEWADDR) 
                                    {
#ifdef DEBUG_NETWORK
                                        reconnDebugPrint("%s: RTM_NEWADDR received \n", __FUNCTION__);
#endif
                                        struct ifaddrmsg *ifa = (struct ifaddrmsg *) NLMSG_DATA(netLinkHeader);
                                        struct rtattr *rth = IFA_RTA(ifa);
                                        int rtl = IFA_PAYLOAD(netLinkHeader);

                                        while (rtl && RTA_OK(rth, rtl)) 
                                        {
                                            if (rth->rta_type == IFA_ADDRESS) 
                                            {
                                                ipaddr = htonl(*((uint32_t *)RTA_DATA(rth)));
                                                if_indextoname(ifa->ifa_index, name);
                                                /*
                                                 * Send message to the master client informing it of the link transition.
                                                 */
                                                memset(theResponsePktPtr, 0, sizeof(theResponsePkt));
                                                ADD_RSPID_TO_PACKET(GENERIC_RESPONSE, theResponsePktPtr);
                                                ADD_MSGID_TO_PACKET(RECONN_LINK_STATUS_NOTIF, theResponsePktPtr);
                                                ADD_DATA_LENGTH_TO_PACKET(1, theResponsePktPtr);
                                                theResponsePktPtr->dataPayload[0] = IP_CHANGE;
                                                if(insertedMasterSocketFd != -1)
                                                {
                                                    libiphoned_tx((unsigned char *)theResponsePktPtr, RECONN_RSPPACKET_HEADER_SIZE + 1);
                                                }
                                                else
                                                {
                                                    sendSocket(masterClientSocketFd, (unsigned char *)theResponsePktPtr, RECONN_RSPPACKET_HEADER_SIZE + 1, 0);
                                                }
#ifdef DEBUG_NETWORK
                                                reconnDebugPrint("%s: %s is now %d.%d.%d.%d\n",
                                                        __FUNCTION__, name,
                                                        (ipaddr >> 24) & 0xff,
                                                        (ipaddr >> 16) & 0xff,
                                                        (ipaddr >> 8) & 0xff,
                                                        ipaddr & 0xff);
#endif
                                            }
                                            rth = RTA_NEXT(rth, rtl);
                                        }
                                    }
                                    else
                                    {
                                        reconnDebugPrint("%s: netLinkHeader->nlmsg_type = 0x%x received \n", __FUNCTION__, netLinkHeader->nlmsg_type);
                                    }
                                    netLinkHeader = NLMSG_NEXT(netLinkHeader, len);
                                }
                            }
                            else
                            {
                                reconnDebugPrint("%s: recv() failed %d(%s)\n",__FUNCTION__, errno, strerror(errno));
                            }
                        }
                        else
                        {
                            reconnDebugPrint("%s: descriptor is not set\n", __FUNCTION__);
                        }
                    }
                }
                else
                {
#ifdef DEBUG_NETWORK
                    reconnDebugPrint("%s: NO master\n", __FUNCTION__);
#endif
                    usleep(IPWATCHSLEEPTIME);
                    messageSent = FALSE;
                }
            }
        }
    }
    reconnDebugPrint("%s: ****** Task exiting\n", __FUNCTION__);
    return (&state);
}
