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

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <signal.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <fcntl.h>


#include "macros.h"
#include "shmsema.h"

sem_t * sem_id;


int main(int argc, char const *argv[]) {

	int pidMaster;
	scanf("%d", &pidMaster);

	sem_id = prepareSemaphore(pidMaster);

	// Creating the shared memory.
	struct shared_data *shared_msg = (struct shared_data *) malloc( sizeof(struct shared_data *) );   /* The shared segment, and head of the messages list */
	prepareSharedMemory(&shared_msg);

	while (1) {
		sem_wait(sem_id);
		int j;
		for (j = 0; j < shared_msg->last; j++) {
			printf("<%s> : <%s>", shared_msg->names[j], shared_msg->buffer[j]);
			printf("\n");
		}
		sem_post(sem_id);
		printf("----------------------------------------------------------\n");
		sleep(1);
	}


	return 0;
}