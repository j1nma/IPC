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

#define SLAVES 5
#define TRUE 1
#define FALSE 0

#define SLAVE_READY 1




int toMasterDescriptors[SLAVES][2];
int toSlavesDescriptors[SLAVES][2];


#define SEMNAME "semy"
#define SHMOBJ_PATH "/itba.so.grupo3.tp1"

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
void signalCallbackHandler(int signum) {

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

void prepareSharedMemoryWithSemaphores(int * shmfd, int * shared_seg_size, struct shared_data * * shared_msg) {

	/* Register signal and signal handler */
	signal(SIGINT, signalCallbackHandler);

	/* Creating the shared memory object */
	*shmfd = shm_open(SHMOBJ_PATH, O_CREAT | O_RDWR, S_IRWXU | S_IRWXG);
	if (*shmfd < 0) {
		perror("Could not create shared memory.");
		exit(1);
	}

	/* Adjusting mapped file size */
	ftruncate(*shmfd, *shared_seg_size);

	/* Semaphore open. Create the semaphore if it does not already exist. Initialized to 1. */
	sem_id = sem_open(SEMNAME, O_CREAT, S_IRUSR | S_IWUSR, 1);

	/* Requesting the shared segment  */
	*shared_msg = (struct shared_data *) mmap(NULL, *shared_seg_size, PROT_READ | PROT_WRITE, MAP_SHARED, *shmfd, 0);
	if (*shared_msg == NULL) {
		perror("Could not obtain shared segment.");
		exit(1);
	}
}

void terminateSemaphore() {

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

	int shmfd;
	int shared_seg_size = (1 * sizeof(struct shared_data));   /* Shared segment capable of storing 1 message */
	struct shared_data * shared_msg = (struct shared_data *) malloc( sizeof(struct shared_data *) ); /* The shared segment, and head of the messages list */

	prepareSharedMemoryWithSemaphores(&shmfd, &shared_seg_size, &shared_msg);

	char md5[MD5_LEN + 1];

	if (!calculateMD5(ret, md5)) {
		printf("Could not calculate md5 of %s.\n", ret);
	}

	//  else {
	// 	printf("%s's md5 is %s.\n\n", ret, md5);
	// }

	sem_wait(sem_id);

	int aux = shared_msg->last;

	if (aux < 1024) {
		strcpy(shared_msg->buffer[++aux], md5);
		shared_msg->last++;
	}

	// sleep(2);
	sem_post(sem_id);

}



void startMaster(int i, struct Queue * q, struct shared_data *shared_msg) {


	// Get the file descriptor for both pipes. One master->slave and the other slave->master.
	int k;
	int toSlave[2];
	int toMaster[2];
	for (k = 0; k < 2; k++) toSlave[k] = toSlavesDescriptors[i][k];
	for (k = 0; k < 2; k++) toMaster[k] = toMasterDescriptors[i][k];


	char* ret = recieve(toMaster, getpid());

	if (ret[0] == SLAVE_READY) {
		printf("Slave sent ready, sending job...\n");

		struct QNode * qnode = deQueue(q);

		// char *job = "/Dev/Data/Testy/SO/So/SOSOSO/archivo.ai";
		char *job = qnode->key;

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


	int shmfd;
	int shared_seg_size = (1 * sizeof(struct shared_data));   /* Shared segment capable of storing 1 message */
	struct shared_data *shared_msg = (struct shared_data *) malloc( sizeof(struct shared_data *) );   /* The shared segment, and head of the messages list */

	prepareSharedMemoryWithSemaphores(&shmfd, &shared_seg_size, &shared_msg);

	sem_wait(sem_id);

	shared_msg->last = 0;

	sem_post(sem_id);

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
			startMaster(i, q, shared_msg);
		}
	}

	// The program only gets here if it's the parent/master.
	while (-1 != wait(&status));

	// Here status can be the following constants: WIFEXITED,WIFEXITSTATUS, etc. No use currently.
	// https://www.tutorialspoint.com/unix_system_calls/wait.htm

	sem_wait(sem_id);

	int j;

	for (j = 0; j <= shared_msg->last; j++) {
		printf("%s", shared_msg->buffer[j]);
		printf("\n");

	}

	// sleep(1);
	sem_post(sem_id);

	terminateSemaphore();

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