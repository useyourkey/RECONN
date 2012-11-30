// AOV_api.cpp : Defines the exported functions for the DLL application.
//

//#define _CRT_SECURE_NO_WARNINGS // Disables the warnings about scanf() and fopen() being unsecure.
//#include <stdio.h>
//#include <string.h>  //for memcpy and memset
#include "aov_opts.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "aov_errno.h"
#include "aov_common.h"
#include "reconn.h"
#include "debugMenu.h"
#include "spectrum.h"
#if (defined(AOV_DEPEND_SA) || defined(AOV_DEPEND_CUS_PACKING))
#include "aov_async.h"
#endif
#include "aov_core.h"
#include "stdafx.h"
#ifdef AOV_DEPEND_USB
#include "ftd2xx.h"
#endif /*AOV_DEPEND_USB */
#include "crc.h"
#include "aov_protocol.h"
#include "spectrum.h"

/* Required for building Windows DLL's */
#ifdef _WIN32
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;

}
#endif

/* Function redefintion macros to sastisfy VC++/Windows _CRT_SECURE_NO_WARNINGS
 * Warnings without disabling them */
#ifdef _WIN32
#define snprintf sprintf_s
#define strncpy(dst,src,size) strncpy_s(dst,size,src,size)
#endif 



#ifdef AOV_DEPEND_SA
int AVCOM_SA_Unregister(void *handle);

AOVhandle **plist=NULL;
int			plen=0;
#endif

#define AOV_DEBUG

#ifdef AOV_DEBUG
//int errno;
#endif

/********************************************************************************
*																				*
*		Static Function Declarations											*
*																				*
********************************************************************************/


#ifdef AOV_DEBUG
static void AVCOM_INT_DumpHex(unsigned char *data, int len);
#endif /* AOV_DEBUG */








#ifdef AOV_DEPEND_ENCODE
/********************************************************************************
*																				*
*		AVCOM_API_DecodeGetActiveBand											*
*																				*
********************************************************************************/
int AVCOM_API_DecodeGetActiveBand ( void *data, int dataLen, int *band )
{
	int j = 0;
	dataConvert	cvt;
	unsigned char *sa_rx = (unsigned char *)data;
        UNUSED_PARAM(dataLen);

	UNPACK2(*band, sa_rx, j);
	
	return AOV_NO_ERROR;

}


/********************************************************************************
*																				*
*		AVCOM_API_DecodeGetActiveRBW											*
*																				*
********************************************************************************/
int AVCOM_API_DecodeGetActiveRBW ( void *data, int dataLen, int *rbw )
{
	int j = 0;
	dataConvert	cvt;
	unsigned char *sa_rx = (unsigned char *)data;

        UNUSED_PARAM(dataLen);
	UNPACK4(*rbw,sa_rx,j);
	
	return AOV_NO_ERROR;

}


/********************************************************************************
*																				*
*		AVCOM_API_DecodeGetActiveRefLevel										*
*																				*
********************************************************************************/
int AVCOM_API_DecodeGetActiveRefLevel ( void *data, int dataLen, int *reflvl )
{
	int j = 0;
	dataConvert	cvt;
	unsigned char *sa_rx = (unsigned char *)data;

        UNUSED_PARAM(dataLen);
	UNPACK2S((*reflvl), sa_rx, j);
	
	return AOV_NO_ERROR;

}


/********************************************************************************
*																				*
*		AVCOM_API_DecodeGetActiveVBW											*
*																				*
********************************************************************************/
int AVCOM_API_DecodeGetActiveVBW ( void *data, int dataLen, int *vbw )
{
	int j = 0;
	dataConvert	cvt;
	unsigned char *sa_rx = (unsigned char *)data;

        UNUSED_PARAM(dataLen);
	UNPACK4(*vbw,sa_rx,j);
	
	return AOV_NO_ERROR;

}


/********************************************************************************
*																				*
*		AVCOM_API_DecodeGetBandInfo												*
*																				*
********************************************************************************/
int AVCOM_API_DecodeGetBandInfo ( void *data, int dataLen, struct aov_band_info *info )
{
	int j = 0;
	dataConvert	cvt;
	unsigned char *sa_rx = (unsigned char *)data;

        UNUSED_PARAM(dataLen);
	UNPACK2(info->phy_input,sa_rx,j);
	UNPACK_STRCPY(info->name,sa_rx,j,AOV_BAND_NAME_LEN);
	UNPACK4(info->min_freq,sa_rx,j);
	UNPACK4(info->max_freq,sa_rx,j);
//	UNPACK4(info->min_step,sa_rx,j);
//	UNPACK2(info->min_db,sa_rx,j);
	UNPACK2(info->max_db,sa_rx,j);
	UNPACK2(info->min_reflvl,sa_rx,j);
	UNPACK2(info->max_reflvl,sa_rx,j);
	
	return AOV_NO_ERROR;

}


/********************************************************************************
*																				*
*		AVCOM_API_DecodeGetBoardTemp											*
*																				*
********************************************************************************/
int AVCOM_API_DecodeGetBoardTemp ( void *data, int dataLen, double *value )
{
	int j = 0;
	dataConvert	cvt;
	unsigned char *sa_rx = (unsigned char *)data;

        UNUSED_PARAM(dataLen);
	UNPACKF(*value, sa_rx, j);
	
	return AOV_NO_ERROR;

}


/********************************************************************************
*																				*
*		AVCOM_API_DecodeGetBuildDateTime										*
*																				*
********************************************************************************/
int AVCOM_API_DecodeGetBuildDateTime ( void *data, int dataLen, char *date, char *time )
{
	int j = 0;
//	dataConvert	cvt;
	unsigned char *sa_rx = (unsigned char *)data;

	
        UNUSED_PARAM(dataLen);
	UNPACK_STRCPY(date, sa_rx, j, MAX_DATE_SIZE);
	UNPACK_STRCPY(time, sa_rx, j, MAX_TIME_SIZE); 
	
	return AOV_NO_ERROR;

}


/********************************************************************************
*																				*
*		AVCOM_API_DecodeGetFirmwareVersion										*
*																				*
********************************************************************************/
int AVCOM_API_DecodeGetFirmwareVersion ( void *data, int dataLen, struct aov_version *ver )
{
	int j = 0;
	dataConvert	cvt;
	unsigned char *sa_rx = (unsigned char *)data;

        UNUSED_PARAM(dataLen);
	UNPACK2(ver->major, sa_rx, j);
	UNPACK2(ver->minor, sa_rx, j);
	UNPACK2(ver->build, sa_rx, j);
	UNPACK1(ver->stage, sa_rx, j);

	switch (ver->stage)
	{
		case 0: // alpha
			snprintf(ver->string, 16, "%i.%ia%i", ver->major, ver->minor, ver->build);
			break;
		case 1: // beta
			snprintf(ver->string, 16, "%i.%ib%i", ver->major, ver->minor, ver->build);
			break;
		case 2: // Rlease Canidate
			snprintf(ver->string, 16, "%i.%irc%i", ver->major, ver->minor, ver->build);
			break;
		case 3: // Full Release
			snprintf(ver->string, 16, "%i.%i", ver->major, ver->minor);
			break;
		default:
			snprintf(ver->string, 16, "NULL");
			break;
	}
	
	return AOV_NO_ERROR;

}


/********************************************************************************
*																				*
*		AVCOM_API_DecodeGetFreqValue											*
*																				*
********************************************************************************/
int AVCOM_API_DecodeGetFreqValue ( void *data, int dataLen, double *db )
{
	int j = 0;
	dataConvert	cvt;
	unsigned char *sa_rx = (unsigned char *)data;

	unsigned short value;

        UNUSED_PARAM(dataLen);
	UNPACK2(value,sa_rx, j);
	*db = (0.01 * value) - 200;
	
	return AOV_NO_ERROR;

}


/********************************************************************************
*																				*
*		AVCOM_API_DecodeGetNetwork												*
*																				*
********************************************************************************/
int AVCOM_API_DecodeGetNetwork ( void *data, int dataLen, struct aov_network *net )
{
	int j = 0;
	dataConvert	cvt;
	unsigned char *sa_rx = (unsigned char *)data;
	
        UNUSED_PARAM(dataLen);
	UNPACK_MEMCPY(net->mac, sa_rx, j, 6);	
	UNPACK_STRCPY(net->hostname, sa_rx, j, (HOSTNAME_SIZE-1));	
	UNPACK1(net->dynip, sa_rx, j);	
	UNPACK_MEMCPY(net->ip, sa_rx, j, sizeof(unsigned long));
	UNPACK_MEMCPY(net->sub, sa_rx, j, sizeof(unsigned long));
	UNPACK_MEMCPY(net->gw, sa_rx, j, sizeof(unsigned long));
	
	UNPACK2(net->tcp_port, sa_rx, j);
	
	return AOV_NO_ERROR;

}


/********************************************************************************
*																				*
*		AVCOM_API_DecodeGetProdDesc												*
*																				*
********************************************************************************/
int AVCOM_API_DecodeGetProdDesc ( void *data, int dataLen, struct aov_product *ppd )
{
	int j = 0;
	dataConvert	cvt;
	unsigned char *sa_rx = (unsigned char *)data;

        UNUSED_PARAM(dataLen);
	UNPACK2(ppd->prod_id,sa_rx,j);
	UNPACK2(ppd->numBands,sa_rx,j);
	UNPACK2(ppd->numRBW,sa_rx,j);
	UNPACK2(ppd->numVBW,sa_rx,j);
	UNPACK2(ppd->max_pts,sa_rx,j);
	
	UNPACK_STRCPY(ppd->serial, sa_rx, j, SERIAL_SIZE);
	UNPACK_STRCPY(ppd->model_name, sa_rx, j, MODEL_NAME_SIZE);
	UNPACK_STRCPY(ppd->analyzer_name, sa_rx, j, ANALYZER_NAME_SIZE);
	
	return AOV_NO_ERROR;

}


/********************************************************************************
*																				*
*		AVCOM_API_DecodeGetRBWList												*
*																				*
********************************************************************************/
int AVCOM_API_DecodeGetRBWList ( void *data, int dataLen, int **RBWs, int *list_size )
{
	int j = 0;
	int i = 0;
	dataConvert	cvt;
	unsigned char *sa_rx = (unsigned char *)data;

        UNUSED_PARAM(dataLen);
	UNPACK2(*list_size, sa_rx, j);
	*RBWs = (int *)malloc( (*list_size) * sizeof(int) );
	for ( i = 0; i < (*list_size); i++ )
	{
		UNPACK4(RBWs[0][i],sa_rx,j);
	}
	
	return AOV_NO_ERROR;

}


/********************************************************************************
*																				*
*		AVCOM_API_DecodeGetTriggerSweep											*
*																				*
********************************************************************************/
int AVCOM_API_DecodeGetTriggerSweep ( void *data, int dataLen, sAOV_SweepData *trace )
{
	int j = 0;
	int i;
	double dFreqCurr;
	double center,span;
	unsigned short tData;
	dataConvert	cvt;
	unsigned char *sa_rx = (unsigned char *)data;

	trace->count=0;
	trace->fStart=0;
	trace->fStop=0;
	trace->fStep=0;

        UNUSED_PARAM(dataLen);
	UNPACK2(trace->band,sa_rx,j);
	UNPACK4(trace->rbw,sa_rx,j);
	UNPACK4(trace->vbw,sa_rx,j);
	UNPACK2S(trace->reflvl,sa_rx,j);
	
	UNPACK2(trace->avg,sa_rx,j);

	UNPACK2(trace->count,sa_rx,j);

	// Not currently dynamically malloc'd
	//trace->sweep=(sAVCOM_ENG_SweepData_Array *)malloc(trace->count*sizeof(sAVCOM_ENG_SweepData_Row));
	UNPACKF(trace->fStart,sa_rx,j);
	UNPACKF(trace->fStop,sa_rx,j);
	UNPACKF(trace->fStep,sa_rx,j);
	UNPACKF(center,sa_rx,j);
	UNPACKF(span,sa_rx,j);
	
	trace->fStart /= 1000;
	trace->fStop /= 1000;
	trace->fStep /= 1000;
	
	center /= 1000;	
	span /= 1000;

	dFreqCurr = trace->fStart;
	if (trace->fStop < trace->fStart)
		dFreqCurr = trace->fStop;

	for (i=0;i<trace->count;i++) 
	{
		UNPACK2(tData,sa_rx,j);
		trace->sweep[i].power = (0.01 * tData) - 200;
		if( (center != 0) && (span != 0) )
			trace->sweep[i].frequency = center - (span/2) + (i*span/(trace->count-1));
		else
			trace->sweep[i].frequency=dFreqCurr;
		dFreqCurr+=trace->fStep;
	}
	if (trace->count == 1)
		trace->sweep[0].frequency = trace->fStart;

	return AOV_NO_ERROR;

}


/********************************************************************************
*																				*
*		AVCOM_API_DecodeGetTriggerZeroSpan										*
*																				*
********************************************************************************/
int AVCOM_API_DecodeGetTriggerZeroSpan ( void *data, int dataLen, sAOV_ZeroSpanData *trace )
{
	int j = 0;
	int i;
	unsigned short tData;
	dataConvert	cvt;
	unsigned char *sa_rx = (unsigned char *)data;

	trace->count=0;
	trace->freq=0;

        UNUSED_PARAM(dataLen);
	UNPACK2(trace->band,sa_rx,j);
	UNPACK4(trace->rbw,sa_rx,j);
	UNPACK4(trace->vbw,sa_rx,j);
	UNPACK2S(trace->reflvl,sa_rx,j);

	// Not currently dynamically malloc'd
	//trace->sweep=(sAVCOM_ENG_SweepData_Array *)malloc(trace->count*sizeof(sAVCOM_ENG_SweepData_Row));
	UNPACKF(trace->freq,sa_rx,j);
	UNPACK2(trace->count,sa_rx,j);
	UNPACK2(trace->mode,sa_rx,j);
	UNPACK4(trace->sample_rate,sa_rx,j);
	
	trace->freq /= 1000;

	for (i=0;i<trace->count;i++) 
	{
		UNPACK2(tData,sa_rx,j);
		trace->trace[i].power = (0.01 * tData) - 200;
		trace->trace[i].time=(i*trace->sample_rate)/1000000.0F;
	}
	return AOV_NO_ERROR;

}


/********************************************************************************
*																				*
*		AVCOM_API_DecodeGetVBWList												*
*																				*
********************************************************************************/
int AVCOM_API_DecodeGetVBWList ( void *data, int dataLen, int **VBWs, int *list_size )
{
	int j = 0;
	int i = 0;
	dataConvert	cvt;
	unsigned char *sa_rx = (unsigned char *)data;

        UNUSED_PARAM(dataLen);
	UNPACK2(*list_size, sa_rx, j);
	*VBWs = (int *)malloc( (*list_size) * sizeof(int) );
	for ( i = 0; i < (*list_size); i++ )
	{
		UNPACK4(VBWs[0][i],sa_rx,j);
	}
	
	return AOV_NO_ERROR;

}
#endif /* AOV_DEPEND_ENCODE */


#if 0
// A MS Function
/********************************************************************************
*																				*
*		AVCOM_API_Deinitialize													*
*																				*
********************************************************************************/
int AVCOM_API_Deinitialize(void)
{
#ifdef AOV_DEPEND_SA
	if (plist == NULL)
		return AOV_ERR_API_NOT_INITIALIZED;
	while (plen)
	{
		AVCOM_SA_Unregister(plist[plen-1]);
	}
	free(plist);
	plist = NULL;
	plen = 0;

#ifdef _WIN32
	if (WSACleanup())
		return AOV_ERR_WSACLEANUP_FAILED;
#endif /* _WIN32 */
#endif /* AOV_DEPEND_SA */

	return AOV_NO_ERROR;

}
#endif


#ifdef AOV_DEPEND_ENCODE
/********************************************************************************
*																				*
*		AVCOM_API_EncodeSetHostname											*
*																				*
********************************************************************************/
void *AVCOM_API_EncodeSetHostname ( int *dataLen, char *hostname )
{
	unsigned char sa_tx[64];
	int k = 0;
	unsigned char *data;
	dataConvert	cvt;
	unsigned short cmd = API_CHANGE_HOSTNAME;

	PACK_API_CMD(sa_tx,cmd,k);
	PACK_STRCPY(sa_tx, hostname, k, HOSTNAME_SIZE);

	data = (unsigned char *)malloc( sizeof(unsigned char) * k );
	memcpy( data, sa_tx, k );
	*dataLen = k;

	return (void *)data;
}


/********************************************************************************
*																				*
*		AVCOM_API_EncodeSetNetwork											*
*																				*
********************************************************************************/
void *AVCOM_API_EncodeSetNetwork ( int *dataLen, int dynip, unsigned long ip, unsigned long sub, unsigned long gw )
{
	unsigned char sa_tx[64];
	int k = 0;
	unsigned char *data;
	unsigned cmd = API_CHANGE_NETWORK;
	dataConvert	cvt;

  if (dynip)
  {
	  ip = 0;
	  sub = 0;
	  gw = 0;
  }
	  
  PACK_API_CMD(sa_tx, cmd, k);
  PACK1(sa_tx, dynip, k);
  PACK_MEMCPY(sa_tx, &ip, k, sizeof(unsigned long));
  PACK_MEMCPY(sa_tx, &sub, k, sizeof(unsigned long));
  PACK_MEMCPY(sa_tx, &gw, k, sizeof(unsigned long));

	data = (unsigned char *)malloc( sizeof(unsigned char) * k );
	memcpy( data, sa_tx, k );
	*dataLen = k;

	return (void *)data;
}


/********************************************************************************
*																				*
*		AVCOM_API_EncodeSetPort												*
*																				*
********************************************************************************/
void *AVCOM_API_EncodeSetPort ( int *dataLen, unsigned short port )
{
	unsigned char sa_tx[64];
	int k = 0;
	unsigned char *data;
	dataConvert	cvt;
	unsigned short cmd = API_CHANGE_PORT;

	PACK_API_CMD(sa_tx,cmd,k);
	PACK2(sa_tx, port, k);

	data = (unsigned char *)malloc( sizeof(unsigned char) * k );
	memcpy( data, sa_tx, k );
	*dataLen = k;

	return (void *)data;
}


/********************************************************************************
*																				*
*		AVCOM_API_EncodeGetActiveBand											*
*																				*
********************************************************************************/
void *AVCOM_API_EncodeGetActiveBand ( int *dataLen )
{
	*dataLen = 2;
	return AVCOM_INT_EncodeCMD ( API_GET_ACTIVE_BAND );
}


/********************************************************************************
*																				*
*		AVCOM_API_EncodeGetActiveRBW											*
*																				*
********************************************************************************/
void *AVCOM_API_EncodeGetActiveRBW ( int *dataLen )
{
	*dataLen = 2;
	return AVCOM_INT_EncodeCMD ( API_GET_ACTIVE_RBW );
}



/********************************************************************************
*																				*
*		AVCOM_API_EncodeGetActiveRefLevel										*
*																				*
********************************************************************************/
void *AVCOM_API_EncodeGetActiveRefLevel ( int *dataLen )
{
	*dataLen = 2;
	return AVCOM_INT_EncodeCMD ( API_GET_REFLVL );
}


/********************************************************************************
*																				*
*		AVCOM_API_EncodeGetActiveVBW											*
*																				*
********************************************************************************/
void *AVCOM_API_EncodeGetActiveVBW ( int *dataLen )
{
	*dataLen = 2;
	return AVCOM_INT_EncodeCMD ( API_GET_ACTIVE_VBW );
}


/********************************************************************************
*																				*
*		AVCOM_API_EncodeGetBandInfo												*
*																				*
********************************************************************************/
void *AVCOM_API_EncodeGetBandInfo ( int *dataLen, int band )
{
	unsigned char sa_tx[64];
	int k = 0;
	unsigned char *data;
	dataConvert	cvt;
	unsigned short cmd = API_GET_BAND_INFO;


	PACK_API_CMD(sa_tx,cmd,k);
	PACK2(sa_tx, band, k);

	data = (unsigned char *)malloc( sizeof(unsigned char) * k );
	memcpy( data, sa_tx, k );
	*dataLen = k;

	return (void *)data;
}


/********************************************************************************
*																				*
*		AVCOM_API_EncodeGetBoardTemp											*
*																				*
********************************************************************************/
void *AVCOM_API_EncodeGetBoardTemp ( int *dataLen )
{
	unsigned char sa_tx[64];
	int k = 0;
	unsigned char *data;
	dataConvert	cvt;
	unsigned short cmd = API_ENG_GET_TEMP;

	PACK_API_CMD(sa_tx,cmd,k);

	data = (unsigned char *)malloc( sizeof(unsigned char) * k );
	memcpy( data, sa_tx, k );
	*dataLen = k;

	return (void *)data;
}


/********************************************************************************
*																				*
*		AVCOM_API_EncodeGetBuildDateTime										*
*																				*
********************************************************************************/
void *AVCOM_API_EncodeGetBuildDateTime ( int *dataLen )
{
	unsigned char sa_tx[64];
	int k = 0;
	unsigned char *data;
	dataConvert	cvt;
	unsigned short cmd = API_BUILD_DATETIME;

	PACK_API_CMD(sa_tx,cmd,k);

	data = (unsigned char *)malloc( sizeof(unsigned char) * k );
	memcpy( data, sa_tx, k );
	*dataLen = k;

	return (void *)data;
}


/********************************************************************************
*																				*
*		AVCOM_API_EncodeGetFirmwareVersion										*
*																				*
********************************************************************************/
void *AVCOM_API_EncodeGetFirmwareVersion ( int *dataLen )
{
	*dataLen = 2;
	return AVCOM_INT_EncodeCMD (  API_BASE_VERSION );
}


/********************************************************************************
*																				*
*		AVCOM_API_EncodeGetFreqValue											*
*																				*
********************************************************************************/
void *AVCOM_API_EncodeGetFreqValue ( int *dataLen )
{
	*dataLen = 2;
	return AVCOM_INT_EncodeCMD ( API_ENG_GET_FREQ );
}


/********************************************************************************
*																				*
*		AVCOM_API_EncodeGetNetwork												*
*																				*
********************************************************************************/
void *AVCOM_API_EncodeGetNetwork ( int *dataLen )
{
	unsigned char sa_tx[64];
	int k = 0;
	unsigned char *data;
	dataConvert	cvt;
	unsigned short cmd = API_GET_NETWORK;

	PACK_API_CMD(sa_tx,cmd,k);

	data = (unsigned char *)malloc( sizeof(unsigned char) * k );
	memcpy( data, sa_tx, k );
	*dataLen = k;

	return (void *)data;
}


/********************************************************************************
*																				*
*		AVCOM_API_EncodeGetProdDesc												*
*																				*
********************************************************************************/
void *AVCOM_API_EncodeGetProdDesc ( int *dataLen )
{
	unsigned char sa_tx[64];
	int k = 0;
	unsigned char *data;
	dataConvert	cvt;
	unsigned short cmd = API_GET_PROD_DESC;

	PACK_API_CMD(sa_tx,cmd,k);

	data = (unsigned char *)malloc( sizeof(unsigned char) * k );
	memcpy( data, sa_tx, k );
	*dataLen = k;

	return (void *)data;
}


/********************************************************************************
*																				*
*		AVCOM_API_EncodeGetRBWList												*
*																				*
********************************************************************************/
void *AVCOM_API_EncodeGetRBWList ( int *dataLen )
{
	unsigned char sa_tx[64];
	int k = 0;
	unsigned char *data;
	dataConvert	cvt;
	unsigned short cmd = API_GET_RBW_LIST;

	PACK_API_CMD(sa_tx,cmd,k);

	data = (unsigned char *)malloc( sizeof(unsigned char) * k );
	memcpy( data, sa_tx, k );
	*dataLen = k;

	return (void *)data;
}


/********************************************************************************
*																				*
*		AVCOM_API_EncodeGetTriggerSweep											*
*																				*
********************************************************************************/
void *AVCOM_API_EncodeGetTriggerSweep ( int *dataLen )
{
	*dataLen = 2;
	return AVCOM_INT_EncodeCMD ( API_TRG_GET_SWEEP );
}


/********************************************************************************
*																				*
*		AVCOM_API_EncodeGetVBWList												*
*																				*
********************************************************************************/
void *AVCOM_API_EncodeGetVBWList ( int *dataLen )
{
	unsigned char sa_tx[64];
	int k = 0;
	unsigned char *data;
	dataConvert	cvt;
	unsigned short cmd = API_GET_VBW_LIST;

	PACK_API_CMD(sa_tx,cmd,k);

	data = (unsigned char *)malloc( sizeof(unsigned char) * k );
	memcpy( data, sa_tx, k );
	*dataLen = k;

	return (void *)data;
}


/********************************************************************************
*																				*
*		AVCOM_API_EncodePing													*
*																				*
********************************************************************************/
void *AVCOM_API_EncodePing ( int *dataLen )
{
	*dataLen = 2;
	return AVCOM_INT_EncodeCMD ( API_PING );
}


/********************************************************************************
*																				*
*		AVCOM_API_EncodeReboot													*
*																				*
********************************************************************************/
void *AVCOM_API_EncodeReboot ( int *dataLen )
{
	*dataLen = 2;
	return AVCOM_INT_EncodeCMD ( API_REBOOT );
}


/********************************************************************************
*																				*
*		AVCOM_API_EncodeSaveBandSettings										*
*																				*
********************************************************************************/
void *AVCOM_API_EncodeSaveBandSettings ( int *dataLen )
{
	*dataLen = 2;
	return AVCOM_INT_EncodeCMD ( API_SAVE_BANDSETTINGS );
}


/********************************************************************************
*																				*
*		AVCOM_API_EncodeSetAnalyzerName											*
*																				*
********************************************************************************/
void *AVCOM_API_EncodeSetAnalyzerName ( int *dataLen, char* name )
{
	unsigned char sa_tx[64];
	int k = 0;
	unsigned short cmd = API_SET_ANALYZER_NAME;
	dataConvert	cvt;
	unsigned char *data;
	

	//Put command at start of data
	PACK_API_CMD(sa_tx,cmd,k);

//	PACK2(sa_tx,band,k);
	PACK_STRCPY(sa_tx,name,k,ANALYZER_NAME_SIZE);

	data = (unsigned char *)malloc( sizeof(unsigned char) * k );
	memcpy( data, sa_tx, k );
	*dataLen = k;

	return (void *)data;

}


/********************************************************************************
*																				*
*		AVCOM_API_EncodeSetBand													*
*																				*
********************************************************************************/
void *AVCOM_API_EncodeSetBand ( int *dataLen, int band )
{
	unsigned char sa_tx[64];
	int k = 0;
	unsigned short cmd = API_SET_BAND;
	dataConvert	cvt;
	unsigned char *data;
	

	//Put command at start of data
	PACK_API_CMD(sa_tx,cmd,k);

	PACK2(sa_tx,band,k);

	data = (unsigned char *)malloc( sizeof(unsigned char) * k );
	memcpy( data, sa_tx, k );
	*dataLen = k;

	return (void *)data;

}


/********************************************************************************
*																				*
*		AVCOM_API_EncodeSetBandName												*
*																				*
********************************************************************************/
void *AVCOM_API_EncodeSetBandName ( int *dataLen, char* bandname )
{
	unsigned char sa_tx[64];
	int k = 0;
	unsigned short cmd = API_SET_BAND_NAME;
	dataConvert	cvt;
	unsigned char *data;
	

	//Put command at start of data
	PACK_API_CMD(sa_tx,cmd,k);

//	PACK2(sa_tx,band,k);
	PACK_STRCPY(sa_tx,bandname,k,AOV_BAND_NAME_LEN);

	data = (unsigned char *)malloc( sizeof(unsigned char) * k );
	memcpy( data, sa_tx, k );
	*dataLen = k;

	return (void *)data;

}


/********************************************************************************
*																				*
*		AVCOM_API_EncodeSetFreqCSD												*
*																				*
********************************************************************************/
void *AVCOM_API_EncodeSetFreqCSD ( int *dataLen, double cfreq, double span, int max_pts )
{
	unsigned char sa_tx[64];
	int k = 0;
	unsigned short cmd = API_SET_FREQ_CSD;
	dataConvert	cvt;
	unsigned char *data;

	cfreq *= 1000;
	span *= 1000;

	PACK_API_CMD(sa_tx,cmd,k);
	PACKF(sa_tx,(float)cfreq,k);
	PACKF(sa_tx,(float)span,k);
	PACK2(sa_tx,(float)max_pts,k);

	data = (unsigned char *)malloc( sizeof(unsigned char) * k );
	memcpy( data, sa_tx, k );
	*dataLen = k;

	return (void *)data;

}


/********************************************************************************
*																				*
*		AVCOM_API_EncodeSetFreqCSS												*
*																				*
********************************************************************************/
void *AVCOM_API_EncodeSetFreqCSS ( int *dataLen, double cfreq, double span, double step )
{
	unsigned char sa_tx[64];
	int k = 0;
	unsigned short cmd = API_SET_FREQ_CSS;
	dataConvert	cvt;
	unsigned char *data;

	cfreq *= 1000;
	span *= 1000;
	step *= 1000;

	PACK_API_CMD(sa_tx,cmd,k);
	PACKF(sa_tx,(float)cfreq,k);
	PACKF(sa_tx,(float)span,k);
	PACKF(sa_tx,(float)step,k);

	data = (unsigned char *)malloc( sizeof(unsigned char) * k );
	memcpy( data, sa_tx, k );
	*dataLen = k;

	return (void *)data;

}


/********************************************************************************
*																				*
*		AVCOM_API_EncodeSetFreqSSS												*
*																				*
********************************************************************************/
void *AVCOM_API_EncodeSetFreqSSS ( int *dataLen, double fstart, double fstop, double fstep )
{
	unsigned char sa_tx[64];
	int k = 0;
	unsigned short cmd = API_SET_FREQ_SSS;
	dataConvert	cvt;
	unsigned char *data;

	fstart *= 1000;
	fstop *= 1000;
	fstep *= 1000;

	//fstart += 0.5;
	//fstop += 0.5;
	//fstep += 0.5;

	PACK_API_CMD(sa_tx,cmd,k);
	PACKF(sa_tx,(float)fstart,k);
	PACKF(sa_tx,(float)fstop,k);
	PACKF(sa_tx,(float)fstep,k);

	data = (unsigned char *)malloc( sizeof(unsigned char) * k );
	memcpy( data, sa_tx, k );
	*dataLen = k;

	return (void *)data;

}


/********************************************************************************
*																				*
*		AVCOM_API_EncodeSetRBW													*
*																				*
********************************************************************************/
void *AVCOM_API_EncodeSetRBW ( int *dataLen, int rbw )
{
	unsigned char sa_tx[64];
	int k = 0;
	unsigned short cmd = API_SET_RBW;
	dataConvert	cvt;
	unsigned char *data;
	

	//Put command at start of data
	PACK_API_CMD(sa_tx,cmd,k);

	PACK4(sa_tx,rbw,k);

	data = (unsigned char *)malloc( sizeof(unsigned char) * k );
	memcpy( data, sa_tx, k );
	*dataLen = k;

	return (void *)data;

}


/********************************************************************************
*																				*
*		AVCOM_API_EncodeSetRefLevel												*
*																				*
********************************************************************************/
void *AVCOM_API_EncodeSetRefLevel ( int *dataLen, int reflvl )
{
	unsigned char sa_tx[64];
	int k = 0;
	unsigned short cmd = API_SET_REFLVL;
	dataConvert	cvt;
	unsigned char *data;
	

	//Put command at start of data
	PACK_API_CMD(sa_tx,cmd,k);

	PACK2(sa_tx,reflvl,k);

	data = (unsigned char *)malloc( sizeof(unsigned char) * k );
	memcpy( data, sa_tx, k );
	*dataLen = k;

	return (void *)data;

}


/********************************************************************************
*																				*
*		AVCOM_API_EncodeSetVBW													*
*																				*
********************************************************************************/
void *AVCOM_API_EncodeSetVBW ( int *dataLen, int vbw )
{
	unsigned char sa_tx[64];
	int k = 0;
	unsigned short cmd = API_SET_VBW;
	dataConvert	cvt;
	unsigned char *data;
	

	//Put command at start of data
	PACK_API_CMD(sa_tx,cmd,k);

	PACK4(sa_tx,vbw,k);

	data = (unsigned char *)malloc( sizeof(unsigned char) * k );
	memcpy( data, sa_tx, k );
	*dataLen = k;

	return (void *)data;

}


/********************************************************************************
 *																				*
 *		AVCOM_API_EncodeSetZeroSpan												*
 *																				*
 ********************************************************************************/
void *AVCOM_API_EncodeSetZeroSpan ( int *dataLen, double freq, int numPts, int sample_rate, int mode)
 {
 	unsigned char sa_tx[64];
 	int k = 0;
	unsigned short cmd = API_SET_ZERO_SPAN;
 	dataConvert	cvt;
 	unsigned char *data;
 	
	freq *= 1000;

	//Put command at start of data
 	PACK_API_CMD(sa_tx,cmd,k);
 
	PACKF(sa_tx,(float)freq,k);
	PACK2(sa_tx,numPts,k);
	PACK2(sa_tx,mode,k);
	PACK4(sa_tx,sample_rate,k);

	data = (unsigned char *)malloc( sizeof(unsigned char) * k );
	memcpy( data, sa_tx, k );
	*dataLen = k;

	return (void *)data;
}


/********************************************************************************
*																				*
*		AVCOM_API_EncodeUDPSetHostname											*
*																				*
********************************************************************************/
void *AVCOM_API_EncodeUDPSetHostname ( int *dataLen, unsigned char *mac, char *hostname )
{
	unsigned char sa_tx[64];
	int k = 0;
	unsigned short cmd = API_CHANGE_HOSTNAME;
	dataConvert	cvt;
	unsigned char *data;
	
	PACK_API_CMD(sa_tx,cmd,k);
	PACK_MEMCPY(sa_tx, mac, k, 6);
	PACK_STRCPY(sa_tx, hostname, k, HOSTNAME_SIZE);

	data = (unsigned char *)malloc( sizeof(unsigned char) * k );
	memcpy( data, sa_tx, k );
	*dataLen = k;

	return (void *)data;

}


/********************************************************************************
*																				*
*		AVCOM_API_EncodeUDPSetNetwork											*
*																				*
********************************************************************************/
void *AVCOM_API_EncodeUDPSetNetwork ( int *dataLen, unsigned char *mac, int dynip, unsigned long ip, unsigned long sub, unsigned long gw )
{
	unsigned char sa_tx[64];
	int k = 0;
	unsigned short cmd = API_CHANGE_NETWORK;
	dataConvert	cvt;
	unsigned char *data;

  if (dynip)
  {
	  ip = 0;
	  sub = 0;
	  gw = 0;
  }
	
	  PACK_API_CMD(sa_tx, cmd, k);
	  PACK_MEMCPY(sa_tx,mac,k,6);
	  PACK1(sa_tx, dynip, k);

	  PACK_MEMCPY(sa_tx,&ip,k,sizeof(unsigned long));
	  PACK_MEMCPY(sa_tx,&sub,k, sizeof(unsigned long));
	  PACK_MEMCPY(sa_tx,&gw,k, sizeof(unsigned long));

	data = (unsigned char *)malloc( sizeof(unsigned char) * k );
	memcpy( data, sa_tx, k );
	*dataLen = k;

	return (void *)data;

}


/********************************************************************************
*																				*
*		AVCOM_API_EncodeUDPSetPort												*
*																				*
********************************************************************************/
void *AVCOM_API_EncodeUDPSetPort ( int *dataLen, unsigned char *mac, unsigned short port )
{
	unsigned char sa_tx[64];
	int k = 0;
	unsigned short cmd = API_CHANGE_NETWORK;
	dataConvert	cvt;
	unsigned char *data;

	PACK_API_CMD(sa_tx,cmd,k);
	PACK_MEMCPY(sa_tx,mac,k,6);
	PACK2(sa_tx, port, k);

	data = (unsigned char *)malloc( sizeof(unsigned char) * k );
	memcpy( data, sa_tx, k );
	*dataLen = k;

	return (void *)data;

}
#endif /* AOV_DEPEND_ENCODE */


/********************************************************************************
*																				*
*		AVCOM_API_GetAPIVersion													*
*																				*
********************************************************************************/
int AVCOM_API_GetAPIVersion ( struct aov_version *ver )
{

	ver->major = AOVAPI_MAJOR;
	ver->minor = AOVAPI_MINOR;
	ver->build = AOVAPI_BUILD;
	ver->stage = AOVAPI_STAGE;

	switch (ver->stage)
	{
		case 0: // alpha
			snprintf(ver->string, 16, "%i.%ia%i", ver->major, ver->minor, ver->build);
			break;
		case 1: // beta
			snprintf(ver->string, 16, "%i.%ib%i", ver->major, ver->minor, ver->build);
			break;
		case 2: // Rlease Canidate
			snprintf(ver->string, 16, "%i.%irc%i", ver->major, ver->minor, ver->build);
			break;
		case 3: // Full Release
			snprintf(ver->string, 16, "%i.%i", ver->major, ver->minor);
			break;
		default:
			snprintf(ver->string, 16, "NULL");
			break;
	}

	return AOV_NO_ERROR;

}


/********************************************************************************
*																				*
*		AVCOM_API_GetAPIVersionString											*
*																				*
********************************************************************************/
int AVCOM_API_GetAPIVersionString ( char *version )
{
	struct aov_version ver;
	int iResult;

	iResult = AVCOM_API_GetAPIVersion ( &ver );

	if (iResult == AOV_NO_ERROR)
	{
		strncpy(version, ver.string, 16);
	}

	return AOV_NO_ERROR;
}

#if 0

// A MS Function
/********************************************************************************
*																				*
*		AVCOM_API_Initialize													*
*																				*
********************************************************************************/
int AVCOM_API_Initialize(void)
{
#ifdef AOV_DEPEND_SA
#ifdef _WIN32
	WSADATA wsaData;
#endif /* _WIN32 */


#ifdef _WIN32
	if (WSAStartup(MAKEWORD(2, 0),&wsaData) != 0)
		return AOV_ERR_WSASTARTUP_FAILED;		// WSAStartup() failed
#endif /* _WIN32 */

	if (plist != NULL)
		return AOV_ERR_API_INITIALIZED;
	plist = malloc( sizeof(AOVhandle *) );
	plen = 0;
#endif /* AOV_DEPEND_SA */

	crcInit();
	
	return AOV_NO_ERROR;

}


#endif

// A MS Function
/********************************************************************************
*																				*
*		AVCOM_INT_AssemblePacket												*
*																				*
********************************************************************************/
unsigned char *AVCOM_INT_AssemblePacket(unsigned char *data_in, int data_in_len, int *data_out_len, int flags)
{ 
	unsigned char *data_out;
	crc crc_16;
	data_out=0;

        UNUSED_PARAM(flags);
	data_out = (unsigned char *)malloc(sizeof(unsigned char) * data_in_len + AVCOM_HEADER_LENGTH);
	if(data_out==NULL) 
		return (unsigned char*)AOV_ERR_HDR_MALLOC;

	data_out[0] = (unsigned char)(AVCOM_PROTOCOL_ID >> 8);
	data_out[1] = (unsigned char)(AVCOM_PROTOCOL_ID);
	data_out[2] = AVCOM_PROTOCOL_VER_MAJ;
	data_out[3] = AVCOM_PROTOCOL_VER_MIN;
	data_out[4] = 12;						// Header length
	data_out[5] = 0;						// Flags
	data_out[6] = data_in_len >> 8;
	data_out[7] = data_in_len & 0xff;
	memset((void *)&data_out[ 8],0,2);
	
	crc_16 = crcFast(data_in, data_in_len);
	data_out[10] = crc_16 >> 8;
	data_out[11] = crc_16 & 0xff;
	
	crc_16 = crcFast(data_out, AVCOM_HEADER_LENGTH);
	data_out[8] = crc_16 >> 8;
	data_out[9] = crc_16 & 0xff;

	memcpy((void *)&data_out[12],data_in,data_in_len);
	*data_out_len = AVCOM_HEADER_LENGTH + data_in_len;

	return data_out;

}



#ifdef AOV_DEPEND_SA
// A MS Function
/********************************************************************************
*																				*
*		AVCOM_INT_CheckHandle													*
*																				*
********************************************************************************/
int AVCOM_INT_CheckHandle(void *handle)
{
#if 0
	int i;
#endif
        UNUSED_PARAM(handle);
	
        return (AOV_NO_ERROR);
#if 0
	for (i = 0; i < plen;  i++)
	{
		if ( plist[i] == handle)
			return AOV_NO_ERROR;
	}

	return AOV_ERR_HANDLE_NOT_FOUND;
#endif
}
#endif /* AOV_DEPEND_SA */


#ifdef AOV_DEBUG
/********************************************************************************
*																				*
*		AVCOM_INT_DumpHex														*
*																				*
********************************************************************************/
static void AVCOM_INT_DumpHex(unsigned char *data, int len) {
	int i;
	div_t divresult;
	if (len<0)
        {
            len=0;
        }
	if (len>1024)
        {
            len=1024;
        }
	reconnDebugPrint("%s: DUMP - Dumping %i bytes.\n", __FUNCTION__, len);
	if (len>0)
        {
            reconnDebugPrint("DUMP: ");
        }
	for(i=0;i<len;i++) {
		reconnDebugPrint(" %2X",data[i]);
		divresult = div ((i+1),16);
		if (divresult.rem==0)
                {
                    reconnDebugPrint("\nDUMP: ");
                }
	}
	if (len>0)
        {
            reconnDebugPrint("\n");
        }
}
#endif /* AOV_DEBUG */


#ifdef AOV_DEPEND_ENCODE
/********************************************************************************
*																				*
*		AVCOM_INT_EncodeCMD														*
*																				*
********************************************************************************/
void *AVCOM_INT_EncodeCMD ( unsigned short cmd )
{
	int k = 0;
	dataConvert	cvt;
	unsigned char *data;

	data = (unsigned char *)malloc( sizeof(unsigned char) * 2 );
        memset(&cvt, 0 ,sizeof(dataConvert));
	PACK_API_CMD(data,cmd,k);
	return (void *)data;

}
#endif /* AOV_DEPEND_ENCODE */









// A MS Function
/********************************************************************************
*																				*
*		AVCOM_INT_ProcessHeader													*
*																				*
********************************************************************************/
int AVCOM_INT_ProcessHeader ( struct aov_header *header, unsigned char *raw_header ) 
{
	unsigned char tmp_header[AVCOM_HEADER_LENGTH];

	memcpy( tmp_header, raw_header, AVCOM_HEADER_LENGTH );
	header->protocol_id 		= (raw_header[0]  << 8) | raw_header[1];
	header->protocol_ver		= (raw_header[2]  << 8) | raw_header[3];
	header->header_len			=  raw_header[4];
	header->flags				=  raw_header[5];
	header->data_len			= (raw_header[6]  << 8) | raw_header[7];
	header->header_crc			= (raw_header[8]  << 8) | raw_header[9];
	memset(&tmp_header[8],0,2);
	header->data_crc			= (raw_header[10] << 8) | raw_header[11];
	
	if (header->header_len != AVCOM_HEADER_LENGTH )
		return AOV_ERR_HDR_HEADER_SIZE;
	
	if ( crcFast((const unsigned char *)tmp_header, AVCOM_HEADER_LENGTH) != header->header_crc )
		return AOV_ERR_HDR_CHECKSUM;
	
	if ( header->protocol_ver != AVCOM_PROTOCOL_VERSION )
		return AOV_ERR_HDR_PROTOCOL_ID;
		
	return AOV_NO_ERROR;
		
}


#ifdef AOV_DEPEND_SA
// A MS Function
/********************************************************************************
*																				*
*		AVCOM_INT_RecvData														*
*																				*
********************************************************************************/
int AVCOM_INT_RecvData(void *handle, unsigned char *data, int data_size)
{
    struct timeval waitTime;
    waitTime.tv_sec = 8;
    waitTime.tv_usec = 0;
    int size = data_size;
    ReconnErrCodes retCode;


    retCode = SpectrumAnalyzerSelectRead(data, &size, &waitTime);
    //reconnDebugPrint("%s: SpectrumAnalyzerSelectRead() returned %s size = %d\n", __FUNCTION__, (retCode == RECONN_SUCCESS) ? "SUCCESS" : "FAILURE", size);
    if(retCode == RECONN_FAILURE)
    {
        return (AOV_ERR_RECV_FAILED);
    }
    else
    {
        return (size);
    }
    UNUSED_PARAM(handle);
#if 0
	int iResult;
//	char onedata;
//	int startpacket = 0;
//	int packetnotfull = 1;
	int i;
//	unsigned short packlen;
	AOVhandle *lhandle = (AOVhandle *)handle;
#ifdef AOV_DEPEND_USB
	DWORD BytesRecieved;
#endif /* AOV_DEPEND_USB */
	int toRecieved = data_size;
	int ptr = 0;
//	FT_STATUS ftStatus;
/* 
	Currently only used inside API's so this is already done
	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;
*/
	
	i = 0;
	if ( lhandle->socket_type == AOV_TCP_DEFAULT )
	{
		while (data_size) {
			iResult = recv(lhandle->socketfd, &data[ptr], data_size, 0);
			if (iResult < 0)
			{
//				return AO_ERR_RECV_FAILED;
				return -1;
			}
			else if (iResult == 0)
			{
//				return AOV_ERR_SOCKET_NOT_OPEN;
				return -2;
			}
			ptr += iResult;
			data_size -= iResult;
		}

		return toRecieved;
 
#ifdef AOV_DEPEND_USB	
	} else if ( lhandle->socket_type == AOV_USB_DEFAULT ) {
		iResult = FT_Read(lhandle->ftHandle, data,data_size,&BytesRecieved);
		if ( iResult != 0)
			return AOV_ERR_RECV_FAILED;
		return BytesRecieved;
#endif /* AOV_DEPEND_USB */
	}


	return AOV_ERR_INVALID_SOCKET;
#endif
}
#endif /* AOV_DEPEND_SA */



#ifdef AOV_DEPEND_SA
// A MS Function
/********************************************************************************
*																				*
*		AVCOM_INT_RecvPacket													*
*																				*
********************************************************************************/
int AVCOM_INT_RecvPacket(void *handle, unsigned char *data, int *data_size)
{
    AOVhandle *lhandle = (AOVhandle *)handle;
    unsigned char avcom_header[12];
    unsigned short temp_checksum;
    int iResult;
    struct aov_header header;

    //reconnDebugPrint("%s: Function Entered\n", __FUNCTION__);

    // get a packet which should be the AVCOM Header
    iResult = AVCOM_INT_RecvData(lhandle, avcom_header, AVCOM_HEADER_LENGTH);		// RETURNS # data recv'd!!!

    // Check first for socket-level error in case we never got any data
    if (iResult == -1) {
        return AOV_ERR_RECV_FAILED;
    }
    else if (iResult == -2) 
    {
        return AOV_ERR_SOCKET_NOT_OPEN;
    }

    // Now check the length
    if (iResult != AVCOM_HEADER_LENGTH) {
        reconnDebugPrint("%s: invalid header length = %d\n", __FUNCTION__, iResult);
#ifdef AOV_DEBUG
        AVCOM_INT_DumpHex(avcom_header, iResult);
#endif /* AOV_DEBUG */
        return AOV_ERR_HDR_PACKET_SIZE;
    }

    if ( (iResult = AVCOM_INT_ProcessHeader(&header, avcom_header))  != AOV_NO_ERROR) 
    {
        reconnDebugPrint("%s: AVCOM_INT_ProcessHeader failed = %d\n", __FUNCTION__, iResult);
#ifdef AOV_DEBUG
        AVCOM_INT_DumpHex(avcom_header, AVCOM_HEADER_LENGTH);
#endif /* AOV_DEBUG */
        return iResult;
    }
	*data_size = header.data_len;

	iResult = AVCOM_INT_RecvData(lhandle,data, *data_size);		// RETURNS # data recv'd!!!

	// Check first for socket-level errors in case we never got any data
	if (iResult == -1) {
		return AOV_ERR_RECV_FAILED;
	}
	else if (iResult == -2) 
	{
		return AOV_ERR_SOCKET_NOT_OPEN;
	}

	// Now check the data length
	if (iResult != *data_size)
	{
#ifdef AOV_DEBUG
		AVCOM_INT_DumpHex(data, iResult);
#endif /* AOV_DEBUG */
		//recv'ed wrong number of bytes!
		return AOV_ERR_HDR_DATA_SIZE;
	}
	
	// perform data checksum and check it
	temp_checksum=crcFast(data,*data_size);
	if (temp_checksum != header.data_crc)
	{
#ifdef AOV_DEBUG
		AVCOM_INT_DumpHex(data, iResult);
#endif /* AOV_DEBUG */
		// bad data checksum
		return AOV_ERR_HDR_DATA_CHECKSUM;
	}

	//Got through processing AVCOM Header successfully
	return AOV_NO_ERROR;

}
#endif /* AOV_DEPEND_SA */



#ifdef AOV_DEPEND_SA
// A MS Function
/********************************************************************************
*																				*
*		AVCOM_INT_ReturnACK														*
*																				*
********************************************************************************/
int AVCOM_INT_ReturnACK ( unsigned short cmd, unsigned char* data, int *j )
{
	unsigned short datacmd;
	int iResult;
	dataConvert cvt;

	// j == 0 j should always be zero when executing this command. Force?
	UNPACK_API_CMD(datacmd, data, (*j));
	if (datacmd != API_ACK)
		return AOV_ERR_UNEXPECTED_CMD;

	UNPACK_API_CMD(datacmd, data, (*j));
	if (datacmd != cmd)
		return AOV_ERR_INCORRECT_ACK;

	UNPACK4(iResult, data, (*j));

	return iResult;
}
#endif /* AOV_DEPEND_SA */








#ifdef AOV_DEPEND_SA
// A MS Function
/********************************************************************************
*																				*
*		AVCOM_INT_SendData														*
*																				*
********************************************************************************/
int AVCOM_INT_SendData(void *handle, unsigned char *data, int data_size)
{

    if(SpectrumAnalyzerWrite(data, data_size) == RECONN_SUCCESS)
    {
        return AOV_NO_ERROR;
    }
    else
        return AOV_ERR_SEND_FAILED;
    UNUSED_PARAM(handle);
#if 0
	AOVhandle *lhandle = (AOVhandle *)handle;
#ifdef AOV_DEPEND_USB
	DWORD BytesWritten;
	FT_STATUS ftStatus;
#endif /* AOV_DEPEND_USB */
// Currently only used inside API's so this is already done
//	if (AVCOM_INT_CheckHandle(handle))
//		return AOV_ERR_HANDLE_NOT_FOUND;

	if (lhandle->socket_type == AOV_TCP_DEFAULT)
	{
		if (send(lhandle->socketfd, (char *)data, data_size, 0) != data_size)
			return AOV_ERR_SEND_FAILED;
#ifdef AOV_DEPEND_USB
	} else if (lhandle->socket_type == AOV_USB_DEFAULT) {
		ftStatus = FT_Write(lhandle->ftHandle,(char *)data,data_size,&BytesWritten);
		if ( ftStatus != FT_OK )
			return AOV_ERR_SEND_FAILED;
		if ( data_size != BytesWritten )
			return AOV_ERR_SEND_FAILED;
#endif /* AOV_DEPEND_USB */
	} else {
		return AOV_ERR_INVALID_SOCKET;
	}
#endif

	return AOV_NO_ERROR;
}
#endif /* AOV_DEPEND_SA */



#ifdef AOV_DEPEND_SA
// A MS Function
/********************************************************************************
*																				*
*		AVCOM_INT_SendPacket													*
*																				*
********************************************************************************/
int AVCOM_INT_SendPacket(void *handle, unsigned char *data, int data_size)
{
	int data_out_len;
	unsigned char *data_out;
	int iResult;
	AOVhandle *lhandle = (AOVhandle *)handle;

	data_out = AVCOM_INT_AssemblePacket(data, data_size, &data_out_len, 0);
	if ((int)data_out < 0)
		return (int)data_out;

	if ( (iResult = AVCOM_INT_SendData(lhandle, data_out, data_out_len)) != AOV_NO_ERROR )
		return iResult;

	free(data_out);


	//memcpy((void *)send_buf[2],(const void *)AVCOM_PROTOCOL_VERSION,2);

	return AOV_NO_ERROR;
}

#endif /* AOV_DEPEND_SA */





























