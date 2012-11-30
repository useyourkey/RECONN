#pragma once
#include "aov_opts.h"
#ifdef AOV_DEPEND_ENG
#ifdef __cplusplus
extern "C" {
#endif
/*************************************************************************//**
 * @internal
 * @file aov_eng.h
 * @author Avcom of Virginia
 *
 * @section LICENSE
 *
 * This software is Copyright (c) 2011-2012 Avcom of Virginia.
 * All Rights Reserved.
 *
 * @section DESCRIPTION
 *
 * The functions in this section are confidential and are for internal use
 * only. Unauthorized use of these functions is against Avcom of Virginia's
 * terms of service.
 *
 *****************************************************************************/

#ifndef _AVCOM_ENG_V2_H_
#define _AVCOM_ENG_V2_H_

#include "aov_common.h"

/*! Structure is used for storing the engineering version of struct aov_band_info */
struct aov_eng_band_info {
	int					phy_input;					//!< Physical Input the band uses
	int					IF;							//!< Internal IF associated with the band
	char				name[AOV_BAND_NAME_LEN];	//!< User configurable identifying name
	unsigned int		min_freq;					//!< Maximum frequency range
	unsigned int		max_freq;					//!< Minimum frequency range
//	int					min_step;					//!< Maximum step size allowed(and frequency must be in multiples of this number
//	short				min_db;						//!< Minimum power level this band can detect
	short				max_db;						//!< Maximum power level this band can detect (Do not exceed this power level!)
	short				min_reflvl;					//!< Minimum reference level this band can handle
	short				max_reflvl;					//!< Maximum reference level this band can handle
};

struct aov_rbwcaldata {
	float				m;
	float				b;
	unsigned short		logoff;
	unsigned short		ifagc;
	unsigned short		wait;
};


/*! Structure is used for retrieving raw data from the SBS-2 
 *  After a DDS or PLL sweep */
typedef struct sAVCOM_ENG_SweepData_Row_ {
    unsigned short	data;		//!< Raw ADC Data
	double			frequency; //!< Frequency in MHz
} sAVCOM_ENG_SweepData_Row;

typedef 
/*! Typedef to define a 4K buffer of data / frequency pairs */
sAVCOM_ENG_SweepData_Row	sAVCOM_ENG_SweepData_Array[4096]; 


/*!Main structure for retrieving SA frequency/power from the SBS-2 
 * via top-level API calls*/
typedef struct	sAVCOM_ENG_SweepData_ {
	unsigned short				count;	//!< Count of the data points in the sweep (and thus the array)
	double						fStart;	//!< Sweep Start Frequency in MHz
	double						fStop;	//!< Sweep Stop Frequency in MHz
	double						fStep;	//!< Sweep Step Frequency in MHz
	sAVCOM_ENG_SweepData_Array	sweep;	//!< Array of sampled power at each frequency step in the sweep
} sAVCOM_ENG_SweepData;

/****************            CONSTANTS             ***************************/

#define AOV_ADC_MIN_VAL				0		// Minumum value for ADCs
#define AOV_ADC_MAX_VAL				0x03FF	// Maximum value for ADCs

#define AOV_20DB_ATTN_OFF			0		// Option for AVCOM_ENG_Set20DB()
#define AOV_20DB_ATTN_ON			1		// Option for AVCOM_ENG_Set20DB()

#define AOV_IF_915MHZ				0		// Option for AVCOM_ENG_SetIF()
#define AOV_IF_70MHZ				1		// Option for AVCOM_ENG_SetIF()

#define AOV_RBW_3KHZ				0		// For setting RBW
#define AOV_RBW_10KHZ				1		// For setting RBW
#define AOV_RBW_30KHZ				2		// For setting RBW
#define AOV_RBW_300KHZ				3		// For setting RBW
#define AOV_RBW_1MHZ				4		// For setting RBW

#define AOV_VBW_30KHZ				2		// For setting VBW
#define AOV_VBW_3KHZ				0		// For setting VBW
#define AOV_VBW_300HZ				1		// For setting VBW
#define AOV_VBW_300KHZ				3		// For setting VBW			


/********************************************************************************
 *																				*
 *				AVCOM_ENG_BootBootloader()										*
 *																				*
 ****************************************************************************//**
 * Shuts down the analyzer and boots it into bootloader mode
 * 
 * @note prerequisite for flashing new firmware. 
 * @note Must be running in normal analyzer mode
 * 
 * @return Status of function call
 *
 * @param [in,out]	handle	handle
 *******************************************************************************/
EXPORT int AVCOM_ENG_BootBootloader ( void *handle );


/********************************************************************************
 *																				*
 *				AVCOM_ENG_FlashSBS2bootloader()									*
 *																				*
 ****************************************************************************//**
 * Flashes the sbs2's bootloader program. 
 * 
 * This program is only accessible when running the uartloader program (inital 
 * programming).
 * 
 * @note Engineering initial setup command
 * 
 * @return Status of function call
 *
 * @param [in,out]	handle	handle
 * @param [in]		data	pointer to array of new firmware data
 * @param [in]		data_len	length of the data array
 ***************************************************************************/
EXPORT int AVCOM_ENG_FlashSBS2bootloader ( void *handle, unsigned char* data, int data_len );


/********************************************************************************
 *																				*
 *				AVCOM_ENG_FlashSBS2init()										*
 *																				*
 ****************************************************************************//**
 * Flashes the sbs2's init_code loader program.
 * 
 * This program is only accessible when running the uartloader program (inital 
 * programming).
 * 
 * @note Engineering initial setup command
 * 
 * @return Status of function call
 *
 * @param [in,out]	handle	handle
 * @param [in]		data	pointer to array of new firmware data
 * @param [in]		data_len	length of the data array
 *******************************************************************************/
EXPORT int AVCOM_ENG_FlashSBS2init ( void *handle, unsigned char* data, int data_len );


/********************************************************************************
*																				*
*		AVCOM_ENG_GetADCValue													*
*																				*
*****************************************************************************//**
 * Retrieve the ADC value (Raw value)
 * 
 * @return Status of function call
 *
 * @param [in,out]	handle	handle
 * @param [out]	value	ADC Value
 *******************************************************************************/
EXPORT int AVCOM_ENG_GetADCValue ( void *handle, unsigned short *value );


/********************************************************************************
 *																				*
 *				AVCOM_ENG_GetBootloaderVersion()								*
 *																				*
 ****************************************************************************//**
 * Retrieve the version number of the bootloader
 * 
 * @note: Must be in bootloader mode to work
 * 
 * Stage follows the representation:
 * <ul>
 * <li>0: alpha
 * <li>1: beta
 * <li>2: release candidate
 * <li>3: release
 * </ul>
 * 
 * Format goes: MAJOR.MINOR STAGE BUILD such as:
 * \li \c 1.0  
 * \li \c 1.1b1  
 * \li \c 1.1b2  
 * \li \c 1.1rc1  
 * \li \c 1.1 
 * 
 * @return Status of function call
 *
 * @param [in,out]	handle	handle
 * @param [out]	ver	Structure that holds the major, minor, build, stage, 
 *					and string of version
 *******************************************************************************/
EXPORT int AVCOM_ENG_GetBootloaderVersion ( void *handle, struct aov_version *ver );


/********************************************************************************
 *																				*
 *				AVCOM_ENG_GetCal20dB()											*
 *																				*
 ****************************************************************************//**
 * Gets the attenuation value of the 20dB pad
 * 
 * @return Status of function call
 *
 * @param [in,out]	handle	handle
 * @param [out]		db		actual dBm value of 20dB pad
 *******************************************************************************/
EXPORT int AVCOM_ENG_GetCal20dB ( void *handle, double *db );


/********************************************************************************
 *																				*
 *				AVCOM_ENG_GetCalDSA()											*
 *																				*
 ****************************************************************************//**
 * Gets the attenuation value of the DSA
 * 
 * @return Status of function call
 *
 * @param [in,out]	handle	handle
 * @param [out]		dbarray	Array size 64 of actual DSA values 
 *******************************************************************************/
EXPORT int AVCOM_ENG_GetCalDSA ( void *handle, double *dbarray );


/********************************************************************************
 *																				*
 *				AVCOM_ENG_GetSweepADC()											*
 *																				*
 ****************************************************************************//**
 * Retrieves a single raw data trace from last PLL or DDS sweep. Data is 
 * passed through a defined pointer to an \c AVCOM_ENG_SweepData() structure.
 * Raw data is the data directly collected from the ADC with no adjustment
 *
 * @return Status of function call
 *
 * @param [in,out]	handle	Pointer to analyzer handle
 * @param [out]		trace	Pointer to structure with 2-D raw-data/frequency information
 * 
 * @deprecated Function may not be actively supported.
 *******************************************************************************/
EXPORT int AVCOM_ENG_GetSweepADC(void *handle, sAVCOM_ENG_SweepData *trace);
EXPORT int AVCOM_ENG_GetSweepADCArrays(void *handle, double* start, double* stop, double* step, unsigned short* num, double* freq,  unsigned short* data);



/********************************************************************************
 *																				*
 *				AVCOM_ENG_GetUartVersion()										*
 *																				*
 ****************************************************************************//**
 * Retrieve the version number of the uartloader
 * 
 * @note Must be in uartloader mode to work
 * 
 * Stage follows the representation:
 * <ul>
 * <li>0: alpha
 * <li>1: beta
 * <li>2: release candidate
 * <li>3: release
 * </ul>
 * 
 * Format goes: MAJOR.MINOR STAGE BUILD such as:
 * \li \c 1.0  
 * \li \c 1.1b1  
 * \li \c 1.1b2  
 * \li \c 1.1rc1  
 * \li \c 1.1 
 * 
 * @return Status of function call
 *
 * @param [in,out]	handle	handle
 * @param [out]	ver	Structure that holds the major, minor, build, stage, 
 *					and string of version
 *******************************************************************************/
EXPORT int AVCOM_ENG_GetUartVersion ( void *handle, struct aov_version *ver );

#ifdef AOV_DEPEND_USB
/********************************************************************************
 *																				*
 *				AVCOM_ENG_LoadUART()											*
 *																				*
 ****************************************************************************//**
 * When analyzer has the UART boot switch engaged, this program initiates and
 * loads the Uartloader program
 * 
 * @note Engineering initial setup use only
 * 
 * @return Status of function call
 *
 * @param [in,out]	handle	handle
 * @param [in]		data	pointer uartloader data array
 * @param [in]		data_len	Length of uartloader array
 *******************************************************************************/
EXPORT int AVCOM_ENG_LoadUART( void *handle, char *data, int data_len );
#endif /* AOV_DEPEND_USB */


/********************************************************************************
 *																				*
 *				AVCOM_ENG_Set20dB()												*
 *																				*
 ****************************************************************************//**
 * Sets/unsets the 20dB path
 *
 * Function is used to control the switch that enables/disables the 20dB
 * attenuation path on the L-Band side.
 * 
 * - \c True will enable the 20dB path
 * - \c False will disable the 20dB path
 * 
 * @return Status of function call
 *
 * @param [in,out]	handle	handle
 * @param [in]		value	state of the 20dB path
 * 
 * @deprecated Function operates but invalidates dBm accuracy
 *******************************************************************************/
EXPORT int AVCOM_ENG_Set20dB ( void *handle, int value );


/********************************************************************************
 *																				*
 *				AVCOM_ENG_SetBandInfo()											*
 *																				*
 ****************************************************************************//**
 * Set the Information/parameters for a provided band index number
 * 
 * @note	This function sets the property of the band but does not save into 
 *			non-volatile memory. Call \c AVCOM_ENG_SaveSettings() to save into
 *			non-volatile memory.
 *
 * @return Status of function call
 *
 * @param [in,out]	handle	Pointer to analyzer handle
 * @param [in]	num	index of the band to be set
 * @param [in]	info	structure containing the band's information/parameters
 * 
 * @warning Calling function may invalidate calibration
 *******************************************************************************/
EXPORT int AVCOM_ENG_SetBandInfo( void *handle, int num, struct aov_eng_band_info info );


/********************************************************************************
 *																				*
 *				AVCOM_ENG_SetCal20dB()											*
 *																				*
 ****************************************************************************//**
 * Tells the analyzer to perform calibration on the 20dB attenuation pad
 *
 * Function tells the analyzer to what frequency signal is on. Signal should be
 * at a proper power
 * 
 * @return Status of function call
 *
 * @param [in,out]	handle	handle
 * @param [in]		freq	Frequency in MHz where signal is 
 *******************************************************************************/
EXPORT int AVCOM_ENG_SetCal20dB ( void *handle, double freq );


/********************************************************************************
 *																				*
 *				AVCOM_ENG_SetCalAttn()											*
 *																				*
 ****************************************************************************//**
 * Tells the analyzer to perform calibration on attenuation (DSA and 20dB)
 *
 * Function tells the analyzer to what frequency signal is on. Signal should be
 * at a proper power
 * 
 * @return Status of function call
 *
 * @param [in,out]	handle	handle
 * @param [in]		freq	Frequency in MHz where signal is 
 *******************************************************************************/
EXPORT int AVCOM_ENG_SetCalAttn ( void *handle, double freq );


/********************************************************************************
 *																				*
 *				AVCOM_ENG_SetCalDSA()											*
 *																				*
 ****************************************************************************//**
 * Tells the analyzer to perform calibration on the DSA
 *
 * Function tells the analyzer to what frequency signal is on. Signal should be
 * at a proper power
 * 
 * @return Status of function call
 *
 * @param [in,out]	handle	handle
 * @param [in]		freq	Frequency in MHz where signal is 
 *******************************************************************************/
EXPORT int AVCOM_ENG_SetCalDSA ( void *handle, double freq );


/********************************************************************************
 *																				*
 *				AVCOM_ENG_SetDAC()												*
 *																				*
 ****************************************************************************//**
 * Sets a DAC to a 12-bit value
 *
 * Function is used to set any of the LTC1669 12-bit DAC's on the SBS2 board.
 * Valid dac ID's are:
 *     - 0 for Beacon Receiver
 *     - 1 for Microtune's IFAGC
 *     - 2 for Logamps DC Offset
 * 
 * @return Status of function call
 *
 * @param [in,out]	handle	handle
 * @param [in]	dac		ID of the DAC to set
 * @param [in]	value	12-bit value from 0-1023
 *******************************************************************************/
EXPORT int AVCOM_ENG_SetDAC ( void *handle, int dac, int value );


/********************************************************************************
 *																				*
 *				AVCOM_ENG_SetDDSFreq()											*
 *																				*
 ****************************************************************************//**
 * Sets DDS Frequency
 * 
 * @return Status of function call
 *
 * @param [in,out]		handle	handle
 * @param [in]	freq	Frequency to set the DDS in MHz
 *******************************************************************************/
EXPORT int AVCOM_ENG_SetDDSFreq ( void *handle, double freq );


/********************************************************************************
 *																				*
 *				AVCOM_ENG_SetDDSSSS()											*
 *																				*
 ****************************************************************************//**
 * Sets DDS Start, Stop, Step values
 *
 * Function is used to set the AD9913 DDS start, stop, step frequencies
 * 
 * @return Status of function call
 *
 * @param [in,out]		handle	handle
 * @param [in]	fStart	Start frequency
 * @param [in]	fStop	Stop frequency
 * @param [in]	fStep	Frequency step
 * 
 * @deprecated Function may not be actively supported.
 *******************************************************************************/
EXPORT int AVCOM_ENG_SetDDSSSS ( void *handle, double fStart, double fStop, double fStep );


/********************************************************************************
 *																				*
 *				AVCOM_ENG_SetDDSWait()											*
 *																				*
 ****************************************************************************//**
 * Sets DDS wait time 
 *
 * Function is used to set the manual wait time between frequency steps
 * 
 * @return Status of function call
 *
 * @param [in,out]		handle	handle
 * @param [in]	tWait	Wait Time in microseconds
 * 
 * @deprecated Function may not be actively supported.
 *******************************************************************************/
EXPORT int AVCOM_ENG_SetDDSWait ( void *handle, unsigned int tWait);


/********************************************************************************
 *																				*
 *				AVCOM_ENG_SetDSA()												*
 *																				*
 ****************************************************************************//**
 * Sets the PE4302 digital step attenuator on the L-Band Path to 
 * the desired value. 
 *
 * Valid range is \c 0 to \c 31.5dB in \c 0.5 step increments.
 * 
 * @return Status of function call
 *
 * @param [in,out]	handle	handle
 * @param [in]		value	Amount of Attenuation
 * 
 * @deprecated Function operates but invalidates dBm accuracy
 *******************************************************************************/
EXPORT int AVCOM_ENG_SetDSA ( void *handle, double value );


/********************************************************************************
 *																				*
 *				AVCOM_ENG_SetGlobalWait()										*
 *																				*
 ****************************************************************************//**
 * Sets a global wait time which is in addition to any specific PLL or DDS wait time
 *
 * Function is used to set the manual wait time between frequency steps
 * 
 * @return Status of function call
 *
 * @param [in,out]		handle	handle
 * @param [in]	tWait	Wait Time in microseconds
 *******************************************************************************/
EXPORT int AVCOM_ENG_SetGlobalWait ( void *handle, unsigned int tWait);


/********************************************************************************
 *																				*
 *				AVCOM_ENG_SetHwDesc()											*
 *																				*
 ****************************************************************************//**
 * Function sets the HW Description for the device. Allocate the 
 * \c sAOV_HW_DESC structure in advance.
 * 
 * @note Not Currently Implemented in Analyzer
 *
 * @return Status of function call
 *
 * @param [in,out]	handle	Pointer to analyzer handle
 * @param [in,out]	vphw	Pointer to the Hardware Description Record
 * 
 * @note This will only work if the analyzer is in calibration mode 
 * @warning Not implemented
 *******************************************************************************/
EXPORT int AVCOM_ENG_SetHwDesc( void *handle, void *vphw );


/********************************************************************************
 *																				*
 *				AVCOM_ENG_SetIF()												*
 *																				*
 ****************************************************************************//**
 * Sets the IF Switch between 915MHz and 70MHz.
 *
 * This command sets the IF switch but does not change any other setting. 
 * The Microtunes' settings must be changed requestingly.
 *
 * 0 sets the 915MHz IF Path
 * 1 sets the 70MHz IF Path
 * 
 * @return Status of function call
 *
 * @param [in,out]	handle	handle
 * @param [in]		value	State of IF Switch
 * 
 * @deprecated Function may not be actively supported.
 *******************************************************************************/
EXPORT int AVCOM_ENG_SetIF ( void *handle, int value );


/********************************************************************************
 *																				*
 *				AVCOM_ENG_SetMAC()												*
 *																				*
 ****************************************************************************//**
 * Set the MAC Address 
 * This command is recognized in the uartloader and bootloader only.
 * 
 * @note This is an Engineering command for initial setup. 
 * 
 * @return Status of function call
 *
 * @param [in,out]	handle	handle
 * @param [in]		mac		MAC address (6-bytes)
 *******************************************************************************/
EXPORT int AVCOM_ENG_SetMAC ( void *handle, unsigned char* mac );


/********************************************************************************
 *																				*
 *				AVCOM_ENG_SetModelName()										*
 *																				*
 ****************************************************************************//**
 * Function sets the Model Name for the device.
 * 
 * @return Status of function call
 *
 * @param [in,out]	handle	Pointer to analyzer handle
 * @param [in,out]	name	Model name of analyzer (32 ASCII char)
 * 
 *******************************************************************************/
EXPORT int AVCOM_ENG_SetModelName( void *handle, char *name );


/********************************************************************************
 *																				*
 *				AVCOM_ENG_SetNumBands()											*
 *																				*
 ****************************************************************************//**
 * Set the number of bands e analyzer can handle.
 * 
 * @note	This function sets the number of available bands but does not save into 
 *			non-volatile memory. Call AVCOM_ENG_SaveSettings() to save into
 *			non-volatile memory.
 *
 * @return Status of function call
 *
 * @param [in,out]	handle	Pointer to analyzer handle
 * @param [in]	num	Number of available bands
 * 
 * @warning Calling function may invalidate calibration
 *******************************************************************************/
EXPORT int AVCOM_ENG_SetNumBands( void *handle, int num );


/********************************************************************************
 *																				*
 *				AVCOM_ENG_SetPLLFreq()											*
 *																				*
 ****************************************************************************//**
 * Sets PLL to a single frequency
 * 
 * @return Status of function call
 *
 * @param [in,out]		handle	handle
 * @param [in]	freq	Frequency to set the PLL
 *******************************************************************************/
EXPORT int AVCOM_ENG_SetPLLFreq ( void *handle, double freq );


/********************************************************************************
 *																				*
 *				AVCOM_ENG_SetPLLFreqReturnLock()								*
 *																				*
 ****************************************************************************//**
 * Sets PLL to a single frequency and returns the lock time in microseconds
 * 
 * @return Status of function call
 *
 * @param [in,out]		handle	handle
 * @param [in]	freq	Frequency to set the PLL
 * @param [out]	lock	Lock time in us
 *******************************************************************************/
EXPORT int AVCOM_ENG_SetPLLFreqReturnLock ( void *handle, double freq, double *lock );


/********************************************************************************
 *																				*
 *				AVCOM_ENG_SetPLLLDCheck()										*
 *																				*
 ****************************************************************************//**
 * Enable or disable the PLL Lock Detection check
 * - \c 0: Disables lock detection check
 * - \c 1: Enables lock detection check
 * 
 * @return Status of function call
 *
 * @param [in,out]		handle	handle
 * @param [in]	value	value for lock detection check
 *******************************************************************************/
EXPORT int AVCOM_ENG_SetPLLLDCheck ( void *handle, int value );


/********************************************************************************
 *																				*
 *				AVCOM_ENG_SetPLLLDP()											*
 *																				*
 ****************************************************************************//**
 * Sets the Lock Detect Precision (LDP) of the PLL
 * - \c 0: 24 consecutive PFD cycles of 15ns
 * - \c 1: 40 consecutive PFD cycles of 15ns  
 * 
 * @note \li Refer to the ADF4157 datasheet for more information
 * 
 * @return Status of function call
 *
 * @param [in,out]		handle	handle
 * @param [in]	value	value for lock detection Precision
 *******************************************************************************/
EXPORT int AVCOM_ENG_SetPLLLDP ( void *handle, int value );


/********************************************************************************
 *																				*
 *				AVCOM_ENG_SetPLLSSS()											*
 *																				*
 ****************************************************************************//**
 * Sets PLL Start, Stop, Step values
 *
 * Function is used to set the ADF4154 PLL start, stop, step frequencies
 * 
 * @return Status of function call
 *
 * @param [in,out]		handle	handle
 * @param [in]	fStart	Start frequency
 * @param [in]	fStop	Stop frequency
 * @param [in]	fStep	Frequency step
 *******************************************************************************/
EXPORT int AVCOM_ENG_SetPLLSSS ( void *handle, double fStart, double fStop, double fStep );


/********************************************************************************
 *																				*
 *				AVCOM_ENG_SetPLLWait()											*
 *																				*
 ****************************************************************************//**
 * Sets PLL wait time 
 *
 * Function is used to set the manual wait time between PLL frequency steps
 * 
 * @return Status of function call
 *
 * @param [in,out]		handle	handle
 * @param [in]	tWait	Wait Time in microseconds
 * 
 * @deprecated Function may not be actively supported.
 *******************************************************************************/
EXPORT int AVCOM_ENG_SetPLLWait ( void *handle, unsigned int tWait);


/********************************************************************************
 *																				*
 *				AVCOM_ENG_SetProductID()										*
 *																				*
 ****************************************************************************//**
 * Function sets the product ID code.
 * 
 * @return Status of function call
 *
 * @param [in,out]	handle	Pointer to analyzer handle
 * @param [in,out]	id	Product ID Code
 *******************************************************************************/
EXPORT int AVCOM_ENG_SetProductID( void *handle, short id );


/********************************************************************************
 *																				*
 *				AVCOM_ENG_SetRBWCal()											*
 *																				*
 ****************************************************************************//**
 * Set calibration for an RBW. Values must be specified - cannot retain settings
 * 
 * @return Status of function call
 *
 * @param [in,out]		handle	handle
 * @param [in]	rbw	RBW to apply settings to
 * @param [in]	caldata	Structure containing cailbration data
 *******************************************************************************/
EXPORT int AVCOM_ENG_SetRBWCal (void *handle, int rbw, struct aov_rbwcaldata *caldata );


/********************************************************************************
 *																				*
 *				AVCOM_ENG_SetRBWmux()											*
 *																				*
 ****************************************************************************//**
 * Sets the RBW MUX'es
 *
 * This command sets the multiplexers that control the RBW's. No other action
 * is taken. Depending on the RBW's center frequency, The Microtunes' settings 
 * may need to be changed requestingly.
 *
 * - \c 0 sets the 3KHz RBW Filter
 * - \c 1 sets the 10KHz RBW Filter
 * - \c 2 sets the 30KHz RBW Filter
 * - \c 3 sets the 300KHz RBW Filter
 * - \c 4 sets the 1MHz RBW Filter
 * 
 * @return Status of function call
 *
 * @param [in,out]	handle	handle
 * @param [in]		value	RBW Selection
 * 
 * @deprecated Function may not be actively supported.
 *******************************************************************************/
EXPORT int AVCOM_ENG_SetRBWmux ( void *handle, int value );


/********************************************************************************
 *																				*
 *				AVCOM_ENG_SetSerialNumber()										*
 *																				*
 ****************************************************************************//**
 * Function sets the serial number for the device.
 * 
 * @return Status of function call
 *
 * @param [in,out]	handle	Pointer to analyzer handle
 * @param [in,out]	serial	Serial number (32 ASCII char)
 *******************************************************************************/
EXPORT int AVCOM_ENG_SetSerialNumber( void *handle, char *serial );


/********************************************************************************
 *																				*
 *				AVCOM_ENG_SetUtuneLO()											*
 *																				*
 ****************************************************************************//**
 * Sets the Microtune's LO Frequencies
 *
 * This command sets the LO1 and LO2 of the Microtune 2068. 
 *
 * - \c 0 sets the Microtune to \c LO_915_TO_44
 * - \c 1 sets the Microtune to \c LO_915_TO_45
 * - \c 2 sets the Microtune to \c LO_70_TO_44
 * - \c 3 sets the Microtune to \c LO_70_TO_45
 * 
 * @return Status of function call
 *
 * @param [in,out]	handle	handle
 * @param [in]		value	Microtune Setting
 * 
 * @deprecated Function may not be actively supported.
 *******************************************************************************/
EXPORT int AVCOM_ENG_SetUtuneLO ( void *handle, int value );


/********************************************************************************
 *																				*
 *				AVCOM_ENG_SetVBWList()											*
 *																				*
 ****************************************************************************//**
 * Sets the available VBW's in Hertz (Hz) given a pointer to containing the list
 * \c VBWs and the number of available vbw's, \c list_size. 
 * 
 * @note	This function sets the available VBW's but does not save into 
 *			non-volatile memory. Call AVCOM_ENG_SaveSettings() to save into
 *			non-volatile memory.
 *
 * @return Status of function call
 *
 * @param [in,out]	handle	Pointer to analyzer handle
 * @param [in,out]	VBWs	List of available VBW's in Hz
 * @param [in]		list_size	the number of elements in the \c VBWs array
 *******************************************************************************/
EXPORT int AVCOM_ENG_SetVBWList (void *handle, int *VBWs, int list_size );


/********************************************************************************
 *																				*
 *				AVCOM_ENG_SetRBWList()											*
 *																				*
 ****************************************************************************//**
 * Sets the available RBW's in Hertz (Hz) given a pointer to containing the list
 * \c RBWs and the number of available rbw's, \c list_size. 
 * 
 * @note	This function sets the available RBW's but does not save into 
 *			non-volatile memory. Call AVCOM_ENG_SaveSettings() to save into
 *			non-volatile memory.
 *
 * @return Status of function call
 *
 * @param [in,out]	handle	Pointer to analyzer handle
 * @param [in,out]	RBWs	List of available RBW's in Hz
 * @param [in]		list_size	the number of elements in the \c RBWs array
 * 
 * @warning Calling function may invalidate calibration
 *******************************************************************************/
EXPORT int AVCOM_ENG_SetRBWList (void *handle, int *RBWs, int list_size );


/********************************************************************************
 *																				*
 *				AVCOM_ENG_SetVBWmux()											*
 *																				*
 ****************************************************************************//**
 * Sets the VBW
 *
 * This command sets the multiplexers that control the VBW's. 
 *
 * - \c 0 sets the 30KHz VBW Filter
 * - \c 1 sets the 100KHz VBW Filter
 * - \c 2 sets the 300Hz VBW Filter
 * - \c 3 sets the 1MHz VBW Filter (AKA No Filter)
 * 
 * @return Status of function call
 *
 * @param [in,out]	handle	handle
 * @param [in]		value	VBW Selection
 * 
 * @deprecated Function may not be actively supported.
 *******************************************************************************/
EXPORT int AVCOM_ENG_SetVBWmux ( void *handle, int value );


/********************************************************************************
 *																				*
 *				AVCOM_ENG_TriggerPLLSweep()										*
 *																				*
 ****************************************************************************//**
 * Triggers a sweep of the PLL
 *
 * Triggers one sweep. Based on parameters set by \c AVCOM_ENG_SetPLLSSS() and
 * \c AVCOM_ENG_SetPLLWait()
 *
 * @return Status of function call
 *
 * @param [in,out]	handle	handle
 *  
 * @note Sweep data is retrieved by calling \c AVCOM_ENG_GetSweepADC()
 *******************************************************************************/
EXPORT int AVCOM_ENG_TriggerPLLSweep( void *handle);


/********************************************************************************
 *																				*
 *				AVCOM_ENG_TriggerDSSSweep()										*
 *																				*
 ****************************************************************************//**
 * Triggers a sweep of the DDS
 *
 * Triggers one sweep. Based on parameters set by \c AVCOM_ENG_SetDDSSSS() and
 * \c AVCOM_ENG_SetDDSWait()
 *
 * @return Status of function call
 *
 * @param [in,out]	handle	handle
 *  
 * @note Sweep data is retrieved by calling \c AVCOM_ENG_GetSweepADC()
 *******************************************************************************/
EXPORT int AVCOM_ENG_TriggerDSSSweep( void *handle);










/********************************************************************************
 *																				*
 *				AVCOM_ENG_SaveSettings()										*
 *																				*
 ****************************************************************************//**
 * Saves / Updates Engineering based 'set' settings
 * 
 * Prior to calling this function, many Engineering "Set" functions are only loaded
 * into memory and not saved into non-volatile memory. This command stores 
 * analyzers settings into non-volatile memory
 *
 * @return Status of function call
 * 
 * @param [in,out]	handle	handle
 * @warning Function not implemented
 *******************************************************************************/
EXPORT int AVCOM_ENG_SaveSettings( void *handle);





// Old outdated stuff, but not ready to get rid of
EXPORT int AVCOM_SA_TEST ( void );



#endif /* _AVCOM_ENG_V2_H_ */
#ifdef __cplusplus
}
#endif
#endif /* AOV_DEPEND_ENG */

