//******************************************************************************
//****************************************************************************** //
// FILE:        wifi.c
//
// FUNCTION:        wifiUpdateConfFile
//
// DESCRIPTION: This file contains interfaces used by the reconn embedded software
//              to update wifi related data and hardware
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
#include <unistd.h>
#include <errno.h>

#include "reconn.h"
#include "wifi.h"
#include "debugMenu.h"

//******************************************************************************
//****************************************************************************** //
// FUNCTION:        wifiUpdateConfFile
//
// CLASSES:     
//
// DESCRIPTION: This function is called to make changes to hostapd's configuration
//              file. This function searches the configuration file for "token"
//              and replaces the text following "token" with "theNewValue"
//
// PARAMTERS:
//              token       -   a string that is searched for in the configuration file
//              theNewValue -   the replacement string.
//
//******************************************************************************
ReconnErrCodes wifiUpdateConfFile(char *token, char *theNewValue)
{
    ReconnErrCodes retCode = RECONN_SUCCESS;
    FILE *theFile, *theOutFile;
    char *theFileString, *theNewString, *theCommand;
    size_t size = 90;
    int count;

    theFile = theOutFile = NULL;
    theFileString = theNewString = theCommand = NULL;

    reconnDebugPrint("%s: Function entered token = %s, the Newvalue = %s\n", __FUNCTION__, token, theNewValue);
    if((theOutFile = fopen(WIFI_CONF_FILE_NEW, "w")) == NULL)
    {
        printf("%s: fopen(%s) failed %d (%s)\n", __FUNCTION__, WIFI_CONF_FILE_NEW, errno, strerror(errno));
        retCode = RECONN_FAILURE;
    }
    else if((theFile = fopen(WIFI_CONF_FILE_ORIG, "r")) == NULL)
    {
        printf("%s: fopen(%s) failed %d (%s)\n", __FUNCTION__, WIFI_CONF_FILE_ORIG, errno, strerror(errno));
        fclose(theOutFile);
        retCode = RECONN_FAILURE;
    }
    else
    {
        if((theFileString = (char *)malloc(size+1)))
        {
            if((theNewString = (char *)malloc(size+1)))
            {
                // get enough memory to create a command similar to " mv <sourcefile>  <destfile> NULL"
                theCommand = (char *)malloc(strlen(WIFI_CONF_FILE_ORIG) + strlen(WIFI_CONF_FILE_NEW) + 5);
                while((count = getline(&theFileString, &size, theFile)) > 0)
                {
                    if(strncmp(theFileString, token, strlen(token)) == 0)
                    {
                        strcat(theNewString, token);
                        strcat(theNewString, theNewValue);
                        strcat(theNewString, "\n");
                        fwrite(theNewString, 1, strlen(theNewString), theOutFile);
                    }
                    else
                    {
                        fwrite(theFileString, 1, strlen(theFileString), theOutFile);
                    }
                    memset(theFileString, 0, size+1);
                    memset(theNewString, 0, size+1);
                    memset(theCommand, 0, strlen(WIFI_CONF_FILE_ORIG) + strlen(WIFI_CONF_FILE_NEW) + 5);
                }
                if(fclose(theFile) != 0)
                {
                    reconnDebugPrint("%s: fclose(%s) failed %d (%s)\n", __FUNCTION__, WIFI_CONF_FILE_ORIG, errno, strerror(errno));
                }
                if(fclose(theOutFile) != 0)
                {
                    reconnDebugPrint("%s: fclose(%s) failed %d (%s)\n", __FUNCTION__, WIFI_CONF_FILE_NEW, errno, strerror(errno));
                }
                strcat(theCommand, "mv ");
                strcat(theCommand, WIFI_CONF_FILE_NEW);
                strcat(theCommand, " ");
                strcat(theCommand, WIFI_CONF_FILE_ORIG);
                printf("%s: calling system(%s)\n", __FUNCTION__, theCommand);
                system(theCommand);
                unlink(WIFI_CONF_FILE_NEW);
                free(theFileString);
                free(theNewString);
            }
            else
            {
                free(theFileString);
                reconnDebugPrint("%s: malloc(theNewString) failed %d (%s)\n", __FUNCTION__, errno, strerror(errno));
                retCode = RECONN_FAILURE;
            }
        }
        else
        {
            reconnDebugPrint("%s: malloc(theFileString) failed %d (%s)\n", __FUNCTION__, errno, strerror(errno));
            retCode = RECONN_FAILURE;
        }
    }
    return(retCode);
}
