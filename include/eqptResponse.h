#ifndef __EQPTRSP__
#define __EQPTRSP__

#define EQPT_MSG_Q_NAME     "/eqptRspQ"
#define EPQT_MSG_Q_FLAGS    0
#define EPQT_MSG_Q_SIZE     20


extern ReconnErrCodes reconnEqptAddMsgToQ(const char *, int);
extern ReconnErrCodes reconnDeRegisterClientApp(short);
extern ReconnErrCodes reconnRegisterClientApp(short, int);
extern int reconnClientsRegistered();
extern void *reconnEqptTask(void *);
extern int reconnClientsRegistered(void);

#endif
