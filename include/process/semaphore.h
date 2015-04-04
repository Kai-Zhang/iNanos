#ifndef __SEMAPHORE_H__
#define __SEMAPHORE_H__

typedef struct Semaphore {
	int count;
	struct list_head block;
} Semaphore;

extern void init_sem(Semaphore *sem, int value);
extern void P(Semaphore *sem);
extern void V(Semaphore *sem);

#endif
