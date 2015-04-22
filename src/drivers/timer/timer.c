#include "kernel.h"
#include "x86/x86.h"
#include "drivers/hal.h"
#include "drivers/time.h"
#include "lib/string.h"

#define PORT_TIME 0x40
#define PORT_RTC  0x70
#define FREQ_8253 1193182

#define NR_TIMER 64
#define TIMER_TIMEOUT 1

struct Timer_t {
	pid_t req_pid;
	uint32_t ring_time;
};

static struct Timer_t timer_pool[NR_TIMER];
static int timer_num = 0;

pid_t TIMER;
static long jiffy = 0;
static Time rt;

static void update_jiffy(void);
static void init_i8253(void);
static void init_rt(void);
static void timer_driver_thread(void);
static void new_timer(pid_t, uint32_t);
static void alarm_ring(void);

void init_timer(void) {
	init_i8253();
	init_rt();
	add_irq_handle(0, update_jiffy);
	PCB *p = create_kthread(timer_driver_thread);
	TIMER = p->pid;
	wakeup(p);
	hal_register("timer", TIMER, 0);
}

static void
timer_driver_thread(void) {
	static Message m;
	
	while (true) {
		receive(ANY, &m);
	
		if (m.src == MSG_HARD_INTR) {
			if (m.type == TIMER_TIMEOUT) {
				alarm_ring();
			}
			else {
				panic("Timer error");
			}
		}
		else {
			if (m.type == NEW_TIMER) {	// NEW_TIMER == DEV_READ
				new_timer(m.req_pid, m.offset + m.len);
			}
			else {
				assert(0);
			}
		}
	}
}

long
get_jiffy() {
	return jiffy;
}

static int
md(int year, int month) {
	bool leap = (year % 400 == 0) || (year % 4 == 0 && year % 100 != 0);
	static int tab[13] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	return tab[month] + (leap && month == 2);
}

static void
update_jiffy(void) {
	jiffy ++;
	if (jiffy % HZ == 0) {
		rt.second ++;
		if (rt.second >= 60) { rt.second = 0; rt.minute ++; }
		if (rt.minute >= 60) { rt.minute = 0; rt.hour ++; }
		if (rt.hour >= 24)   { rt.hour = 0;   rt.day ++;}
		if (rt.day >= md(rt.year, rt.month)) { rt.day = 1; rt.month ++; } 
		if (rt.month >= 13)  { rt.month = 1;  rt.year ++; }
	}
	static Message m;
	if (timer_num && jiffy >= timer_pool[0].ring_time) {
		m.type = TIMER_TIMEOUT;
		m.src = MSG_HARD_INTR;
		send(TIMER, &m);
	}
}

static void
init_i8253(void) {
	int count = FREQ_8253 / HZ;
	assert(count < 65536);
	out_byte(PORT_TIME + 3, 0x34);
	out_byte(PORT_TIME, count & 0xff);
	out_byte(PORT_TIME, count >> 8);	
}

static void
init_rt(void) {
	memset(&rt, 0, sizeof(Time));
	out_byte(PORT_RTC, 0x00);
	rt.second = in_byte(PORT_RTC + 1);
	rt.second -= (rt.second >> 4) * 6;
	out_byte(PORT_RTC, 0x02);
	rt.minute = in_byte(PORT_RTC + 1);
	rt.minute -= (rt.minute >> 4) * 6;
	out_byte(PORT_RTC, 0x04);
	rt.hour = in_byte(PORT_RTC + 1);
	rt.hour -= (rt.hour >> 4) * 6;
	out_byte(PORT_RTC, 0x07);
	rt.day = in_byte(PORT_RTC + 1);
	rt.day -= (rt.day >> 4) * 6;
	out_byte(PORT_RTC, 0x08);
	rt.month = in_byte(PORT_RTC + 1);
	rt.month -= (rt.month >> 4) * 6;
	out_byte(PORT_RTC, 0x09);
	rt.year = in_byte(PORT_RTC + 1);
	rt.year -= (rt.year >> 4) * 6;
}

void 
get_time(Time *tm) {
	memcpy(tm, &rt, sizeof(Time));
}

static void
new_timer(pid_t req_pid, uint32_t countdown) {
	uint32_t ring_time = jiffy + countdown;
	int parent = timer_num / 2 - 1;
	int son = timer_num ++;
	lock();
	while (parent >= 0 && son != 0) {
		if (timer_pool[parent].ring_time <= ring_time) {
			break;
		}
		timer_pool[son].req_pid = timer_pool[parent].req_pid;
		timer_pool[son].ring_time = timer_pool[parent].ring_time;
		son = parent;
		parent = (parent - 1) / 2;
	}
	timer_pool[son].req_pid = req_pid;
	timer_pool[son].ring_time = ring_time;
	unlock();
}

static void
alarm_ring() {
	assert(timer_num);
	static Message m;
	m.src = TIMER;
	m.ret = TIME_OUT;
	int parent = 0;
	int son = 2 * parent + 1;
	uint32_t deadline;
	pid_t temp, req_pid;
	while (timer_num > 0 && timer_pool[0].ring_time <= jiffy) {
		req_pid = timer_pool[0].req_pid;
		lock();
		timer_pool[0].req_pid = timer_pool[timer_num-1].req_pid;
		timer_pool[0].ring_time = timer_pool[timer_num-1].ring_time;
		timer_num --;

		deadline = timer_pool[0].ring_time;
		temp = timer_pool[0].req_pid;
		while (son < timer_num) {
			if (son + 1 < timer_num && timer_pool[son+1].ring_time < timer_pool[son].ring_time) {
				son ++;
			}
			if (timer_pool[son].ring_time > deadline) {
				break;
			}
			timer_pool[parent].req_pid = timer_pool[son].req_pid;
			timer_pool[parent].ring_time = timer_pool[son].ring_time;
			parent = son;
			son = 2 * son + 1;
		}
		timer_pool[parent].req_pid = temp;
		timer_pool[parent].ring_time = deadline;
		unlock();

		send(req_pid, &m);
	}
}

