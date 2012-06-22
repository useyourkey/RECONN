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

#define DEBUG_PASSWORD "Reconn2012\r\n"
#define DEBUG_QUESTION "Enter debug password "
#define DEBUG_INPUT_LEN  50
#define DEBUG_OUTPUT_LEN  100
#define DEBUG_PROMPT_MAX_SIZE 30
#define DEBUG_PROMPT "reconn debug >"
#define DEBUG_TIMEOUT_MESSAGE "\r\n*******You are being disconnected due to inactivity timeout\r\n"

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

void *debugMenuTask(void *argument);
extern void registerDebugCommand(debugMenuStruct *, int);
extern void reconnDebugPrint(const char *fmt, ...);
#endif
