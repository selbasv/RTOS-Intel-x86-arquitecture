/*
 * Scheduler 0.5
 * Javier Perez
 * Selene Basurto
 */

#include <qm_common.h>

/*
 * Configure tasks for kernel
 */
#define MAX_TASKS 12

/*
 * Include RTOS kernel and libraries
 */
#include "board.h"
#include "kernel.h"
#include "alarm.h"
/* #include "mailbox.h" */
#include "pwm.h"
#include "serial.h"


int main(void) {

	QM_PUTS("\r\n-- MAIN --\r\n\r\n\r\n\r\n\r\n\r\n");

 	init_pwm();
	init_serial_interrupts();
	init_scheduler();
	init_alarm();
	/* init_mailboxes(); */

	scheduler();

	QM_PUTS("-- DONE --\r\n");
	return 0;
}
