/* CPPDOC_BEGIN_EXCLUDE  */

//******************************************************************************
//******************************************************************************
//
// FILE:        fuel_gauge.h
//
// CLASSES:     
//
// DESCRIPTION: Public interface definitions to the Apple MFi Authentication
//              Coprocessor fuel gauge.
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
// HISTORY: Created 02/22/2012 by stephen.smith@intelligraphics.com
// $Header:$
// $Revision:$
// $Log:$
// 
//******************************************************************************
//******************************************************************************

/* CPPDOC_END_EXCLUDE  */

#ifndef __FUEL_GAUGE_H_
#define __FUEL_GAUGE_H_

#include <stdint.h>
#include <semaphore.h>

/**
 * host to network support
 */

#ifdef __BIG_ENDIAN__
#define FUEL_GAUGE_HTONL
#define FUEL_GAUGE_HTONS
#define FUEL_GAUGE_NTOHL
#define FUEL_GAUGE_NTOHS
#else
#define FUEL_GAUGE_HTONL htobe32
#define FUEL_GAUGE_HTONS htobe16
#define FUEL_GAUGE_NTOHL be32toh
#define FUEL_GAUGE_NTOHS be16toh
#endif 
#define FUEL_GAUGE_MAX_PERCENTAGE 85 // fuel gauge reading 85% or better is fuel battery charge

#define FUEL_GAUGE_DEVICE_I2C_BUS 3
#define FUEL_GAUGE_RETRY_COUNT 5
#define FUEL_MODEL_SIZE 50

typedef enum
{
    CHARGE_ENABLED  = '0',
    CHARGE_DISABLED = '1',
    CHARGE_NO_STATE = '3'
}FUEL_GAUGE_ENUM;


/**
 * Implementation status codes
 */
typedef enum fuel_gauge_status_e
{
    FUEL_GAUGE_STATUS_SUCCESS = 0,
    FUEL_GAUGE_STATUS_ERROR = -1,
    FUEL_GAUGE_STATUS_BUSY = -2,
    FUEL_GAUGE_STATUS_PENDING = -3,
    FUEL_GAUGE_STATUS_INTERNAL_ERROR = -4,
    FUEL_GAUGE_STATUS_INVALID_PARAM = -5,
    FUEL_GAUGE_STATUS_NO_MEM = -6,
    FUEL_GAUGE_STATUS_NO_ENT = -8,
    FUEL_GAUGE_STATUS_IO_ERR = -9,
    FUEL_GAUGE_STATUS_VERS_ERR = -10,
    FUEL_GAUGE_STATUS_TIMEOUT = -11,
    FUEL_GAUGE_STATUS_UNEXPECTED = -12,
    FUEL_GAUGE_STATUS_BUFFER_TOO_SMALL = -13,
    FUEL_GAUGE_STATUS_FILE_ALREADY_OPEN = -14,
    FUEL_GAUGE_STATUS_AGAIN = -16,
    FUEL_GAUGE_STATUS_NOT_VALID = -17
} fuel_gauge_status_t;

/**
 * Device context handle
 */
/**
 * This contains the fuel gauge context information needed by the fuel gauge.
 * It is allocated in the fuel gauge initialization function and released
 * in the fuel gauge cleanup function.
 */
struct fuel_gauge_context
{
    sem_t ctx_mutext;           // the context mutext
    int inuse;                  // flag that says the fuel gauge is busy

    int fh;                     // file handle to the I2C Coprocessor
};
typedef struct fuel_gauge_context fuel_gauge_context_t, *fuel_gauge_handle_t;

#define FUEL_GAUGE_INVALID_HANDLE	(fuel_gauge_handle_t) 0

#define FUEL_GAUGE_I2C_ADDRESS          (0x6c)

/**
 * Acquires the fuel gauge mutex.
 *
 * @param <h> The fuel gauge context handle.
 * @return Nothing
 */
static inline void fuel_gauge_lock(fuel_gauge_context_t *h)
{
    sem_wait(&h->ctx_mutext);
}

/**
 * Releases the fuel gauge mutex.
 *
 * @param <h> The fuel gauge context handle.
 * @return Nothing
 */
static inline void fuel_gauge_unlock(fuel_gauge_context_t *h)
{
    sem_post(&h->ctx_mutext);
}

/**
 * Acquires use of the fuel gauge.
 *
 * @param <h> The fuel gauge context handle.
 * @return Nothing
 */
static inline fuel_gauge_status_t fuel_gauge_acquire(fuel_gauge_context_t *h)
{
    fuel_gauge_status_t status;

    fuel_gauge_lock(h);
    if (!h->inuse)
    {
        h->inuse = 1;
        status = FUEL_GAUGE_STATUS_SUCCESS;
    }
    else
    {
        status = FUEL_GAUGE_STATUS_BUSY;
    }
    fuel_gauge_unlock(h);

    return status;
}

/**
 * Releases use of the fuel gauge.
 *
 * @param <h> The fuel gauge context handle.
 * @return Nothing
 */
static inline fuel_gauge_status_t fuel_gauge_release(fuel_gauge_context_t *h)
{
    fuel_gauge_status_t status;

    fuel_gauge_lock(h);
    if (h->inuse)
    {
        h->inuse = 0;
        status = FUEL_GAUGE_STATUS_SUCCESS;
    }
    else
    {
        status = FUEL_GAUGE_STATUS_INTERNAL_ERROR;
    }
    fuel_gauge_unlock(h);

    return status;
}

/**
 * MAX17040/17041 Fuel Gauge Registers
 */
#define FUEL_GAUGE_REG_VCELL            (0x02)  // RO Reports 12-bit A/D measurement of battery voltage.
#define FUEL_GAUGE_REG_SOC              (0x04)  // RO Reports 16-bit SOC result calculated by ModelGauge algorithm.
#define FUEL_GAUGE_REG_MODE             (0x06)  // WO Sends special commands to the IC.
#define FUEL_GAUGE_REG_VERSION          (0x08)  // RO Returns IC version.
#define FUEL_GAUGE_REG_RCOMP            (0x0c)  // RW Battery compensation. Adjusts IC performance based on application conditions.
#define FUEL_GAUGE_REG_COMMAND          (0xfe)  // WO Sends special commands to the IC.

/**
 * FUEL_GAUGE_REG_MODE
 */
#define FUEL_GAUGE_MODE_QUICK_START     (0x4000) // Quick Start

/**
 * FUEL_GAUGE_REG_RCOMP
 */
#define FUEL_GAUGE_RCOMP_DEFAULT        (0x9700) // Compensation value for ModelGauge algorithm

/**
 * FUEL_GAUGE_REG_COMMAND
 */
#define FUEL_GAUGE_COMMAND_POR          (0x5400) // Power On Reset

/**
 * Initialization function.
 *
 * @param <ph> A pointer to storage to receive the context handle
 * @return Returns FUEL_GAUGE_STATUS_SUCCESS or errorlevel.
 * @see fuel_gauge_uninit
 */
fuel_gauge_status_t fuel_gauge_init(fuel_gauge_handle_t *ph);

/**
 * Un-initialization/cleanup function.
 *
 * @param <h> The context handle.
 * @return Returns FUEL_GAUGE_STATUS_SUCCESS or errorlevel.
 * @see fuel_gauge_init
 */
fuel_gauge_status_t fuel_gauge_uninit(fuel_gauge_handle_t h);

/**
 * Returns a string that describes the given status code.
 *
 * @param <status> The fuel_gauge_status_t value.
 * @param <ppsstatus> Pointer to receive the status code description string
 * @return Returns FUEL_GAUGE_STATUS_SUCCESS or errorlevel.
 */
fuel_gauge_status_t fuel_gauge_status_string(const fuel_gauge_status_t status,
        const char **ppsstatus);

/**
 * Opens the Fuel Gauge i2c device for communication.
 *
 * @param <h> The fuel gauge context handle.
 * @param <i2c_devnr> The i2c adapter number, i.e., 2 for /dev/i2c-2
 * @return Returns FUEL_GAUGE_STATUS_SUCCESS or errorlevel.
 * @see fuel_gauge_close_dev
 */
fuel_gauge_status_t fuel_gauge_open_dev(fuel_gauge_handle_t h,
        int i2c_devnr);

/**
 * Closes the Fuel Gauge i2c device.
 *
 * @param <h> The fuel gauge context handle.
 * @return Returns FUEL_GAUGE_STATUS_SUCCESS or errorlevel.
 * @see fuel_gauge_open_dev
 */
fuel_gauge_status_t fuel_gauge_close_dev(fuel_gauge_handle_t h);

/**
 * Retrieves the Fuel Gauge battery voltage, in mV
 *
 * @param <h> The fuel gauge context handle.
 * @param <pvoltage> Pointer to receive the voltage.
 * @return Returns FUEL_GAUGE_STATUS_SUCCESS or errorlevel.
 */
fuel_gauge_status_t fuel_gauge_get_battery_voltage(fuel_gauge_handle_t h,
        uint16_t *pvoltage
);

/**
 * Retrieves the Fuel Gauge device charge percent
 *
 * @param <h> The fuel gauge context handle.
 * @param <ppercent> Pointer to receive the charge percent.
 * @return Returns FUEL_GAUGE_STATUS_SUCCESS or errorlevel.
 */
fuel_gauge_status_t fuel_gauge_get_charge_percent(fuel_gauge_handle_t h,
        uint8_t *ppercent
);

/**
 * Tells the Fuel Gauge to restart calculations in same manner as power-up.
 *
 * @param <h> The fuel gauge context handle.
 * @return Returns FUEL_GAUGE_STATUS_SUCCESS or errorlevel.
 */
fuel_gauge_status_t fuel_gauge_quick_start(fuel_gauge_handle_t h);

/**
 * Retrieves the Fuel Gauge version.
 *
 * @param <h> The fuel gauge context handle.
 * @param <pversion> Pointer to receive the MAX17040/17041 product version.
 * @return Returns FUEL_GAUGE_STATUS_SUCCESS or errorlevel.
 */
fuel_gauge_status_t fuel_gauge_get_version(fuel_gauge_handle_t h,
        uint16_t *pversion
);

/**
 * Retrieves the Fuel Gauge compensation value.
 *
 * @param <h> The fuel gauge context handle.
 * @param <prcomp> Pointer to receive the compensation value.
 * @return Returns FUEL_GAUGE_STATUS_SUCCESS or errorlevel.
 */
fuel_gauge_status_t fuel_gauge_get_rcomp(fuel_gauge_handle_t h,
        uint16_t *prcomp
);

/**
 * Sets the Fuel Gauge compensation value.
 *
 * @param <h> The fuel gauge context handle.
 * @param <rcomp> Compensation value to set.
 * @return Returns FUEL_GAUGE_STATUS_SUCCESS or errorlevel.
 */
fuel_gauge_status_t fuel_gauge_set_rcomp(fuel_gauge_handle_t h,
        uint16_t rcomp
);

/**
 * Tells the Fuel Gauge to completely reset as if power had been removed.
 *
 * @param <h> The fuel gauge context handle.
 * @return Returns FUEL_GAUGE_STATUS_SUCCESS or errorlevel.
 */
fuel_gauge_status_t fuel_gauge_power_on_reset(fuel_gauge_handle_t h);


extern void fuelModelCleanUp();
#endif // #ifndef __FUEL_GAUGE_H_
