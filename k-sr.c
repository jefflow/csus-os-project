// k-sr.c, 159

#include "k-include.h"
#include "k-type.h"
#include "k-data.h"
#include "tools.h"
#include "k-sr.h"
#include "sys-call.h"
#include "proc.h"

// to create a process: alloc PID, PCB, and process stack
// build trapframe, initialize PCB, record PID to ready_q
void NewProcSR(func_p_t p) {  // arg: where process code starts
	int pid;

	if( QisEmpty(&pid_q) ) {
		cons_printf("Panic: no more process!\n");
	} else {
		pid = DeQ(&pid_q);   // alloc PID (1st is 0)

		Bzero((char *)&(pcb[pid]),sizeof(pcb_t));        // clear PCB
		Bzero(&proc_stack[pid][0],PROC_STACK_SIZE);      // clear stack

		pcb[pid].state = READY;                          // change process state

		if(pid > 0) { EnQ(pid, &ready_q); }

		pcb[pid].main_table = kernel_main_table;
		pcb[pid].trapframe_p = (trapframe_t *) &proc_stack[pid][PROC_STACK_SIZE];
		pcb[pid].trapframe_p--;                               // lower by trapframe size
		pcb[pid].trapframe_p->efl = EF_DEFAULT_VALUE|EF_INTR; // enables intr
		pcb[pid].trapframe_p->cs = get_cs();                  // dupl from CPU
		pcb[pid].trapframe_p->eip = (int)p;
	}
}

void CheckWakeProc(void) {
	int howManyProc;
	int i, spid;
	howManyProc = sleep_q.tail;

	for (i = 0; i < howManyProc; i++) {
		spid = DeQ(&sleep_q);

		if (pcb[spid].wake_centi_sec <= sys_centi_sec) {
			pcb[spid].state = READY;
			EnQ(spid, &ready_q);
		} else {
			EnQ(spid, &sleep_q);
		}
	}
}     // Change trapframe location info in the PCB of this pid.


// count run_count and switch if hitting time slice
void TimerSR() {
	outportb(PIC_CONTROL, TIMER_DONE);   // notify PIC timer done

	sys_centi_sec++;
	CheckWakeProc();

	if (run_pid == 0) {
		// Nothing
	} else {
		pcb[run_pid].run_count++;
		pcb[run_pid].total_count++;
		if (pcb[run_pid].run_count >= TIME_SLICE) {
			pcb[run_pid].state=READY;
			EnQ(run_pid, &ready_q);
			run_pid = NONE;
		}
	}
}

int GetPidSR(void) { return run_pid; }

void SleepSR(int centi_sec) {
	pcb[run_pid].wake_centi_sec = sys_centi_sec + centi_sec;
	pcb[run_pid].state = SLEEP;
	EnQ(run_pid, &sleep_q);
	run_pid = NONE;
}

void ShowCharSR(int row, int col, char ch) { // show ch at row, col
	unsigned short *p = VID_HOME;              // upper-left corner of display
	p += row * 80;
	p += col;
	*p = ch + VID_MASK;
}

int MuxCreateSR(int flag) {
	int alloc_mux = DeQ(&mux_q);
	Bzero((char*) &mux[alloc_mux], sizeof(mux_t)); // Clear Mux_q
	mux[alloc_mux].flag = flag;                    // Set the flag
	mux[alloc_mux].creator = run_pid;              // Creator = requester/owning PID; setting creator

	return alloc_mux;
}


void MuxOpSR(int mux_id, int opcode) {
	if (opcode == LOCK) {
		if (mux[mux_id].flag > 0) {
			mux[mux_id].flag--;
		} else {
			EnQ(run_pid, &mux[mux_id].suspend_q); // Queue the PID of the current proc to a suspend Q in the mutex
			pcb[run_pid].state = SUSPEND;         // Change state to SUSPEND
			run_pid = NONE;
		}
	} else if (opcode == UNLOCK) {
		if (QisEmpty(&mux[mux_id].suspend_q)) {
			mux[mux_id].flag++;
		} else {
			int released_pid = DeQ(&mux[mux_id].suspend_q);
			EnQ(released_pid, &ready_q);
			pcb[ released_pid].state = READY;
		}
	}
}

void TermSR(int term_no) {
	int type;
	type = inportb(term[term_no].io_base+IIR);
	if (type == TXRDY) {
		TermTxSR(term_no);
	} else if (type == RXRDY) {
		TermRxSR(term_no);
	}
	if (term[term_no].tx_missed == TRUE) { TermTxSR(term_no); }
}

void TermTxSR(int term_no) {
	char ch;
	if(QisEmpty(&term[term_no].out_q) && QisEmpty(&term[term_no].echo_q)) {
		term[term_no].tx_missed = TRUE;
		return;
	}
	if(!QisEmpty(&term[term_no].echo_q)) {
		ch = DeQ(&term[term_no].echo_q);
	} else {
		ch = DeQ(&term[term_no].out_q);
		MuxOpSR(term[term_no].out_mux, UNLOCK);
	}
	outportb(term[term_no].io_base+DATA, ch);
	term[term_no].tx_missed = FALSE;
}

void TermRxSR(int term_no) {
	char ch;
	int pid, device_no;
	ch = inportb(term[term_no].io_base+DATA);
	EnQ(ch, &term[term_no].echo_q);

	if(ch == '\r') { EnQ('\n', &term[term_no].echo_q); }

	if (ch == '\r') {
		EnQ('\0', &term[term_no].in_q);
	} else {
		EnQ(ch, &term[term_no].in_q);
	}

	if (term_no == 0) {
		device_no = TERM0_INTR;
	} else {
		device_no = TERM1_INTR;
	}

	if (ch == SIGINT && !QisEmpty(&mux[term[term_no].in_mux].suspend_q)) {
		pid = mux[term[term_no].in_mux].suspend_q.q[0];
		if (pcb[pid].sigint_handler > 0) {
			WrapperSR(pid, pcb[pid].sigint_handler, device_no);
		}
	}
	MuxOpSR(term[term_no].in_mux, UNLOCK);
}

int ForkSR(void) {
	int childPIDFromQ, thePPID;
	int difference;
	int *p;

	if( QisEmpty(&pid_q) ) {
		cons_printf("PID Queue is empty!");
		return NONE;
	}
	childPIDFromQ = DeQ(&pid_q);

	Bzero((char *)&(pcb[childPIDFromQ]),sizeof(pcb_t));

	pcb[childPIDFromQ].state = READY;
	pcb[childPIDFromQ].ppid = run_pid;
	thePPID = pcb[childPIDFromQ].ppid;

	EnQ(childPIDFromQ, &ready_q);

	difference = &proc_stack[childPIDFromQ][0] - &proc_stack[thePPID][0];

	pcb[childPIDFromQ].trapframe_p = (trapframe_t *)((int)pcb[thePPID].trapframe_p + difference);

	MemCpy(&proc_stack[childPIDFromQ][0], &proc_stack[thePPID][0], PROC_STACK_SIZE);

	pcb[childPIDFromQ].trapframe_p->eax = 0;
	pcb[childPIDFromQ].trapframe_p->esp += difference;
	pcb[childPIDFromQ].trapframe_p->ebp += difference;
	pcb[childPIDFromQ].trapframe_p->esi += difference;
	pcb[childPIDFromQ].trapframe_p->edi += difference;

	p = (int *) pcb[childPIDFromQ].trapframe_p->ebp;

	while (*p != 0 ) {
		*p = *p + difference;
		p = (int *)*p;
	}

	pcb[childPIDFromQ].main_table = kernel_main_table;

	return childPIDFromQ;
}

int WaitSR() {
	int i, exitCode, j;
	int ifFound;
	ifFound = FALSE;

	for (i = 0; i < PROC_SIZE; i++) {
		if (pcb[i].ppid == run_pid && pcb[i].state == ZOMBIE) {
			ifFound = TRUE;
			break;
		}
	}

	if (ifFound != TRUE) {
		pcb[run_pid].state = WAIT;
		run_pid = NONE;
		return NONE;
	}

	set_cr3(pcb[i].main_table);
	exitCode = pcb[i].trapframe_p->eax;
	set_cr3(pcb[run_pid].main_table);

	pcb[i].state = UNUSED;
	EnQ(i, &pid_q);

	for(j = 0; j < PAGE_NUM; j++) {
		if(page_user[j] == i) {
			page_user[j] = NONE;
		}
	}
	return exitCode;
}

void ExitSR(int exit_code) {
	int i;

	if(pcb[pcb[run_pid].ppid].state != WAIT) {
		pcb[run_pid].state = ZOMBIE;
		run_pid = NONE;
		return;
	}

	pcb[pcb[run_pid].ppid].state = READY;
	EnQ(pcb[run_pid].ppid, &ready_q);
	pcb[pcb[run_pid].ppid].trapframe_p->eax = exit_code;

	for(i = 0; i < PAGE_NUM; i++) {
		if(page_user[i] == run_pid) {
			page_user[i] = NONE;
		}
	}

	pcb[run_pid].state = UNUSED;
	EnQ(run_pid, &pid_q);

	set_cr3(kernel_main_table);
	run_pid = NONE;
}

void ExecSR(int code, int arg) {
	int i, j, pages[5], *p, entry;
	trapframe_t *q;
	enum {MAIN_TABLE, CODE_TABLE, STACK_TABLE, CODE_PAGE, STACK_PAGE};
	j = 0;
	for (i = 0; i < PAGE_NUM; i++) {
		if (j == 5) { break; }
		if (page_user[i] == NONE) {
			pages[j] = i;
			j++;
		}
		if(i == PAGE_NUM - 1) {
			cons_printf("PANIC!!!!");
			return;
		}
	}

	for (i = 0; i < 5; i++) { page_user[pages[i]] = run_pid; }

	pages[MAIN_TABLE] = ((pages[0] * PAGE_SIZE) + RAM);
	pages[CODE_TABLE] = ((pages[1] * PAGE_SIZE) + RAM);
	pages[STACK_TABLE] = ((pages[2] * PAGE_SIZE) + RAM);
	pages[CODE_PAGE] = ((pages[3] * PAGE_SIZE) + RAM);
	pages[STACK_PAGE] = ((pages[4] * PAGE_SIZE) + RAM);

	MemCpy((char*)pages[CODE_PAGE], (char*)code, PAGE_SIZE);
	Bzero((char*)pages[STACK_PAGE], PAGE_SIZE);

	p = (int*)((int)pages[STACK_PAGE] + PAGE_SIZE);
	p--;
	*p = arg;
	p--;
	q = (trapframe_t *)p;
	q--;
	q->efl = EF_DEFAULT_VALUE|EF_INTR; // enables intr
	q->cs = get_cs();                  // dupl from CPU
	q->eip = (unsigned int) M256;

	// step 4
	Bzero((char*)pages[MAIN_TABLE], PAGE_SIZE);
	MemCpy( (char *)pages[MAIN_TABLE], (char *)kernel_main_table, sizeof(int[4]) );
	entry = M256 >> 22;
	*((int*)pages[MAIN_TABLE] + entry) = pages[CODE_TABLE] | PRESENT | RW;
	entry = G1_1 >> 22;
	*((int*)pages[MAIN_TABLE] + entry) = pages[STACK_TABLE] | PRESENT | RW;

	// Step 5
	Bzero((char*)pages[CODE_TABLE], PAGE_SIZE);
	entry = M256 & MASK10 >> 12;
	*((int*)pages[CODE_TABLE] + entry) = pages[CODE_PAGE] | PRESENT | RW;

	// Step 6
	Bzero((char*)pages[STACK_TABLE], PAGE_SIZE);
	entry = G1_1 & MASK10 >> 12;
	*((int*)pages[STACK_TABLE] + entry) = pages[STACK_PAGE] | PRESENT | RW;

	// Step 7
	pcb[run_pid].main_table = pages[MAIN_TABLE];
	pcb[run_pid].trapframe_p = (trapframe_t*)V_TF;
}

void SignalSR(int sig_num, int handler) {
	pcb[run_pid].sigint_handler = handler;
}

void WrapperSR(int pid, int handler, int arg) {

	int *addr;

	addr = (int *)((int)pcb[pid].trapframe_p + sizeof(trapframe_t));

	MemCpy((char*)((int)pcb[pid].trapframe_p - sizeof(int[3])), (char*)(pcb[pid].trapframe_p), sizeof(trapframe_t));

	pcb[pid].trapframe_p = (trapframe_t *)((int)pcb[pid].trapframe_p-sizeof(int[3]));

	addr--;
	*addr = arg;
	addr--;
	*addr = handler;
	addr--;
	*addr = pcb[pid].trapframe_p->eip;

	pcb[pid].trapframe_p->eip = (int)Wrapper;
}

void PauseSR(void) {
	pcb[run_pid].state = PAUSE;
	run_pid = NONE;
}

void KillSR(int pid, int sig_num) {
	int i;
	if (pid == 0) {
		for (i = 0; i < PROC_SIZE; i++) {
			if ( pcb[i].state == PAUSE && pcb[i].ppid == run_pid) {
				pcb[i].state = READY;
				EnQ(i, &ready_q);
			}
		}
	}
}

unsigned RandSR(void) {
	rand = run_pid * rand + A_PRIME;
	rand = rand % G2;
	return rand;
}
