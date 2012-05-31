/* CPPDOC_BEGIN_EXCLUDE  */

//******************************************************************************
//******************************************************************************
//
// FILE:        fuel_gauge.c
//
// CLASSES:     
//
// DESCRIPTION: Public Apple MFi Authentication Library functions.
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
#include <memory.h>

#include "fuel_gauge.h"
#include "reconn_i2c.h"

static fuel_gauge_status_t fuel_gauge_init_context(fuel_gauge_handle_t h);

//******************************************************************************
// FUNCTION:  fuel_gauge_init
//
// CREATED:   04/03/2012 by stephen.smith@intelligraphics.com
//******************************************************************************
fuel_gauge_status_t fuel_gauge_init(fuel_gauge_handle_t *ph)
{
    fuel_gauge_handle_t h;
    fuel_gauge_status_t status;

    if (!ph)
    {
        return FUEL_GAUGE_STATUS_INVALID_PARAM;
    }

    // allocate the fuel gauge context
    h = malloc(sizeof(fuel_gauge_context_t));
    if (!h)
    {
        return FUEL_GAUGE_STATUS_NO_MEM;
    }
    memset(h, 0, sizeof(fuel_gauge_context_t));

    // initialize the fuel gauge os objects
    sem_init(&h->ctx_mutext, 0, 1);

    // initialize the context items
    status = fuel_gauge_init_context(h);

    *ph = h;

    return status;
}

//******************************************************************************
// FUNCTION:  fuel_gauge_uninit
//
// CREATED:   04/03/2012 by stephen.smith@intelligraphics.com
//******************************************************************************
fuel_gauge_status_t fuel_gauge_uninit(fuel_gauge_handle_t h)
{
    fuel_gauge_status_t status;

    if (!h)
    {
        return FUEL_GAUGE_STATUS_INVALID_PARAM;
    }

    // use the fuel gauge
    status = fuel_gauge_acquire(h);
    if (status != FUEL_GAUGE_STATUS_SUCCESS)
    {
        return status;
    }

    // destroy the os objects
    sem_destroy(&h->ctx_mutext);

    // free the fuel gauge context
    free(h);

    return FUEL_GAUGE_STATUS_SUCCESS;
}

//******************************************************************************
// FUNCTION:  fuel_gauge_status_string
//
// CREATED:   04/03/2012 by stephen.smith@intelligraphics.com
//******************************************************************************
fuel_gauge_status_t fuel_gauge_status_string(const fuel_gauge_status_t status,
        const char **ppsstatus)
{
    static struct
    {
        const fuel_gauge_status_t status;
        const char *s;
    } ss[] = {
        { FUEL_GAUGE_STATUS_SUCCESS, "Success" },
        { FUEL_GAUGE_STATUS_ERROR, "Error" },
        { FUEL_GAUGE_STATUS_BUSY, "Busy" },
        { FUEL_GAUGE_STATUS_PENDING, "Pending" },
        { FUEL_GAUGE_STATUS_INTERNAL_ERROR, "Internal Error" },
        { FUEL_GAUGE_STATUS_INVALID_PARAM, "Invalid parameter" },
        { FUEL_GAUGE_STATUS_NO_MEM, "Out of memory" },
        { FUEL_GAUGE_STATUS_NO_ENT, "No entry" },
        { FUEL_GAUGE_STATUS_IO_ERR, "I/O Error" },
        { FUEL_GAUGE_STATUS_TIMEOUT, "Timeout" },
        { FUEL_GAUGE_STATUS_UNEXPECTED, "Unexpected" },
        { FUEL_GAUGE_STATUS_BUFFER_TOO_SMALL, "Buffer too small" },
        { FUEL_GAUGE_STATUS_FILE_ALREADY_OPEN, "File already opened" },
        { FUEL_GAUGE_STATUS_AGAIN, "Try again" },
        { FUEL_GAUGE_STATUS_NOT_VALID, "Not valid" }
    };
    int i;

    if (!ppsstatus)
    {
        return FUEL_GAUGE_STATUS_INVALID_PARAM;
    }

    // walk through and retrieve the error string for the given code
    for (i = 0; i < (int) (sizeof(ss) / sizeof(ss[0])); i++)
    {
        if (ss[i].status == status)
        {
            *ppsstatus = ss[i].s;
            return FUEL_GAUGE_STATUS_SUCCESS;
        }
    }

    // oops, didn't find the string, return something
    *ppsstatus = "Unknown error";
    return FUEL_GAUGE_STATUS_NO_ENT;
}

//******************************************************************************
// FUNCTION:  fuel_gauge_init_context
//
// CREATED:   04/03/2012 by stephen.smith@intelligraphics.com
//******************************************************************************
static fuel_gauge_status_t fuel_gauge_init_context(fuel_gauge_handle_t h)
{
    h->fh = -1;

    return FUEL_GAUGE_STATUS_SUCCESS;
}

//******************************************************************************
// FUNCTION:  fuel_gauge_open_dev
//
// CREATED:   04/03/2012 by stephen.smith@intelligraphics.com
//******************************************************************************
fuel_gauge_status_t fuel_gauge_open_dev(fuel_gauge_handle_t h,
        int i2c_devnr)
{
    fuel_gauge_status_t status;

    if (!h)
    {
        return FUEL_GAUGE_STATUS_INVALID_PARAM;
    }

    // use the fuel gauge
    status = fuel_gauge_acquire(h);
    if (status != FUEL_GAUGE_STATUS_SUCCESS)
    {
        return status;
    }

    // make sure the context is initialized
    fuel_gauge_init_context(h);

    // open the i2c device
    h->fh = reconn_i2c_open_dev(i2c_devnr, FUEL_GAUGE_I2C_ADDRESS);
    if (h->fh == -1)
    {
        status = FUEL_GAUGE_STATUS_IO_ERR;
    }

    // release the fuel gauge
    fuel_gauge_release(h);

    return status;
}

//******************************************************************************
// FUNCTION:  fuel_gauge_close_dev
//
// CREATED:   04/03/2012 by stephen.smith@intelligraphics.com
//******************************************************************************
fuel_gauge_status_t fuel_gauge_close_dev(fuel_gauge_handle_t h)
{
    fuel_gauge_status_t status;

    if (!h)
    {
        return FUEL_GAUGE_STATUS_INVALID_PARAM;
    }

    // use the fuel gauge
    status = fuel_gauge_acquire(h);
    if (status != FUEL_GAUGE_STATUS_SUCCESS)
    {
        return status;
    }

    // close the device
    status = reconn_i2c_close_dev(h->fh);

    // re-initialize the fuel gauge context
    fuel_gauge_init_context(h);

    // release the fuel gauge
    fuel_gauge_release(h);

    return status;
}

//******************************************************************************
// FUNCTION:  fuel_gauge_get_fh
//
// CREATED:   04/03/2012 by stephen.smith@intelligraphics.com
//******************************************************************************
fuel_gauge_status_t fuel_gauge_get_fh(fuel_gauge_handle_t h, int *pfh)
{
    fuel_gauge_status_t status;

    if ((!h) || (!pfh))
    {
        return FUEL_GAUGE_STATUS_INVALID_PARAM;
    }

    // use the fuel gauge
    status = fuel_gauge_acquire(h);
    if (status != FUEL_GAUGE_STATUS_SUCCESS)
    {
        return status;
    }

    *pfh = h->fh;

    // release the fuel gauge
    fuel_gauge_release(h);

    return FUEL_GAUGE_STATUS_SUCCESS;
}

//******************************************************************************
// FUNCTION:  fuel_gauge_get_battery_voltage
//
// CREATED:   04/03/2012 by stephen.smith@intelligraphics.com
//******************************************************************************
fuel_gauge_status_t fuel_gauge_get_battery_voltage(fuel_gauge_handle_t h,
        uint16_t *pvoltage
)
{
    fuel_gauge_status_t status;
    uint16_t val;
    int ret;

    if (!h)
    {
        return FUEL_GAUGE_STATUS_INVALID_PARAM;
    }

    // use the fuel gauge
    status = fuel_gauge_acquire(h);
    if (status != FUEL_GAUGE_STATUS_SUCCESS)
    {
        return status;
    }

    // read the data from the fuel gauge
    ret = reconn_i2c_read_reg(h->fh, FUEL_GAUGE_REG_VCELL, (uint8_t *) &val, sizeof(uint16_t));
    if (ret == sizeof(uint16_t))
    {
        // UNITS: 1.25v for MAX17040, 2.50v for MAX17041
        *pvoltage = FUEL_GAUGE_NTOHS(val) >> 4;
    }
    else
    {
        status = FUEL_GAUGE_STATUS_IO_ERR;
    }

    // release the fuel gauge
    fuel_gauge_release(h);

    return status;
}

//******************************************************************************
// FUNCTION:  fuel_gauge_get_charge_percent
//
// CREATED:   04/03/2012 by stephen.smith@intelligraphics.com
//******************************************************************************
fuel_gauge_status_t fuel_gauge_get_charge_percent(fuel_gauge_handle_t h,
        uint8_t *ppercent
)
{
    fuel_gauge_status_t status;
    uint16_t val;
    int ret;

    if (!h)
    {
        return FUEL_GAUGE_STATUS_INVALID_PARAM;
    }

    // use the fuel gauge
    status = fuel_gauge_acquire(h);
    if (status != FUEL_GAUGE_STATUS_SUCCESS)
    {
        return status;
    }

    // read the data from the fuel gauge
    ret = reconn_i2c_read_reg(h->fh, FUEL_GAUGE_REG_SOC, (uint8_t *) &val, sizeof(uint16_t));
    if (ret == sizeof(uint16_t))
    {
        // MSB %, LSB 1/256%
        *ppercent = (uint8_t) (FUEL_GAUGE_NTOHS(val) >> 8);
    }
    else
    {
        status = FUEL_GAUGE_STATUS_IO_ERR;
    }

    // release the fuel gauge
    fuel_gauge_release(h);

    return status;
}

//******************************************************************************
// FUNCTION:  fuel_gauge_quick_start
//
// CREATED:   04/03/2012 by stephen.smith@intelligraphics.com
//******************************************************************************
fuel_gauge_status_t fuel_gauge_quick_start(fuel_gauge_handle_t h)
{
    fuel_gauge_status_t status;
    uint16_t val;
    int ret;

    if (!h)
    {
        return FUEL_GAUGE_STATUS_INVALID_PARAM;
    }

    // use the fuel gauge
    status = fuel_gauge_acquire(h);
    if (status != FUEL_GAUGE_STATUS_SUCCESS)
    {
        return status;
    }

    // write the data to the fuel gauge
    val = FUEL_GAUGE_HTONS(FUEL_GAUGE_MODE_QUICK_START);
    ret = reconn_i2c_write_reg(h->fh, FUEL_GAUGE_REG_MODE, (uint8_t *) &val, sizeof(uint16_t));
    if (ret != sizeof(uint16_t))
    {
        status = FUEL_GAUGE_STATUS_IO_ERR;
    }

    // release the fuel gauge
    fuel_gauge_release(h);

    return status;
}

//******************************************************************************
// FUNCTION:  fuel_gauge_get_version
//
// CREATED:   04/03/2012 by stephen.smith@intelligraphics.com
//******************************************************************************
fuel_gauge_status_t fuel_gauge_get_version(fuel_gauge_handle_t h,
        uint16_t *pversion
)
{
    fuel_gauge_status_t status;
    uint16_t val;
    int ret;

    if (!h)
    {
        return FUEL_GAUGE_STATUS_INVALID_PARAM;
    }

    // use the fuel gauge
    status = fuel_gauge_acquire(h);
    if (status != FUEL_GAUGE_STATUS_SUCCESS)
    {
        return status;
    }

    // read the data from the fuel gauge
    ret = reconn_i2c_read_reg(h->fh, FUEL_GAUGE_REG_VERSION, (uint8_t *) &val, sizeof(uint16_t));
    if (ret == sizeof(uint16_t))
    {
        *pversion = FUEL_GAUGE_NTOHS(val);
    }
    else
    {
        status = FUEL_GAUGE_STATUS_IO_ERR;
    }

    // release the fuel gauge
    fuel_gauge_release(h);

    return status;

}

//******************************************************************************
// FUNCTION:  fuel_gauge_get_rcomp
//
// CREATED:   04/03/2012 by stephen.smith@intelligraphics.com
//******************************************************************************
fuel_gauge_status_t fuel_gauge_get_rcomp(fuel_gauge_handle_t h,
        uint16_t *prcomp
)
{
    fuel_gauge_status_t status;
    uint16_t val;
    int ret;

    if (!h)
    {
        return FUEL_GAUGE_STATUS_INVALID_PARAM;
    }

    // use the fuel gauge
    status = fuel_gauge_acquire(h);
    if (status != FUEL_GAUGE_STATUS_SUCCESS)
    {
        return status;
    }

    // read the data from the fuel gauge
    ret = reconn_i2c_read_reg(h->fh, FUEL_GAUGE_REG_RCOMP, (uint8_t *) &val, sizeof(uint16_t));
    if (ret == sizeof(uint16_t))
    {
        *prcomp = FUEL_GAUGE_NTOHS(val);
    }
    else
    {
        status = FUEL_GAUGE_STATUS_IO_ERR;
    }

    // release the fuel gauge
    fuel_gauge_release(h);

    return status;

}

//******************************************************************************
// FUNCTION:  fuel_gauge_set_rcomp
//
// CREATED:   04/03/2012 by stephen.smith@intelligraphics.com
//******************************************************************************
fuel_gauge_status_t fuel_gauge_set_rcomp(fuel_gauge_handle_t h,
        uint16_t rcomp
)
{
    fuel_gauge_status_t status;
    uint16_t val;
    int ret;

    if (!h)
    {
        return FUEL_GAUGE_STATUS_INVALID_PARAM;
    }

    // use the fuel gauge
    status = fuel_gauge_acquire(h);
    if (status != FUEL_GAUGE_STATUS_SUCCESS)
    {
        return status;
    }

    // write the data to the fuel gauge
    val = FUEL_GAUGE_HTONS(rcomp);
    ret = reconn_i2c_write_reg(h->fh, FUEL_GAUGE_REG_RCOMP, (uint8_t *) &val, sizeof(uint16_t));
    if (ret != sizeof(uint16_t))
    {
        status = FUEL_GAUGE_STATUS_IO_ERR;
    }

    // release the fuel gauge
    fuel_gauge_release(h);

    return status;

}

//******************************************************************************
// FUNCTION:  fuel_gauge_power_on_reset
//
// CREATED:   04/03/2012 by stephen.smith@intelligraphics.com
//******************************************************************************
fuel_gauge_status_t fuel_gauge_power_on_reset(fuel_gauge_handle_t h)
{
    fuel_gauge_status_t status;
    uint16_t val;
    int ret;

    if (!h)
    {
        return FUEL_GAUGE_STATUS_INVALID_PARAM;
    }

    // use the fuel gauge
    status = fuel_gauge_acquire(h);
    if (status != FUEL_GAUGE_STATUS_SUCCESS)
    {
        return status;
    }

    // write the data to the fuel gauge
    val = FUEL_GAUGE_HTONS(FUEL_GAUGE_COMMAND_POR);
    ret = reconn_i2c_write_reg(h->fh, FUEL_GAUGE_REG_COMMAND, (uint8_t *) &val, sizeof(uint16_t));
    if (ret != sizeof(uint16_t))
    {
        status = FUEL_GAUGE_STATUS_IO_ERR;
    }

    // release the fuel gauge
    fuel_gauge_release(h);

    return status;
}

