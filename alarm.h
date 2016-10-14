#include <qm_interrupt.h>
#include <qm_scss.h>
#include <qm_pic_timer.h>

#define MAX_ALARMS 10
#define CYCLE_PER_SECOND 0x1E84800 /* 32 Mhz */
#define CYCLE_TICK 8000
#define TICKS_SECOND 4000

struct alarm_struct {
	uint32_t count;
	char task_id;
};

static struct alarm_struct alarm_table[MAX_ALARMS];
static unsigned int ticks;

/*
 * Alarm services
 */
void init_alarm();
void set_alarm(int id, uint32_t ticks, int task_id);
void refresh_alarm(int id, uint32_t count_ticks);
void refresh_alarm_task(int id, uint32_t count_ticks, int task_id);


/*
 * Timer services
 */
void init_timer();


/*
 * Alarm config
 */
void init_alarm() {
	int i = MAX_ALARMS - 1;
	for(; i>=0; i--) {
		alarm_table[i].task_id = NO_TASK;
		alarm_table[i].count = 0;
	}
}

int i_callback;
char run_scheduler;
uint32_t tmp_flags;
uint32_t tmp_eax;
uint32_t tmp_ebx;
struct alarm_struct * tmp_alarm;

void callback_timer_alarm() {
	__asm__("cli");
	__asm__("pushf");

	ticks++;

	run_scheduler = 0;
	for(i_callback = MAX_ALARMS - 1; i_callback>=0; i_callback--) {
		tmp_alarm = &(alarm_table[i_callback]);
		if(tmp_alarm->task_id != NO_TASK && tmp_alarm->count == ticks) {
			_activate_task(tmp_alarm->task_id);
			run_scheduler++;
		}
	}

	if(run_scheduler > 0) {
		if(current_task != NO_TASK) {
			__asm__("pop %0" : "=b"(tmp_flags));

			__asm__("mov %ebp, %esp");
			__asm__("pop %ebp");
			__asm__("add $4, %esp");

			__asm__("popa");
			__asm__("mov %%ebx, %0" : "=b"(tmp_ebx));
			__asm__("mov %%eax, %0" : "=b"(tmp_eax));

			__asm__("mov 4(%%ebp), %0" : "=b"(queue[current_task]->eip));

			__asm__("add $4, %esp");
			__asm__("pop %eax");
			__asm__("popf");

			__asm__("cmp %esp, %ebp");
			__asm__("je swtch_save_flags");
			__asm__("mov %ebp, %esp"); /* maybe remove this line */
			__asm__("pop %ebp");

			/* save flags */
			__asm__("swtch_save_flags:");
			__asm__("mov %0, %%eax" : : "a"(tmp_flags));
			__asm__("push %eax");

			__asm__("mov %0, %%ebx" : : "b"(tmp_ebx));
			__asm__("mov %0, %%eax" : : "a"(tmp_eax));

			/* save registers */
			__asm__("pushal");

			/* save stack pointers */
			__asm__("mov %%ebp, %0" : "=b"(queue[current_task]->ebp));
			__asm__("mov %%esp, %0" : "=b"(queue[current_task]->esp));

			queue[current_task]->state = TASK_READY;
			queue[current_task]->_interrupted++;
			current_task = NO_TASK;
		}
		/* go to scheduler */
		__asm__("mov %0, %%ebp" : : "a"(scheduler_task.ebp));
		__asm__("mov %0, %%esp" : : "a"(scheduler_task.esp));

		__asm__("pushf");
		__asm__("push %cs");
		__asm__("push %0" : : "a"(eip_scheduler));

		__asm__("movl $0x0,0xfee000b0");
		__asm__("iret");
	}
	__asm__("popf");
}

void set_alarm(int id, uint32_t count_ticks, int task_id) {
	__asm__("cli");
	alarm_table[id].count = count_ticks + ticks;
	alarm_table[id].task_id = task_id;
}

void refresh_alarm(int id, uint32_t count_ticks) {
	__asm__("cli");
	alarm_table[id].count += count_ticks;
}

void refresh_alarm_task(int id, uint32_t count_ticks, int task_id) {
	__asm__("cli");
	alarm_table[id].task_id = task_id;
	if(alarm_table[id].count == 0) {
		alarm_table[id].count = count_ticks + ticks;
	} else {
		alarm_table[id].count += count_ticks;
	}
}

uint32_t get_alarm_counter(int id) {
	return alarm_table[id].count;
}


/*
 * Timer config
 */

void init_timer() {
	ticks = 0;
	qm_pic_timer_config_t cfg;

	cfg.mode = QM_PIC_TIMER_MODE_PERIODIC;
	cfg.int_en = true;

	qm_irq_request(QM_IRQ_PIC_TIMER, callback_timer_alarm);

	qm_pic_timer_set_config(&cfg);

	qm_pic_timer_set(CYCLE_TICK);
}
