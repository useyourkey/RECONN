
#include "aov_opts.h"
#ifdef AOV_DEPEND_ENG


#include <stdlib.h>
#include "stdafx.h"

#include "aov_core.h"
#include "aov_common.h"
#include "aov_sa.h"
#include "aov_eng.h"
#include "aov_protocol.h"
#include "aov_errno.h"
#include "crc.h"
#include "ftd2xx.h"




//static int AVCOM_INT_SetLists ( void *handle, unsigned short cmd, int *list, int list_size );




#ifdef _WIN32
#define snprintf sprintf_s
#define strncpy(dst,src,size) strncpy_s(dst,size,src,size)
#endif 


extern int AVCOM_INT_FlashSBS2Shared ( void *handle, unsigned short cmd, unsigned char* data, int data_len );

#if 0

// A MS Function
/********************************************************************************
*																				*
*		AVCOM_INT_SetLists														*
*																				*
********************************************************************************/
static int AVCOM_INT_SetLists ( void *handle, unsigned short cmd, int *list, int list_size )
{
	unsigned char sa_tx[64];
	unsigned char sa_rx[1024];
	int recvlen;
	int i = 0;
	int j = 0;
	int k = 0;
	int iResult;
	dataConvert	cvt;
//	unsigned short count;//, size, offset;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	//Put command at start of data
	PACK_API_CMD(sa_tx,cmd,k);

	PACK2(sa_tx, list_size, k);

	for ( i = 0; i < list_size; i++ ) {
		PACK4(sa_tx,list[i],k);
	}

	if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
		return iResult;
	if ( (iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR )
		return iResult;	
	// Check ACK
	if ( (iResult = AVCOM_INT_ReturnACK(cmd, sa_rx, &j)) != AOV_NO_ERROR )
		return iResult;

	return AOV_NO_ERROR;

}
#endif


// A MS Function
/********************************************************************************
*																				*
*		AVCOM_ENG_BootBootloader													*
*																				*
********************************************************************************/
int AVCOM_ENG_BootBootloader ( void *handle )
{
	unsigned char sa_tx[1024];
	unsigned char sa_rx[1024];
	int j = 0;
	int k = 0;
	int recvlen;
	int iResult;
	unsigned short cmd = API_BOOT_BOOTLOADER;
	dataConvert cvt;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	PACK_API_CMD(sa_tx, cmd, k);

	if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
		return iResult;
	if ( (iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR )
		return iResult;	//ACK

	iResult = AVCOM_INT_ReturnACK(cmd, sa_rx, &j);

	return iResult;
}

#if 0

// A MS Function
/********************************************************************************
*																				*
*		AVCOM_ENG_GetCal20dB													*
*																				*
********************************************************************************/
int AVCOM_ENG_GetCal20dB (void *handle, double *db)
{
	unsigned char sa_tx[1024];
	unsigned char sa_rx[1024];
//	int i = 0;
	int j = 0;
	int k = 0;
	int recvlen;
	int iResult;
	unsigned short cmd = API_ENG_GET_CAL_20DB;
	unsigned short value;
	dataConvert cvt;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	PACK_API_CMD(sa_tx, cmd, k);

	if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
		return iResult;
	if ( (iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR )
		return iResult;	//ACK

	if ( (iResult = AVCOM_INT_ReturnACK(cmd, sa_rx, &j)) != AOV_NO_ERROR )
		return iResult;
	
	if ( iResult == AOV_NO_ERROR ) {
		UNPACK2(value,sa_rx, j);
	}

	*db = (0.01 * value);

	return iResult;
}


// A MS Function
/********************************************************************************
*																				*
*		AVCOM_ENG_GetCalDSA														*
*																				*
********************************************************************************/
int AVCOM_ENG_GetCalDSA (void *handle, double *dbarray)
{
	unsigned char sa_tx[1024];
	unsigned char sa_rx[1024];
	int i = 0;
	int j = 0;
	int k = 0;
	int recvlen;
	int iResult;
	unsigned short cmd = API_ENG_GET_CAL_DSA;
	unsigned short value;
	dataConvert cvt;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	PACK_API_CMD(sa_tx, cmd, k);

	if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
		return iResult;
	if ( (iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR )
		return iResult;	//ACK

	if ( (iResult = AVCOM_INT_ReturnACK(cmd, sa_rx, &j)) != AOV_NO_ERROR )
		return iResult;
	
	if ( iResult == AOV_NO_ERROR ) {
		for( i = 0; i < 64; i++ ) {
			UNPACK2(value,sa_rx, j);
			dbarray[i] = (0.01 * value);
		}
	}


	return iResult;
}

#endif


// A MS Function
/********************************************************************************
*																				*
*		AVCOM_ENG_FlashSBS2bootloader											*
*																				*
********************************************************************************/
int AVCOM_ENG_FlashSBS2bootloader ( void *handle, unsigned char* data, int data_len )
{
	int iResult;

	iResult = AVCOM_INT_FlashSBS2Shared ( handle, API_FLASH_BOOT_START, data, data_len );
	return iResult;
}

// A MS Function
/********************************************************************************
*																				*
*		AVCOM_ENG_FlashSBS2init													*
*																				*
********************************************************************************/
int AVCOM_ENG_FlashSBS2init ( void *handle, unsigned char* data, int data_len )
{
	int iResult;

	iResult = AVCOM_INT_FlashSBS2Shared ( handle, API_FLASH_INIT_START, data, data_len );
	return iResult;
}


#if 0

// A MS Function
/********************************************************************************
*																				*
*		AVCOM_ENG_GetADCValue													*
*																				*
********************************************************************************/
int AVCOM_ENG_GetADCValue ( void *handle, unsigned short *value )
{
	unsigned char sa_tx[64];
	unsigned char sa_rx[1024];
	int recvlen;
	int j = 0;
	int k = 0;
	unsigned short cmd = API_ENG_GET_ADC;
	int iResult;
	dataConvert	cvt;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	PACK_API_CMD(sa_tx, cmd, k);

	if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
		return iResult;
	if ( (iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR )
		return iResult;	//ACK

	iResult = AVCOM_INT_ReturnACK(cmd, sa_rx, &j);

	if ( iResult == AOV_NO_ERROR ) {
		UNPACK2((*value),sa_rx, j);
	}

	return AOV_NO_ERROR;
}


// A MS Function
/********************************************************************************
*																				*
*		AVCOM_ENG_GetBootloaderVersion											*
*																				*
********************************************************************************/
int AVCOM_ENG_GetBootloaderVersion ( void *handle, struct aov_version *ver )
{
	return AVCOM_INT_GetVersion( handle, API_BOOTLOADER_VERSION, ver);
}


// A MS Function
/********************************************************************************
*																				*
*		AVCOM_ENG_GetSweepADC													*
*																				*
********************************************************************************/
int AVCOM_ENG_GetSweepADC(void *handle, sAVCOM_ENG_SweepData *trace)
{
	unsigned char sa_tx[64];
	unsigned char sa_rx[100000];
	int recvlen;
	int i = 0;
	int j = 0;
	int k = 0;
	unsigned short cmd = API_ENG_GET_SWEEP_DATA;
	int iResult;
	double			dFreqCurr;
	dataConvert	cvt;
	unsigned char *data;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	k = 2;
	data = (unsigned char *)AVCOM_INT_EncodeCMD ( cmd );
	memcpy(sa_tx, data, k);
	free(data);
	if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
		return iResult;
	if ( (iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR )
		return iResult;	
	//ACK
	if ( (iResult = AVCOM_INT_ReturnACK(cmd, sa_rx, &j)) != AOV_NO_ERROR )
		return iResult;

	trace->count=0;
	trace->fStart=0;
	trace->fStop=0;
	trace->fStep=0;
	//trace->sweep=NULL;

	UNPACK2(trace->count, sa_rx, j);
	UNPACK_FREQ_K2M(trace->fStart, sa_rx, j);
	UNPACK_FREQ_K2M(trace->fStop, sa_rx, j);
	UNPACK_FREQ_K2M(trace->fStep, sa_rx, j);
	
	dFreqCurr = trace->fStart;
	if (trace->fStop < trace->fStart)
		dFreqCurr = trace->fStop;

	for (i=0;i<trace->count;i++) 
	{
		UNPACK2(trace->sweep[i].data, sa_rx, j);
		trace->sweep[i].frequency=dFreqCurr;
		dFreqCurr+=trace->fStep;
	}



	return AOV_NO_ERROR;
		
}


// A MS Function
/********************************************************************************
*																				*
*		AVCOM_ENG_GetSweepADCArrays												*
*																				*
********************************************************************************/
int AVCOM_ENG_GetSweepADCArrays(void *handle, double* start, double* stop, double* step, unsigned short* num, double* freq,  unsigned short* data)
{
	int i;
	int iResult;
	sAVCOM_ENG_SweepData sweepdata;

	iResult = AVCOM_ENG_GetSweepADC(handle, &sweepdata);

	*num = sweepdata.count;
	*start = sweepdata.fStart;
	*step = sweepdata.fStep;
	*stop = sweepdata.fStop;

	for (i=0; i<sweepdata.count; i++)
	{
		data[i] = sweepdata.sweep[i].data;
		freq[i] = sweepdata.sweep[i].frequency;
	}

	return iResult;
		
}


/********************************************************************************
*																				*
*		AVCOM_ENG_GetUartVersion												*
*																				*
********************************************************************************/
// A MS Function
int AVCOM_ENG_GetUartVersion ( void *handle, struct aov_version *ver )
{
	return AVCOM_INT_GetVersion( handle, API_UARTLOADER_VERSION, ver );
}


// A MS Function
#ifdef AOV_DEPEND_USB
/********************************************************************************
*																				*
*		AVCOM_ENG_LoadUART														*
*																				*
********************************************************************************/
int AVCOM_ENG_LoadUART( void *handle, char *data, int data_len )
{
	int iResult;
	unsigned char sendbuf[1024];
	unsigned char recvbuf[1024];
	int i;
	FT_STATUS ftStatus;
	AOVhandle *lhandle = (AOVhandle *)handle;
	DWORD dwModemStatus = 0; 


	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	if (lhandle->socket_type != AOV_USB_DEFAULT)
		return AOV_ERR_INVALID_SOCKET;		// I do not support this over ethernet

	ftStatus = FT_OpenEx(lhandle->usblocId,FT_OPEN_BY_LOCATION,&lhandle->ftHandle);
	if (ftStatus == FT_OK) 
	{ 
		// success
	} else { 
		// failure
		AVCOM_SA_Disconnect(handle);
		return AOV_ERR_OPEN_SOCKET_FAILED;
	}

	ftStatus = FT_ResetDevice(lhandle->ftHandle);
	if (ftStatus == FT_OK) 
	{ 
		// success
	} else { 
		// failure
		AVCOM_SA_Disconnect(handle);
		return AOV_ERR_SETSOCKETOPT_FAILED;
	}

	ftStatus = FT_SetBaudRate(lhandle->ftHandle,230400);
	if (ftStatus == FT_OK) 
	{ 
		// success
	} else { 
		// failure
		AVCOM_SA_Disconnect(handle);
		return AOV_ERR_SETSOCKETOPT_FAILED;
	}

	ftStatus = FT_SetDataCharacteristics(lhandle->ftHandle,FT_BITS_8, FT_STOP_BITS_1, FT_PARITY_NONE);
	if (ftStatus == FT_OK) 
	{ 
		// success
	} else { 
		// failure	
		AVCOM_SA_Disconnect(handle);
		return AOV_ERR_SETSOCKETOPT_FAILED;
	}

	ftStatus = FT_SetFlowControl(lhandle->ftHandle,FT_FLOW_RTS_CTS,0,0);
	if (ftStatus == FT_OK) 
	{ 
		// success
	} else { 
		// failure
		AVCOM_SA_Disconnect(handle);
		return AOV_ERR_SETSOCKETOPT_FAILED;
	}

	ftStatus = FT_Purge(lhandle->ftHandle,FT_PURGE_RX | FT_PURGE_TX);
	if (ftStatus == FT_OK) 
	{ 
		// success
	} else {	
		// failure
		AVCOM_SA_Disconnect(handle);
		return AOV_ERR_SETSOCKETOPT_FAILED;
	}


			ftStatus = FT_GetModemStatus(lhandle->ftHandle, &dwModemStatus); 
			ftStatus = FT_GetModemStatus(lhandle->ftHandle, &dwModemStatus);
			ftStatus = FT_SetUSBParameters(lhandle->ftHandle, 64, 64);
	// First step: send autobaud character '@' (0x40)
	// Second step: Recive four bytes of data, look for proper response
	sendbuf[0] = '@';
	iResult =  AVCOM_INT_SendData(handle, sendbuf, 1);
	iResult = AVCOM_INT_RecvData(handle, recvbuf, 4);
	for (i = 0; i < data_len; i+=2)
	{
	//	sendbuf[0] = data[i];
		iResult =  AVCOM_INT_SendData(handle, (unsigned char *)&data[i], 2);
	//	iResult =  AVCOM_INT_SendData(handle, &sendbuf[0], 1);
		//iResult =  AVCOM_INT_SendData(handle, data, data_len);
		
//		if(ftStatus != FT_OK) 
//		{ 
			// FT_Open failed return; 
//		}

//		do {
//			ftStatus = FT_GetModemStatus(lhandle->ftHandle, &dwModemStatus); 
//			if (ftStatus == FT_OK) 
//			{ 
//				dwModemStatus = (dwModemStatus & 0x000000FF); 
//			} else { 
//				dwModemStatus = 0;
//				for (k=0; k<0x8FFF; k++);
//			}
//		} while (!(dwModemStatus & 0x10));
	}

	return AVCOM_SA_Disconnect(handle);
}
#endif /* AOV_DEPEND_USB */


// A MS Function
/********************************************************************************
*																				*
*		AVCOM_ENG_Set20dB														*
*																				*
********************************************************************************/
int AVCOM_ENG_Set20dB ( void *handle, int value )
{
	unsigned char sa_tx[1024];
	unsigned char sa_rx[1024];
//	int i = 0;
	int j = 0;
	int k = 0;
	int recvlen;
	int iResult;
	unsigned short cmd = API_ENG_SET_20DB;
	dataConvert cvt;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	PACK_API_CMD(sa_tx, cmd, k);
	
	PACK2(sa_tx, value, k);

	if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
		return iResult;
	if ( (iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR )
		return iResult;	//ACK

	iResult = AVCOM_INT_ReturnACK(cmd, sa_rx, &j);

	return iResult;
}


// A MS Function
/********************************************************************************
*																				*
*		AVCOM_ENG_SetCal20dB													*
*																				*
********************************************************************************/
int AVCOM_ENG_SetCal20dB (void *handle, double freq)
{
	unsigned char sa_tx[1024];
	unsigned char sa_rx[1024];
//	int i = 0;
	int j = 0;
	int k = 0;
	int recvlen;
	int iResult;
	unsigned short cmd = API_ENG_CAL_20DB;
	dataConvert cvt;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	PACK_API_CMD(sa_tx, cmd, k);
	
	freq *= 1000;
	PACK4(sa_tx, freq, k);

	if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
		return iResult;
	if ( (iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR )
		return iResult;	//ACK

	if ( (iResult = AVCOM_INT_ReturnACK(cmd, sa_rx, &j)) != AOV_NO_ERROR )
		return iResult;

	return iResult;
}


// A MS Function
/********************************************************************************
*																				*
*		AVCOM_ENG_SetCalAttn													*
*																				*
********************************************************************************/
int AVCOM_ENG_SetCalAttn (void *handle, double freq)
{
	unsigned char sa_tx[1024];
	unsigned char sa_rx[1024];
//	int i = 0;
	int j = 0;
	int k = 0;
	int recvlen;
	int iResult;
	unsigned short cmd = API_ENG_CAL_ATTN;
	dataConvert cvt;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	PACK_API_CMD(sa_tx, cmd, k);
	
	freq *= 1000;
	PACK4(sa_tx, freq, k);

	if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
		return iResult;
	if ( (iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR )
		return iResult;	//ACK

	if ( (iResult = AVCOM_INT_ReturnACK(cmd, sa_rx, &j)) != AOV_NO_ERROR )
		return iResult;

	return iResult;
}


// A MS Function
/********************************************************************************
*																				*
*		AVCOM_ENG_SetCalDSA														*
*																				*
********************************************************************************/
int AVCOM_ENG_SetCalDSA (void *handle, double freq)
{
	unsigned char sa_tx[1024];
	unsigned char sa_rx[1024];
//	int i = 0;
	int j = 0;
	int k = 0;
	int recvlen;
	int iResult;
	unsigned short cmd = API_ENG_CAL_DSA;
	dataConvert cvt;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	PACK_API_CMD(sa_tx, cmd, k);
	
	freq *= 1000;
	PACK4(sa_tx, freq, k);

	if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
		return iResult;
	if ( (iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR )
		return iResult;	//ACK

	if ( (iResult = AVCOM_INT_ReturnACK(cmd, sa_rx, &j)) != AOV_NO_ERROR )
		return iResult;

	return iResult;
}


// A MS Function
/********************************************************************************
*																				*
*		AVCOM_ENG_SetBandInfo													*
*																				*
********************************************************************************/
int AVCOM_ENG_SetBandInfo( void *handle, int num, struct aov_eng_band_info info )
{
	unsigned char sa_tx[1024];
	unsigned char sa_rx[1024];
//	int i = 0;
	int j = 0;
	int k = 0;
	unsigned short cmd = API_SET_BAND_INFO;
	int recvlen;
	int iResult;
	dataConvert	cvt;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	//Put command at start of data
	PACK_API_CMD(sa_tx,cmd,k);
	
	PACK2(sa_tx, num, k);

	PACK2(sa_tx, info.phy_input, k);
	PACK2(sa_tx, info.IF, k);
	PACK_STRCPY(sa_tx,info.name,k,AOV_BAND_NAME_LEN-1);
	PACK4(sa_tx,info.min_freq,k);
	PACK4(sa_tx,info.max_freq,k);
//	PACK4(sa_tx,info.min_step,k);
//	PACK2(sa_tx,info.min_db,k);
	PACK2(sa_tx,info.max_db,k);
	PACK2(sa_tx,info.min_reflvl,k);
	PACK2(sa_tx,info.max_reflvl,k);


	if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
		return iResult;
	if ( (iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR )
		return iResult;	
	// Check ACK
	if ( (iResult = AVCOM_INT_ReturnACK(cmd, sa_rx, &j)) != AOV_NO_ERROR )
		return iResult;	

	return iResult;
}



// A MS Function
/********************************************************************************
*																				*
*		AVCOM_ENG_SetDAC														*
*																				*
********************************************************************************/
int AVCOM_ENG_SetDAC ( void *handle, int dac, int value )
{
	unsigned char sa_tx[1024];
	unsigned char sa_rx[1024];
	int j = 0;
	int k = 0;
	int recvlen;
	int iResult;
	unsigned short cmd = API_ENG_SET_DAC;
	dataConvert cvt;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	if ( (value < 0) || (value > 0x3FFF) )	// Only 12-bit values accepted and passed
		return AOV_ERR_INVALID_PARAM;

	PACK_API_CMD(sa_tx, cmd, k);

	PACK2(sa_tx, dac, k);
	PACK2(sa_tx, value, k);

	if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
		return iResult;
	if ( (iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR )
		return iResult;	//ACK

	iResult = AVCOM_INT_ReturnACK(cmd, sa_rx, &j);

	return iResult;
}




// A MS Function
/********************************************************************************
*																				*
*		AVCOM_ENG_SetDDSFreq													*
*																				*
********************************************************************************/
int AVCOM_ENG_SetDDSFreq ( void *handle, double freq ) {
	unsigned char sa_tx[1024];
	unsigned char sa_rx[1024];
	int j = 0;
	int k = 0;
	int recvlen;
	int iResult;
	unsigned short cmd = API_ENG_SET_DDSFREQ;

	dataConvert	cvt;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	PACK_API_CMD(sa_tx, cmd, k);

	PACKF(sa_tx, (float)(freq*1000), k);
	
	if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
		return iResult;
	if ( (iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR )
		return iResult;	//ACK
	if ( (iResult = AVCOM_INT_ReturnACK(cmd, sa_rx, &j)) != AOV_NO_ERROR )
		return iResult;

	return AOV_NO_ERROR;
}




// A MS Function
/********************************************************************************
*																				*
*		AVCOM_ENG_SetDDSSSS														*
*																				*
********************************************************************************/
int AVCOM_ENG_SetDDSSSS ( void *handle, double fStart, double fStop, double fStep ) {
	unsigned char sa_tx[1024];
	unsigned char sa_rx[1024];
	int j = 0;
	int k = 0;
	int recvlen;
	int iResult;
	unsigned short cmd = API_ENG_SET_DDSSSS;

	dataConvert	cvt;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	fStart *= 1000;
	fStop *= 1000;
	fStep *= 1000;

	PACK_API_CMD(sa_tx,cmd,k);
	PACKF(sa_tx,(float)fStart,k);
	PACKF(sa_tx,(float)fStop,k);
	PACKF(sa_tx,(float)fStep,k);
	
	if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
		return iResult;
	if ( (iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR )
		return iResult;	//ACK
	if ( (iResult = AVCOM_INT_ReturnACK(cmd, sa_rx, &j)) != AOV_NO_ERROR )
		return iResult;

	return AOV_NO_ERROR;
}

// A MS Function
/********************************************************************************
*																				*
*		AVCOM_ENG_SetDDSWait													*
*																				*
********************************************************************************/
int AVCOM_ENG_SetDDSWait ( void *handle, unsigned int tWait) 
{
	unsigned char sa_tx[1024];
	unsigned char sa_rx[1024];
	int j = 0;
	int k = 0;
	int recvlen;
	int iResult;
	unsigned short cmd = API_ENG_SET_DDS_WAIT;
	dataConvert	cvt;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	PACK_API_CMD(sa_tx, cmd, k);

	PACK4(sa_tx, tWait, k);

	if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
		return iResult;
	if ( (iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR )
		return iResult;	//ACK
	if ( (iResult = AVCOM_INT_ReturnACK(cmd, sa_rx, &j)) != AOV_NO_ERROR )
		return iResult;

	return AOV_NO_ERROR;
}


// A MS Function
/********************************************************************************
*																				*
*		AVCOM_ENG_SetDSA														*
*																				*
********************************************************************************/
int AVCOM_ENG_SetDSA ( void *handle, double value )
{
	unsigned char sa_tx[1024];
	unsigned char sa_rx[1024];
	int j = 0;
	int k = 0;
	int recvlen;
	int iResult;
//	int value32;
	unsigned short cmd = API_ENG_SET_DSA;
	dataConvert	cvt;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	if ( (value < AOV_DSA_MIN_DB) || (value > AOV_DSA_MAX_DB) )	// Valid 0 to 31.5
		return AOV_ERR_INVALID_PARAM;

	if ( ((unsigned int)(value *2)) != (value*2) )	// Must be in 0.5 steps
		return AOV_ERR_INVALID_PARAM;

//	value32 = (int)(value * 2);

	PACK_API_CMD(sa_tx, cmd, k);

	PACK2(sa_tx, (value * 2), k);

	if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
		return iResult;
	if ( (iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR )
		return iResult;	//ACK

	iResult = AVCOM_INT_ReturnACK(cmd, sa_rx, &j);

	return iResult;
}


// A MS Function
/********************************************************************************
*																				*
*		AVCOM_ENG_SetGlobalWait													*
*																				*
********************************************************************************/
int AVCOM_ENG_SetGlobalWait ( void *handle, unsigned int tWait) 
{
	unsigned char sa_tx[1024];
	unsigned char sa_rx[1024];
//	int i = 0;
	int j = 0;
	int k = 0;
	unsigned short cmd = API_ENG_GLBL_WAIT;
	int recvlen;
	int iResult;
	dataConvert	cvt;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	//Put command at start of data
	PACK_API_CMD(sa_tx,cmd,k);

	PACK4(sa_tx, tWait, k);

	if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
		return iResult;
	if ( (iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR )
		return iResult;	//ACK
	if ( (iResult = AVCOM_INT_ReturnACK(cmd, sa_rx, &j)) != AOV_NO_ERROR )
		return iResult;

	return AOV_NO_ERROR;
}






// A MS Function
/********************************************************************************
*																				*
*		AVCOM_ENG_SetIF															*
*																				*
********************************************************************************/
int AVCOM_ENG_SetIF ( void *handle, int value )
{
	unsigned char sa_tx[1024];
	unsigned char sa_rx[1024];
	int j = 0;
	int k = 0;
	int recvlen;
	int iResult;
	unsigned short cmd = API_ENG_SET_IF;
	dataConvert	cvt;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	PACK_API_CMD(sa_tx, cmd, k);

	PACK2(sa_tx, value, k);

	if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
		return iResult;
	if ( (iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR )
		return iResult;	//ACK

	iResult = AVCOM_INT_ReturnACK(cmd, sa_rx, &j);

	return iResult;
}







// A MS Function
/********************************************************************************
*																				*
*		AVCOM_ENG_SetMAC														*
*																				*
********************************************************************************/
int AVCOM_ENG_SetMAC ( void *handle, unsigned char* mac )
{
	unsigned char sa_tx[1024];
	unsigned char sa_rx[1024];
	int j = 0;
	int k = 0;
	int recvlen;
	int iResult;
	unsigned short cmd = API_ENG_SET_MAC_HOST;
	dataConvert	cvt;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	PACK_API_CMD(sa_tx, cmd, k);

	PACK_MEMCPY(sa_tx, mac, k, 6);

	if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
		return iResult;
	if ( (iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR )
		return iResult;	//ACK

	iResult = AVCOM_INT_ReturnACK(cmd,sa_rx, &j );

	return iResult;
}







// A MS Function
/********************************************************************************
*																				*
*		AVCOM_ENG_SetModelName													*
*																				*
********************************************************************************/
int AVCOM_ENG_SetModelName( void *handle, char *name )
{
	unsigned char sa_tx[1024];
	unsigned char sa_rx[1024];
	int j = 0;
	int k = 0;
	int recvlen;
	int iResult;
	unsigned short cmd = API_ENG_SET_MODEL;
	dataConvert	cvt;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	PACK_API_CMD(sa_tx, cmd, k);

	PACK_STRCPY(sa_tx, name, k, MODEL_NAME_SIZE);

	if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
		return iResult;
	if ( (iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR )
		return iResult;	//ACK

	iResult = AVCOM_INT_ReturnACK(cmd,sa_rx, &j );

	return iResult;
}


// A MS Function
/********************************************************************************
*																				*
*		AVCOM_ENG_SetNumBands													*
*																				*
********************************************************************************/
int AVCOM_ENG_SetNumBands( void *handle, int num )
{
	unsigned char sa_tx[1024];
	unsigned char sa_rx[1024];
//	int i = 0;
	int j = 0;
	int k = 0;
	unsigned short cmd = API_SET_NUM_BANDS;
	int recvlen;
	int iResult;
	dataConvert	cvt;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	//Put command at start of data
	PACK_API_CMD(sa_tx,cmd,k);
	PACK2(sa_tx, num, k);

	if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
		return iResult;
	if ( (iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR )
		return iResult;	
	// Check ACK
	if ( (iResult = AVCOM_INT_ReturnACK(cmd, sa_rx, &j)) != AOV_NO_ERROR )
		return iResult;	

	return iResult;
}


// A MS Function
/********************************************************************************
*																				*
*		AVCOM_ENG_SetPLLFreq													*
*																				*
********************************************************************************/
int AVCOM_ENG_SetPLLFreq ( void *handle, double freq ) {
	unsigned char sa_tx[1024];
	unsigned char sa_rx[1024];
	int j = 0;
	int k = 0;
	unsigned short cmd = API_ENG_SET_PLLFREQ;
	int recvlen;
	int iResult;
	dataConvert	cvt;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	//Put command at start of data
	PACK_API_CMD(sa_tx,cmd,k);

	PACKF(sa_tx, (float)(freq*1000), k);
	
	if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
		return iResult;
	if ( (iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR )
		return iResult;	//ACK
	if ( (iResult = AVCOM_INT_ReturnACK(cmd, sa_rx, &j)) != AOV_NO_ERROR )
		return iResult;

	return AOV_NO_ERROR;
}


// A MS Function
/********************************************************************************
*																				*
*		AVCOM_ENG_SetPLLFreqReturnLock											*
*																				*
********************************************************************************/
int AVCOM_ENG_SetPLLFreqReturnLock ( void *handle, double freq, double *lock ) {
	unsigned char sa_tx[1024];
	unsigned char sa_rx[1024];
	int j = 0;
	int k = 0;
	unsigned short cmd = API_ENG_SET_PLLLOCK;
	int recvlen;
	int iResult;
	dataConvert	cvt;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	//Put command at start of data
	PACK_API_CMD(sa_tx,cmd,k);

	PACKF(sa_tx, (float)(freq*1000), k);
	
	if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
		return iResult;
	if ( (iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR )
		return iResult;	//ACK
	if ( (iResult = AVCOM_INT_ReturnACK(cmd, sa_rx, &j)) != AOV_NO_ERROR )
		return iResult;

	if (iResult == AOV_NO_ERROR )
	{
		UNPACKF((*lock), sa_rx, j);
	}

	return AOV_NO_ERROR;
}


// A MS Function
/********************************************************************************
*																				*
*		AVCOM_ENG_SetPLLLDCheck													*
*																				*
********************************************************************************/
int AVCOM_ENG_SetPLLLDCheck ( void *handle, int value ) {
	unsigned char sa_tx[1024];
	unsigned char sa_rx[1024];
	int j = 0;
	int k = 0;
	unsigned short cmd = API_ENG_PLL_EN_LD;
	int recvlen;
	int iResult;
	dataConvert	cvt;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	//Put command at start of data
	PACK_API_CMD(sa_tx,cmd,k);

	PACK2(sa_tx, value, k);
	
	if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
		return iResult;
	if ( (iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR )
		return iResult;	//ACK
	if ( (iResult = AVCOM_INT_ReturnACK(cmd, sa_rx, &j)) != AOV_NO_ERROR )
		return iResult;

	return AOV_NO_ERROR;
}


// A MS Function
/********************************************************************************
*																				*
*		AVCOM_ENG_SetPLLLDP														*
*																				*
********************************************************************************/
int AVCOM_ENG_SetPLLLDP ( void *handle, int value ) {
	unsigned char sa_tx[1024];
	unsigned char sa_rx[1024];
	int j = 0;
	int k = 0;
	unsigned short cmd = API_ENG_PLL_LDP;
	int recvlen;
	int iResult;
	dataConvert	cvt;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	//Put command at start of data
	PACK_API_CMD(sa_tx,cmd,k);

	PACK2(sa_tx, value, k);
	
	if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
		return iResult;
	if ( (iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR )
		return iResult;	//ACK
	if ( (iResult = AVCOM_INT_ReturnACK(cmd, sa_rx, &j)) != AOV_NO_ERROR )
		return iResult;

	return AOV_NO_ERROR;
}


// A MS Function
/********************************************************************************
*																				*
*		AVCOM_ENG_SetPLLSSS														*
*																				*
********************************************************************************/
int AVCOM_ENG_SetPLLSSS ( void *handle, double fStart, double fStop, double fStep ) {
	unsigned char sa_tx[1024];
	unsigned char sa_rx[1024];
//	int i = 0;
	int j = 0;
	int k = 0;
	unsigned short cmd = API_ENG_SET_PLLSSS;
	int recvlen;
	int iResult;
	dataConvert	cvt;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	fStart *= 1000;
	fStop *= 1000;
	fStep *= 1000;

	PACK_API_CMD(sa_tx,cmd,k);
	PACKF(sa_tx,(float)fStart,k);
	PACKF(sa_tx,(float)fStop,k);
	PACKF(sa_tx,(float)fStep,k);
	
	if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
		return iResult;
	if ( (iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR )
		return iResult;	//ACK
	if ( (iResult = AVCOM_INT_ReturnACK(cmd, sa_rx, &j)) != AOV_NO_ERROR )
		return iResult;

	return AOV_NO_ERROR;
}


// A MS Function
/********************************************************************************
*																				*
*		AVCOM_ENG_SetPLLWait													*
*																				*
********************************************************************************/
int AVCOM_ENG_SetPLLWait ( void *handle, unsigned int tWait) 
{
	unsigned char sa_tx[1024];
	unsigned char sa_rx[1024];
//	int i = 0;
	int j = 0;
	int k = 0;
	unsigned short cmd = API_ENG_SET_PLL_WAIT;
	int recvlen;
	int iResult;
	dataConvert	cvt;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	//Put command at start of data
	PACK_API_CMD(sa_tx,cmd,k);

	PACK4(sa_tx, tWait, k);

	if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
		return iResult;
	if ( (iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR )
		return iResult;	//ACK
	if ( (iResult = AVCOM_INT_ReturnACK(cmd, sa_rx, &j)) != AOV_NO_ERROR )
		return iResult;

	return AOV_NO_ERROR;
}


// A MS Function
/********************************************************************************
*																				*
*		AVCOM_ENG_SetProductID													*
*																				*
********************************************************************************/
int AVCOM_ENG_SetProductID( void *handle, short id )
{
	unsigned char sa_tx[1024];
	unsigned char sa_rx[1024];
	int j = 0;
	int k = 0;
	int recvlen;
	int iResult;
	unsigned short cmd = API_ENG_SET_PROD_ID;
	dataConvert	cvt;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	PACK_API_CMD(sa_tx, cmd, k);

	PACK2(sa_tx, id, k);

	if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
		return iResult;
	if ( (iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR )
		return iResult;	//ACK

	iResult = AVCOM_INT_ReturnACK(cmd,sa_rx, &j );

	return iResult;
}









// A MS Function
/********************************************************************************
*																				*
*		AVCOM_ENG_SetRBWCal														*
*																				*
********************************************************************************/
int AVCOM_ENG_SetRBWCal (void *handle, int rbw, struct aov_rbwcaldata *caldata )
{
	unsigned char sa_tx[1024];
	unsigned char sa_rx[1024];
	int j = 0;
	int k = 0;
	int recvlen;
	int iResult;
	unsigned short cmd = API_ENG_CAL_RBW;
	dataConvert	cvt;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	PACK_API_CMD(sa_tx, cmd, k);

	PACK4(sa_tx, rbw, k);

	PACKF(sa_tx, caldata->m, k);
	PACKF(sa_tx, caldata->b, k);
	PACK2(sa_tx, caldata->logoff, k);
	PACK2(sa_tx, caldata->ifagc, k);
	PACK2(sa_tx, caldata->wait, k);

	if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
		return iResult;
	if ( (iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR )
		return iResult;	//ACK

	iResult = AVCOM_INT_ReturnACK(cmd, sa_rx, &j);

	return iResult;
}






// A MS Function
/********************************************************************************
*																				*
*		AVCOM_ENG_SetRBWList													*
*																				*
********************************************************************************/
int AVCOM_ENG_SetRBWList (void *handle, int *RBWs, int list_size )
{
	return AVCOM_INT_SetLists ( handle, API_SET_RBW_LIST, RBWs, list_size );
}

int AVCOM_SA_GetSpectrumDataArraysLV(void *handle, double* start, double* stop, double* step, double* freq,  double* data)
{
	return AOV_ERR_UNKNOWN_CMD;
}






// A MS Function
/********************************************************************************
*																				*
*		AVCOM_ENG_SetRBWmux														*
*																				*
********************************************************************************/
int AVCOM_ENG_SetRBWmux ( void *handle, int value )
{
	unsigned char sa_tx[1024];
	unsigned char sa_rx[1024];
//	int i = 0;
	int j = 0;
	int k = 0;
	unsigned short cmd = API_ENG_SET_RBW_MUX;
	int recvlen;
	int iResult;
	dataConvert	cvt;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	//Put command at start of data
	PACK_API_CMD(sa_tx,cmd,k);

	PACK2(sa_tx, value, k);

	if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
		return iResult;
	if ( (iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR )
		return iResult;	//ACK

	iResult = AVCOM_INT_ReturnACK(cmd, sa_rx, &j);

	return iResult;
}


// A MS Function
/********************************************************************************
*																				*
*		AVCOM_ENG_SetSerialNumber												*
*																				*
********************************************************************************/
int AVCOM_ENG_SetSerialNumber( void *handle, char *serial )
{
	unsigned char sa_tx[1024];
	unsigned char sa_rx[1024];
	int j = 0;
	int k = 0;
	int recvlen;
	int iResult;
	unsigned short cmd = API_ENG_SET_SERIAL;
	dataConvert	cvt;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	PACK_API_CMD(sa_tx, cmd, k);

	PACK_STRCPY(sa_tx, serial, k, SERIAL_SIZE);

	if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
		return iResult;
	if ( (iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR )
		return iResult;	//ACK

	iResult = AVCOM_INT_ReturnACK(cmd,sa_rx, &j );

	return iResult;
}







// A MS Function
/********************************************************************************
*																				*
*		AVCOM_ENG_SetUtuneLO													*
*																				*
********************************************************************************/
int AVCOM_ENG_SetUtuneLO ( void *handle, int value )
{
	unsigned char sa_tx[1024];
	unsigned char sa_rx[1024];
//	int i = 0;
	int j = 0;
	int k = 0;
	unsigned short cmd = API_ENG_SET_UTUNE_LO;
	int recvlen;
	int iResult;
	dataConvert	cvt;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	//Put command at start of data
	PACK_API_CMD(sa_tx,cmd,k);

	PACK2(sa_tx, value, k);

	if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
		return iResult;
	if ( (iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR )
		return iResult;	//ACK

	iResult = AVCOM_INT_ReturnACK(cmd, sa_rx, &j);

	return iResult;
}



// A MS Function
/********************************************************************************
*																				*
*		AVCOM_ENG_SetVBWList													*
*																				*
********************************************************************************/
int AVCOM_ENG_SetVBWList (void *handle, int *VBWs, int list_size )
{
	return AVCOM_INT_SetLists ( handle, API_SET_VBW_LIST, VBWs, list_size );
}







// A MS Function
/********************************************************************************
*																				*
*		AVCOM_ENG_SetVBWmux														*
*																				*
********************************************************************************/
int AVCOM_ENG_SetVBWmux ( void *handle, int value )
{
	unsigned char sa_tx[1024];
	unsigned char sa_rx[1024];
//	int i = 0;
	int j = 0;
	int k = 0;
	unsigned short cmd = API_ENG_SET_VBW;
	int recvlen;
	int iResult;
	dataConvert	cvt;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	//Put command at start of data
	PACK_API_CMD(sa_tx,cmd,k);

	PACK2(sa_tx, value, k);

	if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
		return iResult;
	if ( (iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR )
		return iResult;	//ACK

	iResult = AVCOM_INT_ReturnACK(cmd, sa_rx, &j);

	return iResult;
}







// A MS Function
/********************************************************************************
*																				*
*		AVCOM_ENG_TriggerPLLSweep												*
*																				*
********************************************************************************/
int AVCOM_ENG_TriggerPLLSweep( void *handle)
{
	unsigned char sa_tx[1024];
	unsigned char sa_rx[1024];
//	int i = 0;
	int j = 0;
	int k = 0;
	unsigned short cmd = API_ENG_TRG_PLL_SWEEP;
	int recvlen;
	int iResult;
	dataConvert	cvt;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	//Put command at start of data
	PACK_API_CMD(sa_tx,cmd,k);

	if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
		return iResult;
	if ( (iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR )
		return iResult;	//ACK
	if ( (iResult = AVCOM_INT_ReturnACK(cmd, sa_rx, &j)) != AOV_NO_ERROR )
		return iResult;

	return AOV_NO_ERROR;
}


// A MS Function
/********************************************************************************
*																				*
*		AVCOM_ENG_TriggerDSSSweep												*
*																				*
********************************************************************************/
int AVCOM_ENG_TriggerDSSSweep( void *handle)
{
	unsigned char sa_tx[1024];
	unsigned char sa_rx[1024];
//	int i = 0;
	int j = 0;
	int k = 0;
	unsigned short cmd = API_ENG_TRG_DDS_SWEEP;
	int recvlen;
	int iResult;
	dataConvert	cvt;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	//Put command at start of data
	PACK_API_CMD(sa_tx,cmd,k);

	if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
		return iResult;
	if ( (iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR )
		return iResult;	//ACK
	if ( (iResult = AVCOM_INT_ReturnACK(cmd, sa_rx, &j)) != AOV_NO_ERROR )
		return iResult;

	return AOV_NO_ERROR;
}


/*
int AVCOM_SA_GetHwDesc(void *handle, void *vphw)
{
	unsigned char sa_tx[64];
	unsigned char sa_rx[1024];
	int recvlen;
	int j,k;
	int iResult;
	dataConvert	cvt;
	sAOV_HW_DESC *hw;
	unsigned short cmd=API_ENG_GET_HW_DESC;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	hw=(sAOV_HW_DESC *)vphw;

	k = 0;

	cvt.us=cmd;
	sa_tx[k++]		 = cvt.ch[1];
	sa_tx[k++]		 = cvt.ch[0];

	if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
		return iResult;
	if ( (iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR )
		return iResult;	
	//ACK
	if ( (iResult = AVCOM_INT_ReturnACK(cmd, sa_rx, j)) != AOV_NO_ERROR )
		return iResult;

	memset(hw,0,sizeof(sAOV_HW_DESC));
	j=8;

	cvt.ch[1] = sa_rx[j++];
	cvt.ch[0] = sa_rx[j++];
	hw->ProdType = cvt.us;

	cvt.ch[1] = sa_rx[j++];
	cvt.ch[0] = sa_rx[j++];
	hw->ProdFamily = cvt.us;

	cvt.ch[1] = sa_rx[j++];
	cvt.ch[0] = sa_rx[j++];
	hw->Model = cvt.us;

	strncpy(hw->ModelName,(char *)&sa_rx[j],MODEL_NAME_SIZE);
	j+=MODEL_NAME_SIZE;

	cvt.ch[1] = sa_rx[j++];
	cvt.ch[0] = sa_rx[j++];
	hw->PcbFab = cvt.us;

	hw->PcbRev = sa_rx[j++];

	hw->ProjId = sa_rx[j++];

	strncpy(hw->Serial,(char *)&sa_rx[j],SERIAL_NUMBER_SIZE);
	j+=SERIAL_NUMBER_SIZE;

	hw->fwMajor = sa_rx[j++];

	hw->fwMinor = sa_rx[j++];

	cvt.ch[1] = sa_rx[j++];
	cvt.ch[0] = sa_rx[j++];
	hw->fwBuild = cvt.us;

	hw->numBands = sa_rx[j++];

	hw->numInputs = sa_rx[j++];

	hw->numRBW	= sa_rx[j++];

	hw->numVBW	= sa_rx[j++];

	cvt.ch[1]	= sa_rx[j++];
	cvt.ch[0]	= sa_rx[j++];
	hw->maxPoints = cvt.us;

	cvt.ch[3]	= sa_rx[j++];
	cvt.ch[2]	= sa_rx[j++];
	cvt.ch[1]	= sa_rx[j++];
	cvt.ch[0]	= sa_rx[j++];
	hw->Options	= cvt.ul;

	return AOV_NO_ERROR;
}
*/

/*
int AVCOM_SA_SetRefLevel ( void *handle, int reflvl)
{
		return AOV_ERR_UNKNOWN_CMD;
}
*/

/*
int AVCOM_SA_DO_BIT ( void *handle, sAOV_BIT_REQUEST bitRequest, sAOV_BIT_RESULT *bitResult )
{
	return AOV_NO_ERROR;
}
*/

























/*
int AVCOM_ENG_SetHwDesc(void *handle, void *vphw) {
	unsigned char sa_tx[256];
	unsigned char sa_rx[1024];
	int recvlen;
	int k;
	int iResult;
	dataConvert	cvt;
	sAOV_HW_DESC *hw;
	unsigned short cmd=API_ENG_SET_HW_DESC;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	k = 0;
	cvt.us=cmd;
	sa_tx[k++]		= cvt.ch[1];
	sa_tx[k++]		= cvt.ch[0];

	memset(&sa_tx[k],0,sizeof(sAOV_HW_DESC));

	hw=(sAOV_HW_DESC *)vphw;


	cvt.us=hw->ProdType;
	sa_tx[k++]		= cvt.ch[1];
	sa_tx[k++]		= cvt.ch[0];

	cvt.us=hw->ProdFamily;
	sa_tx[k++]		= cvt.ch[1];
	sa_tx[k++]		= cvt.ch[0];

	cvt.us=hw->Model;
	sa_tx[k++]		= cvt.ch[1];
	sa_tx[k++]		= cvt.ch[0];

	strncpy((char *)&sa_tx[k],hw->ModelName,MODEL_NAME_SIZE);
	k+=MODEL_NAME_SIZE;

	cvt.us=hw->PcbFab;
	sa_tx[k++]		= cvt.ch[1];
	sa_tx[k++]		= cvt.ch[0];

	sa_tx[k++]		= hw->PcbRev;

	sa_tx[k++]		= hw->ProjId;

	strncpy((char *)&sa_tx[k],hw->Serial,SERIAL_NUMBER_SIZE);
	k+=SERIAL_NUMBER_SIZE;

	sa_tx[k++]		= hw->fwMajor;

	sa_tx[k++]		= hw->fwMinor;

	cvt.us=hw->fwBuild;
	sa_tx[k++]		= cvt.ch[1];
	sa_tx[k++]		= cvt.ch[0];

	sa_tx[k++]		= hw->numBands;

	sa_tx[k++]		= hw->numInputs;

	sa_tx[k++]		= hw->numRBW;

	sa_tx[k++]		= hw->numVBW;

	cvt.us=hw->maxPoints;
	sa_tx[k++]		= cvt.ch[1];
	sa_tx[k++]		= cvt.ch[0];

	cvt.ul=hw->Options;
	sa_tx[k++]		= cvt.ch[3];
	sa_tx[k++]		= cvt.ch[2];
	sa_tx[k++]		= cvt.ch[1];
	sa_tx[k++]		= cvt.ch[0];


	if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
		return iResult;
	if ( (iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR )
		return iResult;	
	//ACK
	if ( (iResult = AVCOM_INT_ReturnACK(cmd, sa_rx, j)) != AOV_NO_ERROR )
		return iResult;

	return AOV_NO_ERROR;
}
*/

#if 0

/**************************************************************
*
*
* Transitional Functions (to be deleted at some point)
*
*
**************************************************************/


int AVCOM_SA_TEST(void)
{
	FT_STATUS ftStatus;
	DWORD numDevs;
	DWORD BytesWritten,RxBytes,BytesReceived;
	char **BufPtrs;
	char **Buffers;
	long *locIdBuf; 
	FT_HANDLE ftHandle1;
	int i;
	unsigned char tx[1024];
	unsigned char rx[1024];

	FT_HANDLE ftHandleTemp; 
	DWORD Flags; 
	DWORD ID; 
	DWORD Type; 
	DWORD LocId; 
	char SerialNumber[16]; 
	char Description[64];



	fprintf(stdout,"FT_CreateDeviceInfoList\n");
	// create the device information list 
	ftStatus = FT_CreateDeviceInfoList(&numDevs); 
	if (ftStatus == FT_OK) 
	{ 
		fprintf(stdout,"Number of devices is %d\n",numDevs); 
	} else { 
		fprintf(stdout,"FT_CreateDeviceInfoList Failed\n"); 
	}
	for (i=0; i < (int)numDevs; i++) 
	{ 
		// get information for device 0 
		ftStatus = FT_GetDeviceInfoDetail((DWORD)i, &Flags, &Type, &ID, &LocId, SerialNumber, Description, &ftHandleTemp); 
		if (ftStatus == FT_OK) 
		{ 
			fprintf(stdout,"Dev %d:\n",i); 
			fprintf(stdout," Flags=0x%x\n",Flags); 
			fprintf(stdout," Type=0x%x\n",Type);
			fprintf(stdout," ID=0x%x\n",ID); 
			fprintf(stdout," LocId=0x%x\n",LocId); 
			fprintf(stdout," SerialNumber=%s\n",SerialNumber); 
			fprintf(stdout," Description=%s\n",Description); 
			fprintf(stdout," ftHandle=0x%x\n",(unsigned int)(long)ftHandleTemp); 
		} 
	}

	fprintf(stdout,"\n");
	fprintf(stdout,"\n");



	fprintf(stdout,"Creating dynamic list of devices ('malloc'ing)\n");
	BufPtrs = (char **)malloc((numDevs+1)*sizeof(char  *)); // plus one since last should be NULL'ed
	if (NULL == BufPtrs) 
	{
		free (BufPtrs);
		return -1;
	}
	Buffers = (char **)malloc((numDevs+1)*sizeof(char *)); // plus one since last should be NULL'ed
	if (NULL == Buffers) 
	{
		free (Buffers);
		return -1;
	}
	for ( i=0; i < (int)numDevs; i++ )
	{
		Buffers[i] = (char *)malloc(64*sizeof(char));
		BufPtrs[i] = Buffers[i];
		if (NULL == Buffers[i])
		{
			free (BufPtrs);
			return -1;
		}
	}
	BufPtrs[i] = NULL;

	fprintf(stdout,"FT_LIST_ALL|FT_OPEN_BY_DESCRIPTION\n");
	ftStatus = FT_ListDevices(BufPtrs,&numDevs,FT_LIST_ALL|FT_OPEN_BY_DESCRIPTION); 
	if (ftStatus == FT_OK) 
	{ 
		// FT_ListDevices OK, product descriptions are in Buffer1 and Buffer2, and 
		// numDevs contains the number of devices connected 
	} else { 
		// FT_ListDevices failed 
	}

	fprintf(stdout,"FT_LIST_ALL|FT_OPEN_BY_LOCATION\n");
	locIdBuf = (long *)malloc(numDevs*sizeof(long));
	ftStatus = FT_ListDevices(locIdBuf,&numDevs,FT_LIST_ALL|FT_OPEN_BY_LOCATION); 
	if (ftStatus == FT_OK) 
	{ 
		// FT_ListDevices OK, location IDs are in locIdBuf, and 
		// numDevs contains the number of devices connected 
	} else { 
		// FT_ListDevices failed 
	}

	fprintf(stdout,"FT_OpenEx\n");
	ftStatus = FT_OpenEx((void *)locIdBuf[0],FT_OPEN_BY_LOCATION,&ftHandle1);
	if (ftStatus == FT_OK) 
	{ 
		// success
	} else { 
		// failure
	}

	fprintf(stdout,"FT_ResetDevice\n");
	ftStatus = FT_ResetDevice(ftHandle1);
	if (ftStatus == FT_OK) 
	{ 
		// success
	} else { 
		// failure
	}

	fprintf(stdout,"FT_SetBaudRate\n");
	ftStatus = FT_SetBaudRate(ftHandle1,115200);
	if (ftStatus == FT_OK) 
	{ 
		// success
	} else { 
		// failure
	}

	fprintf(stdout,"FT_SetDataCharacteristics\n");
	ftStatus = FT_SetDataCharacteristics(ftHandle1,FT_BITS_8, FT_STOP_BITS_1, FT_PARITY_NONE);
	if (ftStatus == FT_OK) 
	{ 
		// success
	} else { 
		// failure
	}

	fprintf(stdout,"FT_SetFlowControl\n");
	ftStatus = FT_SetFlowControl(ftHandle1,FT_FLOW_NONE,0,0);
	if (ftStatus == FT_OK) 
	{ 
		// success
	} else { 
		// failure
	}

	fprintf(stdout,"FT_Purge\n");
	ftStatus = FT_Purge(ftHandle1,FT_PURGE_RX | FT_PURGE_TX);
	if (ftStatus == FT_OK) 
	{ 
		// success
	} else {	
		// failure
	}

	tx[0] = 0x02;
	tx[1] = 0x00;
	tx[2] = 0x03;
	tx[3] = 0x03;
	tx[4] = 0x03;
	tx[5] = 0x03;

	fprintf(stdout,"FT_Write\n");
	ftStatus = FT_Write(ftHandle1,tx,6,&BytesWritten);
	if (ftStatus == FT_OK) 
	{ 
		// success
	} else { 
		// failure
	}

	RxBytes = 0;
//			while (RxBytes != 344 && i < 0x1FFFFFF)
//			{
//				FT_GetQueueStatus(ftHandle1,&RxBytes); 
//				i++;
//			}
	if (RxBytes > 0) 
	{ 
		fprintf(stdout,"FT_Read\n");
		ftStatus = FT_Read(ftHandle1,rx,RxBytes,&BytesReceived);
		if (ftStatus == FT_OK) 
		{ 
			// FT_Read OK 
		} else { 
			// FT_Read Failed 
		} 
	} 

	// create the device information list 
	ftStatus = FT_CreateDeviceInfoList(&numDevs); 
	if (ftStatus == FT_OK) 
	{ fprintf(stdout,"Number of devices is %d\n",numDevs); 
	} else { 
		// FT_CreateDeviceInfoList failed 
	}

	fprintf(stdout,"FT_Close\n");
	ftStatus = FT_Close(ftHandle1);
	if (ftStatus == FT_OK) 
	{ 
		// success
	} else { 
		// failure
	}

	fprintf(stdout,"Recieved %i Bytes in time %i \n",RxBytes,i);
	for (i=0; i < (int)RxBytes; i++)
		fprintf(stdout,"0x%02X ",rx[i]);
	
	fprintf(stdout,"\n");
	fprintf(stdout,"\n");
	fprintf(stdout,"\n");

	return 0;
}
#endif /* Old Fuctions */

#endif

#endif /* AOV_DEPEND_ENG */

