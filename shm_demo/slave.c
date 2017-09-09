#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <signal.h>
#include <semaphore.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "include/queue.h"
#include "include/macros.h"


#define SEGMENTSIZE sizeof(sem_t)
#define SEGMENTPERM 0666 // All users can read and write but not execute.

#define SEMNAME "semy"

#define SHMOBJ_PATH "/itba.so.grupo3.tp1"

#define SEMINIT 1

// Compile either with -lrt or -lpthread

// http://cgi.di.uoa.gr/~ad/k22/k22-lab-notes4.pdf

// https://github.com/Jeshwanth/Linux_code_examples/blob/master/POSIX_semaphore/

/* Semaphore declaration */
sem_t * sem_id;

struct shared_data {
	char buffer[1024][MD5_LEN + 1]; /* +1 for null terminated string. */
	int last;
};

/**
 * Signal Handler for CTRL^C
 * We need this signal handler because some times when
 * manully kill the process by pressing CTRL+C, that time
 * semaphore should also Closed and unlinked. If not Next time
 * when you run the same program, it will use same semaphore
 * and will not work as expected.
**/
void signal_callback_handler(int signum) {

	/**
	* Semaphore unlink: Remove a named semaphore  from the system.
	*/
	if (shm_unlink(SHMOBJ_PATH) < 0) {
		perror("Could not unlink shared memory.");
	}

	/**
	 * Semaphore Close: Close a named semaphore
	 */
	if (sem_close(sem_id) < 0) {
		perror("Could not close semaphore.");
	}

	/**
	 * Semaphore unlink: Remove a named semaphore from the system.
	 */
	if (sem_unlink(SEMNAME) < 0) {
		perror("Could not unlink semaphore.");
	}

	// Terminate program
	exit(signum);
}

int main() {

	int shmfd;
	int shared_seg_size = (1 * sizeof(struct shared_data));   /* Shared segment capable of storing 1 message */
	struct shared_data * shared_msg;      /* The shared segment, and head of the messages list */

	/* Register signal and signal handler */
	signal(SIGINT, signal_callback_handler);

	/* Opening existing shared memory object (created by master) */
	shmfd = shm_open(SHMOBJ_PATH, O_CREAT | O_RDWR, S_IRWXU | S_IRWXG);
	if (shmfd < 0) {
		perror("Could not open/create shared memory.");
		exit(1);
	}

	fprintf(stderr, "Created/opened shared memory object %s\n", SHMOBJ_PATH);

	/* Adjusting mapped file size */
	ftruncate(shmfd, shared_seg_size);

	/**
	 * Semaphore open. Create the semaphore if it does not already exist. Initialized to 1.
	 */
	sem_id = sem_open(SEMNAME, O_CREAT, S_IRUSR | S_IWUSR, SEMINIT);

	/* Requesting the shared segment */
	shared_msg = (struct shared_data *) mmap(NULL, shared_seg_size, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
	if (shared_msg == NULL) {
		perror("Could not obtain shared segment.");
		exit(1);
	}

	fprintf(stderr, "Shared memory segment allocated correctly (%d bytes).\n", shared_seg_size);

	while (1) {

		sleep(2);
		printf("Waiting \n");
		sem_wait(sem_id);
		printf("Locked, About to sleep \n");

		int aux = shared_msg->last;

		char * test = "a527cd69111a5dbe36a198c3591c4005";

		if (aux < 1024) {
			strcpy(shared_msg->buffer[++aux], test);
			shared_msg->last++;
		}

		sleep(2);
		sem_post(sem_id);

		printf("posting \n");

	}


	if (shm_unlink(SHMOBJ_PATH) != 0) {
		perror("Could not unlink shared segment.");
		exit(1);
	}

	if (sem_close(sem_id) < 0) {
		perror("Could not close semaphore");
	}

	if (sem_unlink(SEMNAME) < 0) {
		perror("Could not unlink semaphore.");
	}

	return 0;

}