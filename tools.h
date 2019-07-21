// k-lib.h, 159

#ifndef __K_LIB__
#define __K_LIB__


#include "k-type.h"

void Bzero(char *p, int bytes);     // prototype those in k-lib.c here
int QisEmpty(q_t *p);
int QisFull(q_t *p);
int DeQ(q_t *p);
void EnQ(int,q_t *);
void MemCpy(char *dst, char *src, int size);
int StrCmp (char *str1, char *str2);
void Itoa(char *str, int x);

#endif
