#pragma once
#ifndef _AOV_SA_H_
#define _AOV_SA_H_
#ifdef __cplusplus
extern "C" {
#endif
#include "aov_opts.h"

#ifdef AOV_DEPEND_SA
/*************************************************************************//**
 * @file aov_sa.h
 * @author Avcom of Virginia
 *
 * @section LICENSE
 *
 * This software is Copyright (c) 2011-2012 Avcom of Virginia.
 * All Rights Reserved.
 *
 * @section DESCRIPTION
 *
 * This is the API Library for use with Avcom Spectrum Analyzers.
 *
 * Prior to using the API set, the environment must be initialized by first
 * calling \c AVCOM_API_Initialize(). This sets up a few global parameters,
 * particularly a dynamic linked list which is useful in maintaining handle
 * information.
 *
 * After which, each spectrum analyzer can be registered a handle through
 * \c AVCOM_SA_Register(). 
 *
 * To recap:
 *	- Initialize the API Environment
 *	- Register any number of Avcom Spectrum Analyzers
 *	- Connect to Analyzer(s) if not done in the Register command
 *	- Perform custom tasks
 *	- Disconnect Analyzer(s)
 *	- Unregister Analyzer(s)
 *	- Deinitialize API Environment
 *	- Exit Program 
 *
 * @section EXAMPLE-PROGRAM
 *
 * For reference and example program aov_example.c is attached that will illustrate 
 * basic usage of this API set.
 *
 * @section WARRANTY
 *
 * This software is provided for your convince and without any warranty. 
 * In no event shall Avcom of Virginia or its employees be liable for any lost 
 * profits, revenue, sales, data or costs of procurement of substitute goods 
 * or services, property damage, interruption of business, loss of business 
 * information or for any indirect, incidental, economic or consequential damages.
 * 
 * @example example.c
 *****************************************************************************/

#include "aov_common.h"

/* The following is important and used to distinguish between different IDE's */
#ifdef EXPORT
#undef EXPORT
#endif
#if (defined(_WIN32) && defined(_USRDLL))
#define EXPORT __declspec(dllexport)
#elif (defined(_WIN32))
#define EXPORT __declspec(dllimport)
#else
#define EXPORT
#endif


/*! Structure is used for requesting that the SA test itself (BIT) 
 *  if that feature is available*/
/*typedef struct	sAOV_BIT_REQUEST_ {
	eAOV_bool	bTestEcho;			//!< No specific tests. Just a ping of the BIT function
	eAOV_bool	bTestMinimal;		//!< Basic tests only (to limit time / operation impact)
	eAOV_bool	bTestFull;			//!< Full BIT (may be time intensive. Used for diagnostics not while operating)
} sAOV_BIT_REQUEST;
*/


/*! Structure is used for retrieving SA BIT results
 *  if that feature is available
 */
/*typedef struct	sAOV_BIT_RESULT_ {
	eAOV_bool	bTestPass;			//!< All requested tests passed
	eAOV_bool	bRamTestPass;		//!< RAM test passed (if requested) 
	eAOV_bool	bAdcTestPass;		//!< ADC test passed (if requested)
} sAOV_BIT_RESULT;
*/




/********************************************************************************
 *																				*
 *				AVCOM_SA_Connect()												*
 *																				*
 ****************************************************************************//**
 * Opens the socket to the specified Spectrum Analyzer
 * 
 * If the timeout specified is 0, the default timeout of 15 seconds will be used
 * 
 * @note This timeout is the recieve timeout. The connection attempt timeout is set by 
 * the operating system.
 * 
 * @return Status of function call
 *
 * @param [in,out]	handle	Pointer to analyzer handle
 * @param [in]	timeout	TCP recieve Timeout in milliseconds
 *
 * @pre	handle must be Registered before executing function
 *******************************************************************************/
EXPORT int AVCOM_SA_Connect(void *handle, int timeout);


/********************************************************************************
 *																				*
 *				AVCOM_SA_Disconnect()											*
 *																				*
 ****************************************************************************//**
 * Closes the specified socket
 * 
 * @return Status of function call
 *
 * @param [in,out]	handle	Pointer to analyzer handle
 *******************************************************************************/
EXPORT int AVCOM_SA_Disconnect(void *handle);


/********************************************************************************
 *																				*
 *				AVCOM_SA_DiscoverIP()											*
 *																				*
 ****************************************************************************//**
 * Discovers all SBS2 devices on an ethernet network
 * This utility returns all analyzers that can recieve/send UDP broadcasts
 * and therefore can identify analyzers that are not on the attached subnet.
 * 
 * @note 1. UDP Port 26482 and 26483 must be open for command to work.
 * @note 2. Analyzers found on different subnet must first undergo IP change before using.
 * @note 3. The user must free \code struct aov_product **analyzers \endcode when done.
 * @note 4. This command will take several seconds to return.
 * 
 * @return Status of function call
 *
 * @param [in,out]		analyzers	pointer to array containing analyzers found
 * @param [out]		numAnalyzers	Number of analyzers found, number of analyzers arrays present
 *******************************************************************************/
EXPORT int AVCOM_SA_DiscoverIP ( struct aov_product **analyzers, int *numAnalyzers );


/********************************************************************************
 *																				*
 *				AVCOM_SA_DO_BIT()												*
 *																				*
 ****************************************************************************//*
 * Request analyzer to execute BIT (Built In Test) if available.
 * 
 * @note SBS-2 Initial release will contain little, if any, subsystem-specific data.
 * 
 * @return Status of function call
 *
 * @param [in,out]	handle		handle
 * @param [in]		bitRequest	Tests to be executed
 * @param [in,out]	bitResult	Pointer to test results
 * 
 * @warning Not implemented.
 *******************************************************************************/
//EXPORT int AVCOM_SA_DO_BIT ( void *handle, sAOV_BIT_REQUEST bitRequest, sAOV_BIT_RESULT *bitResult );


/********************************************************************************
 *																				*
 *				AVCOM_SA_Echo()													*
 *																				*
 ****************************************************************************//*
 * An echo command that travels under the AVCOM Header packet
 * 
 * @note A tool for testing connections.
 * @note \c data_len size is currently limited to 64.
 * 
 * @return Status of function call
 * 
 * @warning Not Implemented
 *
 * @param [in,out]	handle	handle
 * @param [in]		data	data to be echoed
 * @param [in]		data_len	Length of data array
 *******************************************************************************/
//EXPORT int AVCOM_SA_Echo ( void *handle, unsigned char data, int data_len );


/********************************************************************************
 *																				*
 *				AVCOM_SA_FlashSBS2()											*
 *																				*
 ****************************************************************************//**
 * Flashes the SBS2 with new firmware. 
 * 
 * Analyzer must be in bootloader or uartloader mode for this sequence to operate.
 * This automates several commands such as sending the file and executing 
 * the reflash sequence.
 * 
 * @note This command may take several seconds to exit. Ensure recieve timeout 
 * is set high enough to allow sequence to complete sucessfully.
 * 
 * @return Status of function call
 *
 * @param [in,out]	handle	handle
 * @param [in]		data	pointer to array of new firmware data
 * @param [in]		data_len	length of the data array
 *******************************************************************************/
EXPORT int AVCOM_SA_FlashSBS2 ( void *handle, unsigned char* data, int data_len );


/********************************************************************************
 *																				*
 *				AVCOM_SA_GetActiveBand()										*
 *																				*
 ****************************************************************************//**
 * Retrieves the active or current Band and loads the index value to \c band. 
 *
 * @return Status of function call
 *
 * @param [in,out]	handle	Pointer to analyzer handle
 * @param [out]		band	Current or active Band
 *******************************************************************************/
EXPORT int AVCOM_SA_GetActiveBand (void *handle,  int *band );


/********************************************************************************
 *																				*
 *				AVCOM_SA_GetActiveRBW()											*
 *																				*
 ****************************************************************************//**
 * Retrieves the active or current RBW and loads the value in Hertz to \c rbw. 
 *
 * @return Status of function call
 *
 * @param [in,out]	handle	Pointer to analyzer handle
 * @param [out]		rbw		Current or active RBW
 *******************************************************************************/
EXPORT int AVCOM_SA_GetActiveRBW (void *handle,  int *rbw );


/********************************************************************************
 *																				*
 *				AVCOM_SA_GetActiveRefLevel()									*
 *																				*
 ****************************************************************************//**
 * Retrieves the active or current Reference Level and loads the value to \c reflvl. 
 *
 * @return Status of function call
 *
 * @param [in,out]	handle	Pointer to analyzer handle
 * @param [out]		reflvl		Current or active Reference Level
 *******************************************************************************/
EXPORT int AVCOM_SA_GetActiveRefLevel (void *handle,  int *reflvl );


/********************************************************************************
 *																				*
 *				AVCOM_SA_GetActiveVBW()											*
 *																				*
 ****************************************************************************//**
 * Retrieves the active or current VBW and loads the value in Hertz to \c vbw. 
 *
 * @return Status of function call
 *
 * @param [in,out]	handle	Pointer to analyzer handle
 * @param [out]		vbw		Current or active VBW
 *******************************************************************************/
EXPORT int AVCOM_SA_GetActiveVBW (void *handle,  int *vbw );


/********************************************************************************
 *																				*
 *				AVCOM_SA_GetBandInfo()											*
 *																				*
 ****************************************************************************//**
 * Retrieves information and parameters for the requested band.
 * 
 * \c band is the requested numeric index of the band and \c info 
 * is the pointer to structure where information will be loaded.
 *
 * @return Status of function call
 *
 * @param [in,out]	handle	Pointer to analyzer handle
 * @param [in]		band	The band to retrieve information on
 * @param [out]		info	Structure containing the requested band's parameters
 *******************************************************************************/
EXPORT int AVCOM_SA_GetBandInfo ( void *handle, int band, struct aov_band_info *info );


/********************************************************************************
*																				*
*		AVCOM_SA_GetBandInfoArrays()											*
*																				*
*****************************************************************************//**
 * Retrieves information and parameters for the requested band through arrays
 * 
 * Performs the same function as AVCOM_SA_GetBandInfo().
 *
 * @return Status of function call
 *
 * @param [in,out]	handle	Pointer to analyzer handle
 * @param [in]		band	The band to retrieve information on
 * @param [out]		phy_input	The physical input associated wit the band
 * @param [out]		min_freq	The minimum frequency of the band in MHz
 * @param [out]		max_freq	The maximum frequency of the band in MHz
 * @param [out]		max_db	The maximum power level the band can handle
 * @param [out]		min_reflvl	The minimum reference level the band can handle
 * @param [out]		max_reflvl	The maximum reference level the band can handle
 * @param [out]		name	The user-defined name of the band
 *******************************************************************************/
EXPORT int AVCOM_SA_GetBandInfoArrays(void *handle, int band, int *phy_input, double *min_freq, double *max_freq, 
								/* int *max_step, double *min_db,*/ double *max_db, double *min_reflvl, double *max_reflvl, char *name);

/********************************************************************************
 *																				*
 *				AVCOM_SA_GetBoardTemp()											*
 *																				*
 ****************************************************************************//**
 * Get the temperature from the on-board temperature setting
 *
 * @return Status of function call
 *
 * @param [in,out]	handle	handle
 * @param [out]		value	Temperature in degrees Celsius
 *******************************************************************************/
EXPORT int AVCOM_SA_GetBoardTemp ( void *handle, double *value );


/********************************************************************************
 *																				*
 *				AVCOM_SA_GetBuildDateTime()										*
 *																				*
 ****************************************************************************//**
 * Retrieves the build date and the build time
 *  
 * @return Status of function call
 *
 * @param [in,out]	handle	handle
 * @param [out]		date	16-byte string to get the date (MMM DD YYYY ascii format)
 * @param [out]		time	16-byte string to get the time (HH:MM:SS ascii format))
 *******************************************************************************/
EXPORT int AVCOM_SA_GetBuildDateTime ( void *handle, char *date, char *time );


/********************************************************************************
 *																				*
 *				AVCOM_SA_GetFirmwareVersion()									*
 *																				*
 ****************************************************************************//**
 * Retrieve the version number of the analyzer
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
EXPORT int AVCOM_SA_GetFirmwareVersion ( void *handle, struct aov_version *ver );


/********************************************************************************
 *																				*
 *				AVCOM_SA_GetFirmwareVersionString()								*
 *																				*
 ****************************************************************************//**
 * Retrieve the version number of the analyzer in a 16-byte string (ASCII)
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
 * @param [out]	version	16-byte (or greater) string to hold the version number
 *******************************************************************************/
EXPORT int AVCOM_SA_GetFirmwareVersionString ( void *handle, char *version );


/********************************************************************************
*																				*
*		AVCOM_SA_GetFreqValue													*
*																				*
*****************************************************************************//**
 * Retrieve the dB measurment at a specified frequency 
 * 
 * @note Currently only supported in L-Band 
 * @note Frequency values are based in L-Band values
 * 
 * @return Status of function call
 *
 * @param [in,out]	handle	handle
 * @param [in]	freq	Frequency to return dB measurement of
 * @param [out]	db	Amplitude Value
 *******************************************************************************/
EXPORT int AVCOM_SA_GetFreqValue ( void *handle, double freq, double *db );


/********************************************************************************
 *																				*
 *				AVCOM_SA_GetHandle()											*
 *																				*
 ****************************************************************************//**
 * Lookups a handle for a registered Spectrum Analyzer
 *
 * For cases when the handle has been 'lost' this function will locate the 
 * handle in the dynamic link and return the location to the desired handle.
 *
 * @warning	Returns (-1) if analyzer is not registered or is not found.
 *
 * @return Location of handle. 
 *
 * @param [in]	ip		IP Address of Spectrum Analyzer(ignored if USB)
 * @param [in]	port	Port Number of Spectrum Analyzer (ignored if USB)
 * @param [in]	usbid	USB Device ID (ignored if TCP)
 * @param [in]	type	Socket Type, AOV_TCP_DEFAULT or AOC_USB_DEFAULT
 *******************************************************************************/
EXPORT void *AVCOM_SA_GetHandle(char *ip, int port, int usbid, int type);


/********************************************************************************
 *																				*
 *				AVCOM_SA_GetHwDesc()											*
 *																				*
 ****************************************************************************//*
 * Function retrieves the HW Description from the devices and populates the
 * record as pointed to by *hw
 *
 * @warning Not Currently Implemented in Analyzer
 * 
 * @return Status of function call
 *
 * @param [in,out]		handle	Pointer to analyzer handle
 * @param [in,out]	vphw	Pointer to the Hardware Description Record
 *******************************************************************************/
//EXPORT int AVCOM_SA_GetHwDesc(void *handle, void *vphw );


/********************************************************************************
 *																				*
 *				AVCOM_SA_GetNetwork()											*
 *																				*
 ****************************************************************************//**
 * Function retrieves the current network settings from the devices and populates the
 * record as pointed to by \c *net
 *
 * @warning Not Currently Implemented in Analyzer
 * 
 * @return Status of function call
 *
 * @param [in,out]		handle	Pointer to analyzer handle
 * @param [in,out]	net	Pointer to the network settings structure
 *******************************************************************************/
EXPORT int AVCOM_SA_GetNetwork (void *handle, struct aov_network *net );


#ifdef AOV_DEPEND_USB
/********************************************************************************
 *																				*
 *				AVCOM_SA_GetNumUSB()											*
 *																				*
 ****************************************************************************//**
 * Determines the number of Avcom Analyzers are connected via USB
 *
 * It is advisable to call this function before retrieving USB Device lists. 
 * This function is used to obtain necessary info prior to registering 
 * and analyzer on the USB port. 
 * 
 * @note Other devices may use a similar USB driver that is used by AVCOM 
 * and may be counted by this command. 
 *
 * @return Status of function call
 *
 * @param [in,out]	numDevs	Number of devices attached
 *******************************************************************************/
EXPORT int AVCOM_SA_GetNumUSB ( unsigned int *numDevs );
#endif /* AOV_DEPEND_USB */


/********************************************************************************
 *																				*
 *				AVCOM_SA_GetProdDesc()											*
 *																				*
 ****************************************************************************//**
 * Function retrieves the Product Description from the device and populates the
 * record (and sub-records) as pointed to by *pd
 * 
 * @return Status of function call
 *
 * @param [in,out]		handle	Pointer to analyzer handle
 * @param [in,out]		ppd	Pointer to the Product Description Record
 *******************************************************************************/
EXPORT int AVCOM_SA_GetProdDesc(void *handle, struct aov_product *ppd );


/********************************************************************************
*																				*
*		AVCOM_SA_GetProdDescArrays()											*
*																				*
*****************************************************************************//**
 * Retrieves information and parameters for the requested band through arrays
 * 
 * Performs the same function as AVCOM_SA_GetProdDesc().
 *
 * @return Status of function call
 *
 * @param [in,out]	handle	Pointer to analyzer handle
 * @param [out]		prod_id	The product ID number
 * @param [out]		numBands	The number of bands the analyzer is capible of
 * @param [out]		numRBW	The number of available RBW's
 * @param [out]		numVBW	The number of available VBW's
 * @param [out]		max_pts	The maximum number of data points allowed
 * @param [out]		serial	The serial number
 * @param [out]		model_name	The full model name of the analyzer
 * @param [out]		analyzer_name	The user-defined name for the analyzer
 *******************************************************************************/
EXPORT int AVCOM_SA_GetProdDescArrays(void *handle, int *prod_id, int *numBands, int *numRBW, 
								int *numVBW, int *max_pts, char *serial, char *model_name,
								char *analyzer_name ) ;


/********************************************************************************
 *																				*
 *				AVCOM_SA_GetRBWList()											*
 *																				*
 ****************************************************************************//**
 * Retrieves a list containing available RBW's in Hertz (Hz) given a pointer to
 * an array \c RBWs the size of \c RBWs, \c list_size. 
 * 
 * If list_size is greater, the function will perform as expected and return 
 * without errors. An error/warning will be returned if the list_size is less than 
 * the number of available RBW's, however a partial list will be returned.
 * 
 * @pre It is advisable to call \c AVOM_SA_GetProdDesc() first to obtain the number
 *		of available RBW's. The user is responsible for defining or malloc'ing 
 *		the \c RBWs array and insuring that the \c VBWs array is large enough. 
 *
 * @return Status of function call
 *
 * @param [in,out]	handle	Pointer to analyzer handle
 * @param [in,out]	RBWs	List of available RBW's in Hz
 * @param [in]		list_size	the size of the RBWs array
 *******************************************************************************/
EXPORT int AVCOM_SA_GetRBWList (void *handle,  int *RBWs, int list_size );


/********************************************************************************
 *																				*
 *				AVCOM_SA_GetSpectrumData()										*
 *																				*
 ****************************************************************************//**
 * Retrieves a single trace from Spectrum Analyzer. Data is passed through a
 * defined pointer to an \c sAOV_SweepData structure.
 * 
 * @return Status of function call
 *
 * @param [in,out]	handle	Pointer to analyzer handle
 * @param [out]		trace	Pointer to structure with amplitude/frequency information
 *******************************************************************************/
EXPORT int AVCOM_SA_GetSpectrumData(void *handle, sAOV_SweepData *trace);


/********************************************************************************
 *																				*
 *				AVCOM_SA_GetSpectrumDataArrays()								*
 *																				*
 ****************************************************************************//**
 * Retrieves a single trace from Spectrum Analyzer. Data is passed through
 * defined pointers to discrete values (start, stop, step) and arrays for
 * the frequency and power information.
 * 
 * @return Status of function call
 *
 * @param [in,out]	handle	Pointer to analyzer handle
 * @param [out]		start	Pointer to start frequency
 * @param [out]		stop	Pointer to stop frequency
 * @param [out]		step	Pointer to step frequency
 * @param [out]		band	Pointer to the current band in use
 * @param [out]		rbw		Pointer to the current rbw in use
 * @param [out]		vbw		Pointer to the current vbw in use
 * @param [out]		reflvl	Pointer to the current reference level
 * @param [out]		avg		Pointer to the current averaging value in use (not implemented)
 * @param [out]		num		Pointer to count of array elements (frequency and power)
 * @param [out]		freq	Pointer to structure with amplitude/frequency information
 * @param [out]		power	Pointer to structure with amplitude/frequency information
 *******************************************************************************/
EXPORT int AVCOM_SA_GetSpectrumDataArrays(void *handle, double* start, double* stop, 
										  double* step, int *band, int *rbw, int *vbw,int *reflvl, int *avg,
										  unsigned short* num, double* freq, double* power);


/********************************************************************************
 *																				*
 *				AVCOM_SA_GetSpectrumDataArraysLV()								*
 *																				*
 ****************************************************************************//*
 * Retrieves a single trace from Spectrum Analyzer. Data is passed through
 * defined pointers to discrete values (start, stop, step) and arrays for
 * the frequency and power information. This function is designed for passing
 * data elements effectively to LabView. In this case each array is presented in
 * the LabView internal format with the first 4 bytes containing the count of 
 * array elements.
 * 
 * @return Status of function call
 *
 * @param [in,out]	handle	Pointer to analyzer handle
 * @param [out]		start	Pointer to start frequency
 * @param [out]		stop	Pointer to stop frequency
 * @param [out]		step	Pointer to step frequency
 * @param [out]		freq	Pointer to structure with amplitude/frequency information
 * @param [out]		data	Pointer to structure with amplitude/frequency information
 *******************************************************************************/
//EXPORT int AVCOM_SA_GetSpectrumDataArraysLV(void *handle, double* start, double* stop, double* step, double* freq,  double* data);


/********************************************************************************
 *																				*
 *				AVCOM_SA_GetTriggerSweep()										*
 *																				*
 ****************************************************************************//**
 * Triggers and retrieves a single sweep on the Spectrum Analyzer
 * 
 * This function provides the combined task of \c AVCOM_SA_TriggerSweep() and
 * \c AVCOM_SA_GetSpectrumData() in one function call.
 *
 * @return Status of function call
 *
 * @param [in,out]	handle	handle
 * @param [out]		trace	Pointer to structure with amplitude/frequency information
 *******************************************************************************/
EXPORT int AVCOM_SA_GetTriggerSweep(void *handle, sAOV_SweepData *trace);


/********************************************************************************
 *																				*
 *				AVCOM_SA_GetTriggerSweepArrays()								*
 *																				*
 ****************************************************************************//**
 * Triggers and retrieves a single sweep on the Spectrum Analyzer
 * 
 * This function provides the combined task of \c AVCOM_SA_TriggerSweep() and
 * \c AVCOM_SA_GetSpectrumDataArrays() in one function call.
 *
 * @return Status of function call
 *
 * @param [in,out]	handle	Pointer to analyzer handle
 * @param [out]		start	Pointer to start frequency
 * @param [out]		stop	Pointer to stop frequency
 * @param [out]		step	Pointer to step frequency
 * @param [out]		band	Pointer to the current band in use
 * @param [out]		rbw		Pointer to the current rbw in use
 * @param [out]		vbw		Pointer to the current vbw in use
 * @param [out]		reflvl	Pointer to the current reference level setting
 * @param [out]		avg		Pointer to the current averaging value in use (not implemented)
 * @param [out]		num		Pointer to count of array elements (frequency and power)
 * @param [out]		freq	Pointer to structure with amplitude/frequency information
 * @param [out]		power	Pointer to structure with amplitude/frequency information
 *******************************************************************************/
EXPORT int AVCOM_SA_GetTriggerSweepArrays(void *handle, double* start, double* stop, 
										  double* step, int *band, int *rbw, int *vbw, int *reflvl, int *avg, 
										  unsigned short* num, double* freq, double* power);


/********************************************************************************
 *																				*
 *				AVCOM_SA_GetTriggerZeroSpan()									*
 *																				*
 ****************************************************************************//**
 * Triggers and retrieves a single zero span capture on the Spectrum Analyzer
 * 
 * This function provides the combined task of \c AVCOM_SA_TriggerZeroSpan() and
 * \c AVCOM_SA_GetZeroSpanSpectrumData() in one function call.
 *
 * @return Status of function call
 *
 * @param [in,out]	handle	handle
 * @param [out]		trace	Pointer to structure with amplitude/frequency/time information
 *******************************************************************************/
EXPORT int AVCOM_SA_GetTriggerZeroSpan(void *handle, sAOV_ZeroSpanData *trace);


/********************************************************************************
 *																				*
 *				AVCOM_SA_GetTriggerZeroSpanArrays()								*
 *																				*
 ****************************************************************************//**
 * Triggers and retrieves a single sweep on the Spectrum Analyzer
 * 
 * This function provides the combined task of \c AVCOM_SA_TriggerSweep() and
 * \c AVCOM_SA_GetSpectrumDataArrays() in one function call.
 *
 * @return Status of function call
 *
 * @param [in,out]	handle	Pointer to analyzer handle
 * @param [out]		freq	Pointer to frequency
 * @param [out]		sample_rate Pointer to sampling rate value (ms)
 * @param [out]		band	Pointer to the current band in use
 * @param [out]		rbw		Pointer to the current rbw in use
 * @param [out]		vbw		Pointer to the current vbw in use
 * @param [out]		reflvl	Pointer to the current reference level setting
 * @param [out]		num		Pointer to count of array elements (frequency and power)
 * @param [out]		mode	Sampling mode
 * @param [out]		time	Pointer to structure with time information
 * @param [out]		power	Pointer to structure with amplitude information
 *******************************************************************************/
EXPORT int AVCOM_SA_GetTriggerZeroSpanArrays(void *handle, double* freq, int *sample_rate,
										  int *band, int *rbw, int *vbw, int *reflvl, int *mode,
										  unsigned short *num, double *time, double *power);


/********************************************************************************
 *																				*
 *				AVCOM_SA_GetVBWList()											*
 *																				*
 ****************************************************************************//**
 * Retrieves a list containing available VBW's in Hertz (Hz) given a pointer to
 * an array \c VBWs the size of \c VBWs, \c list_size. 
 * 
 * If list_size is greater, the function will perform as expected and return 
 * without errors. An error/warning will be returned if the list_size is less than 
 * the number of available VBW's, however a partial list will be returned.
 * 
 * @pre It is advisable to call \c AVOM_SA_GetProdDesc() first to obtain the number
 *		of available VBW's. The user is responsible for defining or malloc'ing 
 *		the \c VBW array and insuring that the \c VBWs array is large enough.
 *
 * @return Status of function call
 *
 * @param [in,out]	handle	Pointer to analyzer handle
 * @param [in,out]	VBWs	List of available VBW's in Hz
 * @param [in]		list_size	the number of elements in the \c VBWs array
 *******************************************************************************/
EXPORT int AVCOM_SA_GetVBWList (void *handle, int *VBWs, int list_size );


/********************************************************************************
 *																				*
 *				AVCOM_SA_Ping()													*
 *																				*
 ****************************************************************************//**
 * An ping command that travels under the AVCOM Header packet
 * 
 * Only header and command data is sent to and from the Analyzer.
 * 
 * @return Status of function call
 * 
 * @param [in,out]	handle	handle
 *******************************************************************************/
EXPORT int AVCOM_SA_Ping ( void *handle );


/********************************************************************************
 *																				*
 *				AVCOM_SA_Reboot()												*
 *																				*
 ****************************************************************************//**
 * Reboots the analyzer
 * 
 * @return Status of function call
 *
 * @param [in,out]	handle	handle
 *******************************************************************************/
EXPORT int AVCOM_SA_Reboot ( void *handle );


/********************************************************************************
 *																				*
 *				AVCOM_SA_Register()												*
 *																				*
 ****************************************************************************//**
 * Registers a Spectrum Analyzer into the API environment returning a handle to
 * the analyzer. The analyzer and related information are maintained internal to
 * the API environment
 *
 * Modes are:
 *		- AOV_DEFAULT		The default and currently only mode (0)
 *
 * Special flags are:
 *		- AOV_TCP_DEFAULT	Connection is TCP
 *		- AOV_USB_DEFAULT	Connection is USB device
 * 
 * @return Status of function call
 *
 * @param [in,out]	handle	handle - pass unary operator, ex: &handle
 * @param [in]		ip		IP address of analyzer (Assign NULL if USB)
 * @param [in]		port	Port number of analyzer (Assign NULL if USB)
 * @param [in]		usb		USB Device number (Assign NULL if TCP)
 * @param [in]		mode	Sets autonomy state 
 * @param [in]		flags	Special Flags for function
 *
 * @pre	Environment must first be initialized by AVCOM_API_Initialize()
 * @pre A USB device that is currently un use cannot be registered.
 *******************************************************************************/
EXPORT int AVCOM_SA_Register(void **handle, char *ip, int port, int usb, int mode, int flags);


/********************************************************************************
 *																				*
 *				AVCOM_SA_SaveBandSettings()										*
 *																				*
 ****************************************************************************//**
 * Saves current band settings and current tuning parameters for each band
 * into non-volitle memory
 *
 * @return Status of function call
 *
 * @param [in,out]	handle	handle
 *******************************************************************************/
EXPORT int AVCOM_SA_SaveBandSettings ( void *handle );


/********************************************************************************
 *																				*
 *				AVCOM_SA_SetAnalyzerName()										*
 *																				*
 ****************************************************************************//**
 * Set the Analyzer's name
 *
 * @return Status of function call
 *
 * @param [in,out]	handle	handle
 * @param [out]		name	Name to assign to analyzer, 32-bit ASCII max
 *******************************************************************************/
EXPORT int AVCOM_SA_SetAnalyzerName ( void *handle, char* name);


/********************************************************************************
 *																				*
 *				AVCOM_SA_SetBand()												*
 *																				*
 ****************************************************************************//**
 * Set the band by using the band index value
 *
 * @return Status of function call
 *
 * @param [in,out]	handle	handle
 * @param [in]		band	index value for the band desired
 *******************************************************************************/
EXPORT int AVCOM_SA_SetBand ( void *handle, int band);


/********************************************************************************
 *																				*
 *				AVCOM_SA_SetBandName()											*
 *																				*
 ****************************************************************************//**
 * Set the active band's name
 *
 * @return Status of function call
 *
 * @param [in,out]	handle	handle
 * @param [out]		bandname	Name to assign to band, 32-bit ASCII max
 *******************************************************************************/
EXPORT int AVCOM_SA_SetBandName ( void *handle, char* bandname);


/********************************************************************************
 *																				*
 *				AVCOM_SA_SetCenterFrequency()									*
 *																				*
 ****************************************************************************//*
 * Set Center Frequency
 *
 * @return Status of function call
 *
 * @param [in,out]	handle	handle
 * @param [in]		cfreq	Center Frequency (MHz)
 *******************************************************************************/
//EXPORT int AVCOM_SA_SetCenterFrequency ( void *handle, double cfreq );


/********************************************************************************
 *																				*
 *				AVCOM_SA_SetFreqCSD()											*
 *																				*
 ****************************************************************************//**
 * Set Frequency Using Center Frequency, Span, and number of data points. The span
 * will be automatically adjusted to be a multiple of the step size which is 
 * calculated by the \c max_pts. 
 *
 * @return Status of function call
 *
 * @param [in,out]	handle	handle
 * @param [in]		cfreq	The center frequency of the sweep (MHZ)
 * @param [in]		span	The requested frequency span of the sweep (MHZ)
 * @param [in]		max_pts	The number of data points to return
 *******************************************************************************/
EXPORT int AVCOM_SA_SetFreqCSD ( void *handle, double cfreq,  double span, int max_pts);


/********************************************************************************
 *																				*
 *				AVCOM_SA_SetFreqCSS()											*
 *																				*
 ****************************************************************************//**
 * Set Frequency Using Center Frequency, Span, and Step Size
 *
 * @return Status of function call
 *
 * @param [in,out]	handle	handle
 * @param [in]		cfreq	The center frequency of the sweep (MHZ)
 * @param [in]		span	The frequency span of the sweep (MHZ)
 * @param [in]		step	The step size of the sweep (MHZ)
 *******************************************************************************/
EXPORT int AVCOM_SA_SetFreqCSS ( void *handle, double cfreq,  double span, double step);


/********************************************************************************
 *																				*
 *				AVCOM_SA_SetFreqSSS()											*
 *																				*
 ****************************************************************************//**
 * Set Frequency Using Start Frequency, Stop Frequency, Step Size
 *
 * @return Status of function call
 *
 * @param [in,out]	handle	handle
 * @param [in]		fstart	The center frequency of the sweep (MHZ)
 * @param [in]		fstop	The frequency span of the sweep (MHZ)
 * @param [in]		fstep	The step size of the sweep (MHZ)
 *******************************************************************************/
EXPORT int AVCOM_SA_SetFreqSSS ( void *handle, double fstart, double fstop, double fstep);


/********************************************************************************
 *																				*
 *				AVCOM_SA_SetHostname()											*
 *																				*
 ****************************************************************************//**
 * Change the hostname of an analyzer through an AVCOM Connection
 * 
 * @note Network connectivity may be disrupted on analyzer and a disconnect/connect
 *		or unregister/register cycle may be necessary.
 * 
 * @return Status of function call
 *
 * @param [in,out]	handle	handle
 * @param [in]		hostname	16-byte null-terminated string
 *******************************************************************************/
EXPORT int AVCOM_SA_SetHostname ( void *handle, char *hostname );


/********************************************************************************
 *																				*
 *				AVCOM_SA_SetNetwork()											*
 *																				*
 ****************************************************************************//**
 * Change the TCP connection port number of an analyzer through an AVCOM Connection.
 * 
 * Values are in Network Byte Order, aka Big-Endian.
 * 
 * @note Network connectivity may be disrupted on analyzer and disconnect/connection
 *		or unregister/register cycle may be necessary.
 * 
 * @return Status of function call
 *
 * @param [in,out]	handle	handle
 * @param [in]	dynip	0 for static IP, non-zero for dynamic IP (DHCP) assignment
 * @param [in]	ip	New IP Address of device (ignored if dynip is non-zero)
 * @param [in]	sub New subnet Address of device (ignored if dynip is non-zero)
 * @param [in]	gw	New Gateway Address of device (ignored if dynip is non-zero)
 *******************************************************************************/
EXPORT int AVCOM_SA_SetNetwork( void *handle, int dynip, unsigned long ip, unsigned long sub, unsigned long gw );


/********************************************************************************
 *																				*
 *				AVCOM_SA_SetPort()												*
 *																				*
 ****************************************************************************//**
 * Change the TCP connection port number of an analyzer through an AVCOM Connection
 * 
 * @note Network connectivity may be disrupted on analyzer and disconnect/connection
 *		or unregister/register cycle may be necessary.
 * 
 * @return Status of function call
 *
 * @param [in,out]	handle	handle
 * @param [in]		port	New tcp port number
 *******************************************************************************/
EXPORT int AVCOM_SA_SetPort( void *handle, unsigned short port );


/********************************************************************************
 *																				*
 *				AVCOM_SA_SetRBW()												*
 *																				*
 ****************************************************************************//**
 * Set Resolution Bandwidth
 *
 * @return Status of function call
 *
 * @param [in,out]	handle	handle
 * @param [in]		rbw		Resolution Bandwidth in Hertz
 *******************************************************************************/
EXPORT int AVCOM_SA_SetRBW ( void *handle, int rbw );


/********************************************************************************
 *																				*
 *				AVCOM_SA_SetRefLevel()											*
 *																				*
 ****************************************************************************//**
 * Set Reference Level
 *
 * @return Status of function call
 *
 * @param [in,out]	handle	handle
 * @param [in]		reflvl	Reference Level (dB)
 * 
 *******************************************************************************/
EXPORT int AVCOM_SA_SetRefLevel ( void *handle, int reflvl );


/********************************************************************************
 *																				*
 *				AVCOM_SA_SetSpan()												*
 *																				*
 ****************************************************************************//*
 * Set Span
 *
 * @return Status of function call
 *
 * @param [in,out]	handle	handle
 * @param [in]		span		Span (MHz)
 *******************************************************************************/
//EXPORT int AVCOM_SA_SetSpan ( void *handle, double span );


/********************************************************************************
 *																				*
 *				AVCOM_SA_SetVBW()												*
 *																				*
 ****************************************************************************//**
 * Set Video Bandwidth
 *
 * @return Status of function call
 *
 * @param [in,out]	handle	handle
 * @param [in]		vbw		Video Bandwidth int Hertz
 *******************************************************************************/
EXPORT int AVCOM_SA_SetVBW ( void *handle, int vbw );


/********************************************************************************
 *																				*
 *				AVCOM_SA_SetZeroSpan()											*
 *																				*
 ****************************************************************************//**
 * Set paremeters for zero span functionality
 *
 * @return Status of function call
 *
 * @param [in,out]	handle	handle
 * @param [in]		freq	Frequency to set zero span to
 * @param [in]		numPts	Number of points to return
 * @param [in]		sample_rate	Time between samples
 * @param [in]		mode	Modes for sampling zero span
 *******************************************************************************/
EXPORT int AVCOM_SA_SetZeroSpan ( void *handle, double freq, int numPts, int sample_rate, int mode);


/********************************************************************************
 *																				*
 *				AVCOM_SA_TriggerSweep()											*
 *																				*
 ****************************************************************************//**
 * Triggers a single sweep on the Spectrum Analyzer
 *
 * @return Status of function call
 *
 * @param [in,out]	handle	handle
 *  
 * @note Sweep data is retrieved by calling \c AVCOM_ENG_GetSweepADC()
 *******************************************************************************/
EXPORT int AVCOM_SA_TriggerSweep( void *handle);


/********************************************************************************
 *																				*
 *				AVCOM_SA_UDPSetHostname()										*
 *																				*
 ****************************************************************************//**
 * Change the hostname of an analyzer through UDP Broadcast
 * No socket connection is necessary. For use when analyzer is on separte subnet
 * 
 * @note UDP Port 26482 and 26483 must be open for command to work.
 * @note This function is asyncronous. Wait two seconds before calling any other command
 *       after this.
 * 
 * @return Status of function call
 *
 * @param [in]		mac	6-byte MAC Address of the analyzer requesting a hostname change
 * @param [in]		hostname	16-byte null-terminated string
 *******************************************************************************/
EXPORT int AVCOM_SA_UDPSetHostname ( unsigned char *mac, char *hostname );


/********************************************************************************
 *																				*
 *				AVCOM_SA_UDPSetNetwork()										*
 *																				*
 ****************************************************************************//**
 * Change the IP/Network settings of an analyzer through UDP Broadcast
 * No socket connection is necessary. For use when analyzer is on separte subnet
 * 
 * @note UDP Port 26482 and 26483 must be open for command to work.
 * @note This function is asyncronous. Wait two seconds before calling any other command
 *       after this.
 * 
 * @return Status of function call
 *
 * @param [in]		mac	6-byte MAC Address of the analyzer requesting a hostname change
 * @param [in]		dynip	0 for static IP, non-zero for dynamic IP (DHCP) assignment
 * @param [in]		ip	New IP Address of device (ignored if dynip is non-zero)
 * @param [in]		sub New subnet Address of device (ignored if dynip is non-zero)
 * @param [in]		gw	New Gateway Address of device (ignored if dynip is non-zero)
 *******************************************************************************/
EXPORT int AVCOM_SA_UDPSetNetwork( unsigned char *mac, int dynip, unsigned long ip, unsigned long sub, unsigned long gw );


/********************************************************************************
 *																				*
 *				AVCOM_SA_UDPSetPort()											*
 *																				*
 ****************************************************************************//**
 * Change the TCP connection port number of an analyzer through UDP Broadcast
 * No socket connection is necessary. For use when analyzer is on separte subnet
 * 
 * @note UDP Port 26482 and 26483 must be open for command to work.
 * @note This function is asyncronous. Wait two seconds before calling any other command
 *       after this.
 * 
 * @return Status of function call
 *
 * @param [in]		mac	6-byte MAC Address of the analyzer requesting a hostname change
 * @param [in]		port	New TCP port number
 *******************************************************************************/
EXPORT int AVCOM_SA_UDPSetPort( unsigned char *mac, unsigned short port );


/********************************************************************************
 *																				*
 *				AVCOM_SA_Unregister()											*
 *																				*
 ****************************************************************************//**
 * Unregisters a handle and removes it from the dynamically linked list
 *
 * This will also disconnect analyzer if connection is open
 *
 * @return Status of function call
 *
 * @param [in,out]	handle	handle
 *******************************************************************************/
EXPORT int AVCOM_SA_Unregister(void *handle);


#endif /* AOV_DEPEND_SA */
#ifdef __cplusplus
}
#endif
#endif /* _AOV_SA_H_ */
