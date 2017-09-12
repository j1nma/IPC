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

#define SLAVES 10
#define TRUE 1
#define FALSE 0

#define SLAVE_READY 1
#define SLAVE_DONE 0


int toMasterDescriptors[SLAVES][2];
int toSlavesDescriptors[SLAVES][2];


/* Semaphore declaration */
sem_t * sem_id;

void send(int descriptor[2], char* data, int length) {

	// This function is basically the one that was given to us in class as an example.
	int w = length;
	int written = 0;
	int fd;

	fd = descriptor[1];

	while (written < w) {
		written += write(fd, data + written, w - written);
	}


}

char* recieve(int descriptor[]) {

	// This function is basically the one that was given to us in class as an example.
	int r = 0;
	int fd;
	int size = 1024;
	char* readBuffer = malloc(sizeof(char) * 1024);
	char * readPtr = readBuffer;


	fd = descriptor[0];


	while (size > 0 && (r = read(fd, readPtr, size))) {
		readPtr += r;
		size -= r;
		// printf("LEI (%i): %d\n", readPtr[0]);
		
	}

	readBuffer[readPtr - readBuffer] = '\0';

	printf("LEO\n");


	return readBuffer;
}

void setupSlavePipe(int i) {

	//Creating both file descriptors. One master->slave and the other slave->master.
	int toSlave[2];
	int toMaster[2];

	if (pipe(toSlave) != 0) {
		printf("Could not create master->slave pipe.\n");
	}

	if (pipe(toMaster) != 0) {
		printf("Could not create slave->master pipe.\n");
	}



	int k;
	for (k = 0; k < 2; k++) toSlavesDescriptors[i][k] = toSlave[k];
	for (k = 0; k < 2; k++) toMasterDescriptors[i][k] = toMaster[k];


}

void startSlave(int i) {

	char data[1];
	int k;
	int toSlave[2];
	int toMaster[2];
	for (k = 0; k < 2; k++) toSlave[k] = toSlavesDescriptors[i][k];
	for (k = 0; k < 2; k++) toMaster[k] = toMasterDescriptors[i][k];

	while(1) {

		// Get the file descriptor for both pipes. One master->slave and the other slave->master.


		struct shared_data * shared_msg = (struct shared_data *) malloc( sizeof(struct shared_data *) ); /* The shared segment, and head of the messages list */
		prepareSharedMemoryWithSemaphores(sem_id, &shared_msg);


		data[0] = SLAVE_READY;
		close(toMaster[0]);
		write(toMaster[1], data, 1);

		// VAMOS A LEER :)

		close(toSlave[1]);
		char buffer[80];
		read(toSlave[0], buffer, 80);

		if (buffer[0] != 0) {
			printf("(%i): El master me devolvio: %s\n", getpid(), buffer);
		}else{
			printf("Mi padre me mato!!!!!\n");
			break;
		}


		char md5[MD5_LEN + 1];
        if (!calculateMD5(buffer, md5)) {
            printf("Could not calculate md5 of %s.\n", buffer);
            return;
        }
        sem_wait(sem_id);
        int aux = shared_msg->last;
        if (aux < 1024) {
            strcpy(shared_msg->buffer[++aux], md5);
            shared_msg->last++;
        }
        sem_post(sem_id);
		
	}


	data[0] = SLAVE_DONE;
	close(toMaster[0]);
	write(toMaster[1], data, 1);

	printf("(%i): TERMINO EL ESCLAVO\n", getpid());

}

void startMaster(int slaves, struct shared_data * shared_msg) {

	puts("Starts master.");



	while (1) {



		fd_set set;
		FD_ZERO(&set);
		struct timeval timeout;
		timeout.tv_sec = 10;
		timeout.tv_usec = 0;
		int max = 0;
		for (int j = 0; j < slaves; j++) {
			if (toMasterDescriptors[j][0] != -1) {
				FD_SET(toMasterDescriptors[j][0], &set);
				if (toMasterDescriptors[j][0] > max)
					max = toMasterDescriptors[j][0];
			}
		}
		errno = 0; // REINICIO errno.

		if (max == 0) {
			printf("Termino el padre <=========================\n");
			return;
		}

		int aux = select(max + 1, &set, NULL, NULL, &timeout);

		for (int i = 0; i < slaves; i++) {

			// Get the file descriptor for both pipes. One master->slave and the other slave->master.
			int k;
			int toSlave[2];
			int toMaster[2];
			for (k = 0; k < 2; k++) toSlave[k] = toSlavesDescriptors[i][k];
			for (k = 0; k < 2; k++) toMaster[k] = toMasterDescriptors[i][k];


			if (aux == -1) {
				printf("Error devolvio -1 con errno: %s <============= FATALITY\n",  strerror(errno));
				return;
			}


			printf("Llame a select = %i\n", aux);

			if (toMasterDescriptors[i][0] != -1 && FD_ISSET(toMaster[0], &set)) {

				// printf("Me fije el FD_ISSET y dio true, voy a llamar a read\n");

				close(toMaster[1]);
				char buffer[80];
				read(toMaster[0], buffer, 1);

				if (buffer[0] != SLAVE_DONE) {
					printf("El esclavo contesto: %d\n", buffer[0]);

					// VAMOS A ESCRIBIR :0

					struct QNode * qnode = deQueue(shared_msg->queue);
					char *data;
					if (qnode == NULL) {
						printf("No hay mas trabajos. Matando esclavo.\n");
						data = "\0";
					}else{
						data = qnode->key;
					}
					close(toSlave[0]);
					printf("Mando: %s\n", data);
					write(toSlave[1], data, strlen(data) + 1); //Mas 1 por el \0.
				} else {
					printf("Retorno 0, murio el esclavo :'(.\n");
					toMasterDescriptors[i][0] = -1;
					break;
				}
			}
		}

		printf("Soy master y di una vuelta :)\n");
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

	// if (SLAVES > q->size) {
	// 	slaves = q->size;
	// }

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

	// terminateSemaphore(sem_id);

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