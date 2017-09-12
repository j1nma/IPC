#include "macros.h"

/* Semaphore declaration */
extern sem_t * sem_id;

#define MAX_FILES 200
#define MAX_FILE_NAME 100

struct shared_data {
	char buffer[MAX_FILES][MD5_LEN + 1]; /* +1 for null terminated string. */
	char names[MAX_FILES][MAX_FILE_NAME]; // Max file lenght.
	int last;
	struct Queue * queue; 
};

void signalCallbackHandler(int signum);

void prepareSharedMemoryWithSemaphores(sem_t * sem_id, struct shared_data * * shared_msg);

void terminateSemaphore(sem_t * sem_id);

