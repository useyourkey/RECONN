//******************************************************************************
//******************************************************************************
//
// FILE:        extractBundle.c
//
// CLASSES:     
//
// DESCRIPTION: This function will get the upgrade bundle, extract and verify the header,
//              the header and payload checksums. If they pass, it will then extract
//              the reconn-service executable decompress it and place it in the
//              running directory. The newly installed application will then be run
//              by /etc/reconn script.
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
// HISTORY: Created 04/09/1012 by Michael A. Carrier
// $Header:$
// $Revision: 1.3 $
// $Log:$
// 
//******************************************************************************
//******************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "reconn.h"
#include "upgrade.h"
#include "debugMenu.h"

ReconnErrCodes extractBundle()
{
    UPGRADEBUNDLE bundle;
    long length;
    int payloadLength;
    long read;
    int sysCode;
    char buff[10];
    FILE *filePtr = NULL;
    FILE *bundleFilePtr = NULL;
    char fileName[FILENAME_SIZE];
    char headerChecksum[MD5SUM_SIZE];
    char command[COMMAND_SIZE];
    ReconnErrCodes retCode = RECONN_SUCCESS;

    memset(&bundle, 0, sizeof(UPGRADEBUNDLE));
    memset(&headerChecksum, 0, MD5SUM_SIZE);

    length = sizeof(bundle); 

    reconnDebugPrint("%s: Function Entered \n", __FUNCTION__);
#ifdef __SIMULATION__
    return 1;
#endif

    if((bundleFilePtr = fopen(UPGRADE_BUNDLE_NAME, "r")))
    {
        // read the upgrade bundle header.
        while((retCode == RECONN_SUCCESS) && 
                (fread(&bundle, 1, sizeof(UPGRADEBUNDLE), bundleFilePtr)!= 0))
        {
            read = 0;
            // Calculate bundles header checksum.
            if((filePtr = fopen(UPGRADE_HEADER_DATA_NAME, "w")))
            {
                fwrite(bundle.headerVersion, 1, HEADER_VERSION_SIZE, filePtr);
                fwrite(bundle.headerLength, 1, HEADER_LENGTH_SIZE, filePtr);
                fwrite(bundle.payloadLength, 1, PAYLOAD_LENGTH_SIZE, filePtr);
                fwrite(bundle.payloadChecksum, 1, MD5SUM_SIZE, filePtr);
                fwrite(bundle.fileName, 1, FILENAME_SIZE, filePtr);
                fclose(filePtr);
                system("md5sum "UPGRADE_HEADER_DATA_NAME " > " UPGRADE_HEADER_SUM_NAME);
                if((filePtr = fopen(UPGRADE_HEADER_SUM_NAME, "r")))
                {
                    if(fread(&(headerChecksum[0]), 1, MD5SUM_SIZE, filePtr) > 0)
                    {
                        // Compare the checksum in the bundle with the checksum
                        // calculated.
                        if(strncmp((const char *)&(bundle.headerChecksum), headerChecksum, MD5SUM_SIZE) == 0)
                        {
                            fclose(filePtr);
                            // The checksums match, extract the binary from the bundle.
                            payloadLength = atoi(&(bundle.payloadLength[0]));
                            memset(fileName, 0, FILENAME_SIZE);
                            strcat(fileName, "/tmp/");
                            strcat(fileName,  bundle.fileName);
                            reconnDebugPrint("%s: extracting %s to %s\n", __FUNCTION__, bundle.fileName, fileName);
                            if((filePtr = fopen(fileName, "w")))
                            {
                                memset(&buff, 0,10);
                                do
                                {
                                    if(fread(buff, 1, 1, bundleFilePtr) > 0)
                                    {
                                        fwrite(buff, 1, 1, filePtr);
                                        read++;
                                        memset(&buff, 0,10);
                                    }
                                    else
                                    {
                                       if(feof(bundleFilePtr))
                                       {
                                           reconnDebugPrint("%s:  end of file detected\n", __FUNCTION__);
                                           break;
                                       }
                                       else if(ferror(bundleFilePtr)) 
                                       {
                                           reconnDebugPrint("%s:  fread failed file detected\n", __FUNCTION__);
                                           break;
                                       }
                                    }
                                }while(read < payloadLength);

                                fclose(filePtr);
                                memset(command, 0, COMMAND_SIZE);
                                strcat ((char *)&command, "gunzip -f ");
                                strcat ((char *)&command, fileName);
                                if((sysCode = system(command)) != 0)
                                {
                                    reconnDebugPrint("%s: system(%s) failed\n", __FUNCTION__, command);
                                    retCode = RECONN_FAILURE;
                                }
                                else
                                {
#if 0
                                    memset(command, 0, COMMAND_SIZE);
                                    strcat ((char *)&command, "chmod ugo+x ");
                                    strcat ((char *)&command, fileName);
                                    if((sysCode = system(command)) != 0)
                                    {
                                        reconnDebugPrint("%s: system(%s) failed\n", __FUNCTION__, command); retCode = RECONN_FAILURE;
                                    }
                                    else
                                    {
#endif
                                        memset(command, 0, COMMAND_SIZE);
                                        strcat ((char *)&command, "touch /tmp/upgrade_inprogress");
                                        if((sysCode = system(command)) != 0)
                                        {
                                            reconnDebugPrint("%s: system(%s) failed\n", __FUNCTION__, command);
                                            retCode = RECONN_FAILURE;
                                        }
#if 0
                                        else 
                                        {
                                            memset(command, 0, COMMAND_SIZE);
                                            strcat ((char *)&command, "mv -f  ");
                                            strcat ((char *)&command, fileName);
                                            strcat ((char *)&command, " /usr/bin");
                                            if((strcmp(fileName, "reconn-service") == 0) &&
                                                    (sysCode = system(command)) != 0)
                                            {
                                                reconnDebugPrint("%s: system(%s) failed\n", __FUNCTION__, command);
                                                retCode = RECONN_FAILURE;
                                            }
                                        }
                                    }
#endif
                                }
                            }
                            else
                            {
                                reconnDebugPrint("%s: Could not open %s for writing\n", __FUNCTION__, fileName);
                                retCode = RECONN_FAILURE;
                            }
                        }
                        else
                        {
                            fclose(filePtr);
                            reconnDebugPrint("%s: corrupt header\n", __FUNCTION__);
                            retCode = RECONN_UPGRADE_BAD_CHECKSUM;
                        }
                    }
                    else
                    {
                        reconnDebugPrint("%s: Error reading checksum from %s \n", __FUNCTION__, UPGRADE_HEADER_SUM_NAME);
                        fclose(filePtr);
                        retCode = RECONN_FAILURE;
                    }
                }
                else
                {
                    reconnDebugPrint("%s: Could not open %s for reading\n", __FUNCTION__, UPGRADE_HEADER_SUM_NAME);
                    retCode = RECONN_FAILURE;
                }
            }
            else
            {
                reconnDebugPrint("%s:Could not open %s for writing %d(%s)\n", __FUNCTION__, UPGRADE_HEADER_DATA_NAME, errno, strerror(errno));
                retCode = RECONN_FAILURE;
            }
        }
        if(feof(bundleFilePtr))
        {
            reconnDebugPrint("%s: End of file indication\n", __FUNCTION__); 
        }
        fclose(bundleFilePtr);
    }
    else
    {
        reconnDebugPrint("%s: Could not open %s %d(%s)\n", __FUNCTION__, UPGRADE_BUNDLE_NAME, errno, strerror(errno));
        retCode = RECONN_UPGRADE_FILE_NOT_FOUND;
    }
    unlink(UPGRADE_HEADER_DATA_NAME);
    unlink(UPGRADE_HEADER_SUM_NAME);
    reconnDebugPrint("%s: Function exiting with %d \n", __FUNCTION__, retCode);
    return (retCode);
} 
