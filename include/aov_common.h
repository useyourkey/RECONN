/*************************************************************************//**
 * @file aov_common.h
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
 * 
 *****************************************************************************/



#pragma once
#ifndef _AOV_COMMON_V2_H_
#define _AOV_COMMON_V2_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "aov_opts.h"

/* The following is important and used to distinguish between different IDE's */
#if (defined(_WIN32) && defined(_USRDLL))
#define EXPORT __declspec(dllexport)
#elif (defined(_WIN32))
#define EXPORT __declspec(dllimport)
#else
#define EXPORT
#endif


#define	MODEL_NAME_SIZE			32
#define	ANALYZER_NAME_SIZE		32
#define	SERIAL_SIZE				32
#define	HOSTNAME_SIZE			16
#define	AOV_BAND_NAME_LEN		32

#define	MAX_TIME_SIZE			16
#define	MAX_DATE_SIZE			16


//#if defined(AOV_DEPEND_SA) || defined(AOV_DEPEND_CUS_PACKING)
/*! Row sub-structure for retrieving SA frequency/power from the SBS-2 
 *  into an array via top-level API calls*/
typedef struct sAOV_SweepData_Row_ {
    double			power;			//!< Power Level in dBm
	double			frequency;		//!< Frequency in MHz
} sAOV_SweepData_Row;


//#if defined(AOV_DEPEND_SA) || defined(AOV_DEPEND_CUS_PACKING)
/*! Row sub-structure for retrieving SA time/power from the SBS-2 
 *  into an array via top-level API calls for zero span*/
typedef struct sAOV_ZeroSpanData_Row_ {
    double			power;			//!< Power Level in dBm
    double			time;			//!< Time
} sAOV_ZeroSpanData_Row;


/*! Array sub-structure for retrieving SA frequency/power from the SBS-2 
 *  consisting of sAOV_SweepData_Row elements */
typedef sAOV_SweepData_Row	sAOV_SweepData_Array[4096]; 


/*! Array sub-structure for retrieving SA frequency/power from the SBS-2 
 *  consisting of sAOV_SweepData_Row elements */
typedef sAOV_ZeroSpanData_Row	sAOV_ZeroSpanData_Array[4096]; 


/*!Main structure for retrieving SA frequency/power from the SBS-2 
 * via top-level API calls*/
typedef struct	sAOV_SweepData_ {
	int							band;
	int							rbw;
	int							vbw;
	int							reflvl;
	int							avg;
	int							count;	//!< Count of the data points in the sweep (and thus the array)
	double						fStart;	//!< Sweep Start Frequency in MHz
	double						fStop;	//!< Sweep Stop Frequency in MHz
	double						fStep;	//!< Sweep Step Frequency in MHz
	sAOV_SweepData_Array		sweep;	//!< Array of sampled power at each frequency step in the sweep
} sAOV_SweepData;


/*!Main structure for retrieving SA power from the SBS-2 for zero span
 * via top-level API calls*/
typedef struct	sAOV_ZeroSpanData_ {
	int							band;
	int							rbw;
	int							vbw;
	int							reflvl;
	int							count;	//!< Count of the data points in the sweep (and thus the array)
	double						freq;	//!< Sweep Start Frequency in MHz
	int							sample_rate; //!< Time between samples (ms)
	int							mode;	//!< Mode for data samples
	sAOV_ZeroSpanData_Array		trace;  //!< Array of sampled power at each frequency step in the trace
} sAOV_ZeroSpanData;

/*! Structure to hold all networking relating information */
struct aov_network {
	int 			dynip;			//!< Zero is using static IP, non-zero is DHCP-enabled
	unsigned char	mac[6];			//!< MAC address of device
	unsigned long 	ip;				//!< IP Address of Device
	unsigned long 	sub;			//!< Subnet Address of Device
	unsigned long 	gw;				//!< Gateway Address of Device
	char 			hostname[16];	//!< Hostname of device (Network must support this)
    unsigned short	tcp_port;		//!< TCP Connection Port of Device
};


/*! Structure to hold all band related information. A Band is defined as a 
 *  continuous range of frequency. */
struct aov_band_info {
	int					phy_input;					//!< Physical Input the band uses
	char				name[AOV_BAND_NAME_LEN];	//!< User configurable identifying name
	unsigned int		min_freq;					//!< Maximum frequency range
	unsigned int		max_freq;					//!< Minimum frequency range
//	int					min_step;					//< Maximum step size allowed(and frequency must be in multiples of this number
//	unsigned int		max_span;		// not used
//	short				min_db;						//!< Minimum power level this band can detect
	short				max_db;						//!< Maximum power level this band can detect (Do not exceed this power level!)
	short				min_reflvl;					//!< Minimum reference level this band can handle
	short				max_reflvl;					//!< Maximum reference level this band can handle
};


/*! Structure to hold the firmware version information */
struct aov_version {
	int						major;			//!< The major version number
	int						minor;			//!< the minor version number
	int						build;			//!< Determines whether build is a \c 0 alpha, \c 1 Beta, \c 2 Release Canidate, or \c 3 Release build
	int						stage;			//!< Incremental build for alpha, beta, and Release Canidate builds. Ignored for full release versions
	char					string[16];		//!< String populating the full version name in human readable form
};

/*! Structure to hold the Product Description information */
struct aov_product {
	char				serial[SERIAL_SIZE];				//!< The serial number of the device
	char				model_name[MODEL_NAME_SIZE];		//!< The model number of the device
	char				analyzer_name[ANALYZER_NAME_SIZE];	//!< A user configurable name for the device
	unsigned short		prod_id;							//!< Product code associated with device
//	unsigned short		prod_family;
//	unsigned short		proj_id;		// Internal?
	unsigned short		numBands;					//!< Number of bands or input/band combinations the device has
	unsigned short		numRBW;						//!< The number of supported RBW the device has
	unsigned short		numVBW;						//!< The number of supported VBW the device has
	unsigned short		max_pts;					//!< Max number of data points the device can send
	struct aov_version		version;				//!< Convienent location to store the firmware version
	struct aov_network		network;				//!< Convienent location to store the networking information
	int					*rbw;						//!< Convienent location for allocating and storing available RBW information
	int					*vbw;						//!< Convienent location for allocating and storing available RBW information
	struct aov_band_info	*bands;					//!< Convienent location for allocating and storing band information
//	unsigned long		Options;		// Internal?
//	unsigned long		pcbinfo;		// Internal?
};

/* Structure is used for retrieving HW description data from the SBS-2 */
/*
typedef struct sAOV_HW_DESC_ {
	unsigned short	ProdType;
	unsigned short	ProdFamily;
	unsigned short	Model;
	char			ModelName[MODEL_NAME_SIZE];
	unsigned short	PcbFab;
	unsigned char	PcbRev;
	unsigned char	ProjId;
	char			Serial[SERIAL_NUMBER_SIZE];
	unsigned char	fwMajor;
	unsigned char	fwMinor;
	unsigned short	fwBuild;
	unsigned short	maxPoints;
	unsigned long	Options;
} sAOV_HW_DESC;
*/

// Additional Defines for Defines for \c AVCOM_API_ValidatePacket()
#define AOV_WARN_PACKET_INCOMPLETE 1
#define AOV_ERR_PACKET_INVALID -1


//#endif /* defined(AOV_DEPEND_SA) || defined(AOV_DEPEND_CUS_PACKING) */



/********************************************************************************
 *																				*
 *				AVCOM_API_Deinitialize()										*
 *																				*
 ****************************************************************************//**
 * De-Initialize the Avcom API environment
 *
 * This function should be called to cleanup the API environment before exiting a program
 * Any open connections will also be terminated and any registered handles will be terminated
 *
 * @note	\li This function must be called before calling any other functions
 * @note	\li You should call this function only once
 *
 * @return Status of function call
 *
 *******************************************************************************/
EXPORT int AVCOM_API_Deinitialize(void);


/********************************************************************************
*																				*
*		AVCOM_API_GetAPIVersion													*
*																				*
*****************************************************************************//**
 * Provides the version number of the API's
 *
 * @param [out]	ver	Structure containing version information
 * 
 * @return Status of function call
 *******************************************************************************/
EXPORT int AVCOM_API_GetAPIVersion ( struct aov_version *ver );


/********************************************************************************
*																				*
*		AVCOM_API_GetAPIVersionString											*
*																				*
*****************************************************************************//**
 * Provides the version number of the API's in a string, human readable format (ASCII)
 *
 * @param [in,out]	version	string at least 16-bytes long where the version will be copied
 * 
 * @return Status of function call
 *******************************************************************************/
EXPORT int AVCOM_API_GetAPIVersionString ( char *version );


/********************************************************************************
 *																				*
 *				AVCOM_API_Initialize()											*
 *																				*
 ****************************************************************************//**
 * Initialized the Avcom API environment.
 *
 * This function must be called before any other, including AVCOM_SA_Register()
 *
 * @return Status of function call
 *******************************************************************************/
EXPORT int AVCOM_API_Initialize(void);









/****************            CONSTANTS             ***************************/

#define AOV_TCP_DEFAULT				0		// Register a TCP-based connection
#define	AOV_USB_DEFAULT				1		// Register a USB-based connection		

#ifdef __cplusplus
}
#endif

#endif /* _AOV_COMMON_V2_H_ */

