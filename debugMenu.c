#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>

#include "reconn.h"
#include "debugMenu.h"

static int mySocketFd;

extern int batteryPercentage;
extern int chargerAttached;
static int debugSocketFd;

static void debugCleanUp()
{
    printf("%s: **** Called\n", __FUNCTION__);
    close(debugSocketFd);
}

void *debugMenuTask(void *argument) 
{
    int connected = TRUE;
    char buff[DEBUG_INPUT_LEN];
    char txBuff[5];
    FILE *cpldFd;
    int debugSocketFd; /* Incoming socket file descriptor for socket 4083     */
    int debugPort, menuItem, retCode, optval = 1;
    unsigned client_len;
    struct sockaddr_in server_addr, client_addr;

    atexit(debugCleanUp);
    /* Create the incoming (server) socket */
    if((debugSocketFd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    {
        printf("%s: Server Failed to initialize the incoming socket %d(%s)\n", __FUNCTION__, errno, strerror(errno));
        exit (1);
    }
    if(setsockopt(debugSocketFd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
    {
        printf("%s: setsockopt failed %d(%s)\n", __FUNCTION__, errno, strerror(errno));
    }

    bzero((unsigned char *) &server_addr, sizeof(server_addr));
    /* bind the socket */
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    debugPort = 4083;
    server_addr.sin_port = htons(debugPort);

    if (bind(debugSocketFd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) 
    {
        printf("%s: Server Failed to bind the socket\n", __FUNCTION__);
        exit (0);
    }
    while (1) 
    {
        /* pend on the incoming socket */
        printf("\n\n%s: Listening\n\n", __FILE__);
        if(listen(debugSocketFd, 5) == 0)
        {
            client_len = sizeof(client_addr);
            /* sit here and wait for a new connection request */
            if((mySocketFd = accept(debugSocketFd, (struct sockaddr *) &client_addr, &client_len)) < 0)
            {
                printf("%s: Failed to open a new Client socket %d %s.\n", __FUNCTION__, errno , strerror(errno));
                continue;
            }
            if (mySocketFd != 0) 
            {
                printf("%s: newSocketFd = %d\r\n", __FUNCTION__, mySocketFd);

                send((int )mySocketFd, "Enter debug password ", 21, 0);
                memset(buff, 0, DEBUG_INPUT_LEN);
                if((retCode = recv((int)mySocketFd, &buff, DEBUG_INPUT_LEN, 0)) <= 0)
                {
                    printf("%s: recv returned %d\n", __FUNCTION__, retCode);
                    close(mySocketFd);
                }
                else 
                {
                    if(strcmp(DEBUG_PASSWORD, buff) == 0)
                    {
                        connected = TRUE;
                        while(connected)
                        {
                            memset(&buff, 0, DEBUG_INPUT_LEN);
                            memset(&txBuff, 0, 5);

                            send((int )mySocketFd, "Debug Menu\n", 11, 0);
                            send((int )mySocketFd, "___________________\n", 20, 0);
                            send((int )mySocketFd, "1. Set GPIO 156\n", 17, 0);
                            send((int )mySocketFd, "2. Set GPIO 159\n", 17, 0);
                            send((int )mySocketFd, "3. Set GPIO 140\n", 17, 0);
                            send((int )mySocketFd, "4. Set LED Green\n", 18, 0);
                            send((int )mySocketFd, "5. Set LED Red\n", 16, 0);
                            send((int )mySocketFd, "6. Set Battery Charge Percentage < 5\n", 38, 0);
                            send((int )mySocketFd, "7. Set Battery Charge Percentage between 5 and 10%\n", 52, 0);
                            send((int )mySocketFd, "8. Set Battery Charge Percentage between 11 and 99%\n", 53, 0);
                            send((int )mySocketFd, "9. Set Battery Charge Percentage 100%\n", 39, 0);
                            send((int )mySocketFd, "10. Set Battery Charger Attached\n", 35, 0);
                            send((int )mySocketFd, "11. Set Battery Charger not Attached\n", 38, 0);
                            send((int )mySocketFd, "12. Quit and exit\n", 19, 0);

                            send((int )mySocketFd, "Select and menu option > ", 25, 0);
                            if((retCode = recv((int)mySocketFd, &buff, DEBUG_INPUT_LEN, 0)) <= 0)
                            {
                                printf("%s: recv returned %d\n", __FUNCTION__, retCode);
                                close(mySocketFd);
                                break;
                            }
                            menuItem = atoi(&(buff[0]));
                            switch(menuItem)
                            {
                                case 1:
                                {
                                    if((cpldFd = fopen("/proc/cpld/CPLD_Tx_DIR_setbit", "w")) == NULL)
                                    {
                                        printf("%s: Could not open /proc/cpld/CPLD_Tx_setbit %d(%s)\n", __FUNCTION__, errno, strerror(errno));
                                    }
                                    else
                                    {
                                        printf("%s: writing to GPIO 156 via /CPLD_Tx_setbit\n", __FUNCTION__);
                                        sprintf((char *)&txBuff, "%x", 8);
                                        printf("%s", txBuff);
                                        fputs(txBuff, cpldFd);
                                        fclose(cpldFd);
                                    }
                                    break;
                                }
                                case 2:
                                {
                                    if((cpldFd = fopen("/proc/cpld/CPLD_Hx_DIR_setbit", "w")) == NULL)
                                    {
                                        printf("%s: Could not open /proc/cpld/CPLD_Tx_setbit %d(%s)\n", __FUNCTION__, errno, strerror(errno));
                                    }
                                    else
                                    {
                                        printf("%s: writing to GPIO 159 via /CPLD_Tx_setbit\n", __FUNCTION__);
                                        sprintf((char *)&txBuff, "%x", 3);
                                        printf("%s", txBuff);
                                        fputs(txBuff, cpldFd);
                                        fclose(cpldFd);
                                    }
                                    break;
                                }
                                case 3:
                                {
                                    if((cpldFd = fopen("/proc/cpld/CPLD_Wx_DIR_setbit", "w")) == NULL)
                                    {
                                        printf("%s: Could not open /proc/cpld/CPLD_Wx_setbit %d(%s)\n", __FUNCTION__, errno, strerror(errno));
                                    }
                                    else
                                    {
                                        printf("%s: writing to GPIO 140 via /CPLD_Wx_setbit\n", __FUNCTION__);
                                        sprintf((char *)&txBuff, "%x", 3);
                                        printf("%s", txBuff);
                                        write(mySocketFd, 0x0, 1);
                                        fclose(cpldFd);
                                    }
                                    break;
                                }
                                case 6:
                                case 7:
                                case 8:
                                case 9:
                                {
                                    if(menuItem == 6)
                                    {
                                        batteryPercentage = 3;
                                    }
                                    else if(menuItem == 7)
                                    {
                                        batteryPercentage = 8;
                                    }
                                    else if(menuItem == 8)
                                    {
                                        batteryPercentage = 50;
                                    }
                                    else
                                    {
                                        batteryPercentage = 100;
                                    }
                                    break;
                                }
                                case 10:
                                {
                                    chargerAttached = TRUE;
                                    break;
                                }
                                case 11:
                                {
                                    chargerAttached = FALSE;
                                    break;
                                }
                                case 12:
                                {
                                    connected = FALSE;
                                    close(mySocketFd);
                                    break;
                                }
                                default:
                                {
                                    write(mySocketFd, "Invalid option\n", 16);
                                }
                            }
                        }
                    }
                    else
                    {
                        send((int )mySocketFd, "Invalid Password entered\n", 26, 0);
                        if(close(mySocketFd) != 0)
                        {
                            printf("%s: close returned  =%d(%s) \n", __FUNCTION__, errno, strerror(errno));
                        }
                    }
                }
            }
        }
        else
        {
            printf("%s: listen returned  =%d(%s) \n", __FUNCTION__, errno, strerror(errno));
            close(debugSocketFd);
            break;
        }
    }
    printf("%s: Exiting *******************\n",__FUNCTION__);
    exit (0);
}
