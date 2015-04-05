#include "kernel.h"
#include "drivers/hal.h"
#include "tty.h"

static int tty_idx = 1;

static void
getty(void) {
	char name[] = "tty0", buf[256];
	lock();
	name[3] += (tty_idx ++);
	unlock();

	int len = 0;
	while(1) {
		if ((len = dev_read(name, current->pid, buf, 0, 256)) < 0) {
			continue;
		}
		int i = 0;
		for (; i < len; i++) {
			if (buf[i] >= 'a' && buf[i] <= 'z') {
				buf[i] = buf[i] - 'a' + 'A';
			}
		}
		dev_write(name, current->pid, buf, 0, len);
	}
}

void
init_getty(void) {
	int i;
	for(i = 0; i < NR_TTY; i ++) {
		wakeup(create_kthread(getty));
	}
}


