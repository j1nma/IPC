#include <stdio.h>
#include <ctype.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/mman.h>
#include <signal.h>
#include <semaphore.h>

#include "macros.h"
#include "hashfunc.h"
#include "shmsema.h"

#define SLAVES 3
#define TRUE 1
#define FALSE 0

#define SLAVE_READY 1
#define SLAVE_DONE 0
#define KILL_SLAVE 0

#define PIPE_CLOSED -1

int toMasterDescriptors[SLAVES][2];
int toSlavesDescriptors[SLAVES][2];


/* Semaphore declaration */
sem_t * sem_id;


void setupSlavePipe(int i) {

	//Creating both file descriptors. One master->slave and the other slave->master.
	int toSlave[2];
	int toMaster[2];

	// Creating pipes!
	if (pipe(toSlave) != 0) {
		printf("Could not create master->slave pipe.\n");
	}
	if (pipe(toMaster) != 0) {
		printf("Could not create slave->master pipe.\n");
	}

	// Copy the fd to the fd arrays.
	int k;
	for (k = 0; k < 2; k++) toSlavesDescriptors[i][k] = toSlave[k];
	for (k = 0; k < 2; k++) toMasterDescriptors[i][k] = toMaster[k];


}

void startSlave(int i) {


	char data[1];
	int k;
	// Get the file descriptor for both pipes. One master->slave and the other slave->master.
	int toSlave[2];
	int toMaster[2];
	for (k = 0; k < 2; k++) toSlave[k] = toSlavesDescriptors[i][k];
	for (k = 0; k < 2; k++) toMaster[k] = toMasterDescriptors[i][k];

	while(1) {

		struct shared_data * shared_msg = (struct shared_data *) malloc( sizeof(struct shared_data *) ); /* The shared segment, and head of the messages list */
		prepareSharedMemoryWithSemaphores(sem_id, &shared_msg);

		// Create message and write it.
		data[0] = SLAVE_READY;
		close(toMaster[0]);
		write(toMaster[1], data, 1);

		// Wait for response.
		close(toSlave[1]);
		char buffer[200]; // Maximum 200 because limitations are great!
		read(toSlave[0], buffer, 200);

		// Should I die?
		if (buffer[0] == KILL_SLAVE)
			break;

		// Calculating MD5!
		char md5[MD5_LEN + 1];
        if (!calculateMD5(buffer, md5)) {
            printf("Could not calculate MD5 of %s.\n", buffer);
            return;
        }
        // Block the shared mem.
        sem_wait(sem_id);
        int aux = shared_msg->last; // Get the last.
        if (aux < 1024) {
            strcpy(shared_msg->buffer[++aux], md5); // Copy the result.
            shared_msg->last++; // Increment the buffer.
        }
        sem_post(sem_id); // Unblock the sheard mem.
		
	}

	// If the slave got here, it shoud dieeee...

	data[0] = SLAVE_DONE; // Create message and send it.
	close(toMaster[0]);
	write(toMaster[1], data, 1);

	// Close the pipes!
	close(toMaster[1]);
	close(toSlave[0]);
}

void startMaster(struct shared_data * shared_msg) {

	while (1) {

		// This sets up the select parameters.
		fd_set set;
		FD_ZERO(&set);
		struct timeval timeout;
		timeout.tv_sec = 10;
		timeout.tv_usec = 0;
		int max = 0;
		for (int j = 0; j < SLAVES; j++) {
			if (toMasterDescriptors[j][0] != PIPE_CLOSED) {
				FD_SET(toMasterDescriptors[j][0], &set);
				if (toMasterDescriptors[j][0] > max)
					max = toMasterDescriptors[j][0];
			}
		}
		errno = 0; 


		// If max is 0 then no more file descriptors are available.
		if (max == 0) {
			// Return the function, master process is done.
			return;
		}

		// This function hangs the process until any file descriptor is ready.
		int available = select(max + 1, &set, NULL, NULL, &timeout);


		// Iterrate slaves.
		for (int i = 0; i < SLAVES; i++) {

			// Get the file descriptor for both pipes. One master->slave and the other slave->master.
			int k;
			int toSlave[2];
			int toMaster[2];
			for (k = 0; k < 2; k++) toSlave[k] = toSlavesDescriptors[i][k];
			for (k = 0; k < 2; k++) toMaster[k] = toMasterDescriptors[i][k];

			if (available == -1) {
				perror("Fatal error. Linux faild.");
				return;
			}

			// If the current pipe isnt closed and select returned an event
			if (toMasterDescriptors[i][0] != -1 && FD_ISSET(toMaster[0], &set)) {

				// Read the slave state.
				close(toMaster[1]);
				char buffer[1];
				read(toMaster[0], buffer, 1);

				if (buffer[0] == SLAVE_READY) {

					// If the slave is done, get next job.
					struct QNode * qnode = deQueue(shared_msg->queue);
					char *data;
					if (qnode == NULL) {
						// If no more jobs, send kill slave :)
						char *killSlave = malloc(sizeof(char));
						killSlave[0] = KILL_SLAVE;
						data = killSlave;
					}else{
						// Otherwise send job dir string.
						data = qnode->key;
					}

					// Write to the pipe.
					close(toSlave[0]);
					write(toSlave[1], data, strlen(data) + 1); //+1 for the null termination.
				} else if (buffer[0] == SLAVE_DONE) {
					// The slave returned SLAVE_DONE, kill it, close pipe.
					toMasterDescriptors[i][0] = PIPE_CLOSED;
					close(toSlave[1]);
					close(toMaster[0]);
					break; // Break the loop so select is called again.
				} else {
					perror("Fatal error. Pipe com.");
					return;
				}
			}
		}
	}
}


int start(struct Queue * q) {

	// No files to process.
	if (q->size == 0) {
		printf("%s\n", "No files to process.");
		exit(EXIT_SUCCESS);
	}

	int i;
	pid_t pid;
	int status;

	// Set up pipe for master.
	int toMaster[2];
	if (pipe(toMaster) != 0) {
		printf("Could not create slave->master pipe.\n");
	}

	// Set up pipes for each slave.
	for (int i = 0; i < SLAVES; ++i)
		setupSlavePipe(i);


	// Creating the shared memory.
	struct shared_data *shared_msg = (struct shared_data *) malloc( sizeof(struct shared_data *) );   /* The shared segment, and head of the messages list */
	prepareSharedMemoryWithSemaphores(sem_id, &shared_msg);


	// Block shmem.
	sem_wait(sem_id);

	// Initiate shmem.
	shared_msg->last = 0;
	shared_msg->queue = q;

	// Unblock shmem.
	sem_post(sem_id);


	// Get to forking...
	for (i = 0; i < SLAVES; i++) {
		if ((pid = fork()) == -1) {
			// Fork returned error.
			perror("Fork error");
			exit(EXIT_FAILURE);

		} else if (pid == 0) {
			// This is a slave.
			startSlave(i);
			exit(EXIT_SUCCESS);
		}
	}

	startMaster(shared_msg);

	// The program only gets here if it's the parent/master.
	while (-1 != wait(&status));
	// No more children of the master remaining.

	// Here status can be the following constants: WIFEXITED,WIFEXITSTATUS, etc. No use currently.
	// https://www.tutorialspoint.com/unix_system_calls/wait.htm


	sem_wait(sem_id);
	int j;
	for (j = 0; j <= shared_msg->last; j++) {
		printf("%s", shared_msg->buffer[j]);
		printf("\n");
	}
	sem_post(sem_id);

	printf("Terminó el padre.\n");
	exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {

	srand(time(NULL));

	struct Queue * q = createQueue();

	loadFiles(argv[1], q);

	start(q);

	return 0;
}