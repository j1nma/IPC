// #include <stdlib.h>
// #include <stdio.h>
// #include <fcntl.h>
// #include <unistd.h>
// #include <errno.h>

// #include <signal.h>
// #include <semaphore.h>

// #include <sys/mman.h>
// #include <sys/stat.h>
// #include <sys/types.h>


// #define SEGMENTSIZE sizeof(sem_t)
// #define SEGMENTPERM 0666 // All users can read and write but not execute.

// #define SEMNAME "semy"

// #define SHMOBJ_PATH "/shm"

// #define SEMINIT 1

// // Compile either with -lrt or -lpthread

// // http://cgi.di.uoa.gr/~ad/k22/k22-lab-notes4.pdf

// // https://github.com/Jeshwanth/Linux_code_examples/blob/master/POSIX_semaphore/

// /* Semaphore declaration */
// sem_t * sem_id;

// struct shared_data {
// 	int var1;
// 	int var2;
// };

// /**
//  * Signal Handler for CTRL^C
//  * We need this signal handler because some times when
//  * manully kill the process by pressing CTRL+C, that time
//  * semaphore should also Closed and unlinked. If not Next time
//  * when you run the same program, it will use same semaphore
//  * and will not work as expected.
// **/
// void signal_callback_handler(int signum) {

// 	/**
// 	* Semaphore unlink: Remove a named semaphore  from the system.
// 	*/
// 	if (shm_unlink(SHMOBJ_PATH) < 0) {
// 		perror("Could not unlink shared memory.");
// 	}

// 	/**
// 	 * Semaphore Close: Close a named semaphore
// 	 */
// 	if (sem_close(sem_id) < 0) {
// 		perror("Could not close semaphore.");
// 	}

// 	/**
// 	 * Semaphore unlink: Remove a named semaphore from the system.
// 	 */
// 	if (sem_unlink(SEMNAME) < 0) {
// 		perror("Could not unlink semaphore.");
// 	}

// 	// Terminate program
// 	exit(signum);
// }

// int main() {

// 	int shmfd;
// 	int vol, cur;
// 	int shared_seg_size = (1 * sizeof(struct shared_data));    Shared segment capable of storing 1 message 
// 	struct shared_data *shared_msg;      /* The shared segment, and head of the messages list */

// 	/* Register signal and signal handler */
// 	signal(SIGINT, signal_callback_handler);

// 	/* Creating the shared memory object */
// 	shmfd = shm_open(SHMOBJ_PATH, O_CREAT | O_RDWR, S_IRWXU | S_IRWXG);
// 	if (shmfd < 0) {
// 		perror("Could not create shared memory.");
// 		exit(1);
// 	}

// 	fprintf(stderr, "Created shared memory object %s\n", SHMOBJ_PATH);

// 	/* Adjusting mapped file size */
// 	ftruncate(shmfd, shared_seg_size);

// 	/**
// 	 * Semaphore open. Create the semaphore if it does not already exist. Initialized to 1.
// 	 */
// 	sem_id = sem_open(SEMNAME, O_CREAT, S_IRUSR | S_IWUSR, SEMINIT);

// 	/* Requesting the shared segment  */
// 	shared_msg = (struct shared_data *) mmap(NULL, shared_seg_size, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
// 	if (shared_msg == NULL) {
// 		perror("Could not obtaine shared segment.");
// 		exit(1);
// 	}

// 	fprintf(stderr, "Shared memory segment allocated correctly (%d bytes).\n", shared_seg_size);



// 	vol = 10;
// 	cur = 0;

// 	while (1) {

// 		sleep(2);
// 		printf("Waiting \n");
// 		sem_wait(sem_id);
// 		printf("Locked, About to sleep \n");
// 		printf("SLAVES TURN \n");
// 		printf("The var1 is %d \n", shared_msg->var1);
// 		printf("The var2 is %d \n", shared_msg->var2);
// 		sleep(5);
// 		sem_post(sem_id);
// 		printf("posting \n");
// 	}


// 	if (shm_unlink(SHMOBJ_PATH) != 0) {
// 		perror("Could not unlink shared segment.");
// 		exit(1);
// 	}

// 	if (sem_close(sem_id) < 0) {
// 		perror("Could not close semaphore");
// 	}

// 	if (sem_unlink(SEMNAME) < 0) {
// 		perror("Could not unlink semaphore.");
// 	}

// 	return 0;

// }