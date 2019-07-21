// sys-call.c
// calls to kernel services

// 159, theOS

#include "k-const.h"
#include "k-data.h"
#include "tools.h"


int GetPidCall(void) {
	int pid;

	asm ("int %1;             // interrupt!
        movl %%eax, %0"     // after, copy eax to variable 'pid'
	     : "=g" (pid)         // output
	     : "g" (GETPID_CALL)  // input
	     : "eax"              // used registers
	     );

	return pid;
}

void ShowCharCall(int row, int col, char ch) {
	asm ("movl %0, %%eax;     // send in row via eax
        movl %1, %%ebx;     // send in col via ebx
        movl %2, %%ecx;     // send in ch via ecx
        int %3"             // initiate call, %3 gets entry_id
	     :                    // no output
	     : "g" (row), "g" (col), "g" (ch), "g" (SHOWCHAR_CALL)
	     : "eax", "ebx", "ecx" //...         // affected/used registers
	     );
}

// # of 1/100 of a second to sleep
void SleepCall(int centi_sec) {
	asm ("movl %0, %%eax;
      int %1"
	     :
	     : "g" (centi_sec), "g" (SLEEP_CALL)
	     : "eax"
	     );
}

// Leads to MuxCreateSR(), returns mutex ID via trapframe.
int MuxCreateCall(int flag) {
	int mutex_id;

	asm ("movl %2, %%eax;
      int %1;
      movl %%eax, %0"
	     : "=g" (mutex_id)
	     : "g" (MUX_CREATE_CALL), "g" (flag)
	     : "eax"
	     );

	return mutex_id;
}

void MuxOpCall(int mux_id, int opcode) {
	asm ("movl %0, %%eax;     // send in mux_id via eax
        movl %1, %%ebx;     // send in opcode via ebx
        int  %2"
	     :                    // no output
	     : "g" (mux_id), "g" (opcode), "g" (MUX_OP_CALL)
	     : "eax", "ebx" //...         // affected/used registers
	     );
}

void WriteCall(int device, char *str) {
	int row, col;
	row = GetPidCall(); // get PID via syscall
	col = 0;            // init col = 0

	if(device == STDOUT) {
		while (*str != '\0') {
			ShowCharCall(row, col, *str);
			str++;
			col++;
		}
	} else {
		int term_no;

		if (device == TERM0_INTR) {
			term_no = 0;
		} else {
			term_no = 1;
		}
		while (*str != '\0') {
			MuxOpCall(term[term_no].out_mux, LOCK);
			EnQ((int)*str, &term[term_no].out_q);

			if (device == TERM0_INTR) {
				asm ("int $35");
			} else {
				asm ("int $36");
			}

			str++;
			col++;
		}
	}
}

void ReadCall(int device, char *str) {
	int term_no;
	int numOfChar = 0;
	char ch;

	if (device == TERM0_INTR) {
		term_no = 0;
	} else {
		term_no = 1;
	}

	while (1) {
		MuxOpCall(term[term_no].in_mux, LOCK);
		ch = DeQ(&term[term_no].in_q);
		*str = ch;

		if (ch == '\0') { return; }

		str++;
		numOfChar++;

		if (numOfChar == STR_SIZE) {
			*str = '\0';
			return;
		}
	}
}

int ForkCall(void) {
	int newChildPID;

	asm ("int %1;
        movl %%eax, %0"
	     : "=g" (newChildPID) // output
	     : "g" (FORK_CALL)    // input
	     : "eax"              // used registers
	     );

	return newChildPID;
}

int WaitCall(void) {
	int exitCode;

	asm ("int %1;             // interrupt!
        movl %%eax, %0"     // after, copy eax to variable 'pid'
	     : "=g" (exitCode)    // output
	     : "g" (WAIT_CALL)    // input
	     : "eax"              // used registers
	     );

	return exitCode;
}

void ExitCall(int exitCode) {
	asm ("movl %0, %%eax;
        int %1"
	     :                    // after, copy eax to variable 'pid'
	     : "g" (exitCode),    // output
	     "g" (EXIT_CALL)      // input
	     : "eax"              // used registers
	     );
}

void ExecCall(int code, int arg) {
	asm ("movl %0, %%eax;     // send in code via eax
        movl %1, %%ebx;     // send in arg via ebx
        int  %2"
	     :                    // no output
	     : "g" (code), "g" (arg), "g" (EXEC_CALL)
	     : "eax", "ebx"       // affected/used registers
	     );
}

void SignalCall(int sig_num, int handler) {
	asm ("movl %0, %%eax;     // send in code via eax
        movl %1, %%ebx;     // send in arg via ebx
        int  %2"
	     :                    // no output
	     : "g" (sig_num), "g" (handler), "g" (SIGNAL_CALL)
	     : "eax", "ebx"       // affected/used registers
	     );
}

void PauseCall(void) {
	asm ("int %0;             // interrupt!
        "                   // interrupt!
	     :                    // output
	     : "g" (PAUSE_CALL)   // input
	     : "eax"              // used registers
	     );
}

void KillCall(int pid, int sig_num) {
	asm ("movl %0, %%eax;     // send in code via eax
        movl %1, %%ebx;     // send in arg via ebx
        int  %2"
	     :                    // no output
	     : "g" (pid), "g" (sig_num), "g" (KILL_CALL)
	     : "eax", "ebx"       // affected/used registers
	     );
}

unsigned RandCall(void) {
	unsigned rand;
	asm ("int %1;             // interrupt!
        movl %%eax, %0"     // after, copy eax to variable 'pid'
	     : "=g" (rand)                 // output
	     : "g" (RAND_CALL) // input
	     : "eax"            // used registers
	     );
	return rand;
}
