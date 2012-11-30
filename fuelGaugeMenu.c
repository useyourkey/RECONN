#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>
#include <net/if.h>

#include "debugMenu.h"
#include "reconn.h"
#include "socket.h"
#include "fuel_gauge.h"
#include "clientApp.h"
#include "upgrade.h"
#include "eqptResponse.h"
#include "powerMgmt.h"
#include "gpio.h"

extern void reconnMasterIphone();
extern int theDebugSocketFd;
int fuelModelTaskDone = TRUE;
YESNO gSimulateLowBattery = NO;

static struct timeval fuelModelTime;

typedef struct{
    int theDebugSocket;
    int theSleepTime;
}FUEL_MODEL_STRUCT;

static FUEL_MODEL_STRUCT theFuelModelParams;
static pthread_t fuelModelTaskId = 0;

static char *percentageMessage = "\r\n\r\nBattery charge is at %d%%\r\n";
static char *voltageMessage = "\r\n\r\nBattery voltage is %2.4fVdc\r\n";
static char *fuelModelQuestion = "\r\n\r\nEnter the minutes to wait between readings or 'q' to quit > ";
static char *invalidMsg = "\r\n\r\n*** Invalid. Must be a number.\r\n";

static int fuelPercent();
static int powerDown();
static int chargerStatus();
static int batteryVoltage();
static int batteryModel();
static void *fuelModelTask(void *);
extern short gBatteryPercentage;
extern float gBatteryVoltage;

debugMenuStruct fuelGaugeDebugMenu[] =
{
    {"fuel", "Fuel gauge debug Menus", NULL, NULL, NULL},
    {NULL, NULL, "percent", "Returns the percentage of battery charge", fuelPercent},
    {NULL, NULL, "voltage", "Returns the battery voltage", batteryVoltage},
    {NULL, NULL, "charger", "Returns the charger status", chargerStatus},
    {NULL, NULL, "power",   "Simulates < 5% battery system power down", powerDown},
    {NULL, NULL, "model",   "Used to model battery charge/discharge characteristics.\r\n\t\t    Enter q to stop modeling", batteryModel}
};

void fuelModelCleanUp()
{
    reconnDebugPrint("%s: Function Entered\n", __FUNCTION__);
    if(fuelModelTaskId) 
    {
        if(pthread_cancel(fuelModelTaskId) != 0)
        {
            reconnDebugPrint("%s: pthread_cancel() failed %d (%s)", __FUNCTION__, errno, strerror(errno));
        }
        else
        {
            reconnDebugPrint("%s: Canceled thread %u\n", __FUNCTION__, fuelModelTaskId);
        }
    }
    fuelModelTaskId = 0;
}
static void *fuelModelTask(void *args)
{
    char *timeStampBuf;
    char outputString[FUEL_MODEL_SIZE];
    struct tm *theDate;
    time_t theSeconds;
    static int state = 0;

    UNUSED_PARAM(args);

    reconnDebugPrint("%s: **** Task Started\n", __FUNCTION__);
    while((theDebugSocketFd !=-1) && (fuelModelTaskDone == FALSE))
    {
        memset(outputString, 0, FUEL_MODEL_SIZE);
        gettimeofday(&fuelModelTime, NULL);
        theSeconds = fuelModelTime.tv_sec;
        theDate = localtime(&theSeconds);
        timeStampBuf = asctime(theDate);

        /*
         * remove the trailing carriage return
         */
        strncpy((char *)&outputString, timeStampBuf, strlen(timeStampBuf) -1);
        strcat((char *)&outputString, ",");
        sprintf(&outputString[strlen(outputString)],"%d",  gBatteryPercentage); 
        strcat((char *)&outputString, ",");
        sprintf(&outputString[strlen(outputString)], "%f", gBatteryVoltage); 
        strcat((char *)&outputString, "\n");
        sendSocket(theFuelModelParams.theDebugSocket, (unsigned char *)&outputString, 
                strlen((char *)&outputString), 0);
        sleep(theFuelModelParams.theSleepTime);
        resetPowerStandbyCounter(RESET_SYSTEM_SHUTDOWN_TIME);
    }
    reconnDebugPrint("%s: **** Task Exiting\n", __FUNCTION__);
    return &state;
}
void registerFuelGaugeDebugMenu()
{
    registerDebugCommand(&fuelGaugeDebugMenu[0], sizeof(fuelGaugeDebugMenu)/sizeof(debugMenuStruct));
}

static int fuelPercent(int theSocketFd)
{
    // Add 3 to account for the percentage number (0 - 100)
    char buf[strlen(percentageMessage+3)];

    memset(buf, 0, strlen(percentageMessage+3));
    sprintf(buf, percentageMessage, gBatteryPercentage);
    sendSocket(theSocketFd, (unsigned char *)&buf, strlen(buf), 0);
    return RECONN_SUCCESS;
}

static int chargerStatus(int theSocketFd)
{
    char buf[30];
    short theValue;
    extern char gChargerState;

    memset(buf, 0, 30);
    if((gChargerState == ATTACHED))
    {
        strcat((char *)buf, "\r\nATTACHED ");
        if(reconnGpioAction(CHARGE_DISABLE_GPIO, READ, &theValue) == RECONN_SUCCESS)
        {
            //reconnDebugPrint("%s: CHARGE_DISABLE_GPIO = %d\n", __FUNCTION__, theValue);
            if(theValue == CHARGE_ENABLED)
            {
                strcat((char *)&buf, "(CHARGING)\r\n");
            }
            else
            {
                strcat((char *)&buf, "(NOT CHARGING)\r\n");
            }
        }
    }
    else
    {
        sprintf((char *)&buf, "\r\nNOT ATTACHED\r\n");
    }
    sendSocket(theSocketFd, (unsigned char *)&buf, strlen(buf), 0);
    return RECONN_SUCCESS;
}

static int batteryVoltage(int theSocketFd)
{
    /*
     * Voltage is between 0 and 10
     * 7 to account for the actual voltage max (10.0000)
     */
    char buf[strlen(voltageMessage)+10];

    memset(buf, 0, strlen(voltageMessage)+10);
    sprintf(buf, voltageMessage, gBatteryVoltage);
    sendSocket(theSocketFd, (unsigned char *)&buf, strlen(buf), 0);
    return RECONN_SUCCESS;
}
static int powerDown(int theSocketFd)
{
    UNUSED_PARAM(theSocketFd);
    gSimulateLowBattery = YES;
    return RECONN_SUCCESS;
}
static int batteryModel(int theSocketFd)
{
    char *theInput, *tmpPtr;
    int isDigit;

    theFuelModelParams.theDebugSocket = theSocketFd;
    while(1)
    {
       isDigit = FALSE;
        sendSocket(theSocketFd, (unsigned char *)fuelModelQuestion, strlen(fuelModelQuestion), 0); 
        theInput = getInput(theSocketFd, YES);
        if(theInput == NULL)
        {
            /*
             * If getInput returns null then the socket has disconnected. This debug session
             * is over.
             */
            timeoutDebugMenus = fuelModelTaskDone = TRUE;
            return RECONN_DEBUG_DONE;
        }
        else if(*theInput == 'q')
        {
            free(theInput);
            return RECONN_SUCCESS;
        }
        else
        {
            tmpPtr = theInput;
            /*
             * See if the user entered a number
             */
            while(*tmpPtr) 
            {
                if(!isdigit(*tmpPtr))
                {
                    isDigit = FALSE;
                    break;
                }
                else
                {
                    isDigit = TRUE;
                }
                tmpPtr++;
            }
            if(isDigit == TRUE)
            {
                /*
                 * change the number to minutes.
                 */
                theFuelModelParams.theSleepTime = (atoi(theInput) * 60);
                break;
            }
            else
            {
                sendSocket(theSocketFd, (unsigned char *)invalidMsg, strlen(invalidMsg), 0);
            }
        }
    }
    free(theInput);
    fuelModelTaskDone = FALSE;
    if(pthread_create(&fuelModelTaskId, NULL, fuelModelTask, (void *) &theSocketFd) < 0)
    {
        reconnDebugPrint("%s: Could not start fuelModelTask, %d %s\n", __FUNCTION__, errno, strerror(errno));
        timeoutDebugMenus = fuelModelTaskDone = TRUE;
        return RECONN_FAILURE;
    }
    reconnDebugPrint("%s: started fuelModelTask with ID = %u\n", __FUNCTION__, fuelModelTaskId);
    timeoutDebugMenus = FALSE;

    while(1)
    {
        theInput = getInput(theSocketFd, NO);
        if(theInput == NULL)
        {
            timeoutDebugMenus = fuelModelTaskDone = TRUE;
            free(theInput);
            return RECONN_DEBUG_DONE;
        }
        else if(*theInput != 'q')
        {
            sendSocket(theSocketFd, (unsigned char *)"\r\nq to quit\r\n",13, 0);
            free(theInput);
            continue;
        }
        free(theInput);
        fuelModelCleanUp();
        timeoutDebugMenus = fuelModelTaskDone = TRUE;
        break;
    }
    return RECONN_SUCCESS;
}

