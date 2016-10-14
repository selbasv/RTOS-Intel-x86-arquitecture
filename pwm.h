#include "qm_gpio.h"
#include "qm_interrupt.h"

#define DUTY_PERIOD 8 /* 2 ms */
#define MAX_PWM 4

/* PWM services */
void pwm_init();
void pwm_0_up();
void pwm_0_down();
void pwm_1_up();
void pwm_1_down();
void pwm_2_up();
void pwm_2_down();
void pwm_3_up();
void pwm_3_down();

void set_duty_cycle(int pwm, int duty_cycle);
void init_pwm();
void calculate_alarm(int pwm, bool up, int alarm_id, int gpio);
void init_pwm_tasks();


struct duty_cycle {
	int cycles_up;
	int cycles_down;
	int task_id_up;
	int task_id_dw;
};

static struct duty_cycle pwm_data[MAX_PWM];

void set_duty_cycle(int pwm, int duty_cycle) {
	pwm_data[pwm].cycles_up = (DUTY_PERIOD * duty_cycle / 100);
	pwm_data[pwm].cycles_down = DUTY_PERIOD - pwm_data[pwm].cycles_up;
}

void set_duty_cycle_byte(int pwm, uint8_t byte) {
	pwm_data[pwm].cycles_up = byte / 255.0 * DUTY_PERIOD;
	pwm_data[pwm].cycles_down = DUTY_PERIOD - pwm_data[pwm].cycles_up;
}

void init_pwm() {
	init_pwm_tasks();

	set_duty_cycle(0, 100);
	set_duty_cycle(1, 75);
	set_duty_cycle(2, 50);
	set_duty_cycle(3, 0);
}


/* Tasks code */
struct task_data pwm_0_up_task;
struct task_data pwm_0_dw_task;
struct task_data pwm_1_up_task;
struct task_data pwm_1_dw_task;
struct task_data pwm_2_up_task;
struct task_data pwm_2_dw_task;
struct task_data pwm_3_up_task;
struct task_data pwm_3_dw_task;

void init_pwm_tasks() {
	pwm_0_up_task.priority = '1';
	pwm_0_up_task.entry_point = (uint32_t)&pwm_0_up;
	pwm_0_up_task.state = TASK_READY;
	add_task(&pwm_0_up_task);
	pwm_0_dw_task.priority = '1';
	pwm_0_dw_task.entry_point = (uint32_t)&pwm_0_down;
	add_task(&pwm_0_dw_task);

	pwm_1_up_task.priority = '1';
	pwm_1_up_task.entry_point = (uint32_t)&pwm_1_up;
	pwm_1_up_task.state = TASK_READY;
	add_task(&pwm_1_up_task);
	pwm_1_dw_task.priority = '1';
	pwm_1_dw_task.entry_point = (uint32_t)&pwm_1_down;
	add_task(&pwm_1_dw_task);

	pwm_2_up_task.priority = '1';
	pwm_2_up_task.entry_point = (uint32_t)&pwm_2_up;
	pwm_2_up_task.state = TASK_READY;
	add_task(&pwm_2_up_task);
	pwm_2_dw_task.priority = '1';
	pwm_2_dw_task.entry_point = (uint32_t)&pwm_2_down;
	add_task(&pwm_2_dw_task);

	pwm_3_up_task.priority = '1';
	pwm_3_up_task.entry_point = (uint32_t)&pwm_3_up;
	pwm_3_up_task.state = TASK_READY;
	add_task(&pwm_3_up_task);
	pwm_3_dw_task.priority = '1';
	pwm_3_dw_task.entry_point = (uint32_t)&pwm_3_down;
	add_task(&pwm_3_dw_task);


	pwm_data[0].task_id_up = pwm_0_up_task.task_id;
	pwm_data[0].task_id_dw = pwm_0_dw_task.task_id;
	pwm_data[1].task_id_up = pwm_1_up_task.task_id;
	pwm_data[1].task_id_dw = pwm_1_dw_task.task_id;
	pwm_data[2].task_id_up = pwm_2_up_task.task_id;
	pwm_data[2].task_id_dw = pwm_2_dw_task.task_id;
	pwm_data[3].task_id_up = pwm_3_up_task.task_id;
	pwm_data[3].task_id_dw = pwm_3_dw_task.task_id;

	qm_gpio_port_config_t cfg;
	qm_gpio_get_config(QM_GPIO_0, &cfg);
	cfg.direction = BIT(GPIO_2);
	cfg.direction |= BIT(GPIO_3);
	cfg.direction |= BIT(GPIO_4);
	cfg.direction |= BIT(GPIO_5);
	qm_gpio_set_config(QM_GPIO_0, &cfg);
}

#define MACRO_UP(pwm_id, port_id) \
	__asm__("cli"); \
	struct duty_cycle * tmp = &pwm_data[pwm_id]; \
	int cycles = tmp->cycles_up; \
	\
	if(cycles > 0) { \
		qm_gpio_set_pin(QM_GPIO_0, port_id); \
	} else { \
		cycles = 1; \
	} \
	\
	set_alarm(pwm_id, cycles, tmp->task_id_dw);

#define MACRO_DOWN(pwm_id, port_id) \
	__asm__("cli"); \
	struct duty_cycle * tmp = &pwm_data[pwm_id]; \
	int cycles = tmp->cycles_down; \
	\
	if(cycles > 0) { \
		qm_gpio_clear_pin(QM_GPIO_0, port_id); \
	} else { \
		cycles = 1; \
	} \
	\
	set_alarm(pwm_id, cycles, tmp->task_id_up);


void pwm_0_up() {
	MACRO_UP(0, GPIO_2)

	terminate_task();
}
void pwm_0_down() {
	MACRO_DOWN(0, GPIO_2)

	terminate_task();
}

void pwm_1_up() {
	MACRO_UP(1, GPIO_3)

	terminate_task();
}
void pwm_1_down() {
	MACRO_DOWN(1, GPIO_3)

	terminate_task();
}

void pwm_2_up() {
	MACRO_UP(2, GPIO_4)

	terminate_task();
}
void pwm_2_down() {
	MACRO_DOWN(2, GPIO_4)

	terminate_task();
}

void pwm_3_up() {
	MACRO_UP(3, GPIO_5)

	terminate_task();
}
void pwm_3_down() {
	MACRO_DOWN(3, GPIO_5)

	terminate_task();
}

