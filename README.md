
# CSC 159, Operating System Pragmatics (Spring 2019)
# OS Project for Sacramento State
The goal for this project was to demonstrate and code key features of an Operating System like virtual memory, system calls, basic device drivers, interprocess communication mechanisms like mutex locks, `exec()`, and signals.

This OS was developed on the SPEDE (System Programmer's Educational Development Environment) framework, built by former professors at Sacramento State. This framework provides an environment to build an OS executable image and download it to an x86 target machine, which the OS will run on. 

The project had two team members and was broken into a total of 9 phases. Each phase built on top of each other and was assigned through out the semester.

# Warning for future 159 students:
**Do not** copy my code or anyone else's code on Github. The professor will know it's old code and it will not work with your variation of the OS project. Also, you'll understand the project more if you don't copy other people's code ;)

# Brief overview of key functions in this project

## OS Bootstrap
### Overview
The OS contains a simple bootstrap that calls these functions in the following order: 

1. Calls `InitKernelData()` and `InitKernelControl()` to initialize the kernel.
2. Calls `NewProcSR()` to create the `init` process.
3. Calls `Scheduler()` to run the OS scheduler.
4. Calls `set_cr3()` to initialize the main table.
5. Calls   `Loader()` to load the assembly functions.

### Related Functions
**Located in `main.c`**
```
int main(void)
```

## Process Creation

### Overview
This function creates a new process. Create a new process involves allocating a new PID from the `pid_q`, PCB, stack, page table, and trapframe.
### Related Functions
**Located in `k-sr.c`**
```
void NewProcSR(func_p_t p)
```

## Mutex Locks

### Overview
The OS has the capability of the creation and manipulation of a mutex lock.  `MuxCreateCall()` leads to `MuxCreateSR()` which allocates a mutex from the Operating System's mutex queue `mux_q`, empties the mutex queue, and more. `MuxOpCall()` leads to `MuxOpSR()` and if the given opcode is `LOCK` or `UNLOCK`, `MuxOpSR` will do the appropriate actions to handle the function call.
### Related Functions
**Located in `sys-call.c`**
```
int MuxCreateCall(int flag)
void MuxOpCall(int mux_id, int opcode)
```

**Located in `k-sr.c`**
```
int MuxCreateSR(int flag)
void MuxOpSR(int mux_id, int opcode)
```


## `Bzero()`, `MemCpy()` and Queue Functions

### Overview
For our project, we reimplemented the standard `Bzero()` and `MemCpy()` that will work with this OS. We also implemented key functions for a simple queue like enqueue, dequeue, checking if the queue is either empty or full. The queues were responsible for allocating and deallocating PID's, ready queues, wait queues, sleep queues, and much more.  

### Related Functions
**Located in `tools.c`**
```
void Bzero(char *p, int bytes)
void MemCpy(char *dst, char *src, int size)
int QisEmpty(q_t *p)
int QisFull(q_t *p)
int DeQ(q_t *p)
void EnQ(int to_add, q_t *p)
```
