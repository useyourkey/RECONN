/* CPPDOC_BEGIN_EXCLUDE  */

//******************************************************************************
//******************************************************************************
//
// FILE:        reconn_i2c.c
//
// CLASSES:     
//
// DESCRIPTION: Public Apple MFi Authentication Linux I2C implementation.
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
//           Portions copyright by Intelligraphics, Inc.
//           Portions copyright by Apple, Inc.
//
//******************************************************************************
//
// HISTORY: Created 04/03/2012 by stephen.smith@intelligraphics.com
// $Header:$
// $Revision:$
// $Log:$
// 
//******************************************************************************
//******************************************************************************

/* CPPDOC_END_EXCLUDE  */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>

#include "reconn_i2c.h"

//******************************************************************************
// FUNCTION:  reconn_i2c_open_dev
//
// CREATED:   04/03/2012 by stephen.smith@intelligraphics.com
//******************************************************************************
int reconn_i2c_open_dev(int i2c_devnr, uint8_t i2c_addr)
{
    char devname[20];
    int fh;
    int ret;

    snprintf(devname, sizeof(devname) - sizeof(devname[0]),
            "/dev/i2c-%d", i2c_devnr);
    fh = open(devname, O_RDWR);
    if (fh != -1)
    {
        /**
         * When you have opened the device, you must specify with what device
         * address you want to communicate.
         *
         * Change slave address. The address is passed in the 7 lower bits of the
         * argument (except for 10 bit addresses, passed in the 10 lower bits in
         * this case).
         */
        ret = ioctl(fh, I2C_SLAVE, i2c_addr >> 1);
        if (ret < 0)
        {
            close(fh);
            return ret;
        }
    }
    return fh;
}

//******************************************************************************
// FUNCTION:  reconn_i2c_close_dev
//
// CREATED:   04/03/2012 by stephen.smith@intelligraphics.com
//******************************************************************************
int reconn_i2c_close_dev(int fh)
{
    return close(fh);
}

//******************************************************************************
// FUNCTION:  reconn_i2c_read_reg
//
// CREATED:   04/03/2012 by stephen.smith@intelligraphics.com
//******************************************************************************
int reconn_i2c_read_reg(int fh, uint8_t reg, uint8_t *pbuf, size_t count)
{
    int ret;
    int retry;

    // write the register address to read
    for (retry = RECONN_I2C_RETRY_COUNT; ; retry--)
    {
        ret = write(fh, &reg, sizeof(reg));
        if (ret == sizeof(reg))
        {
            break;
        }
        else if (!retry)
        {
            return ret;
        }
        else if (ret == -1)
        {
            usleep(RECONN_I2C_RETRY_DELAY_US);
        }
        else
        {
            return ret;
        }
    }
    if (!retry)
    {
        return ret;
    }

    // now read the register's contents
    for (retry = RECONN_I2C_RETRY_COUNT; ; retry--)
    {
        ret = read(fh, pbuf, count);
        if (ret == count)
        {
            break;
        }
        else if (!retry)
        {
            return ret;
        }
        else if (errno == -1)
        {
            usleep(RECONN_I2C_RETRY_DELAY_US);
        }
        else
        {
            return ret;
        }
    }

    return ret;
}

//******************************************************************************
// FUNCTION:  reconn_i2c_write_reg
//
// CREATED:   04/03/2012 by stephen.smith@intelligraphics.com
//******************************************************************************
int reconn_i2c_write_reg(int fh, uint8_t reg, uint8_t *pbuf, size_t count)
{
    int ret;
    int retry;
    uint8_t *p;

    /**
     * since we have to send the register address AND data bytes
     * w/o an intervening possible i2c stop sequence, we have to create
     * a buffer, concatenate the register address AND data buffer into
     * it, transmit it then free the buffer
     */

    p = malloc(count + sizeof(reg));
    if (!p)
    {
        return -1;
    }
    p[0] = reg;
    memcpy(&p[0], &reg, sizeof(reg));
    memcpy(&p[sizeof(reg)], pbuf, count);
    count += sizeof(reg);

    // now write the buffer to the register
    for (retry = RECONN_I2C_RETRY_COUNT; ; retry--)
    {
        ret = write(fh, p, count);
        if (ret == count)
        {
            ret -= sizeof(reg);
            break;
        }
        else if (!retry)
        {
            break;
        }
        else if (ret == -1)
        {
            usleep(RECONN_I2C_RETRY_DELAY_US);
        }
        else
        {
            break;
        }
    }

    free(p);

    return ret;
}

