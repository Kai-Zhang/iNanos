#ifndef __PCB_H__
#define __PCB_H__
#include "lib/list.h"
#include "common/types.h"
#include "process/semaphore.h"
#include "process/message.h"

#define KSTACK_SIZE 2048
#define MSGBUF_SIZE 64
#define KERNEL_PCB_MAX 64

typedef union PCB {
	uint8_t kstack[KSTACK_SIZE];
	struct {
		/* Stack pointer */
		void *tf;
		/* Link node for management */
		struct list_head ready, sleep, free, sem;
		/* PCB data */
		pid_t pid;
		int lock_counter, unlock_status;
		/* Message */
		Semaphore mutex, amount;
		Message *messages;
		Message msgbuf[MSGBUF_SIZE];
	};
} PCB;

extern PCB *current, *next;
extern PCB PCB_pool[KERNEL_PCB_MAX];

#endif
