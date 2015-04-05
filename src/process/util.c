#include "kernel.h"
#include "process/pcb.h"

PCB *readyq, *freeq, *sleepq;
PCB PCB_pool[KERNEL_PCB_MAX];
extern PCB idle;
extern PCB *next;

PCB*
fetch_pcb(pid_t pid) {
	return &PCB_pool[pid];
}

PCB*
create_kthread(void *fun) {
	if (list_empty(&(freeq->free))) {
		return NULL;
	}
	lock();
	PCB *new_kthread = freeq;
	TrapFrame *tf = NULL;
	freeq = list_entry(new_kthread->free.next, PCB, free);
	list_add_tail(&(new_kthread->ready), &(current->ready));
	list_del_init(&(new_kthread->free));
	INIT_LIST_HEAD(&(new_kthread->sleep));
	INIT_LIST_HEAD(&(new_kthread->sem));
	init_sem(&(new_kthread->mutex), 1);
	init_sem(&(new_kthread->amount), 0);
	int i = 0;
	for (; i < MSGBUF_SIZE; i++) {
		INIT_LIST_HEAD(&(new_kthread->msgbuf[i].list));
	}
	new_kthread->messages = NULL;
	new_kthread->lock_counter = 0;
	new_kthread->unlock_status = 0;
	new_kthread->tf = (TrapFrame *)(new_kthread->kstack + KSTACK_SIZE) - 1;
	tf = (TrapFrame *)(new_kthread->tf);
	tf->irq = 1000;
	tf->eip = (uint32_t)fun;
	tf->cs = SELECTOR_KERNEL(SEG_KERNEL_CODE);
	tf->gs = tf->fs = tf->es = tf->ds = SELECTOR_KERNEL(SEG_KERNEL_DATA);
	tf->eflags = 0x202;
	unlock();
	return new_kthread;
}

void
init_proc() {
	current = &idle;
	current->lock_counter = 0;
	current->unlock_status = 0;

	readyq = &idle;
	INIT_LIST_HEAD(&(idle.ready));
	INIT_LIST_HEAD(&(idle.sleep));
	INIT_LIST_HEAD(&(idle.free));
	
	freeq = &PCB_pool[0];
	PCB_pool[0].pid = 0;
	INIT_LIST_HEAD(&(PCB_pool[0].ready));
	INIT_LIST_HEAD(&(PCB_pool[0].sleep));
	INIT_LIST_HEAD(&(PCB_pool[0].free));

	int i = 1;
	for (; i < KERNEL_PCB_MAX; i++) {
		PCB_pool[i].pid = i;
		INIT_LIST_HEAD(&(PCB_pool[i].ready));
		INIT_LIST_HEAD(&(PCB_pool[i].sleep));
		list_add(&(PCB_pool[i].free), &(PCB_pool[i-1].free));
	}
}

void
sleep() {
	lock();
	next = list_entry(current->ready.next, PCB, ready);
	list_del_init(&(current->ready));
	if (sleepq == NULL) {
		sleepq = current;
		INIT_LIST_HEAD(&(current->sleep));
	}
	else {
		list_add_tail(&(current->sleep), &(sleepq->sleep));
	}
	unlock();
	asm volatile("int $0x80");
}

void wakeup(PCB *p) {
	lock();
	if ((list_empty(&(p->sleep)) && sleepq != p) || sleepq == NULL) {
		return;
	}
	if (sleepq == p) {
		if (list_empty(&(sleepq->sleep))) {
			sleepq = NULL;
		}
		else {
			sleepq = list_entry(p->sleep.next, PCB, sleep);
		}
	}
	list_del_init(&(p->sleep));
	list_add_tail(&(p->ready), &(current->ready));
	unlock();
}

void
lock() {
	asm volatile("pushf");
	asm volatile("popl %0" : "=r"(current->unlock_status));
	current->unlock_status = (current->unlock_status & 0x200) >> 9;
	cli();
	current->lock_counter++;
}

void
unlock() {
	current->lock_counter--;
	if (current->lock_counter <= 0) {
		if (current->unlock_status) {
			sti();
		}
	}
}
