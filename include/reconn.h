//******************************************************************************
//******************************************************************************
//
// FILE:        reconn.h
//
// CLASSES:     
//
// DESCRIPTION: Header file for the Reconn Application
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
#ifndef __RECONN_H
#define __RECONN_H

// remove this line for target installation

#define GENERIC_RESPONSE 0xFFFF
#define KEEPALIVE_MESSAGE 0xFFFE
#define TRUE 1
#define FALSE 0
#define MAX_COMMAND_INPUT 1024
#define RECONN_INCOMING_PORT    1068
#define RECONN_MAX_NUM_CLIENTS  10
#define RECONN_STACK_SIZE  16384
#define RECONN_EQPT_PRIORITY 50
#define RECONN_PWR_MGMT_PRIORITY  50
#define RECONN_CLIENT_PRIORITY 50
#define RECONN_PAYLOAD_SIZE 50
#define RECONN_RSP_PAYLOAD_SIZE 2048


//#define DEBUG_EQPT
//#define DEBUG_SPECTRUM
//#define SOCKET_MUTEX
#define DEBUG_CONNECT
//#define DEBUG_CLIENT
//#define DEBUG_GPIO


typedef enum 
{
    CLIENT_ACCESS_REQ =                 0x0001,
    CLIENT_RESIGN_REQ =                 0x0002,
    MASTER_MODE_REQ =                   0x0003, 
    MASTER_MODE_SET_NOTIFICATION =      0x0004,
    MASTER_MODE_REMOVED_NOTIFICATION =  0x0005,
    MASTER_MODE_RESIGN_REQ =            0x0006,
    MASTER_MODE_RESIGN_QUERY =          0x0007
}ConnectionManagmentCommands;

typedef enum
{
    POWER_ON,
    POWER_OFF
}ReconnPower;

typedef enum
{
    RECONN_CONNECTION_STATUS_REQ=   0x0101,
    RECONN_CONNECTION_RESPONSE =    0x01f1,
    RECONN_HARDWARE_FAILURE =       0x0103,
    RECONN_ID_REQ =                 0x0104,
    RECONN_ID_NOTIFICATION =        0x0105,
    RECONN_INIT_STATE_REQ =         0x0106,
    RECONN_INIT_STATE_SET_REQ =     0x0107,
    RECONN_SW_VERSION_REQ =         0x0108,
    RECONN_DEVICE_DEBUGB_REQ =      0x0109,
    RECONN_SW_VERSION_NOTIF =       0x010a
}ReconnStatus;

typedef enum
{
    SPECANA_POWER_SET_REQ =         0x0200,
    SPECANA_IDLE_CFG_REQ =          0x0201,
    SPECANA_PKT_SEND_REQ =          0x0202,
    SPECANA_PKT_RCVD_NOTIFICATION = 0x0203,
    SPECANA_BUILTINTEST_REQ =       0x0204
}SpectrumAnalyzerCommands;

typedef enum
{
    GPS_POWER_SET_REQ =         0x0300,
    GPS_IDLE_CFG_REQ =          0x0301,
    GPS_PKT_SEND_REQ =          0x0302,
    GPS_PKT_RCVD_NOTIFICATION = 0x0303,
    GPS_BUILTINTEST_REQ =       0x0304
}GlobalPositioningSystemCommands;

typedef enum
{
    PMETER_POWER_SET_REQ =          0x0400,
    PMETER_IDLE_CFG_REQ =           0x0401,
    PMETER_PKT_SEND_REQ =           0x0402,
    PMETER_PKT_RCVD_NOTIFICATION =  0x0403,
    PMETER_BUILTINTEST_REQ =        0x0404
}PowerMeterCommands;

typedef enum
{
    DMM_POWER_SET_REQ =         0x0500,
    DMM_IDLE_CFG_REQ =          0x0501,
    DMM_PKT_SEND_REQ =          0x0502,
    DMM_PKT_RCVD_NOTIFICATION = 0x0503,
    DMM_BUILTINTEST_REQ =       0x0504,
    DMM_PORT_CONNECTION_REQ =   0x0504
}DigitalMultiMeterCommands;

typedef enum
{
    LNB_POWER_SET_REQ =         0x0600,
    LNB_IDLE_CONFIG_REQ =       0x0601,
    LNB_PKT_SEND_REQ =          0x0602,
    LNB_PKT_RCV_NOTIFICATION =  0x0603,
    LNB_BUILTINTEST_REQ =       0x0604
}LnbCommands;

typedef enum 
{
    TERMINAL_INIT_COMMS_REQ =       0x0700,
    TERMINAL_IDLE_CONFIG_REQ =      0x0701,
    TERMINAL_PKT_SEND_REQ  =        0x0702,
    TERMINAL_PKT_RCV_NOTIFICATION = 0x0703,
    TERMINAL_BUILTINTEST_REQ =      0x0704
}TerminalInterfaceCommands;

typedef enum 
{
    BATTERY_LEVEL_REQ = 0x0800,
    BATTERY_LEVEL_RSP = 0x08f0
}BatteryStatus;

typedef enum 
{ 
    WIFI_STATUS_REQ =           0x0900,
    WIFI_SET_POWER_REQ =        0x0901,
    WIFI_CHANGE_PASSWORD_REQ =  0x0902,
    WIFI_CHANGE_SSID_REQ =      0x0903
}WifiCommands;

typedef enum
{
    SW_UPGRADE_REQ =    0x0a00
}SwUpgradeCommands;

typedef enum
{
    RECONN_SUCCESS = 1,
    RECONN_DENIED,
    RECONN_INVALID_PARAMETER,
    RECONN_INVALID_STATE,
    RECONN_INVALID_MESSAGE,                 //5
    RECONN_FAILURE,
    RECONN_UPGRADE_CLIENT_CONNECTED,
    RECONN_UPGRADE_FILE_NOT_FOUND,
    RECONN_UPGRADE_BAD_CHECKSUM,
    RECONN_PACKET_READY,                    //10
    RECONN_GPS_INIT_FAILED,
    RECONN_POWER_METER_INIT_FAILED,
    RECONN_SPECTRUM_ANALYZER_INIT_FAILED,
    RECONN_OUT_OF_MEMORY,
    RECONN_SERIAL_PORT_OPEN_FAIL,           //15
    RECONN_GPS_PORT_NOT_INITIALIZED,
    RECONN_POWER_METER_OPEN_FAIL,
    RECONN_PM_PORT_NOT_INITIALIZED,
    RECONN_CONNECT_FAILED,
    RECONN_SA_PORT_NOT_INITIALIZED,         //20
    RECONN_DMM_PORT_NOT_INITIALIZED,
    RECONN_DMM_SERIAL_PORT_OPEN_FAIL,
    RECONN_CLIENT_SOCKET_CLOSED,
    RECONN_CLIENT_SOCKET_EAGAIN,
    RECONN_ERROR_UNKNOWN = 0xFF
}ReconnErrCodes;
// Reconn System tasks
typedef enum
{
    RECONN_EQPT_TASK,
    RECONN_PWR_MGMT_TASK,
    RECONN_PWR_BUTTON_TASK,
    RECONN_BATTERY_MONITOR_TASK,
    RECONN_DEBUG_MENU_TASK,
    RECONN_UPGRADE_CHECK_TASK,
    RECONN_MASTER_SOCKET_TASK,
    RECONN_NUM_SYS_TASKS
}RECONN_TASKS;


#define LOW 0
#define HIGH 1

#define ADD_RSPID_TO_PACKET(theID, thePacket) \
{ \
        thePacket->rspId.Byte[LOW] = (char )((theID & 0xff00)>> 8); \
        thePacket->rspId.Byte[HIGH] = (char )(theID & 0x00ff); \
}

#define ADD_MSGID_TO_PACKET(theID, thePacket) \
{ \
        thePacket->messageId.Byte[LOW] = (char )((theID & 0xff00)>> 8); \
        thePacket->messageId.Byte[HIGH] = (char )(theID & 0x00ff); \
}

#define ADD_DATA_LENGTH_TO_PACKET(theLength, thePacket) \
{ \
        thePacket->dataLength.Byte[LOW] = (char )((theLength & 0xff00)>> 8); \
        thePacket->dataLength.Byte[HIGH] = (char )(theLength & 0x00ff); \
}

#define GET_MSGID_FROM_PACKET(theId, thePacket) \
{\
    theId = (short) thePacket.messageId.Byte[HIGH];\
    theId += (thePacket.messageId.Byte[LOW] << 8); \
}
#define GET_DATA_LENGTH_FROM_PACKET(theLength, thePacket) \
{ \
    theLength = (short) thePacket.dataLength.Byte[HIGH];\
    theLength += (thePacket.dataLength.Byte[LOW] << 8); }
#define UNUSED_PARAM(parm) parm=parm;



typedef struct 
{
    union
    {
        short value;
        char Byte[2];
    }messageId;
    union
    {
        short value;
        char Byte[2];
    }dataLength;
    char dataPayload[RECONN_PAYLOAD_SIZE];
}ReconnPacket;

typedef struct 
{
    union
    {
        short value;
        char Byte[2];
    }rspId;
    union
    {
        short value;
        char Byte[2];
    }dataLength;

    union
    {
        short value;
        char Byte[2];
    }messageId;

    char dataPayload[RECONN_RSP_PAYLOAD_SIZE];
}ReconnResponsePacket;

typedef enum
{
    INITMODE,
    MASTERMODE,
    CLIENTMODE,
    INSERTEDMASTERMODE,
    INVALIDMODE
}ReconnMasterClientMode;

typedef struct
{
    short clientIndex;
    ReconnMasterClientMode clientMode;
    int gpsFd;
    int powerMeterFd;
    int lnbFd;
    int dmmFd;
    int analyzerFd;
    int wiFiFd;
}ReconnModeAndEqptDescriptors;

typedef int ReconnClientIndex;

extern int gNewSocketFd;
extern int gps_enabled;
extern int powermeter_enabled;
extern int specana_enabled;
extern int dmm_enabled;

void * msgifcGetNewInterface(void * parentifc);
int msgifcFreeInterface(void * interface);
int msgifcDataIn(void * interface, void * data, unsigned int datalen);
ReconnErrCodes receive_packet_data(int socket, unsigned char *buffer, int *length);
void reconnReturnClientIndex(short index);

#endif /* __RECONN_H */
