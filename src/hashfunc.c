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

#define SLAVES 5
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
	close(descriptor[1]);
	// descriptor[1] = -1;
	// }

	fd = descriptor[0];

	printf("%d reading from %d\n", getpid(), fd);

	while (size > 0 && (r = read(fd, readPtr, size))) {
		readPtr += r;
		size -= r;
	}

	readBuffer[readPtr - readBuffer] = '\0';

	// if (descriptor[0] != -1) {
	close(descriptor[0]);
	// 	descriptor[0] = -1;
	// }

	return readBuffer;
}

void setupSlavePipe(int i) {

	//Creating both file descriptors. One master->slave and the other slave->master.
	int toSlave[2];

	if (pipe(toSlave) != 0) {
		printf("Could not create master->slave pipe.\n");
	}

	int k;
	for (k = 0; k < 2; k++) toSlavesDescriptors[i][k] = toSlave[k];

}

void startSlave(int i) {

	// Get the file descriptor for both pipes. One master->slave and the other slave->master.
	int k;
	int toSlave[2];
	for (k = 0; k < 2; k++) toSlave[k] = toSlavesDescriptors[i][k];

	struct shared_data * shared_msg = (struct shared_data *) malloc( sizeof(struct shared_data *) ); /* The shared segment, and head of the messages list */
	prepareSharedMemoryWithSemaphores(sem_id, &shared_msg);

	char data[1];
	data[0] = SLAVE_READY;

	printf("This is %d\n", getpid());

	fd_set set;
	int res;
	char buf[256];
	struct timeval timeout;

	/* Initialize the file descriptor set. */
	FD_ZERO(&set);
	FD_SET(toSlave[0], &set);

	/* Initialize the timeout data structure. */
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;

	select(toSlave[0] + 1, &set, NULL, NULL, &timeout);

	if (FD_ISSET(toSlave[0], &set)) {

		res = read(toSlave[0], buf, sizeof(buf));

		if (res > 0) {
			printf("Read %s\n", buf);
		}

		char md5[MD5_LEN + 1];

		if (!calculateMD5(buf, md5)) {
			printf("Could not calculate md5 of %s.\n", buf);
		}

		sem_wait(sem_id);

		int aux = shared_msg->last;

		if (aux < 1024) {
			strcpy(shared_msg->buffer[++aux], md5);
			shared_msg->last++;
		}

		sem_post(sem_id);
	}

}

void startMaster(int slaves, struct shared_data * shared_msg) {

	// Get the file descriptor for both pipes. One master->slave and the other slave->master.
	int k;

	close(toMaster[1]);

	for (k = 0; k < slaves; k++) {

		struct QNode * qnode = deQueue(shared_msg->queue);

		if (qnode == NULL) {
			printf("%s\n", "No more files to process.");
			return;
		}

		char * job = qnode->key;

		write(toSlavesDescriptors[k][1], job, strlen(job));
		printf("Master wrote to %d\n", toSlavesDescriptors[k][1]);
		close(toSlavesDescriptors[k][1]);
	}

}

long int getFileSize(char* fpath) {
	// Based on answer provided at:
	// https://stackoverflow.com/questions/238603/how-can-i-get-a-files-size-in-c

	long int sz = 0;

    FILE *p = popen(cmd, "r");
    if (p == NULL) return 0;

    // Seeks to the end of the file and then returns the position
    fseek(p, 0L, SEEK_END);
    sz = ftell(fp);

    // Resets the pointer to the beginning of the file
    fseek(p, 0L, SEEK_SET);

    pclose(p);

    return sz;
}

void swapNodes(struct QNode **head_ref, int x, int y) {
	// http://www.geeksforgeeks.org/swap-nodes-in-a-linked-list-without-swapping-data/
	// Nothing to do if x and y are same
	if (x == y) return;

	// Search for x (keep track of prevX and CurrX
	struct Node *prevX = NULL, *currX = *head_ref;
	while (currX && currX->data != x)
	{
		prevX = currX;
		currX = currX->next;
	}

	// Search for y (keep track of prevY and CurrY
	struct Node *prevY = NULL, *currY = *head_ref;
	while (currY && currY->data != y)
	{
	   prevY = currY;
	   currY = currY->next;
	}

	// If either x or y is not present, nothing to do
	if (currX == NULL || currY == NULL)
	   return;

	// If x is not head of linked list
	if (prevX != NULL)
	   prevX->next = currY;
	else // Else make y as new head
	   *head_ref = currY;  

	// If y is not head of linked list
	if (prevY != NULL)
	   prevY->next = currX;
	else  // Else make x as new head
	   *head_ref = currX;

	// Swap next pointers
	struct Node *temp = currY->next;
	currY->next = currX->next;
	currX->next  = temp;
}

int partition (Queue q, int low, int high) {
	// www.geeksforgeeks.org/// quick-sort/  adapted to our Queue	Agregar cuando ande todo esto de slave/master con pipes
    struct QNode pivot;		    // pivot
    struct QNode current;
    struct QNode smaller;
	int pivotAux = (((double) high) * (2/5));
    int i = (low - 1);  // Index of smaller element
    int j = low;
    long int pivotSize = 0;

    pivot = q->first;
    while(pivotAux != 0) {
    	pivot = pivot->next;
    	pivotAux--;
    }
    pivotSize = getFileSize(pivot->key);

    current = q->first;
    while(j != 0) {
    	current = current->next;
    	j--;
    }
 
    for (j = low; j <= high- 1; j++) {
        // If current element is smaller than or
        // equal to pivot
        if (getFileSize(current->key) <= pivotSize) {
            i++;    // increment index of smaller element
            swapNodes(q->first, i, j);
        }
    }
    swapNodes(q->first, i + 1, high);
    return (i + 1);
}

void // quickSort(Queue q, int low, int high) {	Agregar cuando ande todo esto de slave/master con pipes
	// www.geeksforgeeks.org/// quick-sort/	Agregar cuando ande todo esto de slave/master con pipes
    if (low < high)
    {
        /* pi is partitioning index, arr[p] is now
           at right place */
        int pi = partition(q, low, high);
 
        // Separately sort elements before
        // partition and after partition
        // quickSort(q, low, pi - 1);	Agregar cuando ande todo esto de slave/master con pipes
        // quickSort(q, pi + 1, high);	Agregar cuando ande todo esto de slave/master con pipes
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

	// quickSort(q);	Agregar cuando ande todo esto de slave/master con pipes

	start(q);

	return 0;
}