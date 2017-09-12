#include "macros.h"

/* Semaphore declaration */
extern sem_t * sem_id;

struct shared_data {
	char buffer[1024][MD5_LEN + 1]; /* +1 for null terminated string. */
	int last;
	struct Queue * queue; 
};

void signalCallbackHandler(int signum);

void prepareSharedMemoryWithSemaphores(sem_t * sem_id, struct shared_data * * shared_msg);

void terminateSemaphore(sem_t * sem_id);

