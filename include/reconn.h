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
#include <sys/time.h>
#include <pthread.h>

// remove this line for target installation

#define GENERIC_RESPONSE 0xFFFF
#define KEEPALIVE_MESSAGE 0xFFFE
#define TRUE 1
#define FALSE 0
#define MAX_COMMAND_INPUT 1024
#define RECONN_INCOMING_PORT    1068
#define RECONN_LIBTOCLIENT_PORT 1070
#define RECONN_STACK_SIZE  16384
#define RECONN_EQPT_PRIORITY 50
#define RECONN_PWR_MGMT_PRIORITY  50
#define RECONN_CLIENT_PRIORITY 50
#define RECONN_PAYLOAD_SIZE 500
#define RECONN_RSP_PAYLOAD_SIZE 2048
#define RECONN_NUM_CLIENTS  5
#define RECONN_MAX_NUM_CLIENTS  REMOTE_MONITOR_MAX_CLIENTS + RECONN_NUM_CLIENTS
#ifdef __SIMULATION__
#define RECONN_SERIALNUM_FILE_NAME "serial.txt"
#define RECONN_MACADDR_FILE_NAME "macAddr.txt"
#define RECONN_DEFAULT_FILE_NAME "defaulted"
#define RECONN_CRASHLOG_FILE_NAME "crashlog.txt"
#define RJ45_MAC_ADDRRESS_FILE_NAME "rj45macAddress.txt"
#else
#define RECONN_SERIALNUM_FILE_NAME "/home/reconn/serial.txt"
#define RECONN_MACADDR_FILE_NAME "/home/reconn/macAddr.txt"
#define RECONN_DEFAULT_FILE_NAME "/home/reconn/defaulted"
#define RECONN_CRASHLOG_FILE_NAME "/home/reconn/crashlog.txt"
#define RJ45_MAC_ADDRRESS_FILE_NAME "/home/reconn/rj45macAddress.txt"
#endif
#define RECONN_MACADDR_PREAMBLE "MAC addr from NVS: "
#define RECONN_MACADDR_LEN 17 // aa:bb:cc:dd:ee:ff
#define RECONN_IPADDR_LEN 15// aaa.bbb.ccc.ddd

#define RECONN_SERIALNUM_MAX_LEN 12 // 11 bytes plus NULL terminator
//#define RECONN_USER_PASSWORD "Reconn2012"

//#define DEBUG_EQPT
//#define DEBUG_SPECTRUM
//#define DEBUG_CONNECT
//#define DEBUG_CLIENT
//#define DEBUG_GPIO
//#define DEBUG_MUTEX
//#define DEBUG_REMOTE
//#define DEBUG_NETWORK
//#define RECONN_TIMING
extern struct timeval stopTime;
extern struct timeval startTime;
extern struct timeval result;
extern struct timeval deviceStopTime;
extern struct timeval deviceStartTime;
extern struct timeval deviceResult;
extern struct timeval queueStopTime;
extern struct timeval queueStartTime;
extern struct timeval queueResult;
extern int timeval_subtract();

typedef enum
{
    YES = 1,
    NO
}YESNO;
typedef enum 
{
    CLIENT_ACCESS_REQ           =   0x0001,
    CLIENT_RESIGN_REQ           =   0x0002,
    MASTER_MODE_REQ             =   0x0003, 
    MASTER_MODE_SET_NOTIF       =   0x0004,
    MASTER_MODE_REMOVED_NOTIF   =   0x0005,
    MASTER_MODE_RESIGN_REQ      =   0x0006,
    MASTER_MODE_RESIGN_QUERY    =   0x0007,
    MASTER_MODE_RESIGN_RESP     =   0x0008,
    SLAVE_MODE_DISABLE_REQ      =   0x0009,
    SLAVE_MODE_ENABLE_REQ       =   0x000a,
    MASTER_LOST_NOTIF           =   0x000b,
    SLAVES_SEND_MSG             =   0x000c
}ConnectionManagmentCommands;

typedef enum
{
    POWER_OFF,
    POWER_ON
}ReconnPower;

typedef enum
{
    NON_DEFAULT,
    DEFAULT
}ReconnInit;

typedef enum
{
    RECONN_CONNECTION_STATUS_REQ=   0x0101,
    RECONN_CONNECTION_RESPONSE  =   0x01f1,
    RECONN_SLAVE_ATTACHED       =   0x0102,
    RECONN_HARDWARE_FAILURE     =   0x0103,
    RECONN_ID_REQ               =   0x0104,
    RECONN_ID_NOTIF             =   0x0105,
    RECONN_INIT_STATE_REQ       =   0x0106,
    RECONN_INIT_STATE_SET_REQ   =   0x0107,
    RECONN_DEVICE_DEBUG_REQ     =   0x0109,
    RECONN_SW_VERSION_REQ       =   0x010a,
    RECONN_IP_ADDRESS_REQ       =   0x010b,
    RECONN_IP_ADDRESS_NOTIF     =   0x010c,
    REMOTE_MONITOR_STATE_REQ    =   0x010d,
    REMOTE_MONITOR_ENABLE_REQ   =   0x010e,
    RECONN_RETRIEVE_CRASHLOG    =   0x010f,
    RECONN_POWER_DOWN_NOTIF     =   0x0110,
    RECONN_QUITING_NOTIF        =   0x0111,
    RECONN_LINK_STATUS_NOTIF    =   0x0112
}ReconnStatus;

typedef enum
{
    SPECANA_POWER_SET_REQ       =   0x0200,
    SPECANA_IDLE_CFG_REQ        =   0x0201,
    SPECANA_PKT_SEND_REQ        =   0x0202,
    SPECANA_PKT_RCVD_NOTIF      =   0x0203,
    SPECANA_HARDWARE_FAIL_NOTIF =   0x0204
}SpectrumAnalyzerCommands;

typedef enum
{
    GPS_POWER_SET_REQ           =   0x0300,
    GPS_IDLE_CFG_REQ            =   0x0301,
    GPS_PKT_SEND_REQ            =   0x0302,
    GPS_PKT_RCVD_NOTIF          =   0x0303,
    GPS_BUILTINTEST_REQ         =   0x0304
}GlobalPositioningSystemCommands;

typedef enum
{
    PMETER_POWER_SET_REQ        =   0x0400,
    PMETER_IDLE_CFG_REQ         =   0x0401,
    PMETER_PKT_SEND_REQ         =   0x0402,
    PMETER_PKT_RCVD_NOTIF       =   0x0403,
    PMETER_BUILTINTEST_REQ      =   0x0404,
    PMETER_STATUS_NOTIF         =   0x0405
}PowerMeterCommands;

typedef enum
{
    DMM_POWER_SET_REQ           =   0x0500,
    DMM_IDLE_CFG_REQ            =   0x0501,
    DMM_PKT_SEND_REQ            =   0x0502,
    DMM_PKT_RCVD_NOTIF          =   0x0503,
    DMM_BUILTINTEST_REQ         =   0x0504,
    DMM_PORT_CONNECTION_REQ     =   0x0505
}DigitalMultiMeterCommands;

typedef enum
{
    LNB_POWER_SET_REQ           =   0x0600,
    LNB_SA_10MHZ                =   0x0601,
    LNB_10MHZ                   =   0x0602,
    LNB_18VDC_BIAS              =   0x0603,
    LNB_HARDWARE_FAIL           =   0x0604,
    LNB_GPIO_STATES             =   0x0605
}LnbCommands;

typedef enum 
{
    TERMINAL_INIT_COMMS_REQ     =   0x0700,
    TERMINAL_IDLE_CONFIG_REQ    =   0x0701,
    TERMINAL_PKT_SEND_REQ       =   0x0702,
    TERMINAL_PKT_RCV_NOTIF      =   0x0703,
    TERMINAL_BUILTINTEST_REQ    =   0x0704
}TerminalInterfaceCommands;

typedef enum 
{
    BATTERY_LEVEL_REQ           =   0x0800,
    BATTERY_LEVEL_RSP           =   0x08f0,
    BATTERY_CHARGE_STATE_REQ    =   0x0801,
    BATTERY_CHARGE_STATE_RSP    =   0x08f1
}BatteryStatus;

typedef enum 
{ 
    WIFI_STATUS_REQ             =   0x0900,
    WIFI_SET_POWER_REQ          =   0x0901,
    WIFI_CHANGE_PASSWORD_REQ    =   0x0902,
    WIFI_CHANGE_SSID_REQ        =   0x0903,
    WIFI_MAC_ADDRESS_REQ        =   0x0904,
    WIFI_MAC_ADDRESS_NOTIF      =   0x0905,
    WIFI_SSID_RETRIEVE          =   0x0906,
    WIFI_PASSWORD_RETRIEVE      =   0x0907
}WifiCommands;

typedef enum
{
    SW_UPGRADE_REQ              =   0x0a00,
    FW_UPGRADE_REQ              =   0x0a01
}SwUpgradeCommands;

typedef enum
{
    WEB_SERVICE_DATA_FLOW       =   0x0b00,
    WEB_SERVICE_IP_AND_PORT     =   0x0b01,
    WEB_SERVICE_STATE           =   0x0b02
}WebServiceCommands;

typedef enum
{
    // The first 3 cannot change value 'cause it will break the 
    // protocol between the embedded software and the iPhone application
    RECONN_SUCCESS = 1,
    RECONN_DENIED,
    RECONN_INVALID_PARAMETER,
    RECONN_INVALID_STATE,
    RECONN_INVALID_MESSAGE,                 //5
    RECONN_FAILURE,
    RECONN_UPGRADE_CLIENT_CONNECTED,
    RECONN_UPGRADE_FILE_NOT_FOUND,
    RECONN_UPGRADE_BAD_CHECKSUM,
    RECONN_LINK_DOWN,                       //10
    RECONN_CONNECTION_TIMEDOUT,
    RECONN_NETWORK_UNREACHABLE,
    RECONN_CONNECTION_REFUSED,
    RECONN_HOSTNAME_UNRESOLVED,
    RECONN_PACKET_READY,                    //15
    RECONN_GPS_INIT_FAILED,
    RECONN_POWER_METER_INIT_FAILED,
    RECONN_SPECTRUM_ANALYZER_INIT_FAILED,
    RECONN_OUT_OF_MEMORY,
    RECONN_SERIAL_PORT_OPEN_FAIL,           //20
    RECONN_GPS_PORT_NOT_INITIALIZED,
    RECONN_POWER_METER_OPEN_FAIL,
    RECONN_PM_PORT_NOT_INITIALIZED,
    RECONN_CONNECT_FAILED,
    RECONN_SA_PORT_NOT_INITIALIZED,         //25
    RECONN_DMM_PORT_NOT_INITIALIZED,
    RECONN_DMM_SERIAL_PORT_OPEN_FAIL,
    RECONN_CLIENT_SOCKET_CLOSED,
    RECONN_CLIENT_SOCKET_EAGAIN,
    RECONN_DEBUG_DONE,                      //30
    RECONN_FILE_NOT_FOUND,
    RECONN_EAGAINOREWOULDBLOCK,
    RECONN_NEEDS_REBOOT,
    RECONN_ERROR_UNKNOWN = 0xFF
}ReconnErrCodes;
// Reconn System tasks
typedef enum
{
    RECONN_EQPT_TASK,
    RECONN_PWR_MGMT_TASK,
    //RECONN_PWR_BUTTON_TASK,
    RECONN_BATTERY_MONITOR_TASK,
    RECONN_DEBUG_MENU_TASK,
    RECONN_UPGRADE_CHECK_TASK,
    RECONN_MASTER_SOCKET_TASK,
    RECONN_POWER_METER_TASK,
    RECONN_DMM_SAVE_CONFIG_TASK,
    RECONN_DMM_SOCKET_TASK,
    RECONN_EQPT_RSP_TASK,
    RECONN_REMOTE_MONITOR_TASK,
    RECONN_WIFI_CONNECTION_TASK,
    RECONN_IP_WATCH_TASK,
    RECONN_XTERNAL_DEBUG_TASK,
    RECONN_KEEPALIVE_TASK,
    RECONN_NUM_SYS_TASKS
}RECONN_TASKS;


#define LOW 0
#define HIGH 1

#define ADD_RSPID_TO_PACKET(theID, thePacket) \
{ \
        thePacket->rspId.Byte[LOW] = (unsigned char )((theID & 0xff00)>> 8); \
        thePacket->rspId.Byte[HIGH] = (unsigned char )(theID & 0x00ff); \
}

#define GET_RSPID_FROM_PACKET(theId, thePacket) \
{ \
        theId = (unsigned short) thePacket.rspId.Byte[HIGH]; \
        theId += (unsigned short)thePacket.rspId.Byte[LOW]; \
}

#define ADD_MSGID_TO_PACKET(theID, thePacket) \
{ \
        thePacket->messageId.Byte[LOW] = (unsigned char )((theID & 0xff00)>> 8); \
        thePacket->messageId.Byte[HIGH] = (unsigned char )(theID & 0x00ff); \
}

#define ADD_DATA_LENGTH_TO_PACKET(theLength, thePacket) \
{ \
        thePacket->dataLength.Byte[LOW] = (unsigned char )((theLength & 0xff00)>> 8); \
        thePacket->dataLength.Byte[HIGH] = (unsigned char )(theLength & 0x00ff); \
}

#define GET_MSGID_FROM_PACKET(theId, thePacket) \
{\
    theId = (unsigned short) thePacket.messageId.Byte[HIGH];\
    theId += (unsigned short)(thePacket.messageId.Byte[LOW] << 8); \
}
#define GET_DATA_LENGTH_FROM_PACKET(theLength, thePacket) \
{ \
    theLength = (unsigned short) thePacket.dataLength.Byte[HIGH];\
    theLength += (unsigned short)(thePacket.dataLength.Byte[LOW] << 8);\
}
#define UNUSED_PARAM(parm) parm=parm;


#define RECONN_PACKET_HEADER_SIZE 4 // messageId+dataLength
typedef struct 
{
    union
    {
        unsigned short value;
        unsigned char Byte[2];
    }messageId;
    union
    {
        unsigned short value;
        unsigned char Byte[2];
    }dataLength;
    char dataPayload[RECONN_PAYLOAD_SIZE];
}ReconnPacket;

#define RECONN_RSPPACKET_HEADER_SIZE 6 // rspId+dataLength+messageId
typedef struct 
{
    union
    {
        unsigned short value;
        unsigned char Byte[2];
    }rspId;
    union
    {
        unsigned short value;
        unsigned char Byte[2];
    }dataLength;

    union
    {
        unsigned short value;
        unsigned char Byte[2];
    }messageId;

    char dataPayload[RECONN_RSP_PAYLOAD_SIZE];
}ReconnResponsePacket;

typedef enum
{
    MASTERMODE,
    SLAVEMODE,
    INSERTEDMASTERMODE,
    REMOTEMODE,
    INVALIDMODE
}ReconnMasterClientMode;

typedef struct
{
    short clientIndex;
    ReconnMasterClientMode clientMode;
    //int gpsFd;
    int powerMeterFd;
    //int lnbFd;
    int dmmFd;
    int analyzerFd;
    //int wiFiFd;
}ReconnEqptDescriptors;

typedef short ReconnClientIndex;

extern int gNewSocketFd;
extern int gps_enabled;
extern int powermeter_enabled;
extern int specana_enabled;
extern int dmm_enabled;
extern ReconnEqptDescriptors gEqptDescriptors;
extern pthread_t reconnThreadIds[];

void * msgifcGetNewInterface(void * parentifc);
int msgifcFreeInterface(void * interface);
int msgifcDataIn(void * interface, void * data, unsigned int datalen);
extern void reconnReturnClientIndex(short index);
extern ReconnErrCodes reconnGetFreeClientIndex(short *theIndex);
#ifdef __SIMULATION__
extern void reconnCleanUp();
#endif

#endif /* __RECONN_H */
