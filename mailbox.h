#define MAX_MAILBOX 10
#define SEM_CAN_WRITE 0
#define SEM_CAN_READ 1

/*
 * Mailboxes services
 */
void init_mailboxes();
void mailbox_write(int mailbox_id, uint32_t data);
void mailbox_read(int mailbox_id, uint32_t * buffer);

/*
 * Semaphore services
 */
void _init_semaphores();
void wait_semaphore(int semaphore_id, char action);
void release_semaphore(int semaphore_id, char action);

/*
 * Mailboxes data
 */
struct mailbox {
	int task_id_writer;
	int task_id_reader;
	uint32_t data;
	char semaphore;
};

static struct mailbox mailbox_table[MAX_MAILBOX];

void init_mailboxes() {
	_init_semaphores();

	int i = MAX_MAILBOX - 1;
	for(; i > 0; i--) {
		mailbox_table[i].semaphore = i;
	}
}

/*
 * Mailboxes services
 */

void mailbox_write(int mailbox_id, uint32_t data) {
	if(mailbox_table[mailbox_id].task_id_writer != current_task) {
		return;
	}
	wait_semaphore(mailbox_table[mailbox_id].semaphore, SEM_CAN_WRITE);
	mailbox_table[mailbox_id].data = data;
	release_semaphore(mailbox_table[mailbox_id].semaphore, SEM_CAN_READ);
}

void mailbox_read(int mailbox_id, uint32_t * buffer) {
	if(mailbox_table[mailbox_id].task_id_reader != current_task) {
		return;
	}
	wait_semaphore(mailbox_table[mailbox_id].semaphore, SEM_CAN_READ);
	*buffer = mailbox_table[mailbox_id].data;
	release_semaphore(mailbox_table[mailbox_id].semaphore, SEM_CAN_WRITE);
}

/*
 * Semaphore data
 */

struct semaphore {
	char count;
	int unblock_task;
};

static struct semaphore semaphore_table[MAX_MAILBOX];

void _init_semaphores() {
	int i = MAX_MAILBOX - 1;
	for(; i > 0; i--) {
		semaphore_table[i].count = SEM_CAN_WRITE;
	}
}

/*
 * Semaphore services
 */
void wait_semaphore(int semaphore_id, char action);
void release_semaphore(int semaphore_id, char action);

void wait_semaphore(int semaphore_id, char action) {

	while(semaphore_table[semaphore_id].count != action) {
		semaphore_table[semaphore_id].unblock_task = current_task;
		wait();
	}
}

void release_semaphore(int semaphore_id, char action) {
	semaphore_table[semaphore_id].count = action;
	_unblock_task(semaphore_table[semaphore_id].unblock_task);
	task_call_scheduler();
}
