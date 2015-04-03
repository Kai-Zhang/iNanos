#ifndef __PROCESS_H__
#define __PROCESS_H__
#include "adt/list.h"

#define KSTACK_SIZE 2048
#define KERNEL_PCB_MAX 64

typedef union PCB {
	uint8_t kstack[KSTACK_SIZE];
	struct {
		void *tf;
		struct list_head ready, sleep, free;
	};
} PCB;

extern PCB *current;

PCB *create_kthread(void *);

#endif
