#include "common.h"
#include "x86/x86.h"
#include "memory.h"
#include "process.h"


void init_page(void);
void init_serial(void);
void init_segment(void);
void init_idt(void);
void init_intr(void);
void init_proc(void);

void os_init_cont(void);

void
os_init(void) {
	/* Notice that when we are here, IF is always 0 (see bootloader) */

	/* We must set up kernel virtual memory first because our kernel
	   thinks it is located in 0xC0000000.
	   Before setting up correct paging, no global variable can be used. */
	init_page();

	/* After paging is enabled, we can jump to the high address to keep 
	 * consistent with virtual memory, although it is not necessary. */
	asm volatile (" addl %0, %%esp\n\t\
					jmp *%1": : "r"(KOFFSET), "r"(os_init_cont));

	assert(0);	// should not reach here
}

PCB *PCB_of_thread_A;
PCB *PCB_of_thread_B;
PCB *PCB_of_thread_C;
PCB *PCB_of_thread_D;
void A () { 
    int x = 0;
    while(1) {
        if(x % 100000 == 0) {
            printk("a");
            wakeup(PCB_of_thread_B);
            sleep();
        }
        x ++;
    }
}
void B () { 
    int x = 0;
    while(1) {
        if(x % 100000 == 0) {
            printk("b");
            wakeup(PCB_of_thread_C);
            sleep();
        }
        x ++;
    }
}
void C () { 
    int x = 0;
    while(1) {
        if(x % 100000 == 0) {
            printk("c");
            wakeup(PCB_of_thread_D);
            sleep();
        }
        x ++;
    }
}
void D () { 
    int x = 0;
    while(1) {
        if(x % 100000 == 0) {
            printk("d");
            wakeup(PCB_of_thread_A);
            sleep();
        }
        x ++;
    }
}
void
os_init_cont(void) {
	/* Reset the GDT. Although user processes in Nanos run in Ring 0,
	   they have their own virtual address space. Therefore, the
	   old GDT located in physical address 0x7C00 cannot be used again. */
	init_segment();

	/* Initialize the serial port. After that, you can use printk() */
	init_serial();

	/* Set up interrupt and exception handlers,
	   just as we did in the game. */
	init_idt();

	/* Initialize the intel 8259 PIC. */
	init_intr();

	/* Initialize processes. */
	init_proc();

	PCB_of_thread_A = create_kthread(A);
	PCB_of_thread_B = create_kthread(B);
	PCB_of_thread_C = create_kthread(C);
	PCB_of_thread_D = create_kthread(D);

	sti();

	/* This context now becomes the idle process. */
	while (1) {
		wait_intr();
	}
}

