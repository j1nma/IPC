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

void* create_shared_memory(size_t size) {
	// Our memory buffer will be readable and writable:
	int protection = PROT_READ | PROT_WRITE;

	// The buffer will be shared (meaning other processes can access it), but
	// anonymous (meaning third-party processes cannot obtain an address for it),
	// so only this process and its children will be able to use it:
	int visibility = MAP_ANONYMOUS | MAP_SHARED;

	// The remaining parameters to `mmap()` are not important for this use case,
	// but the manpage for `mmap` explains their purpose.
	return mmap(NULL, size, protection, visibility, 0, 0);
}

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

void send(int descriptor[2], int pid) {
	int w = 4;
	int written = 0;
	int fd;

	close(descriptor[0]);
	fd = descriptor[1];

	char str[12];
	sprintf(str, "%d", pid);

	
	while (written < w){
		written += write(fd, str + written, w - written);
	}

	close(descriptor[1]);

}

void recieve(int descriptor[], int pid) {
	int r = 0;
	int fd;
	int size = 1024;
	char readBuffer[1024];
	char * readPtr = readBuffer;

	close(descriptor[1]);
	fd = descriptor[0];
	

	while (size > 0 && (r = read(fd, readPtr, size))) {
		readPtr += r;
		size -= r;
	}

	readBuffer[readPtr - readBuffer] = '\0';

	printf("%i -> %s\n", pid, readBuffer);

	close(descriptor[0]);

}


int toMasterDescriptors[SLAVES][2];
int toSlavesDescriptors[SLAVES][2];


void setupSlavePipe(int i) {
	
	int toSlave[2];
	int toMaster[2];

	if (pipe(toSlave) != 0) {
		printf("Could not create pipe.\n");
	}
	if (pipe(toMaster) != 0) {
		printf("Could not create pipe.\n");
	}
	
	int k;
	for (k = 0; k < 2; k++) toSlavesDescriptors[i][k] = toSlave[k];
	for (k = 0; k < 2; k++) toMasterDescriptors[i][k] = toMaster[k];
}



void startSlavePipe(int i) {

	int toSlave[2];
	int toMaster[2];
	int k;
	for (k = 0; k < 2; k++) toMaster[k] = toMasterDescriptors[i][k];
	for (k = 0; k < 2; k++) toSlave[k] = toSlavesDescriptors[i][k];

	
	send(toMaster, getpid());

	recieve(toSlave, getpid());


}

void startMasterPipe(int i) {


	int toSlave[2];
	int toMaster[2];
	int k;
	for (k = 0; k < 2; k++) toSlave[k] = toSlavesDescriptors[i][k];
	for (k = 0; k < 2; k++) toMaster[k] = toMasterDescriptors[i][k];

	
	recieve(toMaster, getpid());

	send(toSlave, getpid());

}

int start(struct Queue * q) {

	int i;
	pid_t pid;
	int status;


	for (int i = 0; i < SLAVES; ++i) {
		setupSlavePipe(i);
	}


	for (i = 0; i < SLAVES; i++) {

		if ((pid = fork()) == -1) {
			// Fork returned error.
			perror("Fork error.");
			exit(EXIT_FAILURE);

		} else if (pid == 0) {
			// This is the child.
			// sleep(i);
			// printf("Child (%d): %d\n", i + 1, getpid());
			sleep(i);
			startSlavePipe(i);
			exit(EXIT_SUCCESS);
		} else {
			// This is the parent.
			// sleep(10);
			// printf("Master (%d): %d\n", i + 1, getpid());
			startMasterPipe(i);
		}
	}

	// The program only gets here if its the parent/master.
	while (-1 != wait(&status));

	// Here status can be the following constants: WIFEXITED,WIFEXITSTATUS, etc. No use currently.
	// https://www.tutorialspoint.com/unix_system_calls/wait.htm


	printf("Termino el padre.\n");
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