#include "kernel.h"
#include "drivers/hal.h"

#define NR_FILE_SIZE (128 * 1024)

#define MSG_FM_READ 1
#define MSG_FM_WRITE 2

pid_t FM;

void
do_read(int file_name, uint8_t *buf, off_t offset, size_t len) {
	Message m;
	m.type = MSG_FM_READ;
	m.req_pid = current->pid;
	m.buf = buf;
	m.offset = file_name * NR_FILE_SIZE + offset;
	m.len = len;
	send(FM, &m);
	receive(FM, &m);
}

static void
fmd(void) {
	Message m;
	int len;

	while (1) {
		receive(ANY, &m);
		switch (m.type) {
			case MSG_FM_READ:
				len = dev_read("ram", m.req_pid, m.buf, m.offset, m.len);
				m.src = FM;
				m.ret = len;
				send(m.req_pid, &m);
				break;
			case MSG_FM_WRITE: assert(0);
			default: assert(0);
		}
	}
}

void
init_fm(void) {
	PCB *p = create_kthread(fmd);
	FM = p->pid;
	wakeup(p);
}
