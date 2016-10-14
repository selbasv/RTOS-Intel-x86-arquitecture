#include "qm_gpio.h"
#include "qm_interrupt.h"

#define ticks_bau_init 20 /* 300 bau */
#define ticks_bau 13
#define MAX_SERIAL 4

#define TASK_READ_SERIAL(serial_id, pin, alarm_id)\
	__asm__("cli"); \
	bool RXD = qm_gpio_read_pin(QM_GPIO_0, pin); \
	struct serial_data * tmp = &serial_queue[serial_id]; \
	\
	if (tmp->data_i > 0) { \
		tmp->data_tmp >>= 1; \
		if(RXD==1) { \
			tmp->data_tmp |= 0x80; \
		} \
		\
		refresh_alarm(alarm_id, ticks_bau); \
		\
		tmp->data_i--; \
	} else { \
		if(RXD == 1) { \
			tmp->data_ok = 1; \
			set_duty_cycle_byte(serial_id, tmp->data_tmp); \
			QM_PRINTF("-- %d (%d) --\r\n", serial_id, tmp->data_tmp); \
		} else { \
			tmp->data_ok = 0; \
			QM_PRINTF("-- %d (%d) --\r\n", serial_id, tmp->data_tmp); \
		} \
		enable_int_adc(pin); \
	}

/* Serial data structures */
enum { ALARM_A0 = 9,  ALARM_A1 = 8, ALARM_A2 = 7, ALARM_A3 = 6};

struct serial_data {
	bool data_ok;
	uint8_t data_tmp;
	char data_i;
	int task_id;
};

static struct serial_data serial_queue[MAX_SERIAL];

/* Services */
void enable_int_adc(int pin);
void init_serial_interrupts();
void init_tasks_serial();
void task_read_a0();
void task_read_a1();
void task_read_a2();
void task_read_a3();
void disable_int_adc(int pin);


void interrupt_callback(uint32_t status) {
	struct serial_data * tmp;

	if(status == BIT(A0)) {
		tmp = &serial_queue[0];
		set_alarm(ALARM_A0, ticks_bau_init, tmp->task_id);
		disable_int_adc(A0);
	} else if(status == BIT(A1)) {
		tmp = &serial_queue[1];
		set_alarm(ALARM_A1, ticks_bau_init, tmp->task_id);
		disable_int_adc(A1);
	} else if(status == BIT(A2)) {
		tmp = &serial_queue[2];
		set_alarm(ALARM_A2, ticks_bau_init, tmp->task_id);
		disable_int_adc(A2);
	} else if(status == BIT(A3)) {
		tmp = &serial_queue[3];
		set_alarm(ALARM_A3, ticks_bau_init, tmp->task_id);
		disable_int_adc(A3);
	} else {
		return;
	}

	tmp->data_i = 8;
	tmp->data_tmp = 0;
	tmp->data_ok = 0;
}

void init_serial_interrupts() {
	qm_gpio_port_config_t cfg;
	qm_gpio_get_config(QM_GPIO_0, &cfg);

	cfg.int_en = 0;
	cfg.int_bothedge = 0x0;		  /* Both edge disabled */
	cfg.int_type = 0;     /* Edge sensitive interrupt */
	cfg.int_polarity = 0;
	cfg.callback = interrupt_callback;

	qm_irq_request(QM_IRQ_GPIO_0, qm_gpio_isr_0); /* interrupts */
	qm_gpio_set_config(QM_GPIO_0, &cfg);

	init_tasks_serial();

	enable_int_adc(A0);
	enable_int_adc(A1);
	enable_int_adc(A2);
	enable_int_adc(A3);
}

void enable_int_adc(int pin) {
	qm_gpio_port_config_t cfg;
	qm_gpio_get_config(QM_GPIO_0, &cfg);

	cfg.int_en |= BIT(pin);       /* Interrupt enabled */
	cfg.int_type |= BIT(pin);     /* Edge sensitive interrupt */
	cfg.int_polarity &= ~BIT(pin); /* Rising edge */
	cfg.int_debounce |= BIT(pin);  /* Debounce enabled */

	qm_gpio_set_config(QM_GPIO_0, &cfg);
}

void disable_int_adc(int pin) {
	qm_gpio_port_config_t cfg;

	qm_gpio_get_config(QM_GPIO_0, &cfg);

	cfg.int_en &= ~BIT(pin);       /* Interrupt enabled */
	cfg.int_type &= ~BIT(pin);     /* Edge sensitive interrupt */
	cfg.int_debounce &= ~BIT(pin); /* Debounce enabled */

	qm_gpio_set_config(QM_GPIO_0, &cfg);
}

/* Tasks code */
struct task_data serial_a0_task;
struct task_data serial_a1_task;
struct task_data serial_a2_task;
struct task_data serial_a3_task;

void init_tasks_serial() {
	serial_a0_task.priority = '0';
	serial_a0_task.entry_point = (uint32_t)&task_read_a0;
	add_task(&serial_a0_task);
	serial_queue[0].task_id = serial_a0_task.task_id;

	serial_a1_task.priority = '0';
	serial_a1_task.entry_point = (uint32_t)&task_read_a1;
	add_task(&serial_a1_task);
	serial_queue[1].task_id = serial_a1_task.task_id;

	serial_a2_task.priority = '0';
	serial_a2_task.entry_point = (uint32_t)&task_read_a2;
	add_task(&serial_a2_task);
	serial_queue[2].task_id = serial_a2_task.task_id;

	serial_a3_task.priority = '0';
	serial_a3_task.entry_point = (uint32_t)&task_read_a3;
	add_task(&serial_a3_task);
	serial_queue[3].task_id = serial_a3_task.task_id;
}

void task_read_a0() {
	TASK_READ_SERIAL(0, A0, ALARM_A0)

	terminate_task();
}

void task_read_a1() {
	TASK_READ_SERIAL(1, A1, ALARM_A1)

	terminate_task();
}

void task_read_a2() {
	TASK_READ_SERIAL(2, A2, ALARM_A2)

	terminate_task();
}

void task_read_a3() {
	TASK_READ_SERIAL(3, A3, ALARM_A3)

	terminate_task();
}
