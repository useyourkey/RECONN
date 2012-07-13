//******************************************************************************
//****************************************************************************** //
// FILE:        crashHandler.c
//
// CLASSES:     
//
// DESCRIPTION: This file is resposible for catching any crashes by the reconn
//              software
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
//           First fixed in 2004, all rights reserved.
//
//******************************************************************************
//
// HISTORY: Created <MM>/<DD>/<YYYY> by <USER>
// $Header:$
// $Revision: 1.3 $
// $Log:$
// 
//******************************************************************************
#ifndef __SIMULATION__

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <execinfo.h>
#include <signal.h>
#include <errno.h>
#include <ucontext.h>
#include "reconn.h"

//#define TRACE_START 3 // NPTL
#define TRACE_START 0  // LinuxThreads
#define TRACE_MAX   50

#define LOG_FS_DIR  "/usr/bin"

// Debug variable which disables card reset after crash when set.
int gDisableCrashReset = FALSE;
extern int MsrIsRestoreInProgress;

// Build info used for crash log
extern char gSMTMVersionBuildDate[];
extern char gSMTMVersionBuildView[];
extern char **gArgv;

// GNU library extension which prints a signal description
extern char *strsignal(int __sig);


// Local Functions
void crashHandler(int signo, siginfo_t *sigInfo, void *ptr);
void abortHandler(int signo, siginfo_t *sigInfo, void *ptr);
void defaultSignalHandler(int signo, siginfo_t *sigInfo, void *ptr);
void crashPrintBacktrace(FILE *fp, void *tracebuf[], int num_frames);
void crashDumpRegisters(FILE*fp, ucontext_t *uc, char *outbuf); void crashPrintSignalInfo(FILE *fp, siginfo_t *sigInfo, char *outbuf);
static void crashWrite(char *, int, int);
static void crashPrint(FILE *fp, char *);
FILE *crashCreateLogFile(char *outbuf);

/* 
 * Install signal handlers for crashes.
 */
void initReconnCrashHandlers(void)
{
    struct sigaction act;
    int signo;

    // Catch all program error signals.
    memset(&act, 0, sizeof(act));
    act.sa_flags = SA_SIGINFO;
    act.sa_sigaction = crashHandler;
    sigaction(SIGBUS,  &act, NULL);
    sigaction(SIGILL,  &act, NULL);
    sigaction(SIGSEGV, &act, NULL);
    sigaction(SIGFPE,  &act, NULL);
    sigaction(SIGTRAP, &act, NULL);

    // Catch additional abnormal signals.
    sigaction(SIGSYS,  &act, NULL);
    sigaction(SIGXCPU, &act, NULL);
    sigaction(SIGXFSZ, &act, NULL);
    sigaction(SIGPIPE, &act, NULL);
    sigaction(SIGSTKFLT, &act, NULL);

    // Catch the Abort signal.
    memset(&act, 0, sizeof(act));
    act.sa_flags = SA_SIGINFO;
    act.sa_sigaction = abortHandler;
    sigaction(SIGABRT, &act, NULL);

    // Set default handler for async and user signals.
    memset(&act, 0, sizeof(act));
    act.sa_flags = SA_SIGINFO;
    act.sa_sigaction = defaultSignalHandler;
    sigaction(SIGALRM, &act, NULL);
    sigaction(SIGUSR1, &act, NULL);
    sigaction(SIGUSR2, &act, NULL);
    sigaction(SIGIO,   &act, NULL);
    sigaction(SIGPROF, &act, NULL);
    sigaction(SIGPWR, &act, NULL);
    sigaction(SIGVTALRM, &act, NULL);

    /*
     * Keep the default actions for the following signals:             
     * SIGTERM - Used by ISS to reload the application.
     * SIGHUP  - Used by GDB, but dev jumper is not available on the CPE.
     */

    // Set default handler for realtime signals.
    for (signo = SIGRTMIN; signo <= SIGRTMAX; signo++)
    {
        sigaction(signo, &act, NULL);
    }
}

/*
 * crashHandler()
 *
 * Signal handler which handles crashes by printing out and
 * logging a register dump and a backtrace.
 */
void crashHandler(int signo, siginfo_t *sigInfo, void *ptr)
{
    int num_frames, j;
    void *tracebuf[TRACE_MAX];
    char **strings;
    char outbuf[120];
    char sigdesc[100];
    char faultdesc[120];
    ucontext_t *uc = (ucontext_t *)ptr;
    FILE *lfp = NULL;

    sleep(1);

    memset(sigdesc, 0, 100);
    memset(faultdesc, 0, 120);
    switch (signo)
    {
        case SIGBUS:
            crashPrint(NULL, "\n\n\n!!! CRASH !!!\n");
            sprintf(sigdesc, "Bus Error (SIGBUS #%d)\n", signo);
            break;
        case SIGILL:
            sprintf(sigdesc, "Illegal Instruction (SIGILL #%d)\n", signo);
            break;
        case SIGSEGV:
            crashPrint(NULL, "\n\n\n!!! CRASH !!!\n");
            sprintf(sigdesc, "Invalid Memory Reference (SIGSEGV #%d)\n", signo);
            break;
        case SIGFPE:
            crashPrint(NULL, "\n\n\n!!! CRASH !!!\n");
            sprintf(sigdesc, "Invalid Arithmetic Operation (SIGFPE #%d)\n", signo);
            break;
        case SIGPIPE:
            sprintf(sigdesc, "Broken Pipe \n");
            break;
        default:
            sprintf(sigdesc, "%.60s (signal #%d)\n", strsignal(signo), signo);
            break;
    }

    sprintf(faultdesc, "Instruction pointer: 0x%08lx\nFaulting address: 0x%08lx\n\n", 
            uc->uc_mcontext.arm_ip, (unsigned long)sigInfo->si_addr);

    // Print some info before accessing the log FS.
    crashPrint(NULL, sigdesc);
    crashPrint(NULL, faultdesc);
    fflush(stdout);

    lfp = crashCreateLogFile(outbuf);

    if (lfp != NULL)
    {
        crashWrite(sigdesc, strlen(sigdesc), fileno(lfp));
        crashWrite(faultdesc, strlen(faultdesc), fileno(lfp));
    }

    crashDumpRegisters(lfp, uc, outbuf);

    crashPrintSignalInfo(lfp, sigInfo, outbuf);

    // Get the backtrace.
    num_frames = backtrace(tracebuf, TRACE_MAX);

    strings = backtrace_symbols(tracebuf, num_frames);
    if (strings == NULL)
    {
        reconnDebugPrint("%s: backtrace_symbols() returned 0", __FUNCTION__);
    }
    else
    {
        for (j = 0; j < num_frames; j++)
        {
            printf("%s\n", strings[j]);
        }
        free(strings);
    }


    // Replace the signal entry point with the crash address.
    //tracebuf[TRACE_START] = (void *)uc->uc_mcontext.arm_ip;

    crashPrintBacktrace(lfp, tracebuf, num_frames);

    fflush(stdout);

    if (lfp != NULL)
    {
        fclose(lfp);
    }

    // Allow time for the crash dump to be output to the serial port.
    sleep(10);

    if(signo != SIGPIPE)
    {
        // Save the time for the next bootup.
        system("killall iphoned");
        system("killall reconn-service");
    }
}

/*
 * Signal handler for abort()
 *
 * Do not save output to a log file, because the reason will likely have 
 * been logged by the application before it calls abort().
 */
void abortHandler(int signo, siginfo_t *sigInfo, void *ptr)
{
    int num_frames;
    void *tracebuf[TRACE_MAX];

    crashPrint(NULL, "\n\nabort() called (SIGABRT)\n\n");

    num_frames = backtrace(tracebuf, TRACE_MAX);
    crashPrintBacktrace(NULL, tracebuf, num_frames);

    fflush(stdout);
    sleep(10);
    // Save the time for the next bootup.
    system("killall iphoned");
    system("killall reconn-service");
}

/*
 * Default Handler for Asynchronous and Realtime signals
 */
void defaultSignalHandler(int signo, siginfo_t *sigInfo, void *ptr)
{
    int num_frames;
    void *tracebuf[TRACE_MAX];
    char outbuf[120];
    char sigdesc[100];
    FILE *lfp = NULL;

    crashPrint(NULL, "\n\n\n!!! Unexpected Signal !!!\n");

    memset(sigdesc, 0, 100);
    memset(outbuf, 0, 120);
    sprintf(sigdesc, "%.60s (signal #%d)\n", strsignal(signo), signo);
    crashPrint(NULL, sigdesc);
    fflush(stdout);

    lfp = crashCreateLogFile(outbuf);
    if (lfp != NULL)
    {
        crashWrite(sigdesc, strlen(sigdesc), fileno(lfp));
    }

    crashPrintSignalInfo(lfp, sigInfo, outbuf);

    num_frames = backtrace(tracebuf, TRACE_MAX);
    crashPrintBacktrace(lfp, tracebuf, num_frames);

    fflush(stdout);
    if (lfp != NULL) 
        fclose(lfp);
    sleep(10);
    // Save the time for the next bootup.
    system("killall iphoned");
    system("killall reconn-service");
}


/*
 * Print a backtrace given a trace buffer filled by backtrace().
 * Output is written to stdout and to a log file (fp).
 * If fp is NULL, only output to stdout.
 */
void crashPrintBacktrace(FILE *fp, void *tracebuf[], int num_frames)
{
    int i;
    char outbuf[180];
    char *ptr;
    FILE *mfp;

    crashPrint(fp, "Backtrace:\n");
    printf("num_frames = %d\n", num_frames);

    for (i=TRACE_START; i < num_frames; i++)
    {
        sprintf(outbuf, "#%02d: 0x%08lx\n", i-TRACE_START, (unsigned long)tracebuf[i]);
        crashPrint(fp, outbuf);
    }

    /*
     * Print a memory map to allow dynamically loaded adresses to be found.
     * Limit the output to the mappings with a path name ("/"). 
     */
    crashPrint(fp, "\n\nMemory map:\n");
    mfp = fopen("/proc/self/maps", "r");

    if (mfp != NULL)
    {
        while ((ptr = fgets(outbuf, sizeof(outbuf), mfp)) != NULL)
        {
            if (strchr(ptr, '/') != NULL)
            {
                crashPrint(fp, ptr);
            }
        }
    }

    crashPrint(fp, "\n");
}

/*
 * Dump PowerPC registers to stdout and the log file, 
 * using context information from the signal handler.
 */
void crashDumpRegisters(FILE* fp, ucontext_t *uc, char *outbuf)
{
    int i;
    unsigned long *armRegPtr;

    armRegPtr = &(uc->uc_mcontext.arm_r0);
    for (i = 0; i < 10; i++, armRegPtr++)
    {
        sprintf(outbuf, "R%02d:%08lx ", i, *armRegPtr);
        crashPrint(fp, outbuf);
        if (!((i+1) % 4)) crashPrint(fp, "\n");
    }
}

/*
 * Print process and signal information.
 */
void crashPrintSignalInfo(FILE *fp, siginfo_t *sigInfo, char *outbuf)
{
    sprintf(outbuf, "si_errno:%d si_pid:%d si_code:%d si_status:%d\n\n",
            sigInfo->si_errno, sigInfo->si_pid, sigInfo->si_code, sigInfo->si_status);
    crashPrint(fp, outbuf);
}

/*
 * Write output that is safe for usage by a crash handler. printf()
 * should not be used.
 */
static void crashWrite(char *str, int bytes, int fd)
{
    int offset = 0;
    int bytesWritten;

    while (bytes > 0)
    {
        bytesWritten = write(fd, &str[offset], bytes);
        if (bytesWritten < 1) break;

        offset += bytesWritten;
        bytes -= bytesWritten;
    }
}

/*
 * Print a string from the crash handler to both stdout and a crash log file.
 * If fp is NULL, only output to stdout.
 */
static void crashPrint(FILE *fp, char *str)
{
    crashWrite(str, strlen(str), STDOUT_FILENO);

    if (fp != NULL)
    {
        crashWrite(str, strlen(str), fileno(fp));
    }
}

/*
 * Create the crash log file and write its header.
 */
FILE *crashCreateLogFile(char *outbuf)
{
    FILE *lfp;

    lfp = fopen(LOG_FS_DIR"/crashlog.txt", "w");
    if (lfp != NULL)
    {
        // Write the build info header.
        sprintf(outbuf, "Crash log for %s\n", "reconn-service");
        crashWrite(outbuf, strlen(outbuf), fileno(lfp));
    }
    else
    {
        reconnDebugPrint("%s: fopen(%s/crashlog.txt, w) failed %d (%s)\n", __FUNCTION__, LOG_FS_DIR, errno, strerror(errno));
    }

    return lfp;
}

#endif

