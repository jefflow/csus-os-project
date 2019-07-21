// proc.h, 159

#ifndef _PROC_H_
#define _PROC_H_

void InitTerm(int term_no);
void InitProc(void);  // prototype those in proc.c here
void UserProc(void);
void Aout(int device);
void Ouch(int device);
void Wrapper(int handler, int arg);

#endif
