#include "kernel.h"
#include "process/pcb.h"

void
send(pid_t dst, Message *msg) {
	msg->dst = dst;
	P(&(PCB_pool[dst].mutex));
	Message *new_msg = NULL;
	int i = 0;
	for (i = 0; i < MSGBUF_SIZE; i++) {
		if (PCB_pool[dst].messages != &PCB_pool[dst].msgbuf[i] &&
				list_empty(&(PCB_pool[dst].msgbuf[i].list))) {
			new_msg = &PCB_pool[dst].msgbuf[i];
			break;
		}
	}
	assert(new_msg);
	memcpy(new_msg, msg, sizeof(Message));
	INIT_LIST_HEAD(&(new_msg->list));
	if (!PCB_pool[dst].messages) {
		PCB_pool[dst].messages = new_msg;
	}
	else {
		list_add_tail(&(new_msg->list), &(PCB_pool[dst].messages->list));
	}
	wakeup(&PCB_pool[dst]);
	V(&(PCB_pool[dst].mutex));
	V(&(PCB_pool[dst].amount));
}

void
receive(pid_t src, Message *msg) {
	Message *trv = current->messages;
	Message *cur_msg = NULL;
	while (1) {
		P(&(current->amount));
		P(&(current->mutex));
		if (src == ANY || current->messages->src == src) {
			cur_msg = current->messages;
			break;
		}
		list_for_each_entry(trv, &(current->messages->list), list) {
			if (trv->src == src) {
				cur_msg = trv;
				break;
			}
		}
		if (!cur_msg) {
			V(&(current->mutex));
			V(&(current->amount));
			sleep();
		}
		else {
			break;
		}
	}
	memcpy(msg, cur_msg, sizeof(Message));
	if (current->messages == cur_msg) {
		current->messages = list_entry(current->messages->list.next, Message, list);
		if (current->messages == cur_msg) {
			current->messages = NULL;
		}
	}
	list_del_init(&(cur_msg->list));
	V(&(current->mutex));
}
