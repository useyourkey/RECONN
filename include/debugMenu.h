//******************************************************************************
//******************************************************************************
//
// FILE:        debugMenu.h
//
// CLASSES:     
//
// DESCRIPTION: Header file for the Reconn debug menu Application
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
#ifndef __DEBUG_H
#define __DEBUG_H
#include "reconn.h"

//#define DEBUG_PASSWORD "Reconn2012\r\n"
#define DEBUG_DELETE 127
#define DEBUG_PASSWORD "Reconn2012"
#define DEBUG_QUESTION "Enter debug password "
#define DEBUG_INPUT_LEN  50
#define DEBUG_OUTPUT_LEN  100
#define DEBUG_PROMPT_MAX_SIZE 30
#define DEBUG_PROMPT "reconn debug >"
#define DEBUG_TIMEOUT_MESSAGE "\r\n*******You are being disconnected due to inactivity timeout\r\n"
#define DEBUG_INVALID_COMMAND "***** Invalid Command\r\n"
#define DEBUG_INACTIVITY_TIMEOUT 1200
#define DEBUG_PORT 4083
#define DEBUG_BACKSPACE "\033[0D"
#define DEBUG_CLEARLINE "\033[0K"
#define DEBUG_CLEARSCREEN "\033[2J"
#define DEBUG_CURSORHOME "\033[0;0H"
#define DEBUG_MAIN_PROMPT "Choose Menu Item >"

extern int gReconnDebugLevel;
#define RECONN_DEBUG_ON(level) (gReconnDebugLevel & level)
#define NO_DEBUG                0x00000
#define DMM_DEBUG_RCV           0x00001
#define DMM_DEBUG_SND           0x00002
#define DMM_DEBUG_EQPT          0x00004
#define PMETER_DEBUG_RCV        0x00008
#define PMETER_DEBUG_SND        0x00010
#define PMETER_DEBUG_EQPT       0x00020
#define BATTERY_DEBUG_RCV       0x00040
#define BATTERY_DEBUG_SND       0x00080
#define SPECANA_DEBUG_RCV       0x00100
#define SPECANA_DEBUG_SND       0x00200
#define SPECANA_DEBUG_EQPT      0x00400
#define LNB_DEBUG_RCV           0x00800
#define SLAVE_DEBUG_RCV         0x01000
#define TIMING_DEBUG            0x02000
#define EXTERNAL_DEBUG          0x04000
#define RM_DEBUG                0x08000
#define EQPT_TASK               0x10000

extern char debugOutputString[DEBUG_OUTPUT_LEN];
extern int externalDebugSocketFd;

typedef struct
{
    char *subSystemName;
    char *subSystemHelp;
    char *commandName;
    char *commandHelp;
    int (*func)(int);
}debugMenuStruct;

struct TableEntry_t
{
    struct TableEntry_t *nextTableEntry;
    debugMenuStruct *theMenu;
    int numMenuItems;
};

typedef struct
{
    struct TableEntry_t *aMenuEntry;
    int numTables;
}debugMenusTableStruct_t;

extern int timeoutDebugMenus;
extern YESNO gExternalMessagesOn;


void *debugMenuTask(void *argument);
extern void registerDebugCommand(debugMenuStruct *, int);
extern void reconnDebugPrint(const char *fmt, ...);
extern char * getInput(int, YESNO);
extern void registerDmmDebugMenu();
extern void showDebugStatus();
extern ReconnErrCodes reconnStartExternalTask();
extern void reconnStopExternalTask();
extern YESNO gDebugTimingEnabled;
#endif
