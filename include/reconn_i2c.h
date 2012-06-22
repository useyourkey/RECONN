/* CPPDOC_BEGIN_EXCLUDE  */

//******************************************************************************
//******************************************************************************
//
// FILE:        reconn_i2c.h
//
// CLASSES:     
//
// DESCRIPTION: Public interface definitions to the RECONN I2C implementation.
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

#ifndef __RECONN_I2C_H_
#define __RECONN_I2C_H_

#include <stdint.h>

/**
 * Amount of retries to attempt successful i2c read/write before returning error.
 */
#define RECONN_I2C_RETRY_COUNT        (10)
#define RECONN_I2C_RETRY_DELAY_US     (1000)

/**
 * Opens the i2c device through the Linux /dev interface.
 *
 * @param <adapter_nr> The i2c adapter number, i.e., 1 for /dev/i2c-1
 * @param <i2c_addr> The i2c slave address.
 * @return Returns file handle on success, or -1 on error (see errno)
 * @see reconn_i2c_close_dev
 */
int reconn_i2c_open_dev(int i2c_devnr, uint8_t i2c_addr);

/**
 * Closes the i2c device.
 *
 * @param <fh> The i2c device file handle.
 * @return Returns 0 on success, -1 on error (see errno).
 * @see reconn_open_i2c_dev
 */
int reconn_i2c_close_dev(int fh);

/**
 * Reads from an i2c register.
 *
 * @param <fh> The i2c device file handle.
 * @param <reg> The i2c device register to read.
 * @param <pbuf> Buffer to receive register contents.
 * @param <count> Number of bytes to read from register.
 * @return Returns number of bytes read on success, -1 on error (see errno).
 * @see reconn_i2c_write_reg
 */
int reconn_i2c_read_reg(int fh, uint8_t reg, uint8_t *pbuf, size_t count);

/**
 * Writes to an i2c register.
 *
 * @param <fh> The i2c device file handle.
 * @param <reg> The i2c device register to read.
 * @param <pbuf> Buffer to receive register contents.
 * @param <count> Number of bytes to read from register.
 * @return Returns number of bytes written on success, -1 on error (see errno).
 * @see reconn_i2c_read_reg
 */
int reconn_i2c_write_reg(int fh, uint8_t reg, uint8_t *pbuf, size_t count);

#endif // #ifndef __RECONN_I2C_H_

