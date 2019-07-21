/*
	CSC 159, Operating System Pragmatics
	Authors: Jeff Low and Michael Chan
	Sacramento State, Spring 2019
*/
#include "k-include.h"  // SPEDE includes
#include "k-entry.h"    // entries to kernel (TimerEntry, etc.)
#include "k-type.h"     // kernel data types
#include "tools.h"      // small handy functions
#include "k-sr.h"       // kernel service routines
#include "proc.h"       // all user process code here

// kernel data are all here:
int run_pid;                                 // current running PID; if -1, none selected
q_t pid_q, mux_q, ready_q;                   // avail PID and those created/ready to run
pcb_t pcb[PROC_SIZE];                        // Process Control Blocks
char proc_stack[PROC_SIZE][PROC_STACK_SIZE]; // process runtime stacks
struct i386_gate *intr_table;                // intr table's DRAM location
int sys_centi_sec;                           // sys time in centi-sec, initilize it to 0
q_t sleep_q;                                 // sleeping proc PID's queued in here
mux_t mux[MUX_SIZE];
q_t mux_q;
int vid_mux;
term_t term[TERM_SIZE] = {
	{ TRUE, TERM0_IO_BASE },
	{ TRUE, TERM1_IO_BASE }
};
int page_user[PAGE_NUM];
unsigned rand = 0;
int kernel_main_table;

void InitKernelData(void) {                    // init kernel data
	int i,j,k;

	intr_table  = get_idt_base();                // get intr table location

	Bzero((char*)&pid_q, sizeof(q_t));           // clear 2 queues
	Bzero((char*)&ready_q, sizeof(q_t));
	Bzero((char*)&mux_q, sizeof(q_t));

	// Puts all PIDs into the PID Queue
	for (i = 0; i < Q_SIZE; i++) { EnQ(i, &pid_q); }
	// Puts all Mutex IDs into the Mutex Queue
	for (j = 0; j < MUX_SIZE; j++) { EnQ(j, &mux_q); }
    // Initializes Pages
	for (k = 0; k < PAGE_NUM; k++) { page_user[k] = NONE; }

	sys_centi_sec = 0;
	Bzero ((char*)&sleep_q, sizeof(q_t));
	run_pid = NONE;

	kernel_main_table = get_cr3();
}

void InitKernelControl(void) {      // init kernel control
	fill_gate(&intr_table[TIMER_INTR], (int)TimerEntry, get_cs(), ACC_INTR_GATE, 0);
	fill_gate(&intr_table[GETPID_CALL], (int)GetPidEntry, get_cs(), ACC_INTR_GATE, 0);
	fill_gate(&intr_table[SHOWCHAR_CALL], (int)ShowCharEntry, get_cs(), ACC_INTR_GATE, 0);
	fill_gate(&intr_table[SLEEP_CALL], (int)SleepEntry, get_cs(), ACC_INTR_GATE, 0);
	fill_gate(&intr_table[MUX_OP_CALL], (int)MuxOpEntry, get_cs(), ACC_INTR_GATE, 0);
	fill_gate(&intr_table[MUX_CREATE_CALL], (int)MuxCreateEntry, get_cs(), ACC_INTR_GATE, 0);
	fill_gate(&intr_table[TERM0_INTR], (int)Term0Entry, get_cs(), ACC_INTR_GATE, 0);
	fill_gate(&intr_table[TERM1_INTR], (int)Term1Entry, get_cs(), ACC_INTR_GATE, 0);
	fill_gate(&intr_table[FORK_CALL], (int)ForkEntry, get_cs(), ACC_INTR_GATE, 0);
	fill_gate(&intr_table[WAIT_CALL], (int)WaitEntry, get_cs(), ACC_INTR_GATE, 0);
	fill_gate(&intr_table[EXIT_CALL], (int)ExitEntry, get_cs(), ACC_INTR_GATE, 0);
	fill_gate(&intr_table[EXEC_CALL], (int)ExecEntry, get_cs(), ACC_INTR_GATE, 0);
	fill_gate(&intr_table[SIGNAL_CALL], (int)SignalEntry, get_cs(), ACC_INTR_GATE, 0);
	fill_gate(&intr_table[PAUSE_CALL], (int)PauseEntry, get_cs(), ACC_INTR_GATE, 0);
	fill_gate(&intr_table[KILL_CALL], (int)KillEntry, get_cs(), ACC_INTR_GATE, 0);
	fill_gate(&intr_table[RAND_CALL], (int)RandEntry, get_cs(), ACC_INTR_GATE, 0);
	outportb(PIC_MASK, MASK);
}

void Scheduler(void) {
	if (run_pid > 0) { return; }

	if (QisEmpty(&ready_q)) {
		run_pid = 0;
	} else {
		pcb[0].state = READY;
		run_pid = DeQ(&ready_q);
	}

	pcb[run_pid].run_count = 0; // reset run_count of selected proc
	pcb[run_pid].state = RUN;   // upgrade its state to run
}

// OS bootstrap
int main(void) {
	InitKernelData();
	InitKernelControl();
	NewProcSR(InitProc);
	Scheduler();
	set_cr3(pcb[run_pid].main_table);
	Loader(pcb[run_pid].trapframe_p);
	return 0; // statement never reached, compiler asks it for syntax
}

void Kernel(trapframe_t *trapframe_p) {
	char ch;
	pcb[run_pid].trapframe_p = trapframe_p;

	if (cons_kbhit()) {
		ch = cons_getchar();
		if (ch == 'b') { breakpoint(); }
		if (ch == 'n') {
			NewProcSR(UserProc);
			if(rand == 0) { rand = sys_centi_sec; }
		}
	}

	switch (trapframe_p->entry_id) {
	case SLEEP_CALL:
		SleepSR(trapframe_p->eax);
		break;
	case GETPID_CALL:
		trapframe_p->eax = GetPidSR();
		break;
	case SHOWCHAR_CALL:
		ShowCharSR(trapframe_p->eax, trapframe_p->ebx, trapframe_p->ecx);
		break;
	case TIMER_INTR:
		TimerSR();
		break;
	case MUX_CREATE_CALL:
		trapframe_p->eax=MuxCreateSR(trapframe_p->eax);
		break;
	case MUX_OP_CALL:
		MuxOpSR(trapframe_p->eax, trapframe_p->ebx);
		break;
	case TERM0_INTR:
		TermSR(0);
		outportb(PIC_CONTROL, TERM0_DONE);
		break;
	case TERM1_INTR:
		TermSR(1);
		outportb(PIC_CONTROL, TERM1_DONE);
		break;
	case FORK_CALL:
		trapframe_p->eax =  ForkSR();
		break;
	case WAIT_CALL:
		trapframe_p->eax = WaitSR();
		break;
	case EXIT_CALL:
		ExitSR(trapframe_p->eax);
		break;
	case EXEC_CALL:
		ExecSR(trapframe_p->eax, trapframe_p->ebx);
		break;
	case SIGNAL_CALL:
		SignalSR(trapframe_p->eax, trapframe_p->ebx);
		break;
	case PAUSE_CALL:
		PauseSR();
		break;
	case KILL_CALL:
		KillSR(trapframe_p->eax, trapframe_p->ebx);
		break;
	case RAND_CALL:
		trapframe_p->eax = RandSR();
		break;
	default:
		cons_printf("PANIC!!!\n");
		breakpoint();
	}

	Scheduler();
	set_cr3(pcb[run_pid].main_table);
	Loader(pcb[run_pid].trapframe_p); // call Loader(...)
}
