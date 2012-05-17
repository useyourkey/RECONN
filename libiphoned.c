//******************************************************************************
//******************************************************************************
//
// FILE:        libiphoned.c
//
// DESCRIPTION: contains library functions to interface with iphoned
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
//           First fixed in 2012, all rights reserved.
//
//******************************************************************************
//
// HISTORY: Created <MM>/<DD>/<YYYY> by <USER>
// $Header:$
// $Revision: $
// $Log:$
//
//******************************************************************************
//******************************************************************************

#include <stdarg.h>
#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>

#include "libiphoned.h"

//#define LIBIPHONED_DEBUG

// global header
#define TRUE 1
#define FALSE 0

typedef enum {
	START = 0, CONNECTED, KILL, KILLWAIT, STARTWAIT
} iphoned_monitor_state_t;

#define LIBIPHONED_DEMO_MAIN

#define CLONE_STACK_SIZE  16384

// --------------------------------
// iphoned protocol definitions  (match this up to iphoned server.c)
#define MAXPKTLEN 1024
#define PKTHEADER_LEN 3

#define IPHONED_PORT            1069
#define START_BYTE               0x89
#define ESCAPE_BYTE              'a'

#define MSG_REPORT_IPHONE_PRESENCE 0x88
#define MSG_FORWARD_DATA 0x02
#define MSG_REPORT_DATA 0x03

#define IPHONE_NOT_PRESENT 0x00
#define IPHONE_PRESENT 0x01
//-----------------------------------------------------

#define IPHONED_ADDR "127.0.0.1"

static int iphoned_fd = -1;

static char const *iphoned_name = "iphoned";
static pthread_t iphonedthread;

static pthread_t iphonedmonitorthread;
static int monitorthreadterminate = FALSE;
static int monitorthreadrunning = FALSE;

static int iphoneconnected = FALSE;

static int max_rx_callback_buffer_sz = 0;
static void(*rxcallbackfn)(unsigned char *, int) = NULL;
static void(*presencechangecallbackfn)(void) = NULL;

static iphoned_monitor_state_t iphoned_monitor_state;

static void startiphoned(void);
static void processmsg(unsigned char *msgbuf, int len);
static void process_msg_report_iphone_presence(unsigned char *buf, int len);
static void process_msg_report_data(unsigned char *buf, int len);
static void stopiphoned(void);
static void processrx(unsigned char *inbuf, int len);
static int getpidof(char const *process);
static void send_sock_msg(unsigned char cmdid, unsigned char *outbuf, int len);
static void stop_iphoned_monitor(void);
static int start_iphoned_monitor(void);
void *iphoned_monitor_thread(void *ptr);
void *iphoned_start_service(void *ptr);
static int connect_to_iphoned(void);

static void libiphoned_log(const char *fmt, ...) {
#ifdef LIBIPHONED_DEBUG
	va_list ap;
	char *fs;
	struct timeval ts;
	struct tm *tp;

	gettimeofday(&ts, NULL);
	tp = localtime(&ts.tv_sec);
	fs = malloc(30 + strlen(fmt));
	sprintf(fs, "[libiphoned] %d.%03d %s\n", (int) ts.tv_sec,
			(int) (ts.tv_usec / 1000), fmt);
	va_start(ap, fmt);
	vfprintf(stderr, fs, ap);
	va_end(ap);
	free(fs);
#endif
}

/**
 *
 * processes MSG_REPORT_IPHONE_PRESENCE
 *
 * processes a MSG_REPORT_IPHONE_PRESENCE message.  monitors presence change as reported
 * and calls the callback if it is assigned.
 *
 * @param buf:  pointer to buffer containing message
 * @param len:  length of message
 */
static void process_msg_report_iphone_presence(unsigned char *buf, int len) {
	static int lastpresence = -1;
	int presence = buf[1];

	if (presence == 1) {
		iphoneconnected = TRUE;
	} else {
		iphoneconnected = FALSE;
	}
	if (presence != lastpresence) {
		lastpresence = presence;
		if (presencechangecallbackfn != NULL) {
			presencechangecallbackfn();
		}
	}
}

/**
 * processes MSG_REPORT_DATA
 *
 * processes a MSG_REPORT_DATA message.  forwards the data buffer pointer and length to
 * the callback function if it is assigned.
 *
 * @param buf:  pointer to buffer containing message
 * @param len:  length of message
 */
static void process_msg_report_data(unsigned char *buf, int len) {
	if (rxcallbackfn != NULL) {
		rxcallbackfn(&(buf[1]), len - 1);
	}
}

/**
 * Message parser for iphoned protocol messages.
 *
 * manages processing of all messages received from iphoned
 * by calling a handler function and passing the message data.
 *
 * @param msgbuf:  pointer to buffer that holds message to be processed
 * @param len:  length of data to be sent
 */
static void processmsg(unsigned char *msgbuf, int len) {
	switch (msgbuf[0]) {
	case MSG_REPORT_IPHONE_PRESENCE:
		process_msg_report_iphone_presence(msgbuf, len);
		break;
	case MSG_REPORT_DATA:
		process_msg_report_data(msgbuf, len);
		break;
	default:
		break;
	}
}

/**
 * Stateful packet parser for iphoned packets
 *
 * manages processing of all data received from iphoned.
 * Escape sequences are decoded and resulting data is parsed into packets.
 * The packets are provided to a parsing function.  State is retained between
 * calls.
 *
 * @param inbuf:  pointer to buffer that holds message to be processed
 * @param len:  length of data to be sent
 */
static void processrx(unsigned char *inbuf, int len) {
	// persistent
	static int packetpos = 0;
	static unsigned char currpkt[MAXPKTLEN];
	static int is_escaped = FALSE;
	static int msglen = 0;

	int inbufpos = 0;
	unsigned char currbyte;
	unsigned int ctrlbyte = FALSE;

	while (inbufpos < len) {
		// handle escape and control data detection
		ctrlbyte = -1;
		if (is_escaped == TRUE) {
			currbyte = inbuf[inbufpos];
			is_escaped = FALSE;
		} else if ((is_escaped == FALSE) && (inbuf[inbufpos] == ESCAPE_BYTE)) {
			is_escaped = TRUE;
		} else if ((is_escaped == FALSE) && (inbuf[inbufpos] == START_BYTE)) {
			currbyte = START_BYTE;
			ctrlbyte = TRUE;
		} else {
			currbyte = inbuf[inbufpos];
			is_escaped = FALSE;
		}
		inbufpos++;

		// process decoded packet data
		if (is_escaped != TRUE) {
			if ((packetpos == 0) && (ctrlbyte != TRUE)) {
				// out of packet data - do nothing
			} else if (ctrlbyte == TRUE) {
				ctrlbyte = FALSE;
				if (currbyte == START_BYTE) {
					packetpos = 0;
					ctrlbyte = 0;
					++packetpos;
					msglen = 0;
				}
			} else if (packetpos == 1) {
				msglen += currbyte * 256;
				++packetpos;
			} else if (packetpos == 2) {
				msglen += currbyte;
				++packetpos;
			} else if (packetpos <= (msglen + 3)) {
				currpkt[packetpos - 3] = currbyte;
				++packetpos;
			}

			// check for packet complete and process if complete
			if ((packetpos > 2) && (packetpos >= (msglen + 3))) {
				processmsg(currpkt, msglen);
				packetpos = 0;
			}
		}
	}
}

/**
 * thread function for starting iphoned service
 *
 * causes an instance of the iphoned daemon to be started and immediately exits.
 * intended to be executed via pthread.
 *
 * @param ptr:  purposeless pointer to comply with expected pthread function prototype
 */
void *iphoned_start_service(void *ptr) {
	system(iphoned_name);
	return 0;
}

/**
 * thread function for monitoring and maintaining connection to iphoned.
 *
 * executes a state machine loop that performs:
 * 		attempts to connect to iphoned with retries
 * 		maintains an open iphoned connection by
 * 			receiving data from the socket file decriptor
 * 			detecting connection loss
 * 		terminates and restarts iphoned when the connection is lost
 *
 *
 * 	also exits the thread when signaled via global variable
 *
 * @param ptr:  purposeless pointer to comply with expected pthread function prototype
 */
void *iphoned_monitor_thread(void *ptr) {
	int res;
	unsigned char rxbuf[1024];
	static int state = 0;
	fd_set clientfdset;
	struct timeval timeout;

	iphoned_monitor_state = START;
	monitorthreadrunning = TRUE;
	while (1) {
		if (monitorthreadterminate == TRUE) {
			if (iphoned_fd != -1) {
				close(iphoned_fd);
				iphoned_fd = -1;
				break;
			}
		}
		switch (iphoned_monitor_state) {
		case START:
			startiphoned();
			iphoned_monitor_state = STARTWAIT;
			break;
		case STARTWAIT: {
			int i;
			for (i = 0; i < 50; ++i) {
				usleep(100000);
				libiphoned_log("", iphoned_monitor_state);
				res = connect_to_iphoned();
				libiphoned_log("connect_to_iphoned %d", res);
				if (res == 0) {
					iphoned_monitor_state = CONNECTED;
					break;
				}
			}
			if (res != 0) {
				iphoned_monitor_state = KILL;
			}
		}
			break;
		case CONNECTED:
			timeout.tv_sec = 1;
			timeout.tv_usec = 0;
			FD_ZERO(&clientfdset); /* clear the set */
			FD_SET(iphoned_fd, &clientfdset); /* add our file descriptor to the set */

			res = select(iphoned_fd + 1, &clientfdset, NULL, NULL, &timeout);
			if (res == -1) {
			} else if (res == 0) {
			} else {
				res = recv(iphoned_fd, rxbuf, 1024, 0);
				if (res > 0) {
					processrx(rxbuf, res);
				} else if (errno != ETIMEDOUT) {
					iphoneconnected = FALSE;
					close(iphoned_fd);
					iphoned_fd = -1;
					iphoned_monitor_state = KILL;
				}
			}
			break;
		case KILL:
			stopiphoned();
			iphoned_monitor_state = KILLWAIT;
			break;
		case KILLWAIT:
			sleep(1);
			iphoned_monitor_state = START;
			break;
		}
	}
	monitorthreadrunning = FALSE;
}

/**
 * starts monitoring for iphoned
 *
 * starts the iphoned monitor thread.
 *
 */
static int start_iphoned_monitor(void) {
	int res;

	monitorthreadterminate = FALSE;
	res = pthread_create(&iphonedmonitorthread, NULL, iphoned_monitor_thread,
			(void*) "libiphoned_monitorthread");
	if (res != 0) {
		return -1;
	}
	return 0;
}

/**
 * stops iphoned monitor thread
 *
 * stops the iphoned monitor thread.  does not return until monitor thread exits.
 *
 */
static void stop_iphoned_monitor(void) {
	monitorthreadterminate = TRUE;
	while (monitorthreadrunning == TRUE)
		;
}

/**
 * starts iphoned instance
 *
 * causes instance of iphoned daemon to start.  uses pthread to allow immediate
 * exit.
 *
 */
static void startiphoned(void) {
	pthread_create(&iphonedthread, NULL, iphoned_start_service,
			(void*) "libiphoned_iphonedthread");
}

/**
 * attempts iphoned connection
 *
 * attempts to connect to running iphoned.  sets global file descriptor if successful.
 *
 * @return 0 if connected, -1 if not connected
 */
static int connect_to_iphoned(void) {
	int iphoned_socket;
	int iphoned_port;
	int flags;
	struct sockaddr_in iphoned_ip_addr;
	struct hostent *hp;

	iphoned_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (iphoned_fd < 0) {
		return -1;
	}
	hp = gethostbyname(IPHONED_ADDR);
	bcopy(hp->h_addr, &(iphoned_ip_addr.sin_addr.s_addr), hp->h_length);

	iphoned_port = IPHONED_PORT;

	bzero((unsigned char *) &iphoned_ip_addr, sizeof(iphoned_ip_addr));

	iphoned_ip_addr.sin_family = AF_INET;

	bcopy((unsigned char *) hp->h_addr,
			(unsigned char *) &iphoned_ip_addr.sin_addr.s_addr, hp->h_length);

	iphoned_ip_addr.sin_port = htons(iphoned_port);

	if (connect(iphoned_fd, (struct sockaddr *) &iphoned_ip_addr,
			sizeof(iphoned_ip_addr)) < 0) {
		close(iphoned_fd);
		return -1;
	}
	flags = fcntl(iphoned_fd, F_GETFD);
	flags = flags | O_NONBLOCK;
	fcntl(iphoned_fd, F_SETFD, &flags);
	return 0;
}

/**
 * stops all running iphoned instances
 *
 * stops all running iphoned instances.  searches for matching PIDs and sends SIGINT to terminate
 * gracefully.  If graceful option times out, uses non-graceful SIGKILL.
 *
 */
static void stopiphoned(void) {
	int thetime = 0;

	int pid = getpidof(iphoned_name);
	while (pid != 0) {
		if (pid > 0) {
			kill(pid, SIGINT);
		}
		usleep(10000);
		if (thetime > 10000000) {
			// if we've waited longer than 10 seconds, use other means
			system("killall -9 iphoned");
			libiphoned_log("timeout in graceful iphoned kill");
			return;
		}
		pid = getpidof(iphoned_name);
	}
}

/**
 * constructs and attempts to send packet to iphoned
 *
 * uses parameters to construct packet by adding start byte, length bytes and escape
 * characters where necessary.
 *
 * @param cmdid:  command ID of packet to send
 * @param outbuf:  buffer containing message to send
 * @param len:  length of buffer containing message
 *
 * @return TRUE if iphoned socket is currently active.  FALSE otherwise.
 */
static void send_sock_msg(unsigned char cmdid, unsigned char *outbuf, int len) {
	unsigned char *buf;
	int i = 0;
	int bufidx = 0;
	unsigned char tmp;
	int res;
	int msglen = len + 1;

	if (iphoned_fd != -1) {
		buf = malloc(len * 2 + 3);
		buf[bufidx++] = START_BYTE;
		tmp = msglen / 256;
		if ((tmp == ESCAPE_BYTE) || (tmp == START_BYTE)) {
			buf[bufidx++] = ESCAPE_BYTE;
		}
		buf[bufidx++] = tmp;
		tmp = msglen & 0xFF;
		if ((tmp == ESCAPE_BYTE) || (tmp == START_BYTE)) {
			buf[bufidx++] = ESCAPE_BYTE;
		}
		buf[bufidx++] = tmp;
		if ((cmdid == ESCAPE_BYTE) || (cmdid == START_BYTE)) {
			buf[bufidx++] = ESCAPE_BYTE;
		}
		buf[bufidx++] = cmdid;
		for (i = 0; i < len; ++i) {
			tmp = outbuf[i];
			if ((tmp == ESCAPE_BYTE) || (tmp == START_BYTE)) {
				buf[bufidx++] = ESCAPE_BYTE;
			}
			buf[bufidx++] = outbuf[i];
		}
		res = send(iphoned_fd, buf, bufidx, 0);
		free(buf);
	}
}

/**
 * finds PID of running process matching name
 *
 * searches procfs for a PID matching the given name.  returns the first PID it finds
 * or -1 if none found.
 *
 * @param process:  string name of process to search for
 *
 * @return PID of running process matching name passed in or -1.
 */
static int getpidof(char const *process) {
	DIR *dp;
	struct dirent *entry;
	struct stat statbuf;
	int res;
	int retval;

	dp = opendir("/proc");
	if (dp == NULL) {
		libiphoned_log("getpidof failed to get open /proc %d", errno);
		return -1;
	}
	chdir("/proc");

	entry = readdir(dp);
	while (entry != NULL) {

		res = lstat(entry->d_name, &statbuf);
		if (res < 0) {
			libiphoned_log("getpidof failed to get stat /proc %d %d", res,
					errno);
			closedir(dp);
			return -1;
		}
		if (S_ISDIR(statbuf.st_mode) && isdigit(entry->d_name[0])) {

			char procpath[14 + strlen(entry->d_name)];
			char procname[50];
			char trash[50];
			int fd;
			unsigned char buf[200];

			strcpy(procpath, "/proc/");
			strcat(procpath, entry->d_name);
			strcat(procpath, "/status");

			fd = open(procpath, O_RDONLY);
			if (fd >= 0) {
				read(fd, buf, 200);
				close(fd);
				sscanf(buf, "%s%s", trash, procname);
				if (strcmp(procname, process) == 0) {
					closedir(dp);
					retval = atoi(entry->d_name);
					return retval;
				}
			}
		}
		entry = readdir(dp);
	}
	closedir(dp);

	libiphoned_log("getpidstart");
	return 0;
}

// API functions--------------------------------------
/**
 * returns presence of iphone
 *
 * reports presence of the iphone.  Presence indicates that the iphone is connected and a data
 * session is open and ready to accept data.
 *
 * @return 0 if iphone -1 if not successful
 */
int libiphoned_isiphonepresent(void) {
	if (iphoneconnected == TRUE) {
		return 0;
	}
	libiphoned_log("libiphoned_start fail");
	return -1;
}

/**
 * Starts libiphoned
 *
 * starts libiphoned's background action and managing of the iphoned process.
 * On startup or after call of libiphoned_stop, no other API functions can be called until
 * this is called.
 *
 * @return 0 if successful, -1 if not successful
 */
int libiphoned_start(void) {
	int res;

	if (monitorthreadrunning == FALSE) {
		res = start_iphoned_monitor();
		if (res == 0) {
			return 0;
		}
	}
	libiphoned_log("libiphoned_start fail");
	return -1;
}

/**
 * Stops all background action in libiphoned and any managed iphoned running
 *
 * intended to be a kill switch for libiphoned/iphoned.  It will
 * cause iphoned and any background action in libiphoned to cease.  It is recommended that
 * not be used in regular operation but more for exceptions & system shutdown proceedings.
 * This cannot be called without first calling libiphoned_start().  After this is called,
 * no other API function can be called other than libiphoned_start().
 *
 * @return 0 if successful, -1 if not
 */
int libiphoned_stop(void) {
	int killtime = 0;

	stop_iphoned_monitor();
	stopiphoned();
	while (monitorthreadrunning != FALSE) {
		// spin here
		usleep(100);
		killtime += 100;
		if (killtime > 10000000) {
			libiphoned_log("libiphoned_stop timeout");
			return -1;
		}
	}
	return 0;
}

/**
 * Causes the passed data buffer to be transmitted to iphoned
 *
 * The data is sent to iphoned for transmission to the iphoned app
 *
 * @param buf:  pointer to buffer that holds data to be sent
 * @param len:  length of data to be sent
 *
 * @return 0 is successful, -1 if not
 */
int libiphoned_tx(unsigned char *buf, unsigned int len) {
#ifdef __SIMULATION__
	int i;
	for(i = 0; i < len; i++)
	{
		printf("0x%x ", buf[i]);
	}
	printf("\n");
#else
	send_sock_msg(MSG_FORWARD_DATA, buf, len);
#endif
	libiphoned_log("libiphoned_tx fail");
	return 0;
}

/**
 * Registers a function as the received data callback
 *
 * The function passed will be called when data is received from iphoned.
 * The function must not free() the buffer passed.
 *
 * @param callbackfn:  pointer to callback function
 * @param max_buffer:  pointer to callback function
 *
 * @return 0 is successful, -1 if not
 */
int libiphoned_register_rx_callback(void(callbackfn)(unsigned char *, int),
		int max_buffer) {
	if (callbackfn != NULL) {
		rxcallbackfn = callbackfn;
		max_rx_callback_buffer_sz = max_buffer;
		return 0;
	} else {
		libiphoned_log("libiphoned_register_rx_callback fail");
		return -1;
	}
}

/**
 * Registers a function as the presence change callback
 *
 * The function passed will be called when iphoned signals there is a change in the
 * presence of an iphone (either connected or disconnected).  At this point libiphoned_isiphonepresent()
 * can be called to determine the current status.
 *
 * @param callbackfn:  pointer to callback function
 *
 * @return 0 is successful, -1 if not
 */
int libiphoned_register_presence_change_callback(void(*callbackfn)(void)) {
	if (callbackfn != NULL) {
		presencechangecallbackfn = callbackfn;
		return 0;
	} else {
		libiphoned_log("libiphoned_register_presence_change_callback fail");
		return -1;
	}
}

/**
 * Gets maximum packet length that can be passed to libiphoned_tx()
 *
 * The maximum packet length is based on the largest iphoned protocol payload size
 *
 * @return maximum packet length
 */
int libiphoned_get_max_packet_len(void) {
	return MAXPKTLEN - PKTHEADER_LEN;
}
