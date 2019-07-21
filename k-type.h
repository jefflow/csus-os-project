// k-type.h, 159

#ifndef __K_TYPE__
#define __K_TYPE__

#include "k-const.h"

typedef void (*func_p_t)(void);

typedef enum {UNUSED, READY, RUN, SLEEP, SUSPEND, ZOMBIE, WAIT, PAUSE} state_t;

typedef struct {
	unsigned int edi, esi, ebp, esp, ebx, edx, ecx, eax,
	             entry_id,
	             eip, cs, efl;
} trapframe_t;

typedef struct {
	state_t state;
	int run_count;
	int total_count;
	int wake_centi_sec;
	trapframe_t* trapframe_p;
	int ppid;
	int sigint_handler;
	int main_table;
} pcb_t;

typedef struct {   // generic queue type
	int tail;
	int q[Q_SIZE];   // for a simple queue
} q_t;

typedef struct {
	int flag;        // max # of processes to enter
	int creator;     // requester/owning PID
	q_t suspend_q;   // suspended PID's
} mux_t;

typedef struct {
	int tx_missed,  // when initialized or after output last char
	    io_base,    // terminal port I/O base #
	    out_mux;    // flow-control mux
	q_t out_q;      // characters to send to terminal buffered here
	q_t in_q;       // to buffer terminal KB input
	q_t echo_q;     // to echo back to terminal
	int in_mux;     // to flow-control in_q
} term_t;

typedef void (*func_p_t2)(int);

#endif
