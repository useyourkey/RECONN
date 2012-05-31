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
    char headerChecksum[MD5SUM_SIZE];
    ReconnErrCodes retCode = RECONN_SUCCESS;

    memset(&bundle, 0, sizeof(UPGRADEBUNDLE));
    memset(&headerChecksum, 0, MD5SUM_SIZE);

    length = sizeof(bundle); 

    reconnDebugPrint("%s: Function Entered \n", __FUNCTION__);

    if((bundleFilePtr = fopen(UPGRADE_BUNDLE_NAME, "r")))
    {
        // read the upgrade bundle header.
        if(fread(&bundle, 1, sizeof(bundle), bundleFilePtr))
        {
            // Calculate bundles header checksum.
            if((filePtr = fopen(UPGRADE_HEADER_DATA_NAME, "w")))
            {
                fwrite(bundle.headerVersion, 1, HEADER_VERSION_SIZE, filePtr);
                fwrite(bundle.headerLength, 1, HEADER_LENGTH_SIZE, filePtr);
                fwrite(bundle.payloadLength, 1, PAYLOAD_LENGTH_SIZE, filePtr);
                fwrite(bundle.payloadChecksum, 1, MD5SUM_SIZE, filePtr);
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
                            if((filePtr = fopen(UPGRADE_GZIP_NAME, "w")))
                            {
                                fseek(bundleFilePtr, sizeof(bundle),  SEEK_SET);
                                memset(&buff, 0,10);
                                while(fread(buff, 1, 1, bundleFilePtr) > 0)
                                {
                                    fwrite(buff, 1, 1, filePtr);
                                    read++;
                                    memset(&buff, 0,10);
                                }
                                fclose(filePtr);
                                fclose(bundleFilePtr);
                                if((sysCode = system("gunzip -f "UPGRADE_GZIP_NAME)) != 0)
                                {
                                    printf("%s: system(\"gunzip -f \"%s) failed\n", __FUNCTION__, UPGRADE_GZIP_NAME);
                                    retCode = RECONN_FAILURE;
                                }
                                else if((sysCode = system("chmod ugo+x "UPGRADE_RECONN_NAME)) != 0)
                                {
                                    printf("%s: system(\"chmod ugo+x \"%s) failed\n", __FUNCTION__, UPGRADE_RECONN_NAME);
                                    retCode = RECONN_FAILURE;
                                }
                                else if((sysCode = system("touch /tmp/upgrade_inprogress")) != 0)
                                {
                                    printf("%s: system(\"touch /tmp/upgrade_inprogress\") failed\n", __FUNCTION__);
                                    retCode = RECONN_FAILURE;
                                }
#ifndef __SIMULATION__
                                else if((sysCode = system("mv -f "UPGRADE_RECONN_NAME " /usr/bin")) != 0)
                                {
                                    printf("%s: system(\"mv -f \"%s \/usr/bin\") failed\n", __FUNCTION__, UPGRADE_GZIP_NAME);
                                    retCode = RECONN_FAILURE;
                                }
#endif
                            }
                            else
                            {
                                printf("%s: Could not open %s\n", __FUNCTION__, UPGRADE_GZIP_NAME);
                                fclose(bundleFilePtr);
                                retCode = RECONN_FAILURE;
                            }
                        }
                        else
                        {
                            printf("%s: corrupt header", __FUNCTION__);
                            fclose(bundleFilePtr);
                            retCode = RECONN_UPGRADE_BAD_CHECKSUM;
                        }
                    }
                    else
                    {
                        printf("%s: Error reading checksum from %s \n", __FUNCTION__, UPGRADE_HEADER_SUM_NAME);
                        fclose(bundleFilePtr);
                        fclose(filePtr);
                        retCode = RECONN_FAILURE;
                    }
                }
                else
                {
                    printf("%s: Could not open %s for reading\n", __FUNCTION__, UPGRADE_HEADER_SUM_NAME);
                    fclose(bundleFilePtr);
                    retCode = RECONN_FAILURE;
                }
            }
            else
            {
                printf("%s:Could not open %s for writing %d(%s)\n", __FUNCTION__, UPGRADE_HEADER_DATA_NAME, errno, strerror(errno));
                fclose(bundleFilePtr);
                retCode = RECONN_FAILURE;
            }
        }
        else
        {
            printf("error reading reconnBundle %d(%s)\n", errno, strerror(errno));
            fclose(bundleFilePtr);
            retCode = RECONN_FAILURE;
        }
    }
    else
    {
        retCode = RECONN_UPGRADE_FILE_NOT_FOUND;
    }
    unlink(UPGRADE_HEADER_DATA_NAME);
    unlink(UPGRADE_HEADER_SUM_NAME);
    printf("%s: Function exiting with %d \n", __FUNCTION__, retCode);
    return (retCode);
} 
