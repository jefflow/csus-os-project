// sys-call.h, 159

#ifndef __SYS_CALL__
#define __SYS_CALL__

#include "k-const.h"

int GetPidCall(void);
void ShowCharCall(int, int, char);
void SleepCall(int);
int MuxCreateCall(int);
void MuxOpCall(int, int);
void WriteCall(int, char *);
void ReadCall(int, char *);
int ForkCall(void);
int WaitCall(void);
void ExitCall(int);
void ExecCall(int, int);
void SignalCall(int, int);
void PauseCall(void);
void KillCall(int pid, int sig_num);
unsigned RandCall(void);

#endif
