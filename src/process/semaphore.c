#include "common/assert.h"
#include "process.h"
#include "process/pcb.h"

void
init_sem(Semaphore *sem, int value) {
	sem->count = value;
	INIT_LIST_HEAD(&(sem->block));
}

void
P(Semaphore *sem) {
	lock();
	sem->count--;
	if (sem->count < 0) {
		list_add(&(current->sem), &(sem->block));
		sleep();
	}
	unlock();
}

void
V(Semaphore *sem) {
	lock();
	sem->count++;
	if (sem->count <= 0) {
		assert(!list_empty(&(sem->block)));
		PCB *p = list_entry(sem->block.next, PCB, sem);
		list_del_init(sem->block.next);
		wakeup(p);
	}
	unlock();
}

