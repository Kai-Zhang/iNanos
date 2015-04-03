#include "kernel.h"

PCB idle, *current = &idle, *next;

void
schedule(void) {
	if (!list_empty(&(current->ready))) {
		current = list_entry(current->ready.next, PCB, ready);
		if (current == &idle) {
			current = list_entry(current->ready.next, PCB, ready);
		}
	}
	else {
		if (current != &idle) {
			current = next;
		}
	}
}
