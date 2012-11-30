#ifndef __EQPTRSP__
#define __EQPTRSP__
#include "clientApp.h"

#define EQPT_MSG_Q_NAME     "/eqptRspQ"
#define EQPT_MSG_Q_FLAGS    0
#define EQPT_MSG_Q_SIZE     30

typedef enum
{
    ALL,
    SLAVE,
    REMOTE
}REGISTRATION_TYPE_e;


extern ReconnErrCodes reconnEqptAddMsgToQ(const char *, int);
extern ReconnErrCodes reconnDeRegisterClientApp(CLIENTCONTEXT *);
extern ReconnErrCodes reconnRegisterClientApp(CLIENTCONTEXT *);
extern int reconnClientsRegistered();
extern void *reconnEqptTask(void *);
extern int reconnClientsRegistered(REGISTRATION_TYPE_e);
extern void reconnEqptCleanUp();
extern void *reconnGetEqptResponseTask(void *);
extern void disconnectAllClients(ReconnMasterClientMode);

#endif
