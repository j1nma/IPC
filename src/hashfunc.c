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

#include <sys/mman.h>

#include "macros.h"
#include "hashfunc.h"


#define SLAVES 5
#define TRUE 1
#define FALSE 0

#define SLAVE_READY 1




int toMasterDescriptors[SLAVES][2];
int toSlavesDescriptors[SLAVES][2];

// https://stackoverflow.com/questions/8465006/how-do-i-concatenate-two-strings-in-c

char* concat(const char *s1, const char *s2) {
	char *result = malloc(strlen(s1) + strlen(s2) + 1);

	if (result == NULL) {
		printf("Memory allocation failed.");
		return NULL;
	}

	strcpy(result, s1);
	strcat(result, s2);
	return result;
}

// https://stackoverflow.com/questions/4553012/checking-if-a-file-is-a-directory-or-just-a-file

int is_regular_file(const char *path) {
	struct stat path_stat;

	if (stat(path, &path_stat) != 0)
		return 0;

	return S_ISREG(path_stat.st_mode);
}

void loadFiles(char* path, struct Queue *q) {
	DIR* dir;
	struct dirent *ent;

	if ((dir = opendir(path)) != NULL) {

		while (( ent = readdir(dir)) != NULL) {

			int isCurrent = !strcmp(ent->d_name, ".");
			int isFaga = !strcmp(ent->d_name, "..");

			if (!isCurrent && !isFaga) {

				char* current = concat(concat(path, "/"), ent->d_name);
				if (current == NULL) {
					closedir(dir);
					return;
				}

				if (is_regular_file(current)) {
					int fd = open(current, O_RDONLY);
					struct stat file_stat;
					fstat(fd, &file_stat);

					enQueue(q, current);
				}

				loadFiles(current, q);
			}
		}

		closedir(dir);
	}
}



void send(int descriptor[2], char* data, int length) {

	// This function is basically the one that was given to us as an example.
	int w = length;
	int written = 0;
	int fd;

	close(descriptor[0]);
	fd = descriptor[1];

	while (written < w) {
		written += write(
		               fd, data + written, w - written
		           );
	}

	close(descriptor[1]);

}

char* recieve(int descriptor[], int pid) {

	// This function is basically the one that was given to us as an example.
	int r = 0;
	int fd;
	int size = 1024;
	char* readBuffer = malloc(sizeof(char) * 1024);
	char * readPtr = readBuffer;

	close(descriptor[1]);
	fd = descriptor[0];


	while (size > 0 && (r = read(fd, readPtr, size))) {
		readPtr += r;
		size -= r;
	}

	readBuffer[readPtr - readBuffer] = '\0';


	close(descriptor[0]);

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

	// Get the file descriptor for both pipes. One master->slave and the other slave->master.
	int k;
	int toSlave[2];
	int toMaster[2];
	for (k = 0; k < 2; k++) toMaster[k] = toMasterDescriptors[i][k];
	for (k = 0; k < 2; k++) toSlave[k] = toSlavesDescriptors[i][k];

	// Sending the SLAVE_READY to notify the master that the slave is ready for work.
	char data[1];
	data[0] = SLAVE_READY;
	send(toMaster, data, 1);

	// After sending, wait for a job directory.
	char* ret = recieve(toSlave, getpid());


	printf("(%i) Got job: %s\n", getpid(), ret);

}



void startMaster(int i) {


	// Get the file descriptor for both pipes. One master->slave and the other slave->master.
	int k;
	int toSlave[2];
	int toMaster[2];
	for (k = 0; k < 2; k++) toSlave[k] = toSlavesDescriptors[i][k];
	for (k = 0; k < 2; k++) toMaster[k] = toMasterDescriptors[i][k];


	char* ret = recieve(toMaster, getpid());

	if (ret[0] == SLAVE_READY) {
		printf("Slave sent ready, sending job...\n");
		char *job = "/Dev/Data/Testy/SO/So/SOSOSO/archivo.ai";
		send(toSlave, job, strlen(job));
	} else {
		printf("Error: %d\n", ret[0]);
	}

}

int start(struct Queue * q) {

	int i;
	pid_t pid;
	int status;


	// First set up pipes for each slave.
	for (int i = 0; i < SLAVES; ++i)
		setupSlavePipe(i);


	// Get it forking...
	for (i = 0; i < SLAVES; i++) {
		if ((pid = fork()) == -1) {
			// Fork returned error.
			perror("Fork error.");
			exit(EXIT_FAILURE);

		} else if (pid == 0) {
			// This is a slave.
			startSlave(i);
			exit(EXIT_SUCCESS);
		} else {
			// This is the master.
			startMaster(i);
		}
	}

	// The program only gets here if it's the parent/master.
	while (-1 != wait(&status));

	// Here status can be the following constants: WIFEXITED,WIFEXITSTATUS, etc. No use currently.
	// https://www.tutorialspoint.com/unix_system_calls/wait.htm


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