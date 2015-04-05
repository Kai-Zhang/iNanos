#include "kernel.h"
#include "tty.h"

static int tty_idx = 1;

static void
getty(void) {
	char name[] = "tty0", buf[256];
	lock();
	name[3] += (tty_idx ++);
	unlock();

	while(1) {
		/* Insert code here to do these:
		 * 1. read key input from ttyd to buf (use dev_read())
		 * 2. convert all small letters in buf into capitcal letters
		 * 3. write the result on screen (use dev_write())
		 */

	}
}

void
init_getty(void) {
	int i;
	for(i = 0; i < NR_TTY; i ++) {
		wakeup(create_kthread(getty));
	}
}


