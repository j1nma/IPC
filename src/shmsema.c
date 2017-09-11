#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <signal.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/types.h>

#include "shmsema.h"

#define SEMNAME "semy"
#define SHMOBJ_PATH "/itba.so.grupo3.tp1"

/**
 * Signal Handler for CTRL^C
 * Needed for manually killing the process and for
 * future executions of the same program.
**/
void signalCallbackHandler(int signum) {

	/**
	* Semaphore unlink: Remove a named semaphore  from the system.
	*/
	if (shm_unlink(SHMOBJ_PATH) < 0) {
		perror("Could not unlink shared memory");
	}

	/**
	 * Semaphore Close: Close a named semaphore
	 */
	if (sem_close(sem_id) < 0) {
		perror("Could not close semaphore");
	}

	/**
	 * Semaphore unlink: Remove a named semaphore from the system.
	 */
	if (sem_unlink(SEMNAME) < 0) {
		perror("Could not unlink semaphore");
	}

	// Terminate program
	exit(signum);
}

void prepareSharedMemoryWithSemaphores(sem_t * sem_id, struct shared_data * * shared_msg) {

	/* Register signal and signal handler */
	signal(SIGINT, signalCallbackHandler);

	int shmfd;
	int shared_seg_size = (1 * sizeof(struct shared_data));   /* Shared segment capable of storing 1 message */

	/* Creating the shared memory object */
	shmfd = shm_open(SHMOBJ_PATH, O_CREAT | O_RDWR, S_IRWXU | S_IRWXG);
	if (shmfd < 0) {
		perror("Could not create shared memory");
		exit(1);
	}

	/* Adjusting mapped file size */
	ftruncate(shmfd, shared_seg_size);

	/* Semaphore open. Create the semaphore if it does not already exist. Initialized to 1. */
	sem_id = sem_open(SEMNAME, O_CREAT, S_IRUSR | S_IWUSR, 1);

	/* Requesting the shared segment  */
	*shared_msg = (struct shared_data *) mmap(NULL, shared_seg_size, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
	if (*shared_msg == NULL) {
		perror("Could not obtain shared segment");
		exit(1);
	}
}

void terminateSemaphore(sem_t * sem_id) {

	if (shm_unlink(SHMOBJ_PATH) != 0) {
		perror("Could not unlink shared segment");
		exit(1);
	}

	if (sem_close(sem_id) < 0) {
		perror("Could not close semaphore");
	}

	if (sem_unlink(SEMNAME) < 0) {
		perror("Could not unlink semaphore");
	}
	
}