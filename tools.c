// k-lib.c, 159

#include "k-include.h"
#include "k-type.h"
#include "k-data.h"

// clear DRAM data block, zero-fill it
void Bzero(char *p, int bytes) {
	int i;
	for (i = 0; i < bytes; i++) {
		*p ^= *p;
		p++;
	}
}

// Checks if the queue is empty.
// Returns 1 if it's empty, returns 0 if it's not.
int QisEmpty(q_t *p) {
	if ((p->tail) == 0 ) {
		return 1;
	} else {
		return 0;
	}
}

// Checks if the queue is full.
// Returns 1 if it's full, returns 0 if it's not.
int QisFull(q_t *p) {
	if (p->tail == Q_SIZE) return 1;
	else return 0;
}

// dequeue, 1st # in queue; if queue empty, return -1
// move rest to front by a notch, set empty spaces -1
int DeQ(q_t *p) { // return -1 if q[] is empty
	int i, to_return;
	if (QisEmpty(p)) {
		cons_printf("Panic: queue is empty!\n");
		return -1;
	}

	p->tail--;
	to_return = p->q[0];

	for (i = 0; i < p->tail; i++) { p->q[i] = p->q[i+1]; }
	p->q[p->tail] = -1;  // eof

	return to_return;

}

// if not full, enqueue # to tail slot in queue
void EnQ(int to_add, q_t *p) {
	if (QisFull(p)) { cons_printf("Panic: queue is full, cannot EnQ!\n"); }
	p->q[p->tail] = to_add;
	p->tail = p->tail++;
}

void MemCpy(char *dst, char *src, int size) {
	int i;
	for(i = 0; i < size; i++) { dst[i] = src[i]; }
}

int StrCmp(char *str1, char *str2){
	while (*str1 == *str2) {
		if (*str1 == '\0' || *str2 == '\0') { break; }
		str1++;
		str2++;
	}
	if (*str1 == '\0' && *str2 == '\0') {
		return TRUE;
	} else {
		return FALSE;
	}
}

void Itoa(char *str, int x) {
	char ch;
	int i, j;
	char *dest = str;
	char dummyStr[STR_SIZE];
	if (x == 0) {
		str[0] = '0';
		str[1] = '0';
	}
	i = 0;
	while (x != 0) {
		ch = x % 10 + '0';
		x /= 10;
		dummyStr[i] = ch;
		i++;
	}
	i--;
	j = 0;
	while(i != -1) {
		*dest = dummyStr[i];
		dest++;
		i--;
		j++;
	}
	*dest = '\0';
}