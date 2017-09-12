#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <signal.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/types.h>

#include "shmsema.h"

#define SHMOBJ_PATH "/itba.so.grupo3.tp1"

char * sem_name;

/**
 * Most functionality inspired by:
 * https://github.com/Jeshwanth/Linux_code_examples/blob/master/POSIX_shared_memory/servershm.c
 * accesed by j1nma, 2 Sep. 2017
**/

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
	if (sem_unlink(sem_name) < 0) {
		perror("Could not unlink semaphore");
	}

	// Terminate program
	exit(signum);
}

char * setSemaphoreName(int pid) {

	/* The maximum number of digits in an 32-bit int is 10.
	 * One more is included because of the format required by semaphore names.
	 * Accesed at https://stackoverflow.com/questions/8090888 by j1nma, 12 Sep. 2017
	 */

	sem_name = malloc(sizeof(char) * 11);
	if (!sem_name) return NULL;

	sem_name[0] = '/';

	sprintf(sem_name + 1, "%d", pid);

	return sem_name;
}

sem_t * prepareSemaphore(int pid) {

	sem_t * sem_id;

	/* Semaphore open. Create the semaphore if it does not already exist. Initialized to 1. */
	sem_id = sem_open(setSemaphoreName(pid), O_CREAT, S_IRUSR | S_IWUSR, 1);

	if (sem_id == SEM_FAILED) {

		perror("Could not create semaphore.\n");

		exit(EXIT_FAILURE);
	}

	return sem_id;
}

void prepareSharedMemory(struct shared_data * * shared_msg) {

	int shmfd;
	int shared_seg_size = (1 * sizeof(struct shared_data));   /* Shared segment capable of storing 1 message */

	/* Creating the shared memory object */
	shmfd = shm_open(SHMOBJ_PATH, O_CREAT | O_RDWR, S_IRWXU | S_IRWXG);
	if (shmfd < 0) {
		perror("Could not create shared memory");
		exit(EXIT_FAILURE);
	}

	/* Adjusting mapped file size */
	ftruncate(shmfd, shared_seg_size);

	//  Semaphore open. Create the semaphore if it does not already exist. Initialized to 1.
	// sem_id = sem_open(sem_name, O_CREAT, S_IRUSR | S_IWUSR, 1);

	/* Requesting the shared segment  */
	*shared_msg = (struct shared_data *) mmap(NULL, shared_seg_size, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
	
	if (*shared_msg == NULL) {
		perror("Could not obtain shared segment");
		exit(EXIT_FAILURE);
	}

}

void terminateSharedMemoryAndSemaphore(sem_t * sem_id) {

	if (shm_unlink(SHMOBJ_PATH) != 0) {
		perror("Could not unlink shared segment");
		exit(EXIT_FAILURE);
	}

	if (sem_close(sem_id) < 0) {
		perror("Could not close semaphore");
	}

	if (sem_unlink(sem_name) < 0) {
		perror("Could not unlink semaphore");
	}

}