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

static int mySocketFd;

extern int batteryPercentage;
extern int chargerAttached;

void *debugMenuTask(void *argument) 
{
    int connected = TRUE;
    char buff[50];
    char txBuff[5];
    FILE *cpldFd;
    int in_socket_fd; /* Incoming socket file descriptor for socket 4083     */
    int intport, menuItem;
    unsigned client_len;
    struct sockaddr_in server_addr, client_addr;

    /* Create the incoming (server) socket */
    if((in_socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    {
        printf("Server Failed to initialize the incoming socket\n");
        exit (1);
    }
    bzero((unsigned char *) &server_addr, sizeof(server_addr));
    /* bind the socket */
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    intport = 4083;
    server_addr.sin_port = htons(intport);

    if (bind(in_socket_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) 
    {
        printf("Server Failed to bind the socket\n");
        exit (0);
    }
    while (1) 
    {
        /* pend on the incoming socket */
        printf("\n\n%s: Listening\n\n", __FILE__);
        listen(in_socket_fd, 5);
        client_len = sizeof(client_addr);
        /* sit here and wait for a new connection request */
        if((mySocketFd = accept(in_socket_fd, (struct sockaddr *) &client_addr, &client_len)) < 0)
        {
            printf("%s: Failed to open a new Client socket %d %s.\n", __FUNCTION__, errno , strerror(errno));
            /* place code here to recover from bad socket accept */
            continue;
            //exit (1);
        }
        if (mySocketFd != 0) 
        {
            printf("%s: newSocketFd = %d\r\n", __FUNCTION__, mySocketFd);
            while(connected)
            {
                memset(&buff, 0, 50);
                memset(&txBuff, 0, 5);

                send((int )mySocketFd, "Debug Menu\n", 11, 0);
                send((int )mySocketFd, "__________\n", 11, 0);
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

                send((int )mySocketFd, "Select and menu option > ", 25, 0);
                recv((int)mySocketFd, &buff, 50, 0 );

                menuItem = atoi(&(buff[0]));
                switch(menuItem)
                {
                    case 1:
                    {
                        if((cpldFd = fopen("/proc/cpld/CPLD_Tx_DIR_setbit", "w")) == NULL)
                        {
                            printf("%s: Could not open /proc/cpld/CPLD_Tx_setbit %d(%s)\n", __FUNCTION__, errno, strerror(errno));
                            sprintf((char *)&txBuff, "%x", 3);
                            printf("%s:  %s\n", __FUNCTION__, txBuff);
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
                            sprintf((char *)&txBuff, "%x", 0);
                            printf("%s:  %s\n", __FUNCTION__, txBuff);
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
                            sprintf((char *)&txBuff, "%x", 0);
                            printf("%s:  %s\n", __FUNCTION__, txBuff);
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
                    default:
                    {
                        write(mySocketFd, "Invalid option\n", 16);
                    }
                }
            }
        }
    }
}
