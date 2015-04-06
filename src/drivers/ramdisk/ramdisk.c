#include "kernel.h"
#include "drivers/hal.h"

#define DEV_MEM 0
#define DEV_KMEM 1
#define DEV_RAM 2
#define DEV_NULL 3
#define DEV_ZERO 4
#define DEV_RANDOM 5

#define NR_MAX_FILE 8
#define NR_FILE_SIZE (128 * 1024)

extern void copy_from_kernel(PCB *, void *, void *, int);
extern void init_fm(void);

static uint8_t file[NR_MAX_FILE][NR_FILE_SIZE] = {
	{ 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xff },
	{ "Hello, world!\n" }
};
static uint8_t *disk = (void *)file;

pid_t RAMDISK;

static void
rd_read(Message *m) {
	switch(m->dev_id) {
		case DEV_MEM:
			if (m->len >= PHY_MEM) {
				printk("Mem is only 128MB.\n");
				m->ret = PHY_MEM;
			}
			else {
				m->ret = m->len;
			}
			copy_from_kernel(fetch_pcb(m->req_pid), m->buf, (void *)m->offset, m->ret);
			break;
		case DEV_KMEM:
			if (m->len >= PHY_MEM) {
				printk("Kernel mem is only 16MB.\n");
				m->ret = KMEM;
			}
			else {
				m->ret = m->len;
			}
			copy_from_kernel(fetch_pcb(m->req_pid), m->buf, (void *)(KOFFSET + m->offset), m->ret);
			break;
		case DEV_RAM:
			copy_from_kernel(fetch_pcb(m->req_pid), m->buf, disk + m->offset, m->len);
			m->ret = m->len;
			break;
		case DEV_NULL:
			m->ret = 0;
			break;
		case DEV_ZERO:
			memset(m->buf, 0, m->len);
			m->ret = m->len;
			break;
		case DEV_RANDOM:
			break;
		default: assert(0);
	}
}

static void
rd_write(Message *m) {
	switch(m->dev_id) {
		case DEV_MEM:
		case DEV_KMEM:
			panic("Trying to write to mem unprotected.");
			break;
		case DEV_RAM:
		case DEV_NULL:
		case DEV_ZERO:
			m->ret = 0;
			break;
		case DEV_RANDOM:
			break;
		default: assert(0);
	}
}

static void
ramdisk_driver_thread(void) {
	Message m;

	while(1) {
		receive(ANY, &m);
		switch(m.type) {
			case DEV_READ:
				rd_read(&m);
				m.dst = m.src;
				m.src = RAMDISK;
				send(m.dst, &m);
				break;
			case DEV_WRITE:
				rd_write(&m);
				m.dst = m.src;
				m.src = RAMDISK;
				send(m.dst, &m);
				break;
			default: assert(0);
		}
	}
}

void
init_ramdisk() {
	init_fm();
	PCB *p = create_kthread(ramdisk_driver_thread);
	RAMDISK = p->pid;
	hal_register("mem", RAMDISK, DEV_MEM);
	hal_register("kmem", RAMDISK, DEV_KMEM);
	hal_register("ram", RAMDISK, DEV_RAM);
	hal_register("null", RAMDISK, DEV_NULL);
	hal_register("zero", RAMDISK, DEV_ZERO);
	hal_register("random", RAMDISK, DEV_RANDOM);
	wakeup(p);
}

