// k-sr.h, 159

#ifndef __K_SR__
#define __K_SR__

#include "k-type.h"

void NewProcSR(func_p_t p);
void CheckWakeProc(void);
void TimerSR();
int GetPidSR(void);
void ShowCharSR(int row, int col, char ch);
void SleepSR(int centi_sec);
int MuxCreateSR(int flag);
void MuxOpSR(int mux_id, int opcode);
void TermSR(int term_no);
void TermTxSR(int term_no);
void TermRxSR(int term_no);
int ForkSR(void);
int WaitSR(void);
void ExitSR(int exit_code);
void ExecSR(int code, int arg);
void SignalSR(int sig_num, int handler);
void WrapperSR(int pid, int handler, int arg);
void PauseSR(void);
void KillSR(int pid, int sig_num);
unsigned RandSR(void);

#endif
