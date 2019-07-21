// k-data.h, 159
// kernel data are all declared in main.c during bootstrap
// kernel .c code reference them as 'extern'

#ifndef __K_DATA__
#define __K_DATA__

#include "k-const.h"                                // defines PROC_SIZE, PROC_STACK_SIZE
#include "k-type.h"                                 // defines q_t, pcb_t, ...

extern int run_pid;                                 // PID of running process
extern q_t pid_q;
extern q_t ready_q;                                 // avail PID and those created/ready to run
extern pcb_t pcb[PROC_SIZE];                        // Process Control Blocks
extern char proc_stack[PROC_SIZE][PROC_STACK_SIZE]; // process runtime stacks
extern struct i386_gate *intr_table;                // intr table's DRAM location
extern int sys_centi_sec;
extern q_t sleep_q;
extern mux_t mux[MUX_SIZE];                         // kernel has these mutexes to spare
extern q_t mux_q;                                   // mutex ID's are initially queued here
extern int vid_mux;                                 // for video access control
extern term_t term[TERM_SIZE];
extern int page_user[PAGE_NUM];
extern unsigned int rand;
extern int kernel_main_table;

#endif
