//******************************************************************************
//******************************************************************************
//
// FILE:        createBundle.c
//
// CLASSES:     
//
// DESCRIPTION: This program will create the upgrade bundle. It is called by
//              the reconn makefile when someone types "make upgrade"'
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
#include "version.h"
#include "reconn.h"
#include "upgrade.h"

int main()
{
    UPGRADEBUNDLE bundle;
    short headerLength;
    int payloadLength = 0;
    struct stat fileStat;
    char buff[10];
    FILE *filePtr, *bundleFilePtr;

    memset(&bundle, 0, sizeof(UPGRADEBUNDLE));

    headerLength = sizeof(bundle); 

    bundle.headerLength[0] = headerLength & 0x00ff;
    bundle.headerLength[1] = (headerLength & 0xff00) >> 8;
    strcat(&(bundle.headerVersion[0]), majorLevel);
    strcat(&(bundle.headerVersion[2]), minorLevel);
    strcat(&(bundle.headerVersion[4]), patchLevel);
    if((filePtr = fopen("reconnSum", "r")))
    {
        if(fread(&(bundle.payloadChecksum[0]), 1, MD5SUM_SIZE, filePtr) > 0)
        {
            fclose(filePtr);
            memset(&fileStat, 0, sizeof(fileStat));
            if(stat("reconn-service.gz", &fileStat) == 0)
            {
                sprintf(&(bundle.payloadLength[0]), "%d", (unsigned int)fileStat.st_size);
                if((filePtr = fopen("headerData", "w")))
                {
                    fwrite(bundle.headerVersion, 1, HEADER_VERSION_SIZE, filePtr);
                    fwrite(bundle.headerLength, 1, HEADER_LENGTH_SIZE, filePtr);
                    fwrite(bundle.payloadLength, 1, PAYLOAD_LENGTH_SIZE, filePtr);
                    fwrite(bundle.payloadChecksum, 1, MD5SUM_SIZE, filePtr);
                    fclose(filePtr);
                    system("md5sum headerData > headerSum");
                    if((filePtr = fopen("headerSum", "r")))
                    {
                        if(fread(&(bundle.headerChecksum[0]), 1, MD5SUM_SIZE, filePtr) > 0)
                        {
                            fclose(filePtr);
                            if((bundleFilePtr = fopen("reconnBundle", "w")))
                            {
                                fwrite(bundle.headerVersion, 1, HEADER_VERSION_SIZE, bundleFilePtr);
                                fwrite(bundle.headerLength, 1, HEADER_LENGTH_SIZE, bundleFilePtr);
                                fwrite(bundle.headerChecksum, 1, MD5SUM_SIZE, bundleFilePtr);
                                fwrite(bundle.payloadLength, 1, PAYLOAD_LENGTH_SIZE, bundleFilePtr);
                                fwrite(bundle.payloadChecksum, 1, MD5SUM_SIZE, bundleFilePtr);
                                // Now cat the reconn-service.gz to the bundle
                                //system("cat reconn-service.gz >> reconnBundle");
                                if((filePtr = fopen("reconn-service.gz", "r")))
                                {
                                    while(fread(buff, 1, 1, filePtr) > 0)
                                    {
                                        fwrite(buff, 1, 1, bundleFilePtr);
                                        payloadLength++;
                                    }
                                    fclose(bundleFilePtr);
                                    fclose(filePtr);
                                    if(payloadLength != fileStat.st_size)
                                    { 
                                        printf("%s: write failure to reconnBundle %i %li\n", __FILE__, payloadLength, fileStat.st_size );
                                    }
                                }
                                else
                                {
                                    fclose(bundleFilePtr);
                                    printf("%s: Could not open reconn-service.gz for reading %d(%s)\n", __FILE__, errno, strerror(errno));
                                }
                            }
                            else
                            {
                                printf("%s: Could not open reconnBundle for writing %d(%s)\n", __FILE__, errno, strerror(errno));
                            }
                        }
                        else
                        {
                            printf("%s: Could not read headerSum %d(%s)\n", __FILE__, errno, strerror(errno));
                            fclose(filePtr);
                        }
                    }
                    else
                    {
                        printf("%s: Could not open headerSum for reading %d(%s)\n", __FILE__, errno, strerror(errno));
                    }
                }
                else
                {
                    printf("%s: Could not open headerData for writing %d(%s)\n", __FILE__, errno, strerror(errno));
                }
            }
            else
            {
                printf("%s: stat() failed %d(%s)\n", __FILE__, errno, strerror(errno));
            }
        }
        else
        {
            printf("%s: Could not reconnSum file %d(%s)\n", __FILE__, errno, strerror(errno));
            fclose(filePtr);
        }
    }
    else
    {
        printf("%s: Could not open reconnSum for reading %d(%s)\n", __FILE__, errno, strerror(errno));
    }
    // Now transfer the file into the bundle
    unlink("reconnSum");
    unlink("headerSum");
    unlink("headerData");
    return 0;
}


