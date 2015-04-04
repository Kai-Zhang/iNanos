#ifndef __PCB_H__
#define __PCB_H__
#include "lib/list.h"
#include "common/types.h"

#define KSTACK_SIZE 2048
#define KERNEL_PCB_MAX 64

typedef union PCB {
	uint8_t kstack[KSTACK_SIZE];
	struct {
		void *tf;
		struct list_head ready, sleep, free, sem;
		int lock_counter, unlock_status;
	};
} PCB;

extern PCB *current, *next;

#endif
