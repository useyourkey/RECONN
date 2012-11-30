#pragma once
#ifndef AOV_CORE_V2_H
#define AOV_CORE_V2_H
#include "aov_opts.h"

#define AOVAPI_MAJOR  0
#define AOVAPI_MINOR  1
#define AOVAPI_BUILD  0
#define AOVAPI_STAGE  3


#include "aov_protocol.h"

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

#ifndef TRUE
#define TRUE	1;
#endif
#ifndef FALSE
#define FALSE	0;
#endif


/*struct AOV_time_ {
	int seconds;
	int minutes;
	int hours;
	int day;
	int month;
	int year;
};*/



typedef struct AOVhandle_ {
	int						socket_type;
	int						usbId;
	void					*usblocId;
	int						socketfd;
	void*					ftHandle;
	struct addrinfo			*servinfo;
	char					*ip;
	unsigned short			portno;
//	char					serial[16];
//	unsigned char			avail_inputs;
	int						version_major;
	int						version_minor;
	int						version_patch;
//	struct AOV_time_		cal_date;
//	int						aapi_support;
//	sAOV_PROD_DESC			pd;

} AOVhandle;



typedef union dataConvert {
	double					db;
	float					fl;
	int						i;
	long int				li;
	unsigned int			ui;
	unsigned long			ul;
	unsigned short			us;
	short					s;
	unsigned long long		ull;
	unsigned char			ch[8];
} dataConvert;



#define		AOV_DSA_MIN_DB			0.0		// Lower range for AVCOM_ENG_SetDsa()
#define		AOV_DSA_MAX_DB			31.5	// Upper range for AVCOM_ENG_SetDsa()
#define		AOV_DSA_STEP_DB			0.5		// Step size for AVCOM_ENG_SetDsa()


#ifdef AOV_DEBUG
extern int errno;
#endif




/****************           HELPER MACROS          ***************************/
// UNPACK_FREQ - convert frequency: unsigned long in khz to double in mhz
#define UNPACK_FREQ(X)		((double)X)/1000.0

// PACK_FREQ - convert frequency: double in mhz to unsigned long in khz
#define PACK_FREQ(X)		(unsigned long)(X*1000.0)

// UNPACK_POWER - convert frequency: unsigned long in 1/10ths of db to double in db
#define UNPACK_POWER(X)		((double)X)/10.0

// PACK_POWER - convert frequency: double in db to unsigned long in 1/10db
#define PACK_POWER(X)		(unsigned long)(X*10.0)

/* Packing refers to going from analyzer to TX buffer  *
 * Unpacking refers to going from RX buffer to analyer *
 *                                                     *
 * dataConvert cvt; must be within function for use    *
 *                                                     *
 * Convention is k for sa_tx, j for sa_rx              *
 * D = Destination                                     *
 * S = Source                                          *
 * K or J = counter                                    *
 * L = Length                                          */
 
 
#define PACK_API_CMD PACK2
#define UNPACK_API_CMD UNPACK2



#ifdef LITTLE_ENDIAN
 
//PACK4 - place an unsigned long into the packet array
#define PACK4(D,S,K)			\
	cvt.ul=(unsigned long)S;	\
	D[K++]= cvt.ch[3];			\
	D[K++]= cvt.ch[2];			\
	D[K++]= cvt.ch[1];			\
	D[K++]= cvt.ch[0]
 
//PACK2 - place an unsigned short into the packet array
#define PACK2(D,S,K)			\
	cvt.us=(unsigned short)S;	\
	D[K++]= cvt.ch[1];			\
	D[K++]= cvt.ch[0]
	
//PACKF - place a float into the packet array
#define PACKF(D,S,K)			\
	cvt.fl= S;					\
	D[K++]= cvt.ch[3];			\
	D[K++]= cvt.ch[2];			\
	D[K++]= cvt.ch[1];			\
	D[K++]= cvt.ch[0]
	
//UNPACKF - Extract a float from the packet array
#define UNPACKF(D,S,J)			\
	cvt.ch[3]=S[J++];			\
	cvt.ch[2]=S[J++];			\
	cvt.ch[1]=S[J++];			\
	cvt.ch[0]=S[J++];			\
	D=cvt.fl
	
//UNPACK4 - Extract an unsigned long from the packet array
#define UNPACK4(D,S,J)			\
	cvt.ch[3]=S[J++];			\
	cvt.ch[2]=S[J++];			\
	cvt.ch[1]=S[J++];			\
	cvt.ch[0]=S[J++];			\
	D=cvt.ul
	
//UNPACK4 - Extract a signed long from the packet array
#define UNPACK4S(D,S,J)			\
	cvt.ch[3]=S[J++];			\
	cvt.ch[2]=S[J++];			\
	cvt.ch[1]=S[J++];			\
	cvt.ch[0]=S[J++];			\
	D=cvt.li
	
//UNPACK4 - Extract an unsigned short from the packet array
#define UNPACK2(D,S,J)			\
	cvt.ch[1]=S[J++];			\
	cvt.ch[0]=S[J++];			\
	D=cvt.us
	
//UNPACK4 - Extract a signed short from the packet array
#define UNPACK2S(D,S,J)			\
	cvt.ch[1]=S[J++];			\
	cvt.ch[0]=S[J++];			\
	D=cvt.s
	
//PACK1 - place an unsigned short into the packet array
#define PACK1(D,S,K)			\
	D[K++]=(unsigned char)S

//UNPACK4 - Extract an unsigned short from the packet array
#define UNPACK1(D,S,J)			\
	D=S[J++];
	
//UNPACK4 - Extract a signed short from the packet array
#define UNPACK1S(D,S,J)			\
	D=(char)S[J++]



#define	PACK_STRCPY(D,S,K,L)				\
	strncpy((char *)&D[K], (char *)S, L);	\
	K += strlen((char *)S);					\
	D[K++] = 0x00


#define	UNPACK_STRCPY(D,S,J,L)				\
	strncpy((char *)D, (char *)&S[J], L);	\
	D[strlen((char *)&S[J])] = 0x00;		\
	J += strlen((char *)&S[J])+1
	
	
// PACK_FREQ_M2K - convert frq from double in mhz to unsigned in khz. Add to packet.
#define PACK_FREQ_M2K(F,K,P)			\
	cvt.ul	= (unsigned long)(F*1000);	\
	P[k++]	= cvt.ch[3];				\
	P[k++]	= cvt.ch[2];				\
	P[k++]	= cvt.ch[1];				\
	P[k++]	= cvt.ch[0]

// UNPACK_FREQ_K2M - Extract from packet. convert unsigned long in khz to double in mhz
#define UNPACK_FREQ_K2M(F,S,J)		\
	cvt.ch[3] = S[J++];				\
	cvt.ch[2] = S[J++];				\
	cvt.ch[1] = S[J++];				\
	cvt.ch[0] = S[J++];				\
	F = ((double)(cvt.ul))/1000.0
#endif /* LITTLE_ENDIAN */





#define PACK_MEMCPY(D,S,K,L)					\
	memcpy((char *)&D[K],(char *)S,L);			\
	K += L

#define UNPACK_MEMCPY(D,S,J,L)					\
	memcpy((char *)&D,(char *)&S[J],L);			\
	J += L





#ifdef AOV_DEPEND_ENCODE
#include "aov_common.h"
unsigned char *AVCOM_INT_AssemblePacket(unsigned char *data_in, int data_in_len, int *data_out_len, int flags);
void *AVCOM_INT_EncodeCMD ( unsigned short cmd );
int AVCOM_INT_ProcessHeader ( struct aov_header *header, unsigned char *raw_header );
#endif /* (AOV_DEPEND_SA) || (AOV_DEPEND_ENCODE) */

#ifdef AOV_DEPEND_SA
int AVCOM_INT_CheckHandle(void *handle);
int AVCOM_INT_GetVersion ( void *handle, unsigned short cmd, struct aov_version *ver );
int AVCOM_INT_ReturnACK ( unsigned short cmd, unsigned char* data, int *j );
int AVCOM_INT_RecvData(void *handle, unsigned char *data, int data_size);
int AVCOM_INT_SendData(void *handle, unsigned char *data, int data_size);
#endif /* AOV_DEPEND_SA */

#if defined(AOV_DEPEND_ENG)
EXPORT int AVCOM_INT_SendPacket(void *handle, unsigned char *data, int data_size);
EXPORT int AVCOM_INT_RecvPacket(void *handle, unsigned char *data, int *data_size);
#elif defined(AOV_DEPEND_SA)
int AVCOM_INT_SendPacket(void *handle, unsigned char *data, int data_size);
int AVCOM_INT_RecvPacket(void *handle, unsigned char *data, int *data_size);
#endif


extern AOVhandle **plist;
extern int			plen;


#endif /* AOV_CORE_V2_H*/

