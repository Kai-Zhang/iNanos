#include "kernel.h"
#define NBUF 5
#define NR_PROD 3
#define NR_CONS 4
 
int buf[NBUF];
volatile int f = 0, r = 0, g = 1;
int last = 0;
Semaphore empty, full, mutex;

extern void do_read(int, uint8_t*, off_t, size_t);
 
// Accelerate the time clock to expose the problem of critical area
void
accelerate(void) {
	#define PORT_TIME 0x40
	#define FREQ_8253 1193182
	#define HZ        10000
	 
	int count = FREQ_8253 / HZ;
	assert(count < 65536);
	out_byte(PORT_TIME + 3, 0x34);
	out_byte(PORT_TIME    , count % 256);
	out_byte(PORT_TIME    , count / 256);
}


// --------------------------------------------------
// Test Semaphore - producer and consumer
// --------------------------------------------------
void
producer(void) {
	while (1) {
		P(&empty);
		P(&mutex);
		printk(".");	// tell us threads are really working
		buf[f ++] = g ++;
		f %= NBUF;
		V(&mutex);
		V(&full);
	}
}
 
void
consumer(void) {
	int get;
	while (1) {
		P(&full);
		P(&mutex);
		get = buf[r ++];
		assert(last == get - 1);	// the products should be strictly increasing
		last = get;
		r %= NBUF;
		V(&mutex);
		V(&empty);
	}
}
 
void
test_semaphore(void) {
	init_sem(&full, 0);
	init_sem(&empty, NBUF);
	init_sem(&mutex, 1);
	int i;
	for(i = 0; i < NR_PROD; i ++) {
		wakeup(create_kthread(producer));
	}
	for(i = 0; i < NR_CONS; i ++) {
		wakeup(create_kthread(consumer));
	}
}


// --------------------------------------------------
// Test kthread create and schedule
// --------------------------------------------------
void
kthread_A (void) { 
    int x = 0;
    while(1) {
        if(x % 100000 == 0) {printk("a");}
        x ++;
    }
}

void
kthread_B (void) { 
    int x = 0;
    while(1) {
        if(x % 100000 == 0) {printk("b");}
        x ++;
    }
}

void
test_basic_kthread(void) {
	create_kthread(kthread_A);
	create_kthread(kthread_B);
}


// --------------------------------------------------
// Test printk function
// --------------------------------------------------
void
test_printk(void) {
	printk("Printk test begin...\n");
	printk("the answer should be:\n");
	printk("#######################################################\n");
	printk("Hello, welcome to OSlab! I'm the body of the game. ");
	printk("Bootblock loads me to the memory position of 0x100000, and Makefile also tells me that I'm at the location of 0x100000. ");
	printk("~!@#$^&*()_+`1234567890-=...... ");
	printk("Now I will test your printk: ");
	printk("1 + 1 = 2, 123 * 456 = 56088\n0, -1, -2147483648, -1412505855, -32768, 102030\n0, ffffffff, 80000000, abcdef01, ffff8000, 18e8e\n");
	printk("#######################################################\n");
	printk("your answer:\n");
	printk("=======================================================\n");
	printk("%s %s%scome %co%s", "Hello,", "", "wel", 't', " ");
	printk("%c%c%c%c%c! ", 'O', 'S', 'l', 'a', 'b');
	printk("I'm the %s of %s. %s 0x%x, %s 0x%x. ", "body", "the game", "Bootblock loads me to the memory position of", 
		0x100000, "and Makefile also tells me that I'm at the location of", 0x100000);
	printk("~!@#$^&*()_+`1234567890-=...... ");
	printk("Now I will test your printk: ");
	printk("%d + %d = %d, %d * %d = %d\n", 1, 1, 1 + 1, 123, 456, 123 * 456);
	printk("%d, %d, %d, %d, %d, %d\n", 0, 0xffffffff, 0x80000000, 0xabcedf01, -32768, 102030);
	printk("%x, %x, %x, %x, %x, %x\n", 0, 0xffffffff, 0x80000000, 0xabcedf01, -32768, 102030);
	printk("=======================================================\n");
	printk("Test end!!! Good luck!!!\n");
}


// --------------------------------------------------
// Test stack overflow
// --------------------------------------------------
void
stackoverflow(int x) {
    if(x == 0)
        printk("%d ",x);
    if(x > 0)
        stackoverflow(x - 1);
}

void
test_stackoverflow(void) {
    while(1) {
        stackoverflow(16384*1000);
    }
}


// --------------------------------------------------
// Test kthread sleep and wakeup
// --------------------------------------------------
static PCB *PCB_of_thread_A;
static PCB *PCB_of_thread_B;
static PCB *PCB_of_thread_C;
static PCB *PCB_of_thread_D;

void
A (void) { 
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

void
B (void) { 
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

void
C (void) { 
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

void
D (void) { 
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
test_sleep_wakeup(void) {
	PCB_of_thread_A = create_kthread(A);
	PCB_of_thread_B = create_kthread(B);
	PCB_of_thread_C = create_kthread(C);
	PCB_of_thread_D = create_kthread(D);
}


// --------------------------------------------------
// Test message
// --------------------------------------------------
int pidA, pidB, pidC, pidD, pidE;

void
kthread_a(void) { 
	Message m1, m2;
	m1.src = current->pid;
	int x = 0;
	while(1) {
		if(x % 1000000 == 0) {
			printk("a"); 
			send(pidE, &m1);
			receive(pidE, &m2);
		}
		x ++;
	}
}

void
kthread_b(void) { 
	Message m1, m2;
	m1.src = current->pid;
	int x = 0;
	receive(pidE, &m2);
	while(1) {
		if(x % 1000000 == 0) {
			printk("b"); 
			send(pidE, &m1);
			receive(pidE, &m2);
		}
		x ++;
	}
}

void
kthread_c(void) { 
	Message m1, m2;
	m1.src = current->pid;
	int x = 0;
	receive(pidE, &m2);
	while(1) {
		if(x % 1000000 == 0) {
			printk("c"); 
			send(pidE, &m1);
			receive(pidE, &m2);
		}
		x ++;
	}
}

void
kthread_d(void) { 
	Message m1, m2;
	m1.src = current->pid;
	receive(pidE, &m2);
	int x = 0;
	while(1) {
		if(x % 1000000 == 0) {
			printk("d"); 
			send(pidE, &m1);
			receive(pidE, &m2);
		}
		x ++;
	}
}
 
void
kthread_e(void) {
	Message m1, m2;
	m2.src = current->pid;
	char c;
	while(1) {
		receive(ANY, &m1);
		if(m1.src == pidA) {c = '|'; m2.dst = pidB; }
		else if(m1.src == pidB) {c = '/'; m2.dst = pidC;}
		else if(m1.src == pidC) {c = '-'; m2.dst = pidD;}
		else if(m1.src == pidD) {c = '\\'; m2.dst = pidA;}
		else assert(0);
 
		printk("\033[s\033[1000;1000H%c\033[u", c);
		send(m2.dst, &m2);
	}
 
}

void
test_message(void) {
	pidA = (create_kthread(kthread_a))->pid;
	pidB = (create_kthread(kthread_b))->pid;
	pidC = (create_kthread(kthread_c))->pid;
	pidD = (create_kthread(kthread_d))->pid;
	pidE = (create_kthread(kthread_e))->pid;
}


// --------------------------------------------------
// Test drivers
// --------------------------------------------------
void
driver_kthread(void) {
	uint8_t buf[32];
	int len = dev_read("null", current->pid, buf, 0, 32);
	assert(len == 0);
	len = dev_read("zero", current->pid, buf, 0, 32);
	assert(len == 32);
	int i;
	for (i = 0; i < len; i++) {
		assert(buf[i] == 0);
	}
	printk("Dev test approved.\n");
	do_read(1, buf, 0, 32);
	printk("Get: %s\n", buf);
	sleep();
}

void
test_drivers(void) {
	create_kthread(driver_kthread);
}


// --------------------------------------------------
// Test drivers
// --------------------------------------------------
void
timer_kthread(void) {
	printk("Timer starts.\n");
	dev_read("timer", current->pid, NULL, 0, 200);
	printk("Time out.\n");
	sleep();
}

void
test_timer(void) {
	create_kthread(timer_kthread);
	create_kthread(timer_kthread);
	create_kthread(timer_kthread);
	create_kthread(timer_kthread);
	create_kthread(timer_kthread);
}


// Main entry of testcases
void
test_setup(void) {
	// Select testcases here.

	// accelerate();
	// test_printk();
	// test_basic_kthread();
	// test_stackoverflow();
	// test_sleep_wakeup();
	// test_semaphore();
	// test_message();
	// test_drivers();
	test_timer();
}
