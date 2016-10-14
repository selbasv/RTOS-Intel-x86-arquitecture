#include <qm_interrupt.h>
#include <qm_scss.h>
#include <qm_pic_timer.h>

#define STACK_SIZE 96
#define TASK_RUNNING 'r'
#define TASK_WAITING 'w'
#define TASK_READY 'g'
#define TASK_SUSPENDED 's'
#define NO_TASK -1

#define REMOVE_CALL_NO_ARGS_FROM_STACK \
		__asm__("mov %ebp, %esp"); \
		__asm__("pop %ebp"); \
		__asm__("add $4, %esp"); /* add esp, 4 */

#define SAVE_TASK_EIP \
		__asm__("mov 4(%%ebp), %0" : "=r"(queue[current_task]->eip));

#define SAVE_TASK_CONTEXT \
		__asm__("mov %%ebp, %0" : "=r"(queue[current_task]->ebp)); \
		__asm__("mov %%esp, %0" : "=r"(queue[current_task]->esp));

#define SWITCH_AND_RUN_SCHEDULER \
		__asm__("mov %0, %%ebp" : : "a"(scheduler_task.ebp)); \
		__asm__("mov %0, %%esp" : : "a"(scheduler_task.esp)); \
		__asm__("jmp *%0" : : "a"(eip_scheduler));

struct task_data {
	char auto_start;
	char priority;
	char state;
	int task_id;

	uint32_t entry_point;
	uint32_t eip;
	uint32_t ebp;
	uint32_t esp;

	char _interrupted;
	uint32_t stack[STACK_SIZE];
};

/*
 * Global variables
 */
static struct task_data scheduler_task;
static int current_task;
static int next_task;
static uint32_t eip_scheduler;

static struct task_data * queue[MAX_TASKS];
static int task_counter;

/*
 * Services
 */
void activate_task(int task_id);
void chain_task(int task_id);
void terminate_task();
void init_scheduler();
void _run_task(int task_id);
extern void init_scheduler();
void _set_eip_scheduler();
int schedule();
void scheduler();
void add_task(struct task_data * task);
int get_current_task_id();

/* Extern */
extern void init_timer();


void init_scheduler() {
	current_task = NO_TASK;
}

int schedule() {
	int next_task = NO_TASK;
	char best_priority = 0;
	struct task_data * tmp;
	int i;
	for(i = task_counter-1; i >= 0; i--) {
		tmp = queue[i];
		if(tmp->state == TASK_READY) {
			if(tmp->priority > best_priority) {
				next_task = i;
				best_priority = tmp->priority;
			}
		}
	}
	if(next_task == NO_TASK) {
		return NO_TASK;
	}

	return next_task;
}

void _set_eip_scheduler() {
	__asm__("mov 4(%%ebp), %0" : "=r"(eip_scheduler));
	__asm__("mov 0(%%ebp), %0" : "=r"(scheduler_task.ebp));
	__asm__("mov 0(%%ebp), %0" : "=r"(scheduler_task.esp));
	init_timer();
}

void scheduler() {
	if(!eip_scheduler) {
		__asm__("cli");
		_set_eip_scheduler();
	}
	do {
		__asm__("cli");
		next_task = schedule();
		if(next_task == NO_TASK) {
			__asm__("sti");
			continue;
		}

		current_task = next_task;

		__asm__("mov %0, %%ebp" : : "a"(queue[current_task]->ebp));
		__asm__("mov %0, %%esp" : : "a"(queue[current_task]->esp));

		queue[current_task]->state = TASK_RUNNING;
		if(queue[current_task]->_interrupted != 0) {
			queue[current_task]->_interrupted--;
			__asm__("popal");
			__asm__("popf");
			/* __asm__("sti"); */
			__asm__("jmp *%0" : : "a"(queue[current_task]->eip));
		}
		/* __asm__("sti"); */
		__asm__("jmp *%0" : : "a"(queue[current_task]->eip));
	} while(1);
}

void task_call_scheduler() {
	__asm__("cli");
	queue[current_task]->state = TASK_READY;

	SAVE_TASK_EIP

	REMOVE_CALL_NO_ARGS_FROM_STACK

	SAVE_TASK_CONTEXT

	current_task = NO_TASK;
	SWITCH_AND_RUN_SCHEDULER
}

void _terminate_task() {
	struct task_data * tmp = queue[current_task];

	tmp->state = TASK_SUSPENDED;
	tmp->ebp = (uint32_t)&tmp->stack[STACK_SIZE - 1];
	tmp->esp = (uint32_t)&tmp->stack[STACK_SIZE - 1];
	tmp->eip = tmp->entry_point;

	current_task = NO_TASK;
}

void terminate_task() {
	__asm__("cli");
	_terminate_task();

	SWITCH_AND_RUN_SCHEDULER
}

void _activate_task(int task_id) {
	queue[task_id]->state = TASK_READY;
}

void _unblock_task(int task_id) {
	if(queue[task_id]->state == TASK_WAITING) {
		queue[task_id]->state = TASK_READY;
	}
}

void activate_task(int task_id) {
	__asm__("cli");
	_activate_task(task_id);

	SAVE_TASK_EIP

	SAVE_TASK_CONTEXT

	current_task = NO_TASK;
	SWITCH_AND_RUN_SCHEDULER
}

void chain_task(int task_id) {
	__asm__("cli");
	_activate_task(task_id);
	_terminate_task();

	SWITCH_AND_RUN_SCHEDULER
}

void wait() {
	__asm__("cli");
	queue[current_task]->state = TASK_WAITING;

	SAVE_TASK_EIP

	REMOVE_CALL_NO_ARGS_FROM_STACK

	SAVE_TASK_CONTEXT

	current_task = NO_TASK;
	SWITCH_AND_RUN_SCHEDULER
}

void add_task(struct task_data * task) {
	task->task_id = task_counter;
	task->ebp = (uint32_t)&task->stack[STACK_SIZE - 1];
	task->esp = (uint32_t)&task->stack[STACK_SIZE - 1];
	task->eip = task->entry_point;
	queue[task_counter++] = task;
}

int get_current_task_id() {
	return current_task;
}
