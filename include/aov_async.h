

#pragma once
#ifndef _AOV_ASYNC_V2_H_
#define _AOV_ASYNC_V2_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "aov_opts.h"




#ifdef AOV_DOC_ENCODE
/*************************************************************************//**
 * @internal
 * @file aov_async.h
 * @author Avcom of Virginia
 *
 * @section LICENSE
 *
 * This software is Copyright (c) 2011-2012 Avcom of Virginia.
 * All Rights Reserved.
 *
 * @section DESCRIPTION
 *
 * The functions in this section are for packing and unpacking data where the
 * user application performs its own send and recieve functions.
 *
 * @example async-example.c
 *****************************************************************************/
#endif /* AOV_DOC_ENCODE */


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


//#ifndef AOV_DOC_ENCODE
//#undef EXPORT
//#define EXPORT
//#endif

#if (defined(AOV_DEPEND_SA) || defined(AOV_DEPEND_CUS_PACKING))
/********************************************************************************
 *																				*
 *				AVCOM_API_DecodeGetActiveBand()									*
 *																				*
 ****************************************************************************//**
 * Decodes the data associated with \c AVCOM_SA_GetActiveBand() function response
 * 
 * The \c dataLen parameter is determined by \c AVCOM_API_ValidatePacket() and the 
 * \c data array is the return of the \c AVCOM_API_UnpackPacket() function.
 *     	   
 * \param [in]	data	Pointer to the array containing the data unpacked from the AVCOM Packet
 * \param [in]	dataLen	Len	The length of the \c data array
 * @param [out]	band	The active band index number
 *    				
 * \return Status of the decoding operation
 * 		   
 * \note This function must assume accurate data input (as validated by \c AVCOM_API_UnpackPacket() )
 * 		 and minimal error detection is possible.
 *
 *******************************************************************************/
EXPORT int AVCOM_API_DecodeGetActiveBand ( void *data, int dataLen, int *band );


/********************************************************************************
 *																				*
 *				AVCOM_API_DecodeGetActiveRBW()									*
 *																				*
 ****************************************************************************//**
 * Decodes the data associated with \c AVCOM_SA_GetActiveRBW() function response
 * 
 * The \c dataLen parameter is determined by \c AVCOM_API_ValidatePacket() and the 
 * \c data array is the return of the \c AVCOM_API_UnpackPacket() function.
 *     	   
 * \param [in]	data	Pointer to the array containing the data unpacked from the AVCOM Packet
 * \param [in]	dataLen	Len	The length of the \c data array
 * @param [out]	rbw	The active RBW
 *    				
 * \return Status of the decoding operation
 * 		   
 * \note This function must assume accurate data input (as validated by \c AVCOM_API_UnpackPacket() )
 * 		 and minimal error detection is possible.
 *
 *******************************************************************************/
EXPORT int AVCOM_API_DecodeGetActiveRBW ( void *data, int dataLen, int *rbw );


/********************************************************************************
 *																				*
 *				AVCOM_API_DecodeGetActiveRefLevel()								*
 *																				*
 ****************************************************************************//**
 * Decodes the data associated with \c AVCOM_SA_GetActiveRefLevel() function response
 * 
 * The \c dataLen parameter is determined by \c AVCOM_API_ValidatePacket() and the 
 * \c data array is the return of the \c AVCOM_API_UnpackPacket() function.
 *     	   
 * \param [in]	data	Pointer to the array containing the data unpacked from the AVCOM Packet
 * \param [in]	dataLen	Len	The length of the \c data array
 * @param [out]	reflvl	The active reference level
 *    				
 * \return Status of the decoding operation
 * 		   
 * \note This function must assume accurate data input (as validated by \c AVCOM_API_UnpackPacket() )
 * 		 and minimal error detection is possible.
 *
 *******************************************************************************/
EXPORT int AVCOM_API_DecodeGetActiveRefLevel ( void *data, int dataLen, int *reflvl );


/********************************************************************************
 *																				*
 *				AVCOM_API_DecodeGetActiveVBW()									*
 *																				*
 ****************************************************************************//**
 * Decodes the data associated with \c AVCOM_SA_GetActiveVBW() function response
 * 
 * The \c dataLen parameter is determined by \c AVCOM_API_ValidatePacket() and the 
 * \c data array is the return of the \c AVCOM_API_UnpackPacket() function.
 *     	   
 * \param [in]	data	Pointer to the array containing the data unpacked from the AVCOM Packet
 * \param [in]	dataLen	Len	The length of the \c data array
 * @param [out]	vbw	The active VBW in Hz
 *    				
 * \return Status of the decoding operation
 * 		   
 * \note This function must assume accurate data input (as validated by \c AVCOM_API_UnpackPacket() )
 * 		 and minimal error detection is possible.
 *
 *******************************************************************************/
EXPORT int AVCOM_API_DecodeGetActiveVBW ( void *data, int dataLen, int *vbw );


/********************************************************************************
 *																				*
 *				AVCOM_API_DecodeGetBandInfo()									*
 *																				*
 ****************************************************************************//**
 * Decodes the data associated with \c AVCOM_SA_GetBandInfo()() function response
 * 
 * The \c dataLen parameter is determined by \c AVCOM_API_ValidatePacket() and the 
 * \c data array is the return of the \c AVCOM_API_UnpackPacket() function.
 *     	   
 * \param [in]	data	Pointer to the array containing the data unpacked from the AVCOM Packet
 * \param [in]	dataLen	Len	The length of the \c data array
 * @param [out]	info	Pointer to Band Info structure
 *    				
 * \return Status of the decoding operation
 * 		   
 * \note This function must assume accurate data input (as validated by \c AVCOM_API_UnpackPacket() )
 * 		 and minimal error detection is possible.
 *
 *******************************************************************************/
EXPORT int AVCOM_API_DecodeGetBandInfo ( void *data, int dataLen, struct aov_band_info *info );


/********************************************************************************
 *																				*
 *				AVCOM_API_DecodeGetBoardTemp()									*
 *																				*
 ****************************************************************************//**
 * Decodes the data associated with \c AVCOM_SA_GetBoardTemp() function response
 * 
 * The \c dataLen parameter is determined by \c AVCOM_API_ValidatePacket() and the 
 * \c data array is the return of the \c AVCOM_API_UnpackPacket() function.
 *     	   
 * \param [in]	data	Pointer to the array containing the data unpacked from the AVCOM Packet
 * \param [in]	dataLen	Len	The length of the \c data array
 * @param [out]	value	pointer toCurrent Board Temperature
 *    				
 * \return Status of the decoding operation
 * 		   
 * \note This function must assume accurate data input (as validated by \c AVCOM_API_UnpackPacket() )
 * 		 and minimal error detection is possible.
 * \note The \c date and \c time must be pre-allocated arrays of at least 16 bytes long
 *
 *******************************************************************************/
EXPORT int AVCOM_API_DecodeGetBoardTemp ( void *data, int dataLen, double *value );


/********************************************************************************
 *																				*
 *				AVCOM_API_DecodeGetBuildDateTime()								*
 *																				*
 ****************************************************************************//**
 * Decodes the data associated with \c AVCOM_SA_GetBuildDateTime() function response
 * 
 * The \c dataLen parameter is determined by \c AVCOM_API_ValidatePacket() and the 
 * \c data array is the return of the \c AVCOM_API_UnpackPacket() function.
 *     	   
 * \param [in]	data	Pointer to the array containing the data unpacked from the AVCOM Packet
 * \param [in]	dataLen	Len	The length of the \c data array
 * @param [out]	date	16-byte string to get the date (MMM DD YYYY ASCII format)
 * @param [out]	time	16-byte string to get the time (HH:MM:SS ASCII format))
 *    				
 * \return Status of the decoding operation
 * 		   
 * \note This function must assume accurate data input (as validated by \c AVCOM_API_UnpackPacket() )
 * 		 and minimal error detection is possible.
 * \note The \c date and \c time must be pre-allocated arrays of at least 16 bytes long
 *
 *******************************************************************************/
EXPORT int AVCOM_API_DecodeGetBuildDateTime ( void *data, int dataLen, char *date, char *time );


/********************************************************************************
 *																				*
 *				AVCOM_API_DecodeGetFirmwareVersion()							*
 *																				*
 ****************************************************************************//**
 * Decodes the data associated with \c AVCOM_SA_GetFirmwareVersion() function response
 * 
 * The \c dataLen parameter is determined by \c AVCOM_API_ValidatePacket() and the 
 * \c data array is the return of the \c AVCOM_API_UnpackPacket() function.
 *     	   
 * \param [in]	data	Pointer to the array containing the data unpacked from the AVCOM Packet
 * \param [in]	dataLen	Len	The length of the \c data array
 * @param [out]	ver	Structure that holds the major, minor, build, stage, 
 *					and string of version
 *    				
 * \return Status of the decoding operation
 * 		   
 * \note This function must assume accurate data input (as validated by \c AVCOM_API_UnpackPacket() )
 * 		 and minimal error detection is possible.
 *******************************************************************************/
EXPORT int AVCOM_API_DecodeGetFirmwareVersion ( void *data, int dataLen, struct aov_version *ver );


/********************************************************************************
 *																				*
 *				AVCOM_API_DecodeGetFreqValue()									*
 *																				*
 ****************************************************************************//**
 * Decodes the data associated with \c AVCOM_SA_GetFreqValue() function response
 * 
 * The \c dataLen parameter is determined by \c AVCOM_API_ValidatePacket() and the 
 * \c data array is the return of the \c AVCOM_API_UnpackPacket() function.
 *     	   
 * \param [in]	data	Pointer to the array containing the data unpacked from the AVCOM Packet
 * \param [in]	dataLen	Len	The length of the \c data array
 * \param [out]	db	Amplitude in dBm at specified frequency
 *    				
 * \return Status of the decoding operation
 * 		   
 * \note This function must assume accurate data input (as validated by \c AVCOM_API_UnpackPacket() )
 * 		 and minimal error detection is possible.
 *******************************************************************************/
EXPORT int AVCOM_API_DecodeGetFreqValue ( void *data, int dataLen, double *db );


/********************************************************************************
 *																				*
 *				AVCOM_API_DecodeGetNetwork()									*
 *																				*
 ****************************************************************************//**
 * Decodes the data associated with \c AVCOM_SA_GetNetwork() function response
 * 
 * The \c dataLen parameter is determined by \c AVCOM_API_ValidatePacket() and the 
 * \c data array is the return of the \c AVCOM_API_UnpackPacket() function.
 *     	   
 * \param [in]	data	Pointer to the array containing the data unpacked from the AVCOM Packet
 * \param [in]	dataLen	Len	The length of the \c data array
 * @param [out]	net	Structure that holds network info array
 *    				
 * \return Status of the decoding operation
 * 		   
 * \note This function must assume accurate data input (as validated by \c AVCOM_API_UnpackPacket() )
 * 		 and minimal error detection is possible.
 *******************************************************************************/
EXPORT int AVCOM_API_DecodeGetNetwork ( void *data, int dataLen, struct aov_network *net );


/********************************************************************************
 *																				*
 *				AVCOM_API_DecodeGetProdDesc()									*
 *																				*
 ****************************************************************************//**
 * Decodes the data associated with \c AVCOM_SA_GetProdDesc() function response
 * 
 * The \c dataLen parameter is determined by \c AVCOM_API_ValidatePacket() and the 
 * \c data array is the return of the \c AVCOM_API_UnpackPacket() function.
 *     	   
 * \param [in]	data	Pointer to the array containing the data unpacked from the AVCOM Packet
 * \param [in]	dataLen	Len	The length of the \c data array
 * @param [out]	ppd	Structure that holds product description array
 *    				
 * \return Status of the decoding operation
 * 		   
 * \note This function must assume accurate data input (as validated by \c AVCOM_API_UnpackPacket() )
 * 		 and minimal error detection is possible.
 *******************************************************************************/
EXPORT int AVCOM_API_DecodeGetProdDesc ( void *data, int dataLen, struct aov_product *ppd );


/********************************************************************************
 *																				*
 *				AVCOM_API_DecodeGetRBWList()									*
 *																				*
 ****************************************************************************//**
 * Decodes the data associated with \c AVCOM_SA_GetRBWList() function response
 * 
 * The \c dataLen parameter is determined by \c AVCOM_API_ValidatePacket() and the 
 * \c data array is the return of the \c AVCOM_API_UnpackPacket() function.
 *     	   
 * \param [in]	data	Pointer to the array containing the data unpacked from the AVCOM Packet
 * \param [in]	dataLen	Len	The length of the \c data array
 * @param [out]	RBWs	Pointer to list of available RBW's - this function will malloc() this array
 * @param [out]	list_size	Number of elements in \c RBWs
 *    				
 * \return Status of the decoding operation
 * 		   
 * \note This function must assume accurate data input (as validated by \c AVCOM_API_UnpackPacket() )
 * 		 and minimal error detection is possible.
 * \note The application must free \c RBWs when done.
 *******************************************************************************/
EXPORT int AVCOM_API_DecodeGetRBWList ( void *data, int dataLen, int **RBWs, int *list_size );


/********************************************************************************
 *																				*
 *				AVCOM_API_DecodeGetTriggerSweep()								*
 *																				*
 ****************************************************************************//**
 * Decodes the data associated with \c AVCOM_SA_GetTriggerSweep() function response
 * 
 * The \c dataLen parameter is determined by \c AVCOM_API_ValidatePacket() and the 
 * \c data array is the return of the \c AVCOM_API_UnpackPacket() function.
 *     	   
 * \param [in]	data	Pointer to the array containing the data unpacked from the AVCOM Packet
 * \param [in]	dataLen	Len	The length of the \c data array
 * @param [out]		trace	Pointer to structure with amplitude/frequency information
 *    				
 * \return Status of the decoding operation
 * 		   
 * \note This function must assume accurate data input (as validated by \c AVCOM_API_UnpackPacket() )
 * 		 and minimal error detection is possible.
 *******************************************************************************/
EXPORT int AVCOM_API_DecodeGetTriggerSweep ( void *data, int dataLen, sAOV_SweepData *trace );


/********************************************************************************
 *																				*
 *				AVCOM_API_DecodeGetVBWList()									*
 *																				*
 ****************************************************************************//**
 * Decodes the data associated with \c AVCOM_SA_GetVBWList() function response
 * 
 * The \c dataLen parameter is determined by \c AVCOM_API_ValidatePacket() and the 
 * \c data array is the return of the \c AVCOM_API_UnpackPacket() function.
 *     	   
 * \param [in]	data	Pointer to the array containing the data unpacked from the AVCOM Packet
 * \param [in]	dataLen	Len	The length of the \c data array
 * @param [out]	VBWs	Pointer to list of available VBW's - this function will malloc() this array
 * @param [out]	list_size	Number of elements in \c VBWs
 *    				
 * \return Status of the decoding operation
 * 		   
 * \note This function must assume accurate data input (as validated by \c AVCOM_API_UnpackPacket() )
 * 		 and minimal error detection is possible.
 * \note The application must free \c VBWs when done.
 *******************************************************************************/
EXPORT int AVCOM_API_DecodeGetVBWList ( void *data, int dataLen, int **VBWs, int *list_size );


/********************************************************************************
 *																				*
 *				AVCOM_API_DecodeGetTriggerZeroSpan()							*
 *																				*
 ****************************************************************************//**
 * Decodes the data associated with \c AVCOM_SA_GetTriggerZeroSpan() function response
 * 
 * The \c dataLen parameter is determined by \c AVCOM_API_ValidatePacket() and the 
 * \c data array is the return of the \c AVCOM_API_UnpackPacket() function.
 *     	   
 * \param [in]	data	Pointer to the array containing the data unpacked from the AVCOM Packet
 * \param [in]	dataLen	Len	The length of the \c data array
 * @param [out]		trace	Pointer to structure with amplitude/frequency information
 *    				
 * \return Status of the decoding operation
 * 		   
 * \note This function must assume accurate data input (as validated by \c AVCOM_API_UnpackPacket() )
 * 		 and minimal error detection is possible.
 *******************************************************************************/
EXPORT int AVCOM_API_DecodeGetTriggerZeroSpan ( void *data, int dataLen, sAOV_ZeroSpanData *trace );


/********************************************************************************
 *																				*
 *				AVCOM_API_EncodeGetActiveBand()									*
 *																				*
 ****************************************************************************//**
 * Creates and encodes the data to perform the \c AVCOM_SA_GetActiveBand() 
 * function request
 * 
 * After this data encoding process is done, the data should then be packed via
 * \c AVCOM_API_PackPacket() prior to sending. The length of the data array is 
 * \c dataLen
 *    
 * Upon error, the function will return -1.
 *     	   
 * \param [out]	dataLen	The length of the created data array
 *    				
 * \return Pointer to the allocated data array
 * 		   
 * \note This function returns an allocated array. It is the user applications
 * 		 responsibility to free when done.
 *******************************************************************************/
EXPORT void *AVCOM_API_EncodeGetActiveBand ( int *dataLen );


/********************************************************************************
 *																				*
 *				AVCOM_API_EncodeGetActiveRBW()									*
 *																				*
 ****************************************************************************//**
 * Creates and encodes the data to perform the \c AVCOM_SA_GetActiveRBW() 
 * function request
 * 
 * After this data encoding process is done, the data should then be packed via
 * \c AVCOM_API_PackPacket() prior to sending. The length of the data array is 
 * \c dataLen
 *    
 * Upon error, the function will return -1.
 *     	   
 * \param [out]	dataLen	The length of the created data array
 *    				
 * \return Pointer to the allocated data array
 * 		   
 * \note This function returns an allocated array. It is the user applications
 * 		 responsibility to free when done.
 *******************************************************************************/
EXPORT void *AVCOM_API_EncodeGetActiveRBW ( int *dataLen );


/********************************************************************************
 *																				*
 *				AVCOM_API_GetActiveRefLevel()									*
 *																				*
 ****************************************************************************//**
 * Creates and encodes the data to perform the \c AVCOM_SA_GetActiveRefLevel() 
 * function request
 * 
 * After this data encoding process is done, the data should then be packed via
 * \c AVCOM_API_PackPacket() prior to sending. The length of the data array is 
 * \c dataLen
 *    
 * Upon error, the function will return -1.
 *     	   
 * \param [out]	dataLen	The length of the created data array
 *    				
 * \return Pointer to the allocated data array
 * 		   
 * \note This function returns an allocated array. It is the user applications
 * 		 responsibility to free when done.
 *******************************************************************************/
EXPORT void *AVCOM_API_EncodeGetActiveRefLevel ( int *dataLen );


/********************************************************************************
 *																				*
 *				AVCOM_API_EncodeGetActiveVBW()									*
 *																				*
 ****************************************************************************//**
 * Creates and encodes the data to perform the \c AVCOM_SA_GetActiveVBW() 
 * function request
 * 
 * After this data encoding process is done, the data should then be packed via
 * \c AVCOM_API_PackPacket() prior to sending. The length of the data array is 
 * \c dataLen
 *    
 * Upon error, the function will return -1.
 *     	   
 * \param [out]	dataLen	The length of the created data array
 *    				
 * \return Pointer to the allocated data array
 * 		   
 * \note This function returns an allocated array. It is the user applications
 * 		 responsibility to free when done.
 *******************************************************************************/
EXPORT void *AVCOM_API_EncodeGetActiveVBW ( int *dataLen );


/********************************************************************************
 *																				*
 *				AVCOM_API_EncodeGetBandInfo()									*
 *																				*
 ****************************************************************************//**
 * Creates and encodes the data to perform the \c AVCOM_SA_GetBandInfo() 
 * function request
 * 
 * After this data encoding process is done, the data should then be packed via
 * \c AVCOM_API_PackPacket() prior to sending. The length of the data array is 
 * \c dataLen
 *    
 * Upon error, the function will return -1.
 *     	   
 * \param [out]	dataLen	The length of the created data array
 * \param [out]	band	The band to get information from
 *    				
 * \return Pointer to the allocated data array
 * 		   
 * \note This function returns an allocated array. It is the user applications
 * 		 responsibility to free when done.
 *******************************************************************************/
EXPORT void *AVCOM_API_EncodeGetBandInfo ( int *dataLen, int band );


/********************************************************************************
 *																				*
 *				AVCOM_API_EncodeGetBoardTemp()									*
 *																				*
 ****************************************************************************//**
 * Creates and encodes the data to perform the \c AVCOM_SA_GetBoardTemp() 
 * function request
 * 
 * After this data encoding process is done, the data should then be packed via
 * \c AVCOM_API_PackPacket() prior to sending. The length of the data array is 
 * \c dataLen
 *    
 * Upon error, the function will return -1.
 *     	   
 * \param [out]	dataLen	The length of the created data array
 *    				
 * \return Pointer to the allocated data array
 * 		   
 * \note This function returns an allocated array. It is the user applications
 * 		 responsibility to free when done.
 *******************************************************************************/
EXPORT void *AVCOM_API_EncodeGetBoardTemp ( int *dataLen );


/********************************************************************************
 *																				*
 *				AVCOM_API_EncodeGetBuildDateTime()								*
 *																				*
 ****************************************************************************//**
 * Creates and encodes the data to perform the \c AVCOM_SA_GetBuildDateTime() 
 * function request
 * 
 * After this data encoding process is done, the data should then be packed via
 * \c AVCOM_API_PackPacket() prior to sending. The length of the data array is 
 * \c dataLen
 *    
 * Upon error, the function will return -1.
 *     	   
 * \param [out]	dataLen	The length of the created data array
 *    				
 * \return Pointer to the allocated data array
 * 		   
 * \note This function returns an allocated array. It is the user applications
 * 		 responsibility to free when done.
 *******************************************************************************/
EXPORT void *AVCOM_API_EncodeGetBuildDateTime ( int *dataLen );


/********************************************************************************
 *																				*
 *				AVCOM_API_EncodeGetFirmwareVersion()							*
 *																				*
 ****************************************************************************//**
 * Creates and encodes the data to perform the \c AVCOM_SA_GetFirmwareVersion() 
 * function request
 * 
 * After this data encoding process is done, the data should then be packed via
 * \c AVCOM_API_PackPacket() prior to sending. The length of the data array is 
 * \c dataLen
 *    
 * Upon error, the function will return -1.
 *     	   
 * \param [out]	dataLen	The length of the created data array
 *    				
 * \return Pointer to the allocated data array
 * 		   
 * \note This function returns an allocated array. It is the user applications
 * 		 responsibility to free when done.
 *******************************************************************************/
EXPORT void *AVCOM_API_EncodeGetFirmwareVersion ( int *dataLen );


/********************************************************************************
 *																				*
 *				AVCOM_API_EncodeGetFreqValue()									*
 *																				*
 ****************************************************************************//**
 * Creates and encodes the data to perform the \c AVCOM_SA_GetFreqValue() 
 * function request
 * 
 * After this data encoding process is done, the data should then be packed via
 * \c AVCOM_API_PackPacket() prior to sending. The length of the data array is 
 * \c dataLen
 *    
 * Upon error, the function will return -1.
 *     	   
 * \param [out]	dataLen	The length of the created data array
 *    				
 * \return Pointer to the allocated data array
 * 		   
 * \note This function returns an allocated array. It is the user applications
 * 		 responsibility to free when done.
 *******************************************************************************/
EXPORT void *AVCOM_API_EncodeGetFreqValue ( int *dataLen );



/********************************************************************************
 *																				*
 *				AVCOM_API_EncodeGetNetwork()									*
 *																				*
 ****************************************************************************//**
 * Creates and encodes the data to perform the \c AVCOM_SA_GetNetwork() 
 * function request
 * 
 * After this data encoding process is done, the data should then be packed via
 * \c AVCOM_API_PackPacket() prior to sending. The length of the data array is 
 * \c dataLen
 *    
 * Upon error, the function will return -1.
 *     	   
 * \param [out]	dataLen	The length of the created data array
 *    				
 * \return Pointer to the allocated data array
 * 		   
 * \note This function returns an allocated array. It is the user applications
 * 		 responsibility to free when done.
 *******************************************************************************/
EXPORT void *AVCOM_API_EncodeGetNetwork ( int *dataLen );


/********************************************************************************
 *																				*
 *				AVCOM_API_EncodeGetProdDesc()									*
 *																				*
 ****************************************************************************//**
 * Creates and encodes the data to perform the \c AVCOM_SA_GetProdDesc() 
 * function request
 * 
 * After this data encoding process is done, the data should then be packed via
 * \c AVCOM_API_PackPacket() prior to sending. The length of the data array is 
 * \c dataLen
 *    
 * Upon error, the function will return -1.
 *     	   
 * \param [out]	dataLen	The length of the created data array
 *    				
 * \return Pointer to the allocated data array
 * 		   
 * \note This function returns an allocated array. It is the user applications
 * 		 responsibility to free when done.
 *******************************************************************************/
EXPORT void *AVCOM_API_EncodeGetProdDesc ( int *dataLen );


/********************************************************************************
 *																				*
 *				AVCOM_API_EncodeGetRBWList()									*
 *																				*
 ****************************************************************************//**
 * Creates and encodes the data to perform the \c AVCOM_SA_GetRBWList() 
 * function request
 * 
 * After this data encoding process is done, the data should then be packed via
 * \c AVCOM_API_PackPacket() prior to sending. The length of the data array is 
 * \c dataLen
 *    
 * Upon error, the function will return -1.
 *     	   
 * \param [out]	dataLen	The length of the created data array
 *    				
 * \return Pointer to the allocated data array
 * 		   
 * \note This function returns an allocated array. It is the user applications
 * 		 responsibility to free when done.
 *******************************************************************************/
EXPORT void *AVCOM_API_EncodeGetRBWList ( int *dataLen );


/********************************************************************************
 *																				*
 *				AVCOM_API_EncodeGetTriggerSweep()								*
 *																				*
 ****************************************************************************//**
 * Creates and encodes the data to perform the \c AVCOM_SA_GetTriggerSweep() 
 * function request
 * 
 * After this data encoding process is done, the data should then be packed via
 * \c AVCOM_API_PackPacket() prior to sending. The length of the data array is 
 * \c dataLen
 *    
 * Upon error, the function will return -1.
 *     	   
 * \param [out]	dataLen	The length of the created data array
 *    				
 * \return Pointer to the allocated data array
 * 		   
 * \note This function returns an allocated array. It is the user applications
 * 		 responsibility to free when done.
 *******************************************************************************/
EXPORT void *AVCOM_API_EncodeGetTriggerSweep ( int *dataLen );


/********************************************************************************
 *																				*
 *				AVCOM_API_EncodeGetVBWList()									*
 *																				*
 ****************************************************************************//**
 * Creates and encodes the data to perform the \c AVCOM_SA_GetVBWList() 
 * function request
 * 
 * After this data encoding process is done, the data should then be packed via
 * \c AVCOM_API_PackPacket() prior to sending. The length of the data array is 
 * \c dataLen
 *    
 * Upon error, the function will return -1.
 *     	   
 * \param [out]	dataLen	The length of the created data array
 *    				
 * \return Pointer to the allocated data array
 * 		   
 * \note This function returns an allocated array. It is the user applications
 * 		 responsibility to free when done.
 *******************************************************************************/
EXPORT void *AVCOM_API_EncodeGetVBWList ( int *dataLen );


/********************************************************************************
 *																				*
 *				AVCOM_API_EncodePing()											*
 *																				*
 ****************************************************************************//**
 * Creates and encodes the data to perform the \c AVCOM_SA_Ping() function request
 * 
 * After this data encoding process is done, the data should then be packed via
 * \c AVCOM_API_PackPacket() prior to sending. The length of the data array is 
 * \c dataLen
 *    
 * Upon error, the function will return -1.
 *     	   
 * \param [out]	dataLen	The length of the created data array
 *    				
 * \return Pointer to the allocated data array
 * 		   
 * \note This function returns an allocated array. It is the user applications
 * 		 responsibility to free when done.
 *******************************************************************************/
EXPORT void *AVCOM_API_EncodePing ( int *dataLen );


/********************************************************************************
 *																				*
 *				AVCOM_API_EncodeReboot()										*
 *																				*
 ****************************************************************************//**
 * Creates and encodes the data to perform the \c AVCOM_SA_Reboot() function request
 * 
 * After this data encoding process is done, the data should then be packed via
 * \c AVCOM_API_PackPacket() prior to sending. The length of the data array is 
 * \c dataLen
 *    
 * Upon error, the function will return -1.
 *     	   
 * \param [out]	dataLen	The length of the created data array
 *    				
 * \return Pointer to the allocated data array
 * 		   
 * \note This function returns an allocated array. It is the user applications
 * 		 responsibility to free when done.
 *******************************************************************************/
EXPORT void *AVCOM_API_EncodeReboot ( int *dataLen );


/********************************************************************************
 *																				*
 *				AVCOM_API_EncodeSaveBandSettings()								*
 *																				*
 ****************************************************************************//**
 * Creates and encodes the data to perform the \c AVCOM_SA_SaveBandSettings() function request
 * 
 * After this data encoding process is done, the data should then be packed via
 * \c AVCOM_API_PackPacket() prior to sending. The length of the data array is 
 * \c dataLen
 *    
 * Upon error, the function will return -1.
 *     	   
 * \param [out]	dataLen	The length of the created data array
 *    				
 * \return Pointer to the allocated data array
 * 		   
 * \note This function returns an allocated array. It is the user applications
 * 		 responsibility to free when done.
 *******************************************************************************/
EXPORT void *AVCOM_API_EncodeSaveBandSettings ( int *dataLen );


/********************************************************************************
 *																				*
 *				AVCOM_API_EncodeSetAnalyzerName()								*
 *																				*
 ****************************************************************************//**
 * Creates and encodes the data to perform the \c AVCOM_SA_SetAnalyzerName() function request
 * 
 * After this data encoding process is done, the data should then be packed via
 * \c AVCOM_API_PackPacket() prior to sending. The length of the data array is 
 * \c dataLen
 *    
 * Upon error, the function will return -1.
 *     	   
 * \param [out]	dataLen	The length of the created data array
 * @param [in]		name	Name to set the Analyzer to
 *    				
 * \return Pointer to the allocated data array
 * 		   
 * \note This function returns an allocated array. It is the user applications
 * 		 responsibility to free when done.
 *******************************************************************************/
EXPORT void *AVCOM_API_EncodeSetAnalyzerName ( int *dataLen, char *name );


/********************************************************************************
 *																				*
 *				AVCOM_API_EncodeSetBand()										*
 *																				*
 ****************************************************************************//**
 * Creates and encodes the data to perform the \c AVCOM_SA_SetBand() function request
 * 
 * After this data encoding process is done, the data should then be packed via
 * \c AVCOM_API_PackPacket() prior to sending. The length of the data array is 
 * \c dataLen
 *    
 * Upon error, the function will return -1.
 *     	   
 * \param [out]	dataLen	The length of the created data array
 * @param [in]	band	index value for the band desired
 *    				
 * \return Pointer to the allocated data array
 * 		   
 * \note This function returns an allocated array. It is the user applications
 * 		 responsibility to free when done.
 *******************************************************************************/
EXPORT void *AVCOM_API_EncodeSetBand ( int *dataLen, int band );


/********************************************************************************
 *																				*
 *				AVCOM_API_EncodeSetBandName()									*
 *																				*
 ****************************************************************************//**
 * Creates and encodes the data to perform the \c AVCOM_SA_SetBandName() function request
 * 
 * After this data encoding process is done, the data should then be packed via
 * \c AVCOM_API_PackPacket() prior to sending. The length of the data array is 
 * \c dataLen
 *    
 * Upon error, the function will return -1.
 *     	   
 * \param [out]	dataLen	The length of the created data array
 * @param [in]		bandname	The band's name
 *    				
 * \return Pointer to the allocated data array
 * 		   
 * \note This function returns an allocated array. It is the user applications
 * 		 responsibility to free when done.
 *******************************************************************************/
EXPORT void *AVCOM_API_EncodeSetBandName ( int *dataLen, char *bandname );


/********************************************************************************
 *																				*
 *				AVCOM_API_EncodeSetFreqCSD()									*
 *																				*
 ****************************************************************************//**
 * Creates and encodes the data to perform the \c AVCOM_SA_SetFreqCSD() function request
 * 
 * After this data encoding process is done, the data should then be packed via
 * \c AVCOM_API_PackPacket() prior to sending. The length of the data array is 
 * \c dataLen
 *    
 * Upon error, the function will return -1.
 *     	   
 * \param [out]	dataLen	The length of the created data array
 * @param [in]		cfreq	The center frequency of the sweep (MHZ)
 * @param [in]		span	The requested frequency span of the sweep (MHZ)
 * @param [in]		max_pts	The number of data points to return
 *    				
 * \return Pointer to the allocated data array
 * 		   
 * \note This function returns an allocated array. It is the user applications
 * 		 responsibility to free when done.
 *******************************************************************************/
EXPORT void *AVCOM_API_EncodeSetFreqCSD ( int *dataLen, double cfreq, double span, int max_pts );


/********************************************************************************
 *																				*
 *				AVCOM_API_EncodeSetFreqCSS()									*
 *																				*
 ****************************************************************************//**
 * Creates and encodes the data to perform the \c AVCOM_SA_SetFreqCSS() function request
 * 
 * After this data encoding process is done, the data should then be packed via
 * \c AVCOM_API_PackPacket() prior to sending. The length of the data array is 
 * \c dataLen
 *    
 * Upon error, the function will return -1.
 *     	   
 * \param [out]	dataLen	The length of the created data array
 * @param [in]		cfreq	The center frequency of the sweep (MHZ)
 * @param [in]		span	The frequency span of the sweep (MHZ)
 * @param [in]		step	The step size of the sweep (MHZ)
 *    				
 * \return Pointer to the allocated data array
 * 		   
 * \note This function returns an allocated array. It is the user applications
 * 		 responsibility to free when done.
 *******************************************************************************/
EXPORT void *AVCOM_API_EncodeSetFreqCSS ( int *dataLen, double cfreq, double span, double step );


/********************************************************************************
 *																				*
 *				AVCOM_API_EncodeSetFreqSSS()									*
 *																				*
 ****************************************************************************//**
 * Creates and encodes the data to perform the \c AVCOM_SA_SetFreqSSS() function request
 * 
 * After this data encoding process is done, the data should then be packed via
 * \c AVCOM_API_PackPacket() prior to sending. The length of the data array is 
 * \c dataLen
 *    
 * Upon error, the function will return -1.
 *     	   
 * \param [out]	dataLen	The length of the created data array
 * @param [in]		fstart	The center frequency of the sweep (MHZ)
 * @param [in]		fstop	The frequency span of the sweep (MHZ)
 * @param [in]		fstep	The step size of the sweep (MHZ)
 *    				
 * \return Pointer to the allocated data array
 * 		   
 * \note This function returns an allocated array. It is the user applications
 * 		 responsibility to free when done.
 *******************************************************************************/
EXPORT void *AVCOM_API_EncodeSetFreqSSS ( int *dataLen, double fstart, double fstop, double fstep );


/********************************************************************************
 *																				*
 *				AVCOM_API_EncodeSetHostname()									*
 *																				*
 ****************************************************************************//**
 * Creates and encodes the data to perform the \c AVCOM_SA_SetHostname() 
 * function request
 * 
 * After this data encoding process is done, the data should then be packed via
 * \c AVCOM_API_PackPacket() prior to sending. The length of the data array is 
 * \c dataLen
 *    
 * Upon error, the function will return -1.
 *     	   
 * \param [out]	dataLen	The length of the created data array
 * \param [in]	hostname	The desired hostname. null terminated 16-byte ASCII max
 *    				
 * \return Pointer to the allocated data array
 * 		   
 * \note This function returns an allocated array. It is the user applications
 * 		 responsibility to free when done.
 * \note Only valid hostname characters (alpha, numeric, hyphen)
 *******************************************************************************/
EXPORT void *AVCOM_API_EncodeSetHostname ( int *dataLen, char *hostname );


/********************************************************************************
 *																				*
 *				AVCOM_API_EncodeSetNetwork()									*
 *																				*
 ****************************************************************************//**
 * Creates and encodes the data to perform the \c AVCOM_SA_SetNetwork() 
 * function request
 * 
 * After this data encoding process is done, the data should then be packed via
 * \c AVCOM_API_PackPacket() prior to sending. The length of the data array is 
 * \c dataLen
 *    
 * Upon error, the function will return -1.
 *     	   
 * \param [out]	dataLen	The length of the created data array
 * @param [in]	dynip	0 for static IP, non-zero for dynamic IP (DHCP) assignment
 * @param [in]	ip	New IP Address of device (ignored if dynip is non-zero)
 * @param [in]	sub New subnet Address of device (ignored if dynip is non-zero)
 * @param [in]	gw	New Gateway Address of device (ignored if dynip is non-zero)
 * 
 * \return Pointer to the allocated data array
 * 		   
 * \note This function returns an allocated array. It is the user applications
 * 		 responsibility to free when done.
 *******************************************************************************/
EXPORT void *AVCOM_API_EncodeSetNetwork ( int *dataLen, int dynip, unsigned long ip, unsigned long sub, unsigned long gw );



/********************************************************************************
 *																				*
 *				AVCOM_API_EncodeSetPort()										*
 *																				*
 ****************************************************************************//**
 * Creates and encodes the data to perform the \c AVCOM_SA_SetPort() 
 * function request
 * 
 * After this data encoding process is done, the data should then be packed via
 * \c AVCOM_API_PackPacket() prior to sending. The length of the data array is 
 * \c dataLen
 *    
 * Upon error, the function will return -1.
 *     	   
 * \param [out]	dataLen	The length of the created data array
 * \param [out]	port	The new desired port number 
 *    				
 * \return Pointer to the allocated data array
 * 		   
 * \note This function returns an allocated array. It is the user applications
 * 		 responsibility to free when done.
 *******************************************************************************/
EXPORT void *AVCOM_API_EncodeSetPort ( int *dataLen, unsigned short port );


/********************************************************************************
 *																				*
 *				AVCOM_API_EncodeSetRBW()										*
 *																				*
 ****************************************************************************//**
 * Creates and encodes the data to perform the \c AVCOM_SA_SetRBW() function request
 * 
 * After this data encoding process is done, the data should then be packed via
 * \c AVCOM_API_PackPacket() prior to sending. The length of the data array is 
 * \c dataLen
 *    
 * Upon error, the function will return -1.
 *     	   
 * \param [out]	dataLen	The length of the created data array
 * @param [in]	rbw 	Resolution Bandwidth in Hertz
 *    				
 * \return Pointer to the allocated data array
 * 		   
 * \note This function returns an allocated array. It is the user applications
 * 		 responsibility to free when done.
 *******************************************************************************/
EXPORT void *AVCOM_API_EncodeSetRBW ( int *dataLen, int rbw );


/********************************************************************************
 *																				*
 *				AVCOM_API_EncodeSetRefLevel()									*
 *																				*
 ****************************************************************************//**
 * Creates and encodes the data to perform the \c AVCOM_SA_SetRefLevel() function request
 * 
 * After this data encoding process is done, the data should then be packed via
 * \c AVCOM_API_PackPacket() prior to sending. The length of the data array is 
 * \c dataLen
 *    
 * Upon error, the function will return -1.
 *     	   
 * \param [out]	dataLen	The length of the created data array
 * @param [in]	reflvl	Reference level in dBm
 *    				
 * \return Pointer to the allocated data array
 * 		   
 * \note This function returns an allocated array. It is the user applications
 * 		 responsibility to free when done.
 *******************************************************************************/
EXPORT void *AVCOM_API_EncodeSetRefLevel ( int *dataLen, int reflvl );


/********************************************************************************
 *																				*
 *				AVCOM_API_EncodeSetVBW()										*
 *																				*
 ****************************************************************************//**
 * Creates and encodes the data to perform the \c AVCOM_SA_SetVBW() function request
 * 
 * After this data encoding process is done, the data should then be packed via
 * \c AVCOM_API_PackPacket() prior to sending. The length of the data array is 
 * \c dataLen
 *    
 * Upon error, the function will return -1.
 *     	   
 * \param [out]	dataLen	The length of the created data array
 * @param [in]	vbw		Video Bandwidth in Hertz
 *    				
 * \return Pointer to the allocated data array
 * 		   
 * \note This function returns an allocated array. It is the user applications
 * 		 responsibility to free when done.
 *******************************************************************************/
EXPORT void *AVCOM_API_EncodeSetVBW ( int *dataLen, int vbw );




/********************************************************************************
 *																				*
 *				AVCOM_API_EncodeSetZeroSpan()									*
 *																				*
 ****************************************************************************//**
 * Creates and encodes the data to perform the \c AVCOM_API_EncodeSetZeroSpan() function request
 *
 * @return Status of function call
 *
 * \param [out]	dataLen	The length of the created data array
 * @param [in]	freq	Frequency to set zero span to
 * @param [in]	numPts	Number of points to return
 * @param [in]	sample_rate	Time between samples
 * @param [in]	mode	Modes for sampling zero span
 *******************************************************************************/
EXPORT void *AVCOM_API_EncodeSetZeroSpan ( int *dataLen, double freq, int numPts, int sample_rate, int mode);


/********************************************************************************
 *																				*
 *				AVCOM_API_EncodeUDPSetHostname()								*
 *																				*
 ****************************************************************************//**
 * Creates and encodes the data to perform the \c AVCOM_SA_UDPSetHostname() 
 * function request
 * 
 * After this data encoding process is done, the data should then be packed via
 * \c AVCOM_API_PackPacket() prior to sending. The length of the data array is 
 * \c dataLen
 *    
 * Upon error, the function will return -1.
 *     	   
 * \param [out]	dataLen	The length of the created data array
 * \param [in]	mac	6-byte MAC Address of the analyzer requesting a hostname change
 * \param [in]	hostname	The desired hostname. null terminated 16-byte ASCII max
 *    				
 * \return Pointer to the allocated data array
 * 		   
 * \note This function returns an allocated array. It is the user applications
 * 		 responsibility to free when done.
 * \note Only valid hostname characters (alpha, numeric, hyphen)
 *******************************************************************************/
EXPORT void *AVCOM_API_EncodeUDPSetHostname ( int *dataLen, unsigned char *mac, char *hostname );


/********************************************************************************
 *																				*
 *				AVCOM_API_EncodeUDPSetNetwork()									*
 *																				*
 ****************************************************************************//**
 * Creates and encodes the data to perform the \c AVCOM_SA_UDPSetNetwork() 
 * function request
 * 
 * After this data encoding process is done, the data should then be packed via
 * \c AVCOM_API_PackPacket() prior to sending. The length of the data array is 
 * \c dataLen
 *    
 * Upon error, the function will return -1.
 *     	   
 * \param [out]	dataLen	The length of the created data array
 * 
 * @param [out]	dataLen	The length of the created data array
 * @param [in]	dynip	0 for static IP, non-zero for dynamic IP (DHCP) assignment
 * @param [in]	mac	6-byte MAC Address of the analyzer requesting a hostname change
 * @param [in]	ip	New IP Address of device (ignored if dynip is non-zero)
 * @param [in]	sub New subnet Address of device (ignored if dynip is non-zero)
 * @param [in]	gw	New Gateway Address of device (ignored if dynip is non-zero)
 * 
 * \return Pointer to the allocated data array
 * 		   
 * \note This function returns an allocated array. It is the user applications
 * 		 responsibility to free when done.
 *******************************************************************************/
EXPORT void *AVCOM_API_EncodeUDPSetNetwork ( int *dataLen, unsigned char *mac, int dynip, unsigned long ip, unsigned long sub, unsigned long gw );



/********************************************************************************
 *																				*
 *				AVCOM_API_EncodeUDPSetPort()									*
 *																				*
 ****************************************************************************//**
 * Creates and encodes the data to perform the \c AVCOM_SA_UDPSetPort() 
 * function request
 * 
 * After this data encoding process is done, the data should then be packed via
 * \c AVCOM_API_PackPacket() prior to sending. The length of the data array is 
 * \c dataLen
 *    
 * Upon error, the function will return -1.
 *     	   
 * \param [out]	dataLen	The length of the created data array
 * \param [in]	mac	6-byte MAC Address of the analyzer requesting a hostname change
 * \param [out]	port	The new desired port number 
 *    				
 * \return Pointer to the allocated data array
 * 		   
 * \note This function returns an allocated array. It is the user applications
 * 		 responsibility to free when done.
 *******************************************************************************/
EXPORT void *AVCOM_API_EncodeUDPSetPort ( int *dataLen, unsigned char *mac, unsigned short port );

#endif /* defined(AOV_DEPEND_SA) || defined(AOV_DEPEND_CUS_PACKING) */


#ifdef AOV_DEPEND_CUS_PACKING
/********************************************************************************
 *																				*
 *				AVCOM_API_PackPacket()											*
 *																				*
 ****************************************************************************//**
 * Packs an AVCOM Packet based a provided data array, and data array length.
 * 
 * The function will pack data into an AVCOM Packet given an AVCOM data array \c data
 * and data length \c dataLen. The function will return the AVCOM Packet pointer location and
 * the AVCOM Packet length \c packetLen.
 * 
 * \note The user application is responsible for free'ing the allocated array when it is
 * no longer needed. 
 * \note The function will return \c -1 if an error is encountered.
 *  	   
 * \param [in]	data	The location of the array containing AVCOM data information
 * \param [in]	dataLen		The length of the AVCOM data array
 * \param [out]	packetLen	The length of the created AVCOM Packet array
 * 				
 * \return Pointer to the allocated AVCOM Packet array
 *******************************************************************************/
EXPORT void *AVCOM_API_PackPacket ( void *data, int dataLen, int *packetLen );


/********************************************************************************
 *																				*
 *				AVCOM_API_UnpackPacket()										*
 *																				*
 ****************************************************************************//**
 * Unpacks an AVCOM Packet and extracts the command, status return, and data 
 * from an AVCOM Packet. 
 * 
 * The \c cmd is the packet type or command that the AVCOM Packet is. This \c cmd
 * can then be used to enter a switch statement so that the appropriate decode function
 * can be called.
 * 
 * The \c packet is the start location of the AVCOM Packet to be unpacked and \c data
 * is the allocated array where the packet's data will be copied into. The \c dataLen
 * should equal exactly the amount of data inside the AVCOM Packet, otherwise a 
 * AOV_WARN_PACKET_INCOMPLETE error will be returned. 
 * 
 * This function neither allocates nor free's arrays, so both operations must be 
 * done by the user application if appropriate.
 *    
 * The return argument will return the status of the analyzer. A Specific error code (-1)
 * will be returned if the function itself encounters an error. Otherwise the error code
 * will be from the Spectrum Analyzer, and might be command specific.
 *     	   
 * \param [in]	packet	The location of the AVCOM Packet found in \c AVCOM_API_ValidatePacket()
 * \param [out]	cmd		The Packet type or Command, used for decode switch operation
 * \param [in,out]	data	The pointer to where the packet's data is to be extracted
 * \param [in]	dataLen	The length of \c data, in bytes, as determined in \c AVCOM_API_ValidatePacket()
 *    				
 * \return Normally The Spectrum Analyzer's command status response but can also be
 * 		   AOV_ERR_PACKET_INVALID, AOV_WARN_PACKET_INCOMPLETE, or AOV_ERR_UNEXPECTED_CMD
 *    	
 *******************************************************************************/
EXPORT int AVCOM_API_UnpackPacket ( void *packet, int *cmd, void *data, int dataLen );


/********************************************************************************
 *																				*
 *				AVCOM_API_ValidatePacket()										*
 *																				*
 ****************************************************************************//**
 * Validates or verifies if a valid and complete AVCOM packet resides at the
 * beginning of a buffer.
 * 
 * The fist byte of the AVCOM Packet must be at the first byte of the incoming
 * data buffer, \c buffer. At a minimum the buffer size (\c size) needs to be provided to
 * define the buffer length to prevent memory access conditions. Ideally \c size would define
 * the number of bytes currently in the \c buffer as it will improve ability to detect between
 * incomplete and invalid packets.
 *   
 * \c packet should be passed as a NULL void pointer. The function will malloc the 
 * buffer to the necessary size. The size of the \c packet array in bytes will be defined
 *  but \c packetLen.
 *   
 * The function will return the status. Possible status returns are as follows:
 * - Function completed successfully and an AVCOM Packet exists at \c packet of size \c packetLen  
 * - No AVCOM Packets found. A critical error and the buffer many need to be flushed  
 * - Partial or incomplete AVCOM Packet found.
 * 
 * \param [in]	buffer	The buffer in which the function will look for a complete AVCOM 
 *						Packet in
 * \param [in]	size	The size of \c buffer in bytes, or number of bytes in buffer
 * \param [out]	packetLen	The length of the AVCOM Packet, in bytes
 * \param [in]	dataLen	Len	The length of the \c data array
 * 	   				
 * \return status of the function
 * 	
 * \note It is the users responsibility to free \c packet when no longer needed.
 *******************************************************************************/
EXPORT int AVCOM_API_ValidatePacket ( void *buffer, int size, int *packetLen, int *dataLen );
#endif /* AOV_DEPEND_CUS_PACKING */




#ifdef __cplusplus
}
#endif


#endif /* _AOV_ASYNC_H_ */
