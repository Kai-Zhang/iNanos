#ifndef __PROCESS_H__
#define __PROCESS_H__
#include "process/pcb.h"

PCB *create_kthread(void *fun);
void sleep();
void wakeup(PCB *);

void lock();
void unlock();

#endif
