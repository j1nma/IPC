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

#define SLAVES 2
#define TRUE 1
#define FALSE 0

#define SLAVE_READY 1


int toMasterDescriptors[SLAVES][2];
int toSlavesDescriptors[SLAVES][2];
int toMaster[2];

/* Semaphore declaration */
sem_t * sem_id;

void send(int descriptor[2], char* data, int length) {

	// This function is basically the one that was given to us in class as an example.
	int w = length;
	int written = 0;
	int fd;

	// if (descriptor[0] != -1) {
	// close(descriptor[0]);
	// descriptor[0] = -1;
	// }

	fd = descriptor[1];

	// printf("fd %d\n", fd);

	while (written < w) {
		written += write(fd, data + written, w - written);
		// printf("errno %d\n", errno);

	}

	// if (descriptor[1] != -1) {
	// close(descriptor[1]);
	// descriptor[1] = -1;
	// }

}

char* recieve(int descriptor[]) {

	// This function is basically the one that was given to us in class as an example.
	int r = 0;
	int fd;
	int size = 1024;
	char* readBuffer = malloc(sizeof(char) * 1024);
	char * readPtr = readBuffer;

	// if (descriptor[1] != -1) {
	// close(descriptor[1]);
	// descriptor[1] = -1;
	// }

	fd = descriptor[0];

	while (size > 0 && (r = read(fd, readPtr, size))) {
		readPtr += r;
		size -= r;
	}

	readBuffer[readPtr - readBuffer] = '\0';

	// if (descriptor[0] != -1) {
	// close(descriptor[0]);
	// 	descriptor[0] = -1;
	// }

	return readBuffer;
}

void setupSlavePipe(int i) {

	//Creating both file descriptors. One master->slave and the other slave->master.
	int toSlave[2];
	// int toMaster[2];

	if (pipe(toSlave) != 0) {
		printf("Could not create master->slave pipe.\n");
	}

	// if (pipe(toMaster) != 0) {
	// 	printf("Could not create slave->master pipe.\n");
	// }

	int k;
	for (k = 0; k < 2; k++) toSlavesDescriptors[i][k] = toSlave[k];
	// for (k = 0; k < 2; k++) toMasterDescriptors[i][k] = toMaster[k];

}

void startSlave(int i) {

	// Get the file descriptor for both pipes. One master->slave and the other slave->master.
	int k;
	int toSlave[2];
	// int toMaster[2];
	// for (k = 0; k < 2; k++) toMaster[k] = toMasterDescriptors[i][k];
	for (k = 0; k < 2; k++) toSlave[k] = toSlavesDescriptors[i][k];

	struct shared_data * shared_msg = (struct shared_data *) malloc( sizeof(struct shared_data *) ); /* The shared segment, and head of the messages list */
	prepareSharedMemoryWithSemaphores(sem_id, &shared_msg);

	char* ret;
	char data[1];
	data[0] = SLAVE_READY;

	// Sending the SLAVE_READY to notify the master that the slave is ready for work.
	// send(toMaster, data, 1);

	write(toMaster[1], data + 0, 1);
	close(toMaster[1]);

	// After sending, wait for a job directory.
	// close(toSlave[1]);
	ret = recieve(toSlave);
	// close(toSlave[0]);

	// printf("read %zd\n", read(toSlave[0], ret, sizeof(ret)));

	// printf("(%i) Got job: %s\n", getpid(), ret);

	// char md5[MD5_LEN + 1];

	// if (!calculateMD5(ret, md5)) {
	// 	printf("Could not calculate md5 of %s.\n", ret);
	// }

	// sem_wait(sem_id);

	// int aux = shared_msg->last;

	// if (aux < 1024) {
	// 	strcpy(shared_msg->buffer[++aux], md5);
	// 	shared_msg->last++;
	// }

	// sem_post(sem_id);

	// } while (ret[0] != '\0');

}

void startMaster(int slaves, struct shared_data * shared_msg) {

	// // Get the file descriptor for both pipes. One master->slave and the other slave->master.
	int k;
	// int toSlave[2];

	// for (k = 0; k < 2; k++) toSlave[k] = toSlavesDescriptors[i][k];
	// // for (k = 0; k < 2; k++) toMaster[k] = toMasterDescriptors[i][k];

	// close(toMaster[1]);

	for (k = 0; k < slaves; k++) {

		// ret = recieve(toMaster, getpid());

		struct QNode * qnode = deQueue(shared_msg->queue);

		if (qnode == NULL) {
			printf("%s\n", "No more files to process.");
			return;
		}

		char * job = qnode->key;

		// send(toSlavesDescriptors[k], job, strlen(job));

		write(toSlavesDescriptors[k][1], job, strlen(job));
		close(toSlavesDescriptors[k][1]);

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

	// Set the number of slaves for the forking loop.
	int slaves = SLAVES;

	if (SLAVES > q->size) {
		slaves = q->size;
	}

	// int jobsPerSlave = q->size / slaves;

	struct shared_data *shared_msg = (struct shared_data *) malloc( sizeof(struct shared_data *) );   /* The shared segment, and head of the messages list */
	prepareSharedMemoryWithSemaphores(sem_id, &shared_msg);

	sem_wait(sem_id);

	shared_msg->last = 0;
	shared_msg->queue = q;

	sem_post(sem_id);

	// Get it forking...
	for (i = 0; i < slaves; i++) {

		if ((pid = fork()) == -1) {
			// Fork returned error.

			perror("Fork error");
			exit(EXIT_FAILURE);

		} else if (pid == 0) {
			// This is a slave.

			printf("This is %d\n", getpid());
			close(toSlavesDescriptors[i][0]);
			startSlave(i);

			exit(EXIT_SUCCESS);
		}
	}

	startMaster(slaves, shared_msg);

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

	terminateSemaphore(sem_id);

	printf("Terminó el padre.\n");
	exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {

	srand(time(NULL));

	struct Queue * q = createQueue();

	loadFiles(argv[1], q);

	// struct QNode * aux;
	// while ( (aux = deQueue(q)) != NULL)
	// 	printf("%s\n", aux->key);

	start(q);

	return 0;
}