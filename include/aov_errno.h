/*************************************************************************//**
 * @file aov_errno.h
 * @author M. Severo
 ****************************************************************************/

#pragma once
#ifndef _AOV_ERRNO_H_
#define _AOV_ERRNO_H_

/*! Avcom's errno and Error Code List */
enum aov_errno {
	AOV_NO_ERROR = 0,					//!< 0: No Error

	AOV_ERR_API_INITIALIZED,			//!< 1: The Avcom API environment failed to initialize, perhaps already initialized
	AOV_ERR_API_NOT_INITIALIZED,		//!< 2: The Avcom API environment is not initialized

	AOV_ERR_HANDLE_NOT_FOUND,			//!< 3: The given handle does not appear to be valid
	AOV_ERR_HANDLE_EXISTS,				//!< 4: Unable to create new handle as handle appears to already exist
	AOV_ERR_HANDLE_CREATE,				//!< 5: Unable to create handle / \c malloc() or \c realloc() handle

	AOV_ERR_WSASTARTUP_FAILED,			//!< 6: Windows Only: Winsock2 \c WSAStartup() function failed
	AOV_ERR_WSACLEANUP_FAILED,			//!< 7: Windows Only: Winsock2 \c WSACleanup() function failed
	AOV_ERR_OPEN_SOCKET_FAILED,			//!< 8: Unable to open socket
	AOV_ERR_CLOSESOCKET_FAILED,			//!< 9: Unable to close TCP socket
	AOV_ERR_SETSOCKETOPT_FAILED,		//!< 10: Unable to set socket options. Function \c setsockopt()/USB options failed
	AOV_ERR_CONNECT_FAILED,				//!< 11: Unable to connect to socket. Function \c connect() failed
	AOV_ERR_SEND_FAILED,				//!< 12: Unable to send data. Function \c send() failed
	AOV_ERR_RECV_FAILED,				//!< 13: Unable to receive data. Function \c recv() failed.  Often a timeout has occurred waiting for data
	AOV_ERR_SOCKET_NOT_OPEN,			//!< 14: Specified socket is not open
	AOV_ERR_SOCKET_ALREADY_OPEN,		//!< 15: Specified socket is open
	AOV_ERR_INVALID_SOCKET,				//!< 16: Socket Type is invalid or socket undefined
	AOV_ERR_GET_ADDR_INFO_FAIL,			//!< 17: Function \c getaddrinfo() failed. Typically an invalid ip address/host name or port is specified, or hostname not found

	AOV_ERR_USB_COMM,					//!< 18: Error communicating to FT2XX USB Driver

	AOV_ERR_HDR_PACKET_SIZE,			//!< 19: AVCOM Header: Received packet is wrong size for AVCOM header
	AOV_ERR_HDR_HEADER_SIZE,			//!< 20: AVCOM Header: Size field does not match the packet size
	AOV_ERR_HDR_PROTOCOL_ID,			//!< 21: AVCOM Header: Protocol ID is incorrect
	AOV_ERR_HDR_CHECKSUM,				//!< 22: AVCOM Header: Checksum is incorrect
	AOV_ERR_HDR_DATA_SIZE,				//!< 23: AVCOM Header: Data Size field does not match size of data received
	AOV_ERR_HDR_DATA_CHECKSUM,			//!< 24: AVCOM Header: Data Checksum is incorrect
	AOV_ERR_HDR_MALLOC,					//!< 25: AVCOM Header: \c malloc() failure in header processing

	AOV_ERR_UNKNOWN_CMD,				//!< 26: Command set is of unknown or not supported
	AOV_ERR_UNEXPECTED_CMD,				//!< 27: An unexpected command was received from the Spectrum Analyzer
	AOV_ERR_INCORRECT_ACK,				//!< 28: ACK Response expected different command in ACK
	AOV_ERR_INVALID_DEV_DRIVER,			//!< 29: Invalid Device Driver Number/Handle

	AOV_ERR_INVALID_VAL,				//!< 30: Value is unexpected or out of range
	AOV_ERR_INVALID_PARAM,				//!< 31: Parameters is unexpected or out of range

	AOV_ERR_SWEEP_STEPS,				//!< 32: Max number of sweep points exceeded

	AOV_ERR_MEM_BLOCK_NUM,				//!< 33: Bad Block Number in Memory Transfer
	AOV_ERR_MEM_NOT_ALLOC,				//!< 34: Memory is not allocated in Memory Transfer
	AOV_ERR_MEM_INVALID_FILESIZE,		//!< 35: Incorrect File Size in Memory Transfer
	AOV_ERR_MEM_NOT_FULL,				//!< 36: Memory fill incomplete in Memory Transfer
	AOV_ERR_MEM_INVALID_CRC,			//!< 37: Invalid CRC in Memory in Memory Transfer
	AOV_ERR_MEM_FILE_LEN,				//!< 38: File length error in Memory Transfer

	AOV_ERR_PFLASH_OPEN,				//!< 39: Unable to open onboard flash
	AOV_ERR_PFLASH_INIT,				//!< 40: Unable to initialize onboard flash
	AOV_ERR_PFLASH_ID,					//!< 41: Incorrect onboard flash ID
	AOV_ERR_PFLASH_CMD,					//!< 42: Bad command for onboard flash
	AOV_ERR_PFLASH_ERASE,				//!< 43: Error performing erase function
	AOV_ERR_PFLASH_WRITE_FAIL,			//!< 44: Unable to write to onboard flash
	AOV_ERR_PFLASH_READ_VER_FAIL,		//!< 45: Unable to verify write operation on onboard flash
	AOV_ERR_PFLASH_READ_FAIL,			//!< 46: Unable to read onboard flash
	
	AOV_ERR_TELNET_TX,					//!< 47: Telnet transmission error
	AOV_WARN_LIST_SIZE,					//!< 48: Warning that the list size is too small and will be truncated
	AOV_ERR_LIST_UNDEF,					//!< 49: The specified list is undefined
	AOV_ERR_VALUE_UNSUPPORTED,			//!< 50: The entered value is not supported on this platform
	AOV_ERR_VALUE_UNDEF,				//!< 51: Value is undefined - may not have been programmed yet
	AOV_ERR_BAND_OOR,					//!< 52: Band is out of range
	AOV_ERR_FREQ_OOR,					//!< 53: Frequency is out of range
	AOV_ERR_INVALID_STEP,				//!< 54: Invalid Frequency Step
	AOV_ERR_SWEEP_POINTS,				//!< 55: Error in number of sweep points or sweep step size
	AOV_ERR_SWEEP_UNEVEN_STEP,			//!< 56: Step size is uneven
	AOV_ERR_FREQ_SPAN,					//!< 57: Span is invalid or not even for step size

	AOV_ERR_USB_DEV_NUM,				//!< 58: Error in USB Device Number selection - invalid number or is not AVCOM Analyzer
	AOV_ERR_USB_IN_USE,					//!< 59: USB PID & VID equal zero. Driver/USB Chipset error, typically from device already being in use.

};


#endif /* _AOV_ERRNO_H_ */
