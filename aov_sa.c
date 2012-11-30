
#include "aov_opts.h"
#ifdef AOV_DEPEND_SA

#include <stdlib.h>
#include "stdafx.h"

#include "debugMenu.h"
#include "reconn.h"

#include "aov_core.h"
#include "aov_common.h"
#include "aov_async.h"
#include "aov_sa.h"
#include "aov_protocol.h"
#include "aov_errno.h"
#include "crc.h"
#include "ftd2xx.h"





#ifdef _WIN32
#define snprintf sprintf_s
#define strncpy(dst,src,size) strncpy_s(dst,size,src,size)
#endif 


#if 0
#ifdef AOV_DEPEND_SA
static int AVCOM_INT_GetLists ( void *handle, unsigned short cmd, int *list, int list_size );
static int AVCOM_INT_GetSpectrumData(void *handle, unsigned short cmd, sAOV_SweepData *trace);
#ifdef AOV_DEPEND_USB
static int AVCOM_INT_ValidateUSBLocation( int id );
#endif /* AOV_DEPEND_USB */
#endif /* AOV_DEPEND_SA */
#endif




// A MS Function
/********************************************************************************
*																				*
*		AVCOM_INT_FlashSBS2Shared												*
*																				*
********************************************************************************/
int AVCOM_INT_FlashSBS2Shared ( void *handle, unsigned short cmd, unsigned char* data, int data_len )
{
	unsigned char sa_tx[1024];
	unsigned char sa_rx[1024];
	crc file_crc;
	int i;
	int j = 0;
	int k = 0;
	int numi;
	int recvlen;
	int iResult;
	unsigned short tmpcmd = API_RECV_START;
	dataConvert cvt;
//	AOVhandle *lhandle = (AOVhandle *)handle; 

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;
	
	numi = data_len / 512;
	if (data_len % 512)
		numi++;		
	file_crc = crcFast(data, data_len);

	PACK_API_CMD( sa_tx, tmpcmd, k);

	PACK4( sa_tx, data_len, k);
	PACK4( sa_tx, numi, k);
	PACK2( sa_tx, file_crc, k);

	if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
		return iResult;
	if ( (iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR )
		return iResult;	//ACK
	if ( (iResult = AVCOM_INT_ReturnACK(tmpcmd, sa_rx, &j)) != AOV_NO_ERROR )
		return iResult;


	for ( i = 1; i <= numi; i++)
	{
		int send_len = 512;
		int ii;

		if ( (data_len - ((i-1) * 512)) < 512)
			send_len = data_len - ((i-1) * 512);

		k = 0;
		tmpcmd = API_FILE_BLOCK;
		PACK_API_CMD( sa_tx, tmpcmd, k);

		PACK4( sa_tx, send_len, k);
		PACK4( sa_tx, i, k);

		for (ii = 0; ii < send_len; ii++) {
			PACK1(sa_tx, (data[(i-1)*512 + ii]), k);
		}

		j = 0;
		if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
			return iResult;
		if ( (iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR )
			return iResult;	//ACK
		if ( (iResult = AVCOM_INT_ReturnACK(tmpcmd, sa_rx, &j)) != AOV_NO_ERROR )
			return iResult;
	}

	k = 0;
        reconnDebugPrint("\n\n%s: sending command %d\n\n", __FUNCTION__, cmd);
	PACK_API_CMD(sa_tx, cmd, k);

	j = 0;
	if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
		return iResult;
	if ( (iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR )
		return iResult;	//ACK
	if ( (iResult = AVCOM_INT_ReturnACK(cmd, sa_rx, &j)) != AOV_NO_ERROR )
		return iResult;


	return AOV_NO_ERROR;
}

#if 0
// A MS Function
/********************************************************************************
*																				*
*		AVCOM_INT_GetLists														*
*																				*
********************************************************************************/
static int AVCOM_INT_GetLists ( void *handle, unsigned short cmd, int *list, int list_size )
{
	unsigned char sa_tx[64];
	unsigned char sa_rx[1024];
	int recvlen;
	int i = 0;
	int j = 0;
	int k = 0;
	int iResult;
	dataConvert	cvt;
	unsigned short count;//, size, offset;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	//Put command at start of data
	PACK_API_CMD(sa_tx,cmd,k);


	if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
		return iResult;
	if ( (iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR )
		return iResult;	
	// Check ACK
	if ( (iResult = AVCOM_INT_ReturnACK(cmd, sa_rx, &j)) != AOV_NO_ERROR )
		return iResult;

	// If we get here, assume no error as default.
	iResult = AOV_NO_ERROR;

	//Process return data
	UNPACK2(count, sa_rx, j);
	if (count > list_size)
	{
		iResult = AOV_WARN_LIST_SIZE;
		count = list_size;
	}

	for ( i = 0; i < count; i++ )
	{
		UNPACK4(list[i],sa_rx,j);
	}

	return iResult;
}

#endif







#if 0
// A MS Function but not implemented
/********************************************************************************
*																				*
*		AVCOM_INT_GetSpectrumData												*
*																				*
********************************************************************************/
int AVCOM_INT_GetSpectrumData(void *handle, unsigned short cmd, sAOV_SweepData *trace)
{
	unsigned char sa_tx[64];
	unsigned char sa_rx[100000];
	int recvlen;
//	int i = 0;
	int j = 0;
	int k = 0;
	int iResult;
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

	AVCOM_API_DecodeGetTriggerSweep ( &sa_rx[j], (recvlen-j), trace );
/*
	UNPACK2(trace->count,sa_rx,j);

	// Not currently dynamically malloc'd
	//trace->sweep=(sAVCOM_ENG_SweepData_Array *)malloc(trace->count*sizeof(sAVCOM_ENG_SweepData_Row));
	UNPACK_FREQ_K2M(trace->fStart,j,sa_rx);
	UNPACK_FREQ_K2M(trace->fStop,j,sa_rx);
	UNPACK_FREQ_K2M(trace->fStep,j,sa_rx);
	
	dFreqCurr = trace->fStart;
	if (trace->fStop < trace->fStart)
		dFreqCurr = trace->fStop;

	for (i=0;i<trace->count;i++) 
	{
		UNPACKF(trace->sweep[i].power,sa_rx,j);
		trace->sweep[i].frequency=dFreqCurr;
		dFreqCurr+=trace->fStep;
	}
*/
	return AOV_NO_ERROR;
		
}
#endif


/********************************************************************************
 *																				*
 *				AVCOM_INT_GetVersion()											*
 *																				*
 *******************************************************************************/
int AVCOM_INT_GetVersion ( void *handle, unsigned short cmd, struct aov_version *ver )
{
	unsigned char sa_tx[1024];
	unsigned char sa_rx[1024];
	int k = 0;
	int j = 0;
	int recvlen;
	int iResult;
	unsigned char *data;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	k = 2;
	data = (unsigned char *)AVCOM_INT_EncodeCMD ( cmd );
	memcpy(sa_tx, data, k);
	free(data);
	if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
		return iResult;
	if ((iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR)
        {
            return iResult;	
        }
	//ACK
	if ( (iResult = AVCOM_INT_ReturnACK(cmd, sa_rx, &j)) != AOV_NO_ERROR )
		return iResult;

	AVCOM_API_DecodeGetFirmwareVersion ( &sa_rx[j], (recvlen-j), ver );
/*
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
*/
	return iResult;
}

#if 0

#ifdef AOV_DEPEND_USB
/********************************************************************************
 *																				*
 *				AVCOM_INT_ValidateUSBLocation()									*
 *																				*
 *******************************************************************************/
static int AVCOM_INT_ValidateUSBLocation( int id )
{
	FT_STATUS ftStatus;
	DWORD numDevs;

	FT_HANDLE ftHandleTemp; 
	DWORD Flags; 
	DWORD ID; 
	DWORD Type; 
	DWORD LocId; 
	char SerialNumber[16]; 
	char Description[64];


	if ( (ftStatus = FT_CreateDeviceInfoList(&numDevs)) != FT_OK )
		return AOV_ERR_USB_COMM;
	
	// If selected id is larger than number of available ID's (also, it wouldn't connect, but we'll handle that sooner)
	if ( (unsigned int)id >= numDevs )
		return AOV_ERR_USB_DEV_NUM;

	// get information for selected id (selected device)
	if( (ftStatus = FT_GetDeviceInfoDetail(id, &Flags, &Type, &ID, &LocId, SerialNumber, Description, &ftHandleTemp)) != FT_OK )
		return AOV_ERR_USB_COMM;

	// VID = 0x0403 and PID = 0x6001, it may be ours. 
	// Need to add more to uniquly identify the USB device as ours
	if (ID == 0)
		return AOV_ERR_USB_IN_USE;

	if (ID != 0x04036001)
		return AOV_ERR_USB_DEV_NUM;
	
	// If we made it here, it might be ours
	return AOV_NO_ERROR;
}
#endif	




// A MS Function but not implemented
/********************************************************************************
*																				*
*		AVCOM_INT_GetSpectrumData												*
*																				*
********************************************************************************/
int AVCOM_INT_GetZeroSpan(void *handle, unsigned short cmd, sAOV_ZeroSpanData *trace)
{
	unsigned char sa_tx[64];
	unsigned char sa_rx[100000];
	int recvlen;
//	int i = 0;
	int j = 0;
	int k = 0;
	int iResult;
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

	AVCOM_API_DecodeGetTriggerZeroSpan ( &sa_rx[j], (recvlen-j), trace );

	return AOV_NO_ERROR;
		
}





// A MS Function
/********************************************************************************
 *																				*
 *				AVCOM_SA_Connect()												*
 *																				*
 *******************************************************************************/
int AVCOM_SA_Connect(void *handle, int timeout) 
{
	int *socketfd;
	//unsigned char sa_rx[1024];
	//int bytes_recv;
	AOVhandle *lhandle = (AOVhandle *)handle;
	int iResult;
#ifdef AOV_DEPEND_USB
	FT_STATUS ftStatus;
	FTTIMEOUTS ftTS;
#endif /* AOV_DEPEND_USB */
	
	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	if ( lhandle->socket_type == AOV_TCP_DEFAULT )
	{
		socketfd = (&lhandle->socketfd);
		
		if (*socketfd != -1) // already open
		{
			return AOV_ERR_SOCKET_ALREADY_OPEN;
		}

		if ((*socketfd = socket(lhandle->servinfo->ai_family, lhandle->servinfo->ai_socktype, lhandle->servinfo->ai_protocol)) < 0)
			return AOV_ERR_OPEN_SOCKET_FAILED;		// socket() failed

		if (timeout == 0)
			timeout = 15000;
		
#ifdef _WIN32		// If windows

		if ((iResult = setsockopt((int)*socketfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,  sizeof(timeout))) != 0)
#else				// Otherwise Linux
		struct timeval tv;
		tv.tv_sec = timeout / 1000;
		tv.tv_usec = (timeout - (tv.tv_sec*1000)) *1000;
		if ((iResult = setsockopt((int)*socketfd, SOL_SOCKET, SO_RCVTIMEO, (void *)&tv,  sizeof(tv))) != 0)
#endif				// End Win/linux set SO_RCVTIMEO difference - but same return value below!
			return AOV_ERR_SETSOCKETOPT_FAILED;

		if (connect((int)*socketfd, lhandle->servinfo->ai_addr, lhandle->servinfo->ai_addrlen) < 0)
		{
			//iResult = WSAGetLastError();  // Extra Error checking for windows only
			AVCOM_SA_Disconnect(handle);
			return AOV_ERR_CONNECT_FAILED;		// connect() failed
		}

//		AVCOM_INT_RecvData(handle, sa_rx, &bytes_recv);
#ifdef AOV_DEPEND_USB
	} else if ( lhandle->socket_type == AOV_USB_DEFAULT ) {

		ftStatus = FT_OpenEx(lhandle->usblocId,FT_OPEN_BY_LOCATION,&lhandle->ftHandle);
		if (ftStatus == FT_OK) 
		{ 
			// success
		} else { 
			// failure
			return AOV_ERR_OPEN_SOCKET_FAILED;
		}

		ftStatus = FT_ResetDevice(lhandle->ftHandle);
		if (ftStatus == FT_OK) 
		{ 
			// success
		} else { 
			// failure
			return AOV_ERR_SETSOCKETOPT_FAILED;
		}
		
		ftTS.ReadIntervalTimeout = 0; 
		ftTS.ReadTotalTimeoutMultiplier = 0; 
		ftTS.ReadTotalTimeoutConstant = timeout; 
		ftTS.WriteTotalTimeoutMultiplier = 0; 
		ftTS.WriteTotalTimeoutConstant = timeout; 
		
		if (FT_W32_SetCommTimeouts(lhandle->ftHandle,&ftTS)) 
		if (ftStatus == FT_OK) 
		{ 
			// success
		} else { 
			// failure
			return AOV_ERR_SETSOCKETOPT_FAILED;
		}

		ftStatus = FT_SetBaudRate(lhandle->ftHandle,460800);
		if (ftStatus == FT_OK) 
		{ 
			// success
		} else { 
			// failure
			return AOV_ERR_SETSOCKETOPT_FAILED;
		}

		ftStatus = FT_SetDataCharacteristics(lhandle->ftHandle,FT_BITS_8, FT_STOP_BITS_1, FT_PARITY_NONE);
		if (ftStatus == FT_OK) 
		{ 
			// success
		} else { 
			// failure	
			return AOV_ERR_SETSOCKETOPT_FAILED;
		}

		ftStatus = FT_SetFlowControl(lhandle->ftHandle,FT_FLOW_NONE,0,0);
		if (ftStatus == FT_OK) 
		{ 
			// success
		} else { 
			// failure
			return AOV_ERR_SETSOCKETOPT_FAILED;
		}

		ftStatus = FT_Purge(lhandle->ftHandle,FT_PURGE_RX | FT_PURGE_TX);
		if (ftStatus == FT_OK) 
		{ 
			// success
		} else {	
			// failure
			return AOV_ERR_SETSOCKETOPT_FAILED;
		}
#endif /* AOV_DEPEND_USB */
	} else {
		return AOV_ERR_INVALID_SOCKET;
	}

	return AOV_NO_ERROR;
}


// A MS Function
/********************************************************************************
*																				*
*		AVCOM_SA_Disconnect														*
*																				*
********************************************************************************/
int AVCOM_SA_Disconnect(void *handle) 
{
	AOVhandle *lhandle = (AOVhandle *)handle;
#ifdef AOV_DEPEND_USB
	FT_STATUS ftStatus;
#endif /* AOV_DEPEND_USB */
//	int i;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	if (lhandle->socket_type == AOV_TCP_DEFAULT)
	{
		if (lhandle->socketfd == -1)
			return AOV_ERR_SOCKET_NOT_OPEN;

#ifdef _WIN32
		if (closesocket(lhandle->socketfd))
			return AOV_ERR_CLOSESOCKET_FAILED;
#else
		int i;
		if (close(lhandle->socketfd))
			return AOV_ERR_CLOSESOCKET_FAILED;
		for (i=0; i< 0x0FFFFFF; i++);
#endif
		lhandle->socketfd = -1;
#ifdef AOV_DEPEND_USB
	} else if ( lhandle->socket_type == AOV_USB_DEFAULT ) {
		ftStatus = FT_Close(lhandle->ftHandle);
		if (ftStatus == FT_OK) 
		{	
			// success
		} else { 
			return AOV_ERR_SOCKET_NOT_OPEN;
		}
#endif /* AOV_DEPEND_USB */
	} else {
		return AOV_ERR_INVALID_SOCKET;
	}

	return AOV_NO_ERROR;
}


// A MS Function
/********************************************************************************
*																				*
*		AVCOM_SA_DiscoverIP														*
*																				*
********************************************************************************/
int AVCOM_SA_DiscoverIP ( struct aov_product **analyzers, int *numAnalyzers )
{
  int SendSocket, RecvSocket;
  struct sockaddr_in SendAddr;
  struct sockaddr_in RecvAddr;
  struct aov_header header;
  struct sockaddr_storage addr_store;
  unsigned short portlisten = 26483;
  unsigned short portsend = 26482;
  char RecvBuf[1024];
  char sa_tx[1024];
  int BufLen = 1024;
  int fromlen;
  unsigned char *sendbuf;
  int iResult = 0;
  int sendlen;
  int j = 0;
  int k = 0;
  int so_broadcast;
  int z;
  dataConvert cvt;
#ifdef _WIN32
	DWORD timeout;
#endif
#ifdef _WIN32
  int y;
#else
  int i;
#endif
  

  //---------------------------------------------
  // Create a socket for sending data
  SendSocket = socket(AF_INET, SOCK_DGRAM, 0);
  RecvSocket = socket(AF_INET, SOCK_DGRAM, 0);

 #ifdef _WIN32		// If windows
	timeout = 2000;

	if ((iResult = setsockopt((int)RecvSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,  sizeof(timeout))) != 0)
#else				// Otherwise Linux
	struct timeval tv;
	tv.tv_sec = 2;
	tv.tv_usec = 0;
	if ((iResult = setsockopt((int)RecvSocket, SOL_SOCKET, SO_RCVTIMEO, (void *)&tv,  sizeof(tv))) != 0)
#endif				// End Win/linux set SO_RCVTIMEO difference - but same return value below!
		return AOV_ERR_SETSOCKETOPT_FAILED;

  //---------------------------------------------
  // Set up the RecvAddr structure with the IP address of
  // the receiver (in this example case "192.168.1.1")
  // and the specified port number.
  memset(&SendAddr, 0, sizeof(struct sockaddr_in));
  SendAddr.sin_family = AF_INET;
  SendAddr.sin_port = htons(portsend);
  SendAddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

  memset(&RecvAddr, 0, sizeof(struct sockaddr_in));
  RecvAddr.sin_family = AF_INET;
  RecvAddr.sin_port = htons(portlisten);
  RecvAddr.sin_addr.s_addr = INADDR_ANY;
  
  so_broadcast = 1;
  z = setsockopt((int)SendSocket,SOL_SOCKET,SO_BROADCAST, (char *)&so_broadcast, sizeof(so_broadcast));
#ifdef AOV_DEBUG
  printf("%i\n",errno);
#endif

  z = setsockopt((int)RecvSocket,SOL_SOCKET,SO_BROADCAST, (char *)&so_broadcast, sizeof(so_broadcast));
#ifdef AOV_DEBUG
  printf("%i\n",errno);
#endif

  if (  bind((int)RecvSocket, (struct sockaddr *)&RecvAddr, sizeof(RecvAddr)) == -1 ) 
  {
#ifdef AOV_DEBUG
		reconnDebugPrint("Could not bind name to socket.\n");
#endif
#ifdef _WIN32
//	y = WSAGetLastError();
		closesocket(SendSocket);
//		WSACleanup();
//		exit(0);
#else
		close(RecvSocket);
		for (i=0; i< 0x0FFFFFF; i++);
		close(RecvSocket);
		for (i=0; i< 0x0FFFFFF; i++);
#endif
		return AOV_ERR_OPEN_SOCKET_FAILED;
  }

  //---------------------------------------------
  // Send a datagram to the receiver
#ifdef AOV_DEBUG
  printf("Sending a datagram to the receiver...\n");
#endif
	  
  k=0;
  PACK_API_CMD(sa_tx, API_DISCOVERY_HELLO, k);

  sendbuf = AVCOM_INT_AssemblePacket((unsigned char *)sa_tx, 2, &sendlen, 0);

  z=sendto((int)SendSocket, 
    sendbuf, 
    sendlen, 
    0, 
    (struct sockaddr *) &SendAddr, 
    sizeof(SendAddr));

  free(sendbuf);
//	y = WSAGetLastError();

  fromlen = sizeof(addr_store);

  *analyzers = NULL;
  z=0;
  *numAnalyzers = 0;
  while ( z >= 0)
  {
	  z=recvfrom((int)RecvSocket, 
		  RecvBuf, 
		  BufLen, 
		  0, 
		  (struct sockaddr *) &addr_store, 
		  (socklen_t*)&fromlen );

	  if (( z != sendlen ) && ( z > 0 )) // don't process if recving an echo (look for something larger than sendlen)
	  {
		  (*numAnalyzers)++;
		  j = 14;

		  if ( (iResult = AVCOM_INT_ProcessHeader(&header, (unsigned char *)RecvBuf)) != AOV_NO_ERROR)
			break;

		  if ( (*analyzers) == NULL )
			*analyzers = (struct aov_product *)malloc( sizeof(struct aov_product) );
		  else
		    *analyzers = (struct aov_product *)realloc(*analyzers, *numAnalyzers * sizeof(struct aov_product));

//		  *analyzers = (struct aov_hwdata *)lanal;
		  
//		  lanal = ( struct aov_hwdata *)&analyzers[*numAnalyzers-1];

		  UNPACK_STRCPY(analyzers[0][*numAnalyzers-1].model_name, RecvBuf, j, MODEL_NAME_SIZE);
		  UNPACK_STRCPY(analyzers[0][*numAnalyzers-1].analyzer_name, RecvBuf, j, MODEL_NAME_SIZE);
		  UNPACK2(analyzers[0][*numAnalyzers-1].prod_id, RecvBuf, j);
		  UNPACK_STRCPY(analyzers[0][*numAnalyzers-1].serial, RecvBuf, j, SERIAL_SIZE);
		  UNPACK_MEMCPY(analyzers[0][*numAnalyzers-1].network.mac, RecvBuf, j, 6);
		  UNPACK_STRCPY(analyzers[0][*numAnalyzers-1].network.hostname, RecvBuf, j, HOSTNAME_SIZE);
		  UNPACK1(analyzers[0][*numAnalyzers-1].network.dynip, RecvBuf, j);
		  UNPACK_MEMCPY(analyzers[0][*numAnalyzers-1].network.ip, RecvBuf, j, sizeof(unsigned long));
		  UNPACK_MEMCPY(analyzers[0][*numAnalyzers-1].network.sub, RecvBuf, j, sizeof(unsigned long));
		  UNPACK_MEMCPY(analyzers[0][*numAnalyzers-1].network.gw, RecvBuf, j, sizeof(unsigned long));
		  UNPACK2(analyzers[0][*numAnalyzers-1].network.tcp_port, RecvBuf, j);
/*
		  strncpy((char *)analyzers[0][*numAnalyzers-1].model_name,&RecvBuf[k], MODEL_NAME_SIZE);
		  k += strlen(&RecvBuf[k])+1;

		  strncpy((char *)analyzers[0][*numAnalyzers-1].analyzer_name,&RecvBuf[k], ANALYZER_NAME_SIZE);
		  k += strlen(&RecvBuf[k])+1;

		  analyzers[0][*numAnalyzers-1].prod_id  = (unsigned char)RecvBuf[k++] << 8;
		  analyzers[0][*numAnalyzers-1].prod_id |= (unsigned char)RecvBuf[k++];

		  strncpy((char *)analyzers[0][*numAnalyzers-1].serial,&RecvBuf[k], SERIAL_SIZE);
		  k += strlen(&RecvBuf[k])+1;

  		  memcpy(&analyzers[0][*numAnalyzers-1].network.mac,(char *)&RecvBuf[k],6);
		  k += 6;

		  strncpy((char *)analyzers[0][*numAnalyzers-1].network.hostname,&RecvBuf[k], HOSTNAME_SIZE);
		  k += strlen(&RecvBuf[k])+1;
		  
		  analyzers[0][*numAnalyzers-1].network.dynip = (unsigned char)RecvBuf[k++];
				
  		  memcpy(&analyzers[0][*numAnalyzers-1].network.ip,(char *)&RecvBuf[k],4);
		  analyzers[0][*numAnalyzers-1].network.ip = htonl(analyzers[0][*numAnalyzers-1].network.ip);
		  k += 4;
				
  		  memcpy(&analyzers[0][*numAnalyzers-1].network.sub,(char *)&RecvBuf[k],4);
		  analyzers[0][*numAnalyzers-1].network.sub = htonl(analyzers[0][*numAnalyzers-1].network.sub);
		  k += 4;
				
  		  memcpy(&analyzers[0][*numAnalyzers-1].network.gw,(char *)&RecvBuf[k],4);
		  analyzers[0][*numAnalyzers-1].network.gw = htonl(analyzers[0][*numAnalyzers-1].network.gw);
		  k += 4;

		  analyzers[0][*numAnalyzers-1].network.tcp_port  = (unsigned char)RecvBuf[k++] << 8;
		  analyzers[0][*numAnalyzers-1].network.tcp_port |= (unsigned char)RecvBuf[k++];
		  */

	  }
  }

  
  //---------------------------------------------
  // When the application is finished sending, close the socket.
#ifdef AOV_DEBUG
  printf("Finished sending. Closing socket.\n");
#endif
#ifdef _WIN32
	y = WSAGetLastError();
		closesocket(SendSocket);
		closesocket(RecvSocket);
//		WSACleanup();
//		exit(0);
#else
	close(SendSocket);
	for (i=0; i< 0x0FFFFFF; i++);
	close(RecvSocket);
	for (i=0; i< 0x0FFFFFF; i++);
#endif

  //---------------------------------------------
  // Clean up and quit.
#ifdef AOV_DEBUG
  printf("Exiting.\n");
#endif
	
	return iResult;
}





/********************************************************************************
 *																				*
 *				AVCOM_SA_Echo													*
 *																				*
 *******************************************************************************/
/*int AVCOM_SA_Echo ( void *handle )//, unsigned char data, int data_len )
{
	unsigned char sa_tx[1024];
	unsigned char sa_rx[1024];
	int j = 0;
	int k = 0;
	int i = 0;
	int recvlen;
	int iResult;
	unsigned short cmd = API_ECHO;
	dataConvert cvt;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	PACK_API_CMD(sa_tx, cmd, k);
	
	if (data_len>64) return (AOV_ERR_INVALID_PARAM);

	for (i=0;i<data_len;i++) {
		PACK1(sa_tx, data, k);
	}

	if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
		return iResult;
	if ( (iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR )
		return iResult;	//ACK
	if ( (iResult = AVCOM_INT_ReturnACK(cmd, sa_rx, &j)) != AOV_NO_ERROR )
		return iResult;

	return AOV_NO_ERROR;
}*/
#endif




// A MS Function
/********************************************************************************
*																				*
*		AVCOM_SA_FlashSBS2														*
*																				*
********************************************************************************/
int AVCOM_SA_FlashSBS2 ( void *handle, unsigned char* data, int data_len )
{
	int iResult;

	iResult = AVCOM_INT_FlashSBS2Shared ( handle, API_FLASH_MAIN_START, data, data_len );
	return iResult;
}

#if 0

// A MS Function
/********************************************************************************
 *																				*
 *				AVCOM_SA_GetActiveBand											*
 *																				*
 *******************************************************************************/
int AVCOM_SA_GetActiveBand (void *handle,  int *band )
{
	unsigned char sa_tx[64];
	unsigned char sa_rx[1024];
	int recvlen;
//	int i = 0;
	int j = 0;
	int k = 0;
	int iResult;
	unsigned short cmd = API_GET_ACTIVE_BAND;
	unsigned char *data;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	data = (unsigned char *)AVCOM_API_EncodeGetActiveBand( &k );

	memcpy(sa_tx, data, k);
	free(data);
	if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
		return iResult;
	if ( (iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR )
		return iResult;	
	// Check ACK
	if ( (iResult = AVCOM_INT_ReturnACK(cmd, sa_rx, &j)) != AOV_NO_ERROR )
		return iResult;

	// If we get here, assume no error as default.
	iResult = AOV_NO_ERROR;

	AVCOM_API_DecodeGetActiveBand ( &sa_rx[j], (recvlen-j), band );


	return iResult;
}



/********************************************************************************
 *																				*
 *				AVCOM_SA_GetActiveRBW											*
 *																				*
 *******************************************************************************/
int AVCOM_SA_GetActiveRBW (void *handle,  int *rbw )
{
	unsigned char sa_tx[64];
	unsigned char sa_rx[1024];
	int recvlen;
//	int i = 0;
	int j = 0;
	int k = 0;
	int iResult;
	unsigned short cmd = API_GET_ACTIVE_RBW;
	unsigned char *data;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	data = (unsigned char *)AVCOM_API_EncodeGetActiveRBW( &k );

	memcpy(sa_tx, data, k);
	free(data);
	if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
		return iResult;
	if ( (iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR )
		return iResult;	
	// Check ACK
	if ( (iResult = AVCOM_INT_ReturnACK(cmd, sa_rx, &j)) != AOV_NO_ERROR )
		return iResult;

	// If we get here, assume no error as default.
	iResult = AOV_NO_ERROR;

	//Process return data
	AVCOM_API_DecodeGetActiveRBW ( &sa_rx[j], (recvlen-j), rbw );

	return iResult;
}



#endif

// A MS Function
/********************************************************************************
 *																				*
 *				AVCOM_SA_GetActiveRefLevel										*
 *																				*
 *******************************************************************************/
int AVCOM_SA_GetActiveRefLevel (void *handle,  int *reflvl )
{
	unsigned char sa_tx[64];
	unsigned char sa_rx[1024];
	int recvlen;
//	int i = 0;
	int j = 0;
	int k = 0;
	int iResult;
	unsigned short cmd = API_GET_REFLVL;
	unsigned char *data;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	data = (unsigned char *)AVCOM_API_EncodeGetActiveRefLevel( &k );
	memcpy(sa_tx, data, k);
	free(data);

	if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
		return iResult;
	if ( (iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR )
		return iResult;	
	// Check ACK
	if ( (iResult = AVCOM_INT_ReturnACK(cmd, sa_rx, &j)) != AOV_NO_ERROR )
		return iResult;

	// If we get here, assume no error as default.
	iResult = AOV_NO_ERROR;

	AVCOM_API_DecodeGetActiveRefLevel ( &sa_rx[j], (recvlen-j), reflvl );

	return iResult;
}


#if 0
// A MS Function
/********************************************************************************
 *																				*
 *				AVCOM_SA_GetActiveVBW											*
 *																				*
 *******************************************************************************/
int AVCOM_SA_GetActiveVBW (void *handle,  int *vbw )
{
	unsigned char sa_tx[64];
	unsigned char sa_rx[1024];
	int recvlen;
//	int i = 0;
	int j = 0;
	int k = 0;
	int iResult;
	unsigned short cmd = API_GET_ACTIVE_VBW;
	unsigned char *data;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	data = (unsigned char *)AVCOM_API_EncodeGetActiveVBW( &k );

	memcpy(sa_tx, data, k);
	free(data);
	if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
		return iResult;
	if ( (iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR )
		return iResult;	
	// Check ACK
	if ( (iResult = AVCOM_INT_ReturnACK(cmd, sa_rx, &j)) != AOV_NO_ERROR )
		return iResult;

	// If we get here, assume no error as default.
	iResult = AOV_NO_ERROR;

	//Process return data
	AVCOM_API_DecodeGetActiveVBW ( &sa_rx[j], (recvlen-j), vbw );

	return iResult;
}


// A MS Function
/********************************************************************************
*																				*
*		AVCOM_SA_GetBandInfo													*
*																				*
********************************************************************************/
int AVCOM_SA_GetBandInfo ( void *handle, int band, struct aov_band_info *info )
{
	unsigned char sa_tx[1024];
	unsigned char sa_rx[1024];
//	int i = 0;
	int j = 0;
	int k = 0;
	unsigned short cmd = API_GET_BAND_INFO;
	int recvlen;
	int iResult;
//	dataConvert	cvt;
	unsigned char *data;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	data = (unsigned char *)AVCOM_API_EncodeGetBandInfo( &k, band );
	memcpy(sa_tx, data, k);
	free(data);

	if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
		return iResult;
	if ( (iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR )
		return iResult;	
	// Check ACK
	if ( (iResult = AVCOM_INT_ReturnACK(cmd, sa_rx, &j)) != AOV_NO_ERROR )
		return iResult;

	iResult = AOV_NO_ERROR;

	//Process return data
	AVCOM_API_DecodeGetBandInfo ( &sa_rx[j], (recvlen-j), info );


	return AOV_NO_ERROR;
}


// A MS Function
/********************************************************************************
*																				*
*		AVCOM_SA_GetBandInfoArrays												*
*																				*
********************************************************************************/
int AVCOM_SA_GetBandInfoArrays(void *handle, int band, int *phy_input, double *min_freq, double *max_freq, 
								/* int *max_step, double *min_db,*/ double *max_db, double *min_reflvl, double *max_reflvl, char *name)
{
//	int i;
	int iResult;
	struct aov_band_info info;

	iResult = AVCOM_SA_GetBandInfo( handle, band, &info );

	if (iResult == AOV_NO_ERROR)
	{
		*phy_input = info.phy_input;
		*min_freq = info.min_freq/1000.F;
		*max_freq = info.max_freq/1000.F;
//		*max_step = info.min_step;
//		*min_db = info.min_db;
		*max_db = info.max_db;
		*min_reflvl = info.min_reflvl;
		*max_reflvl = info.max_reflvl;
		
		strncpy(name, info.name, AOV_BAND_NAME_LEN);
	}

	return iResult;
}



// A MS Function
/********************************************************************************
 *																				*
 *				AVCOM_SA_GetBoardTemp()											*
 *																				*
 *******************************************************************************/
int AVCOM_SA_GetBoardTemp ( void *handle, double *value )
{
	unsigned char sa_tx[1024];
	unsigned char sa_rx[1024];
	int k = 0;
	int j = 0;
	int recvlen;
	int iResult;
	unsigned short cmd = API_ENG_GET_TEMP;
//	dataConvert cvt;
	unsigned char *data;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	data = (unsigned char *)AVCOM_API_EncodeGetBoardTemp( &k );
	memcpy(sa_tx, data, k);
	free(data);

	if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
		return iResult;
	if ( (iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR )
		return iResult;	//ACK

	iResult = AVCOM_INT_ReturnACK(cmd, sa_rx, &j);
	
	//Process return data
	AVCOM_API_DecodeGetBoardTemp ( &sa_rx[j], (recvlen-j), value );

	return iResult;
}





// A MS Function
/********************************************************************************
 *																				*
 *				AVCOM_SA_GetBuildDateTime()										*
 *																				*
 *******************************************************************************/
int AVCOM_SA_GetBuildDateTime ( void *handle, char *date, char *time )
{
	unsigned char sa_tx[1024];
	unsigned char sa_rx[1024];
	int k = 0;
	int j = 0;
	int recvlen;
	int iResult;
	unsigned short cmd = API_BUILD_DATETIME;
	unsigned char *data;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	data = (unsigned char *)AVCOM_API_EncodeGetBuildDateTime( &k );
	memcpy(sa_tx, data, k);
	free(data);

	if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
		return iResult;
	if ( (iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR )
		return iResult;	//ACK
	if ( (iResult = AVCOM_INT_ReturnACK(cmd, sa_rx, &j)) != AOV_NO_ERROR )
		return iResult;

	AVCOM_API_DecodeGetBuildDateTime ( &sa_rx[j], (recvlen-j), date, time );

	return iResult;
}

#endif

// A MS Function
/********************************************************************************
 *																				*
 *				AVCOM_SA_GetFirmwareVersion()									*
 *																				*
 *******************************************************************************/
int AVCOM_SA_GetFirmwareVersion ( void *handle, struct aov_version *ver )
{
    return AVCOM_INT_GetVersion (handle, API_BASE_VERSION, ver);
}


// A MS Function
/********************************************************************************
 *																				*
 *				AVCOM_SA_GetFirmwareVersionString()								*
 *																				*
 *******************************************************************************/
int AVCOM_SA_GetFirmwareVersionString ( void *handle, char *version )
{
	struct aov_version ver;
	int iResult;

	iResult = AVCOM_INT_GetVersion (handle, API_BASE_VERSION, &ver);

	if (iResult == AOV_NO_ERROR)
	{
		strncpy(version, ver.string, 16);
	}

	return iResult;
}

#if 0

/********************************************************************************
*																				*
*		AVCOM_SA_GetFreqValue													*
*																				*
********************************************************************************/
EXPORT int AVCOM_SA_GetFreqValue ( void *handle, double freq, double *db )
{
	unsigned char sa_tx[1024];
	unsigned char sa_rx[1024];
	int k = 0;
	int j = 0;
	int recvlen;
	int iResult;
	unsigned short cmd = API_ENG_GET_FREQ;
//	dataConvert cvt;
	unsigned char *data;

        UNUSED_PARAM(freq);
	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	data = (unsigned char *)AVCOM_API_EncodeGetFreqValue( &k );
	memcpy(sa_tx, data, k);
	free(data);

	if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
		return iResult;
	if ( (iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR )
		return iResult;	//ACK

	iResult = AVCOM_INT_ReturnACK(cmd, sa_rx, &j);
	
	//Process return data
	AVCOM_API_DecodeGetFreqValue ( &sa_rx[j], (recvlen-j), db );

	return iResult;
}


// A MS Function
/********************************************************************************
*																				*
*		AVCOM_SA_GetHandle														*
*																				*
********************************************************************************/
void *AVCOM_SA_GetHandle(char *ip, int port, int usbid, int type)
{
	int i;
	struct addrinfo hints;
	struct addrinfo *servinfo;  // will point to the results
	char portbuf[10];
	int iResult;

	memset(&hints, 0, sizeof hints); // make sure the struct is empty
	hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
	hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

	snprintf(portbuf, sizeof(portbuf), "%i",port);
	if ((iResult = getaddrinfo(ip, portbuf, &hints, &servinfo)) != 0) {
		return (void *)-1;
	}

	for ( i = 0; i < plen; i++)
	{
		if ( ( plist[i]->socket_type == AOV_TCP_DEFAULT ) && ( type == AOV_TCP_DEFAULT ) )
		{
			if (!(memcmp(plist[i]->servinfo->ai_addr->sa_data,servinfo->ai_addr->sa_data,sizeof(plist[i]->servinfo->ai_addr->sa_data))))
			{
				if (port == plist[i]->portno)
					return (void *)plist[i];
			}
#ifdef AOV_DEPEND_USB
		} else if ( ( plist[i]->socket_type == AOV_USB_DEFAULT ) && ( type == AOV_USB_DEFAULT ) ) {
			if (plist[i]->usbId == usbid) {
					return (void *)plist[i];
			}
#endif /* AOV_DEPEND_USB */
		}
	}

	return (void *)-1;
}





// A MS Function
/********************************************************************************
*																				*
*		AVCOM_SA_GetNetwork														*
*																				*
********************************************************************************/
int AVCOM_SA_GetNetwork (void *handle, struct aov_network *net )
{
	unsigned char sa_tx[1024];
	unsigned char sa_rx[1024];
	int k = 0;
	int j = 0;
	int recvlen;
	unsigned short cmd = API_GET_NETWORK;
//	dataConvert cvt;
	int iResult;
	unsigned char *data;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	data = (unsigned char *)AVCOM_API_EncodeGetNetwork( &k );
	memcpy(sa_tx, data, k);
	free(data);

	if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
		return iResult;
	if ( (iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR )
		return iResult;	//ACK

	if ( (iResult = AVCOM_INT_ReturnACK(cmd, sa_rx, &j)) != AOV_NO_ERROR )
		return iResult;

	AVCOM_API_DecodeGetNetwork ( &sa_rx[j], (recvlen-j), net );

	return iResult;
}


// A MS Function but not fully implemented
#ifdef AOV_DEPEND_USB
int AVCOM_SA_GetNumUSB ( unsigned int *numDevs )
{
	FT_STATUS ftStatus;

	ftStatus = FT_CreateDeviceInfoList(numDevs);
	if (ftStatus != FT_OK) 
		return AOV_ERR_USB_COMM;
	
	return AOV_NO_ERROR;
}
#endif /* AOV_DEPEND_USB */


// A MS Function
/********************************************************************************
*																				*
*		AVCOM_SA_GetProdDesc													*
*																				*
********************************************************************************/
int AVCOM_SA_GetProdDesc(void *handle, struct aov_product *ppd ) 
{
	unsigned char sa_tx[64];
	unsigned char sa_rx[1024];
	int recvlen;
//	int i = 0;
	int j = 0;
	int k = 0;
	unsigned short cmd = API_GET_PROD_DESC;
	int iResult;
//	dataConvert	cvt;
	unsigned char *data;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	data = (unsigned char *)AVCOM_API_EncodeGetProdDesc( &k );
	memcpy(sa_tx, data, k);
	free(data);

	if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
		return iResult;
	if ( (iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR )
		return iResult;	
	//ACK
	if ( (iResult = AVCOM_INT_ReturnACK(cmd, sa_rx, &j)) != AOV_NO_ERROR )
		return iResult;

	AVCOM_API_DecodeGetProdDesc ( &sa_rx[j], (recvlen-j), ppd );

	return iResult;
}


// A MS Function
/********************************************************************************
*																				*
*		AVCOM_SA_GetProdDescArrays												*
*																				*
********************************************************************************/
int AVCOM_SA_GetProdDescArrays(void *handle, int *prod_id, int *numBands, int *numRBW, 
								int *numVBW, int *max_pts, char *serial, char *model_name,
								char *analyzer_name ) 
{
//	int i;
	int iResult;
	struct aov_product ppd;

	iResult = AVCOM_SA_GetProdDesc( handle, &ppd );

	if (iResult == AOV_NO_ERROR)
	{
		*prod_id = ppd.prod_id;
		*numBands = ppd.numBands;
		*numRBW = ppd.numRBW;
		*numVBW = ppd.numVBW;
		*max_pts = ppd.max_pts;
		*prod_id = ppd.prod_id;
		*prod_id = ppd.prod_id;

		strncpy(serial, ppd.serial, SERIAL_SIZE);
		strncpy(model_name, ppd.model_name, MODEL_NAME_SIZE);
		strncpy(analyzer_name, ppd.analyzer_name, ANALYZER_NAME_SIZE);
	}

	return iResult;
}





// A MS Function
/********************************************************************************
*																				*
*		AVCOM_SA_GetRBWList														*
*																				*
********************************************************************************/
//int AVCOM_ENG_GetRbwList(void *handle, sAOV_RBW_LIST *list)
int AVCOM_SA_GetRBWList (void *handle, int *RBWs, int list_size )
{
	unsigned char sa_tx[64];
	unsigned char sa_rx[1024];
	int recvlen;
	int i = 0;
	int j = 0;
	int k = 0;
	unsigned short cmd = API_GET_RBW_LIST;
	int iResult;
//	dataConvert	cvt;
	unsigned char *data;
	int count;
	int *lists;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	data = (unsigned char *)AVCOM_API_EncodeGetRBWList( &k );
	memcpy(sa_tx, data, k);
	free(data);

	if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
		return iResult;
	if ( (iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR )
		return iResult;	
	//ACK
	if ( (iResult = AVCOM_INT_ReturnACK(cmd, sa_rx, &j)) != AOV_NO_ERROR )
		return iResult;

	iResult = AVCOM_API_DecodeGetRBWList ( &sa_rx[j], (recvlen-j), &lists, &count );

	//Process return data
	if (count > list_size)
	{
		iResult = AOV_WARN_LIST_SIZE;
		count = list_size;
	}

	for ( i = 0; i < count; i++ )
	{
		RBWs[i] = lists[i];
	}

	free(lists);
	return iResult;
}


// A MS Function but not implemented
/********************************************************************************
*																				*
*		AVCOM_SA_GetSpectrumData												*
*																				*
********************************************************************************/
int AVCOM_SA_GetSpectrumData(void *handle, sAOV_SweepData *trace)
{
	return AVCOM_INT_GetSpectrumData(handle, API_TRG_SWEEP, trace);
}


// A MS Function but not implemented
/********************************************************************************
*																				*
*		AVCOM_SA_GetSpectrumDataArrays											*
*																				*
********************************************************************************/
int AVCOM_SA_GetSpectrumDataArrays(void *handle, double* start, double* stop, double* step, int *band, int *rbw, int *vbw, int *reflvl, int *avg, unsigned short* num, double* freq,  double* power)
{
	int i;
	int iResult;
	sAOV_SweepData sweepdata;

	iResult = AVCOM_INT_GetSpectrumData(handle, API_GET_SWEEP_ARRAY, &sweepdata);

	if (iResult == AOV_NO_ERROR)
	{
		*num = sweepdata.count;
		*start = sweepdata.fStart;
		*step = sweepdata.fStep;
		*stop = sweepdata.fStop;
		
		*band = sweepdata.band;
		*rbw = sweepdata.rbw;
		*vbw = sweepdata.vbw;
		*reflvl = sweepdata.reflvl;

		*avg = sweepdata.avg;

		for (i=0; i<sweepdata.count; i++)
		{
			power[i] = sweepdata.sweep[i].power;
			freq[i] = sweepdata.sweep[i].frequency;
		}
	}

	return iResult;
}


// A MS Function
/********************************************************************************
*																				*
*		AVCOM_SA_GetTriggerSweep												*
*																				*
********************************************************************************/
int AVCOM_SA_GetTriggerSweep(void *handle, sAOV_SweepData *trace)
{
	return AVCOM_INT_GetSpectrumData(handle, API_TRG_GET_SWEEP, trace);
}


// A MS Function but not implemented
/********************************************************************************
*																				*
*		AVCOM_SA_GetTriggerSweepArrays											*
*																				*
********************************************************************************/
int AVCOM_SA_GetTriggerSweepArrays(void *handle, double* start, double* stop, double* step, int *band, int *rbw, int *vbw, int *reflvl, int *avg, unsigned short* num, double* freq, double* power)
{
	int i;
	int iResult;
	sAOV_SweepData sweepdata;

	iResult = AVCOM_INT_GetSpectrumData(handle, API_GET_TRG_SWEEP_ARRAY, &sweepdata);

	if (iResult == AOV_NO_ERROR)
	{
		*num = sweepdata.count;
		*start = sweepdata.fStart;
		*step = sweepdata.fStep;
		*stop = sweepdata.fStop;
	
		*band = sweepdata.band;
		*rbw = sweepdata.rbw;
		*vbw = sweepdata.vbw;
		*reflvl = sweepdata.reflvl;

		*avg = sweepdata.avg;

		for (i=0; i<sweepdata.count; i++)
		{
			power[i] = sweepdata.sweep[i].power;
			freq[i] = sweepdata.sweep[i].frequency;
		}
	}

	return iResult;
}


// A MS Function
/********************************************************************************
*																				*
*		AVCOM_SA_GetTriggerZeroSpan												*
*																				*
********************************************************************************/
int AVCOM_SA_GetTriggerZeroSpan(void *handle, sAOV_ZeroSpanData *trace)
{
	return AVCOM_INT_GetZeroSpan(handle, API_TRIGGET_ZERO_SPAN, trace);
}


// A MS Function but not implemented
/********************************************************************************
*																				*
*		AVCOM_SA_GetTriggerZeroSpanArrays										*
*																				*
********************************************************************************/
int AVCOM_SA_GetTriggerZeroSpanArrays(void *handle, double* freq, int *sample_rate,
										  int *band, int *rbw, int *vbw, int *reflvl, int *mode,
										  unsigned short* num, double* time, double* power)
{
	int i;
	int iResult;
	sAOV_ZeroSpanData trace;

	iResult = AVCOM_INT_GetZeroSpan(handle, API_TRIGGET_ZERO_SPAN, &trace);

	if (iResult == AOV_NO_ERROR)
	{
		*num = trace.count;
		*freq = trace.freq;
	
		*band = trace.band;
		*rbw = trace.rbw;
		*vbw = trace.vbw;
		*reflvl = trace.reflvl;
		
		*vbw = trace.mode;
		*sample_rate = trace.sample_rate;


		for (i=0; i<trace.count; i++)
		{
			power[i] = trace.trace[i].power;
			time[i] = trace.trace[i].time;
		}
	}

	return iResult;
}


// A MS Function
/********************************************************************************
*																				*
*		AVCOM_SA_GetVBWList														*
*																				*
********************************************************************************/
int AVCOM_SA_GetVBWList (void *handle, int *VBWs, int list_size )
{
	unsigned char sa_tx[64];
	unsigned char sa_rx[1024];
	int recvlen;
	int i = 0;
	int j = 0;
	int k = 0;
	unsigned short cmd = API_GET_VBW_LIST;
	int iResult;
//	dataConvert	cvt;
	unsigned char *data;
	int count;
	int *lists;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	data = (unsigned char *)AVCOM_API_EncodeGetVBWList( &k );
	memcpy(sa_tx, data, k);
	free(data);

	if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
		return iResult;
	if ( (iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR )
		return iResult;	
	//ACK
	if ( (iResult = AVCOM_INT_ReturnACK(cmd, sa_rx, &j)) != AOV_NO_ERROR )
		return iResult;

	iResult = AVCOM_API_DecodeGetVBWList ( &sa_rx[j], (recvlen-j), &lists, &count );

	//Process return data
	if (count > list_size)
	{
		iResult = AOV_WARN_LIST_SIZE;
		count = list_size;
	}

	for ( i = 0; i < count; i++ )
	{
		VBWs[i] = lists[i];
	}

	free(lists);
	return iResult;
}










/********************************************************************************
 *																				*
 *				AVCOM_SA_Ping													*
 *																				*
 *******************************************************************************/
int AVCOM_SA_Ping ( void *handle )//, unsigned char data, int data_len )
{
	unsigned char sa_tx[1024];
	unsigned char sa_rx[1024];
	int j = 0;
	int k = 0;
//	int i = 0;
	int recvlen;
	int iResult;
	unsigned short cmd = API_PING;
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
		return iResult;	//ACK
	if ( (iResult = AVCOM_INT_ReturnACK(cmd, sa_rx, &j)) != AOV_NO_ERROR )
		return iResult;

	return AOV_NO_ERROR;
}                    
#endif


// A MS Function
/********************************************************************************
 *																				*
 *				AVCOM_SA_Reboot()												*
 *																				*
 *******************************************************************************/
int AVCOM_SA_Reboot ( void *handle )
{
	unsigned char sa_tx[1024];
	unsigned char sa_rx[1024];
	int j = 0;
	int k = 0;
	int recvlen;
	int iResult;
	unsigned short cmd = API_REBOOT;
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
		return iResult;	//ACK

	iResult = AVCOM_INT_ReturnACK(cmd, sa_rx, &j);

	return iResult;
}






#if 0
// A MS Function
/********************************************************************************
 *																				*
 *				AVCOM_SA_Register()												*
 *																				*
 *******************************************************************************/
int AVCOM_SA_Register(void **handle, char *ip, int port, int usb, int mode, int flags)
{
//	unsigned char sa_tx[64];
//	unsigned char sa_rx[1024];
	int i;//,k;
//	int txlen;
//	int recvlen;
	int iResult;
	int *socketfd;
	AOVhandle *lhandle;
	struct addrinfo hints;
//	sAOV_PROD_DESC	pd;

	char portbuf[10];
#ifdef AOV_DEPEND_USB
	FT_STATUS ftStatus;
#endif /* AOV_DEPEND_USB */
	
	if (plist == NULL)
		return AOV_ERR_API_NOT_INITIALIZED;

	if (( lhandle = (void *)malloc( sizeof(AOVhandle) ) ) == NULL) {
		return AOV_ERR_HANDLE_CREATE;
	}

	socketfd = (&lhandle->socketfd);
	*socketfd = -1; // say it's not established
	
	plen++;
	plist = realloc( plist, (plen) * sizeof(AOVhandle *) );
	plist[plen-1] = lhandle;

	*handle = (void *)lhandle;	

	lhandle->version_major = AOVAPI_MAJOR;
	lhandle->version_minor = AOVAPI_MINOR;
	lhandle->version_patch = AOVAPI_BUILD;

	lhandle->socket_type = flags;

	if ( flags == AOV_TCP_DEFAULT )
	{
		lhandle->ip = ip;
		lhandle->portno = port;

		memset(&hints, 0, sizeof hints); // make sure the struct is empty
		hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
		hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
		hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

		snprintf(portbuf, sizeof(portbuf), "%d", lhandle->portno);
		if ((iResult = getaddrinfo(lhandle->ip, portbuf, &hints, &lhandle->servinfo)) != 0) {
			AVCOM_SA_Unregister(lhandle);
			return AOV_ERR_GET_ADDR_INFO_FAIL;
		}

		for ( i = 0; i < (plen-1); i++)
		{
			if ( plist[i]->socket_type == AOV_TCP_DEFAULT )
			{
				if (!(memcmp(plist[i]->servinfo->ai_addr->sa_data,lhandle->servinfo->ai_addr->sa_data,sizeof(plist[i]->servinfo->ai_addr->sa_data))))
				{
					if (port == plist[i]->portno)
						AVCOM_SA_Unregister(lhandle);
						return AOV_ERR_HANDLE_EXISTS;
				}
			} 
		}
#ifdef AOV_DEPEND_USB
	} else if ( flags == AOV_USB_DEFAULT ) {

		if ( (iResult = AVCOM_INT_ValidateUSBLocation(usb)) != AOV_NO_ERROR)
			return iResult;

		lhandle->usbId = usb;

//		locIdBuf = (long *)malloc(numDevs*sizeof(long));
		ftStatus = FT_ListDevices((void *)(long)lhandle->usbId,&lhandle->usblocId,FT_LIST_BY_INDEX|FT_OPEN_BY_LOCATION); 
		if (ftStatus == FT_OK) 
		{ 
			// FT_ListDevices OK, location IDs are in locIdBuf, and 
			// numDevs contains the number of devices connected 
		} else { 
			// FT_ListDevices failed 
		}

		for ( i = 0; i < (plen-1); i++)
		{
			if ( plist[i]->socket_type == AOV_USB_DEFAULT )
			{
				if ( plist[i]->usblocId == lhandle->usblocId )
				{
					AVCOM_SA_Unregister(lhandle);
					return AOV_ERR_HANDLE_EXISTS;
				}
			} 
		}
#endif /* AOV_DEPEND_USB */
	} else {
		return AOV_ERR_INVALID_PARAM;
	}

//	if ((iResult = AVCOM_SA_Connect(lhandle)) != AOV_NO_ERROR)
//	{
//		AVCOM_SA_Unregister(lhandle);
//		return iResult;
//	}
	
	//Attempt to get the product description information
//	iResult = AVCOM_SA_GetProdDesc(lhandle, &(lhandle->pd));

	//TODO - Remove this "if block" later in development
//	if (iResult == AOV_ERR_UNKNOWN_CMD)
//	{
//		if ((iResult = AVCOM_SA_Disconnect(lhandle)) != AOV_NO_ERROR)
//			return iResult;
//	}
//
//	if (iResult != AOV_NO_ERROR)
//	{
//
//		AVCOM_SA_Unregister(lhandle);
//		return iResult;
//	}



//	if ((iResult = AVCOM_SA_Disconnect(lhandle)) != AOV_NO_ERROR)
//		return iResult;
//
	return (AOV_NO_ERROR);


}
#endif


// A MS Function
/********************************************************************************
*																				*
*		AVCOM_SA_SaveBandSettings												*
*																				*
********************************************************************************/
int AVCOM_SA_SaveBandSettings ( void *handle )
{
	unsigned char sa_tx[64];
	unsigned char sa_rx[1024];
	int recvlen;
//	int i = 0;
	int j = 0;
	int k = 0;
	int iResult;
	unsigned short cmd = API_SAVE_BANDSETTINGS;
//	dataConvert cvt;
	unsigned char *data;


	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	data = (unsigned char *)AVCOM_API_EncodeSaveBandSettings( &k );
	memcpy(sa_tx, data, k);
	free(data);

	if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
		return iResult;
	if ( (iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR )
		return iResult;	//ACK
	if ( (iResult = AVCOM_INT_ReturnACK(cmd, sa_rx, &j)) != AOV_NO_ERROR )
		return iResult;

	return AOV_NO_ERROR;
}


/********************************************************************************
*																				*
*		AVCOM_SA_SetAnalyzerName												*
*																				*
********************************************************************************/
EXPORT int AVCOM_SA_SetAnalyzerName ( void *handle, char* name)
{
	unsigned char sa_tx[1024];
	unsigned char sa_rx[1024];
//	int i = 0;
	int j = 0;
	int k = 0;
	unsigned short cmd = API_SET_ANALYZER_NAME;
	int recvlen;
	int iResult;
	unsigned char *data;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	data = (unsigned char *)AVCOM_API_EncodeSetAnalyzerName( &k, name );

	memcpy(sa_tx, data, k);
	free(data);
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
*		AVCOM_SA_SetBand														*
*																				*
********************************************************************************/
int AVCOM_SA_SetBand ( void *handle, int band )
{
	unsigned char sa_tx[64];
	unsigned char sa_rx[1024];
	int recvlen;
//	int i = 0;
	int j = 0;
	int k = 0;
	int iResult;
	unsigned short cmd = API_SET_BAND;
	unsigned char * data;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	data = (unsigned char *)AVCOM_API_EncodeSetBand( &k, band );

	memcpy(sa_tx, data, k);
	free(data);
	if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
		return iResult;
	if ( (iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR )
		return iResult;	
	// Check ACK
	if ( (iResult = AVCOM_INT_ReturnACK(cmd, sa_rx, &j)) != AOV_NO_ERROR )
		return iResult;

	// If we get here, assume no error as default.
	iResult = AOV_NO_ERROR;

	return iResult;
}


/********************************************************************************
*																				*
*		AVCOM_SA_SetBandName													*
*																				*
********************************************************************************/
EXPORT int AVCOM_SA_SetBandName ( void *handle, char* bandname)
{
	unsigned char sa_tx[1024];
	unsigned char sa_rx[1024];
//	int i = 0;
	int j = 0;
	int k = 0;
	unsigned short cmd = API_SET_BAND_NAME;
	int recvlen;
	int iResult;
	unsigned char *data;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	data = (unsigned char *)AVCOM_API_EncodeSetBandName( &k, bandname );

	memcpy(sa_tx, data, k);
	free(data);
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
 *				AVCOM_SA_SetFreqCSD()											*
 *																				*
 *******************************************************************************/
int AVCOM_SA_SetFreqCSD ( void *handle, double cfreq,  double span, int max_pts)
{
	unsigned char sa_tx[1024];
	unsigned char sa_rx[1024];
//	int i = 0;
	int j = 0;
	int k = 0;
	unsigned short cmd = API_SET_FREQ_CSD;
	int recvlen;
	int iResult;
	unsigned char *data;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	data = (unsigned char *)AVCOM_API_EncodeSetFreqCSD( &k, cfreq, span, max_pts );

	memcpy(sa_tx, data, k);
	free(data);
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
 *				AVCOM_SA_SetFreqCSS()											*
 *																				*
 *******************************************************************************/
int AVCOM_SA_SetFreqCSS ( void *handle, double cfreq,  double span, double step)
{
	unsigned char sa_tx[1024];
	unsigned char sa_rx[1024];
//	int i = 0;
	int j = 0;
	int k = 0;
	unsigned short cmd = API_SET_FREQ_CSS;
	int recvlen;
	int iResult;
	unsigned char *data;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	data = (unsigned char *)AVCOM_API_EncodeSetFreqCSS( &k, cfreq, span, step );

	memcpy(sa_tx, data, k);
	free(data);
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
 *				AVCOM_SA_SetFreqSSS()											*
 *																				*
 *******************************************************************************/
int AVCOM_SA_SetFreqSSS ( void *handle, double fstart, double fstop, double fstep)
{
	unsigned char sa_tx[1024];
	unsigned char sa_rx[1024];
//	int i = 0;
	int j = 0;
	int k = 0;
	unsigned short cmd = API_SET_FREQ_SSS;
	int recvlen;
	int iResult;
	unsigned char *data;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	data = (unsigned char *)AVCOM_API_EncodeSetFreqSSS( &k, fstart, fstop, fstep );

	memcpy(sa_tx, data, k);
	free(data);
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
*		AVCOM_SA_SetHostname													*
*																				*
********************************************************************************/
int AVCOM_SA_SetHostname ( void *handle, char *hostname )
{
  unsigned char sa_tx[128];
  unsigned char sa_rx[128];
	int j = 0;
	int k = 0;
	int recvlen;
	int iResult;
	unsigned cmd = API_CHANGE_HOSTNAME;
//	dataConvert cvt;
	unsigned char *data;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	data = (unsigned char *)AVCOM_API_EncodeSetHostname( &k, hostname );
	memcpy(sa_tx, data, k);
	free(data);

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
*		AVCOM_SA_SetNetwork														*
*																				*
********************************************************************************/
int AVCOM_SA_SetNetwork ( void *handle, int dynip, unsigned long ip, unsigned long sub, unsigned long gw )
{
  unsigned char sa_tx[128];
  unsigned char sa_rx[128];
	int j = 0;
	int k = 0;
	int recvlen;
	int iResult;
	unsigned cmd = API_CHANGE_NETWORK;
//	dataConvert	cvt;
	unsigned char *data;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	data = (unsigned char *)AVCOM_API_EncodeSetNetwork( &k, dynip, ip, sub, gw );
	memcpy(sa_tx, data, k);
	free(data);

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
*		AVCOM_SA_SetPort														*
*																				*
********************************************************************************/
int AVCOM_SA_SetPort ( void *handle, unsigned short port )
{
  unsigned char sa_tx[128];
  unsigned char sa_rx[128];
	int j = 0;
	int k = 0;
	int recvlen;
	int iResult;
	unsigned cmd = API_CHANGE_PORT;
//	dataConvert cvt;
	unsigned char *data;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	data = (unsigned char *)AVCOM_API_EncodeSetPort( &k, port );
	memcpy(sa_tx, data, k);
	free(data);

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
*		AVCOM_SA_SetRBW															*
*																				*
********************************************************************************/
int AVCOM_SA_SetRBW ( void *handle, int rbw )
{
	unsigned char sa_tx[64];
	unsigned char sa_rx[1024];
	int recvlen;
//	int i = 0;
	int j = 0;
	int k = 0;
	int iResult;
	unsigned short cmd = API_SET_RBW;
	unsigned char *data;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	data = (unsigned char *)AVCOM_API_EncodeSetRBW( &k, rbw );

	memcpy(sa_tx, data, k);
	free(data);
	if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
		return iResult;
	if ( (iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR )
		return iResult;	
	// Check ACK
	if ( (iResult = AVCOM_INT_ReturnACK(cmd, sa_rx, &j)) != AOV_NO_ERROR )
		return iResult;

	// If we get here, assume no error as default.
	iResult = AOV_NO_ERROR;
	

	return iResult;
}

// A MS Function
/********************************************************************************
*																				*
*		AVCOM_SA_SetRefLevel													*
*																				*
********************************************************************************/
int AVCOM_SA_SetRefLevel ( void *handle, int revlvl )
{
	unsigned char sa_tx[64];
	unsigned char sa_rx[1024];
	int recvlen;
	int j = 0;
	int k = 0;
	int iResult;
	unsigned short cmd = API_SET_REFLVL;
	unsigned char *data;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	data = (unsigned char *)AVCOM_API_EncodeSetRefLevel( &k, revlvl );

	memcpy(sa_tx, data, k);
	free(data);
	if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
		return iResult;
	if ( (iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR )
		return iResult;	
	// Check ACK
	if ( (iResult = AVCOM_INT_ReturnACK(cmd, sa_rx, &j)) != AOV_NO_ERROR )
		return iResult;

	// If we get here, assume no error as default.
	iResult = AOV_NO_ERROR;
	

	return iResult;
}

// A MS Function
/********************************************************************************
*																				*
*		AVCOM_SA_SetVBW															*
*																				*
********************************************************************************/
int AVCOM_SA_SetVBW ( void *handle, int vbw )
{
	unsigned char sa_tx[64];
	unsigned char sa_rx[1024];
	int recvlen;
//	int i = 0;
	int j = 0;
	int k = 0;
	int iResult;
	unsigned short cmd = API_SET_VBW;
	unsigned char* data;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	data = (unsigned char *)AVCOM_API_EncodeSetVBW( &k, vbw );

	memcpy(sa_tx, data, k);
	free(data);
	if ( (iResult = AVCOM_INT_SendPacket(handle, sa_tx, k)) != AOV_NO_ERROR )
		return iResult;
	if ( (iResult = AVCOM_INT_RecvPacket(handle, sa_rx, &recvlen)) != AOV_NO_ERROR )
		return iResult;	
	// Check ACK
	if ( (iResult = AVCOM_INT_ReturnACK(cmd, sa_rx, &j)) != AOV_NO_ERROR )
		return iResult;

	// If we get here, assume no error as default.
	iResult = AOV_NO_ERROR;

	return iResult;
}

// A MS Function
/********************************************************************************
 *																				*
 *				AVCOM_SA_SetZeroSpan()											*
 *																				*
 *******************************************************************************/
int AVCOM_SA_SetZeroSpan ( void *handle, double freq, int numPts, int sample_rate, int mode)
{
	unsigned char sa_tx[1024];
	unsigned char sa_rx[1024];
//	int i = 0;
	int j = 0;
	int k = 0;
	unsigned short cmd = API_SET_ZERO_SPAN;
	int recvlen;
	int iResult;
	unsigned char *data;

	if (AVCOM_INT_CheckHandle(handle))
		return AOV_ERR_HANDLE_NOT_FOUND;

	data = (unsigned char *)AVCOM_API_EncodeSetZeroSpan( &k, freq, numPts, sample_rate, mode );

	memcpy(sa_tx, data, k);
	free(data);
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
 *				AVCOM_SA_TriggerSweep()											*
 *																				*
 *******************************************************************************/
int AVCOM_SA_TriggerSweep( void *handle)
{
	unsigned char sa_tx[1024];
	unsigned char sa_rx[1024];
//	int i = 0;
	int j = 0;
	int k = 0;
	unsigned short cmd = API_TRG_SWEEP;
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
 *				AVCOM_SA_UDPSetHostname()										*
 *																				*
 *******************************************************************************/
int AVCOM_SA_UDPSetHostname ( unsigned char *mac, char *hostname )
{
  int SendSocket;
  struct sockaddr_in SendAddr;
  unsigned short portsend = 26482;
  unsigned char sa_tx[1024];
	unsigned char *data;
//  int BufLen = 1024;
  unsigned char *sendbuf;
  int iResult = 0;
  int sendlen;
  int k;
  int so_broadcast;
  int z;
#ifdef _WIN32
#endif
#ifdef _WIN32
  int y;
#else
  int i;
#endif
  

  //---------------------------------------------
  // Create a socket for sending data
  SendSocket = socket(AF_INET, SOCK_DGRAM, 0);

  //---------------------------------------------
  // Set up the RecvAddr structure with the IP address of
  // the receiver (in this example case "192.168.1.1")
  // and the specified port number.
  memset(&SendAddr, 0, sizeof(struct sockaddr_in));
  SendAddr.sin_family = AF_INET;
  SendAddr.sin_port = htons(portsend);
  SendAddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
  
  so_broadcast = 1;
  z = setsockopt((int)SendSocket,SOL_SOCKET,SO_BROADCAST, (char *)&so_broadcast, sizeof(so_broadcast));
#ifdef AOV_DEBUG
  printf("%i\n",errno);
#endif

  //---------------------------------------------
  // Send a datagram to the receiver
#ifdef AOV_DEBUG
  printf("Sending a datagram to the receiver...\n");
#endif
	  
	data = (unsigned char *)AVCOM_API_EncodeUDPSetHostname( &k, mac, hostname );
	memcpy(sa_tx, data, k);
	free(data);

  sendbuf = AVCOM_INT_AssemblePacket(sa_tx, k, &sendlen, 0);

  z=sendto((int)SendSocket, 
    sendbuf, 
    sendlen, 
    0, 
    (struct sockaddr *) &SendAddr, 
    sizeof(SendAddr));

  free(sendbuf);
//	y = WSAGetLastError();


  
  //---------------------------------------------
  // When the application is finished sending, close the socket.
#ifdef AOV_DEBUG
  printf("Finished sending. Closing socket.\n");
#endif
#ifdef _WIN32
	y = WSAGetLastError();
		closesocket(SendSocket);
//		WSACleanup();
//		exit(0);
#else
	close(SendSocket);
	for (i=0; i< 0x0FFFFFF; i++);
#endif

  //---------------------------------------------
  // Clean up and quit.
#ifdef AOV_DEBUG
  printf("Exiting.\n");
#endif
	
	return iResult;
}




// A MS Function
/********************************************************************************
 *																				*
 *				AVCOM_SA_UDPSetNetwork()										*
 *																				*
 *******************************************************************************/
int AVCOM_SA_UDPSetNetwork ( unsigned char *mac, int dynip, unsigned long ip, unsigned long sub, unsigned long gw )
{
  int SendSocket;
  struct sockaddr_in SendAddr;
  unsigned short portsend = 26482;
  unsigned char sa_tx[1024];
	unsigned char *data;
//  int BufLen = 1024;
  unsigned char *sendbuf;
  int iResult = 0;
  int sendlen;
  int k;
  int so_broadcast;
  int z;
//	dataConvert	cvt;

#ifdef _WIN32
#endif
#ifdef _WIN32
  int y;
#else
  int i;
#endif
  

  //---------------------------------------------
  // Create a socket for sending data
  SendSocket = socket(AF_INET, SOCK_DGRAM, 0);

  //---------------------------------------------
  // Set up the RecvAddr structure with the IP address of
  // the receiver (in this example case "192.168.1.1")
  // and the specified port number.
  memset(&SendAddr, 0, sizeof(struct sockaddr_in));
  SendAddr.sin_family = AF_INET;
  SendAddr.sin_port = htons(portsend);
  SendAddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
  
  so_broadcast = 1;
  z = setsockopt((int)SendSocket,SOL_SOCKET,SO_BROADCAST, (char *)&so_broadcast, sizeof(so_broadcast));
#ifdef AOV_DEBUG
  printf("%i\n",errno);
#endif

  //---------------------------------------------
  // Send a datagram to the receiver
#ifdef AOV_DEBUG
  printf("Sending a datagram to the receiver...\n");
#endif
	  
  k=0;

	data = (unsigned char *)AVCOM_API_EncodeUDPSetNetwork( &k, mac, dynip, ip, sub, gw );
	memcpy(sa_tx, data, k);
	free(data);

  sendbuf = AVCOM_INT_AssemblePacket(sa_tx, k, &sendlen, 0);

  z=sendto((int)SendSocket, 
    sendbuf, 
    sendlen, 
    0, 
    (struct sockaddr *) &SendAddr, 
    sizeof(SendAddr));

  free(sendbuf);
//	y = WSAGetLastError();


  
  //---------------------------------------------
  // When the application is finished sending, close the socket.
#ifdef AOV_DEBUG
  printf("Finished sending. Closing socket.\n");
#endif
#ifdef _WIN32
	y = WSAGetLastError();
		closesocket(SendSocket);
//		WSACleanup();
//		exit(0);
#else
	close(SendSocket);
	for (i=0; i< 0x0FFFFFF; i++);
#endif

  //---------------------------------------------
  // Clean up and quit.
#ifdef AOV_DEBUG
  printf("Exiting.\n");
#endif
	
	return iResult;
}






// A MS Function
/********************************************************************************
 *																				*
 *				AVCOM_SA_UDPSetPort()											*
 *																				*
 *******************************************************************************/
int AVCOM_SA_UDPSetPort ( unsigned char *mac, unsigned short port )
{
  int SendSocket;
  struct sockaddr_in SendAddr;
  unsigned short portsend = 26482;
  unsigned char sa_tx[1024];
	unsigned char *data;
//  int BufLen = 1024;
  unsigned char *sendbuf;
  int iResult = 0;
  int sendlen;
  int k;
  int so_broadcast;
  int z;
#ifdef _WIN32
#endif
#ifdef _WIN32
  int y;
#else
  int i;
#endif
  

  //---------------------------------------------
  // Create a socket for sending data
  SendSocket = socket(AF_INET, SOCK_DGRAM, 0);

  //---------------------------------------------
  // Set up the RecvAddr structure with the IP address of
  // the receiver (in this example case "192.168.1.1")
  // and the specified port number.
  memset(&SendAddr, 0, sizeof(struct sockaddr_in));
  SendAddr.sin_family = AF_INET;
  SendAddr.sin_port = htons(portsend);
  SendAddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
  
  so_broadcast = 1;
  z = setsockopt((int)SendSocket,SOL_SOCKET,SO_BROADCAST, (char *)&so_broadcast, sizeof(so_broadcast));
#ifdef AOV_DEBUG
  printf("%i\n",errno);
#endif

  //---------------------------------------------
  // Send a datagram to the receiver
#ifdef AOV_DEBUG
  printf("Sending a datagram to the receiver...\n");
#endif
	  
  k=0;

	data = (unsigned char *)AVCOM_API_EncodeUDPSetPort( &k, mac, port );
	memcpy(sa_tx, data, k);
	free(data);

  sendbuf = AVCOM_INT_AssemblePacket(sa_tx, k, &sendlen, 0);

  z=sendto((int)SendSocket, 
    sendbuf, 
    sendlen, 
    0, 
    (struct sockaddr *) &SendAddr, 
    sizeof(SendAddr));

  free(sendbuf);
//	y = WSAGetLastError();


  
  //---------------------------------------------
  // When the application is finished sending, close the socket.
#ifdef AOV_DEBUG
  printf("Finished sending. Closing socket.\n");
#endif
#ifdef _WIN32
	y = WSAGetLastError();
		closesocket(SendSocket);
//		WSACleanup();
//		exit(0);
#else
	close(SendSocket);
	for (i=0; i< 0x0FFFFFF; i++);
#endif

  //---------------------------------------------
  // Clean up and quit.
#ifdef AOV_DEBUG
  printf("Exiting.\n");
#endif
	
	return iResult;
}




#if 0
// A MS Function
/********************************************************************************
 *																				*
 *				AVCOM_SA_Unregister()											*
 *																				*
 *******************************************************************************/
int AVCOM_SA_Unregister(void *handle)
{
	int i;
	AOVhandle *lhandle = (AOVhandle *)handle; 

	for ( i = 0; i < plen; i++ )
	{
		if (plist[i] == (AOVhandle *)handle)
			break;
	}

	if (i == plen)
		return AOV_ERR_HANDLE_NOT_FOUND;

	AVCOM_SA_Disconnect(lhandle);

	for (i=i; i < (plen-1); i++ )
	{
		plist[i] = plist[i+1];
	}

	plen --;
	if (plen) // if no active handles, don't realloc()
		plist = realloc( plist, (plen) * sizeof(AOVhandle *) );


	free(handle);
	return AOV_NO_ERROR;
}
#endif

#endif /* AOV_DEPEND_SA */
