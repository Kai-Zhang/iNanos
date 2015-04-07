#include "kernel.h"
#include "x86/x86.h"
#include "drivers/hal.h"
#include "drivers/time.h"
#include "lib/string.h"

#define PORT_TIME 0x40
#define PORT_RTC  0x70
#define FREQ_8253 1193182

#define NR_TIMER 64
#define TIMER_TICK 1

struct Timer_t {
	pid_t req_pid;
	uint32_t countdown;
	struct list_head list;
};

static struct Timer_t timer_pool[NR_TIMER];
static struct Timer_t *timers = NULL;
static struct Timer_t *free = timer_pool;

pid_t TIMER;
static long jiffy = 0;
static Time rt;

static void update_jiffy(void);
static void init_i8253(void);
static void init_rt(void);
static void timer_driver_thread(void);
static void new_timer(pid_t, uint32_t);
static void tick(void);

void
init_alarm(void) {
	INIT_LIST_HEAD(&(timer_pool[0].list));
	int i;
	for (i = 1;i < NR_TIMER; i++) {
		list_add_tail(&(timer_pool[i].list), &(timer_pool[i-1].list));
	}
}

void init_timer(void) {
	init_i8253();
	init_rt();
	init_alarm();
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
			if (m.type == TIMER_TICK) {
				tick();
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
	m.type = TIMER_TICK;
	m.src = MSG_HARD_INTR;
	send(TIMER, &m);
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
	out_byte(PORT_RTC, 0x02);
	rt.minute = in_byte(PORT_RTC + 1);
	out_byte(PORT_RTC, 0x04);
	rt.hour = in_byte(PORT_RTC + 1);
	out_byte(PORT_RTC, 0x07);
	rt.day = in_byte(PORT_RTC + 1);
	out_byte(PORT_RTC, 0x08);
	rt.month = in_byte(PORT_RTC + 1);
	out_byte(PORT_RTC, 0x09);
	rt.year = in_byte(PORT_RTC + 1);
}

void 
get_time(Time *tm) {
	memcpy(tm, &rt, sizeof(Time));
}

static void
new_timer(pid_t req_pid, uint32_t countdown) {
	assert(free);
	struct Timer_t *t = free;
	free = list_entry(free->list.next, struct Timer_t, list);
	t->req_pid = req_pid;
	t->countdown = countdown;
	if (timers) {
		list_add_tail(&(t->list), &(timers->list));
	}
	else {
		timers = t;
	}
}

static void
alarm_ring(struct Timer_t *timer) {
	assert(timers);
	static Message m;
	m.src = TIMER;
	m.ret = TIME_OUT;
	send(timer->req_pid, &m);

	if (timers == timer) {
		if (list_empty(&(timer->list))) {
			timers = NULL;
		}
		else {
			timers = list_entry(timers->list.next, struct Timer_t, list);
		}
	}
	list_del(&(timer->list));
}

static void
tick(void) {
	if (timers) {
		timers->countdown --;
		if (!timers->countdown) {
			alarm_ring(timers);
		}
		struct Timer_t *t;
		list_for_each_entry(t, &(timers->list), list) {
			t->countdown --;
			if (!t->countdown) {
				alarm_ring(t);
			}
		}
	}
}
