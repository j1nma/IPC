#include <stdio.h>
#include <ctype.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/mman.h>

#include "macros.h"
#include "hashfunc.h"

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

int startSlaves(struct Queue * q) {
	int i;
	pid_t pid;
	int status;

	// int filesCount = q->size;

	// char md5[MD5_LEN + 1];

	void* shmem = create_shared_memory(1024);
	memcpy(shmem, q, sizeof(*q));

	for (i = 0; i < 5; i++) {

		if ((pid = fork()) == -1) {

			perror("Fork error.");
			exit(EXIT_FAILURE);

		} else if (pid == 0) {

			printf("Child (%d): %d\n", i + 1, getpid());

			printf("child dequed %s\n", deQueue(q)->key);
			memcpy(shmem, q, sizeof(*q));

			exit(EXIT_SUCCESS);

		} else {
			printf("parent first %s\n", q->front->key);
			sleep(5);
			printf("parent first %s\n", q->front->key);
		}
	}

	while (-1 != wait(&status));

	exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {

	struct Queue * q = createQueue();

	loadFiles(argv[1], q);

	// struct QNode * aux;

	// while ( (aux = deQueue(q)) != NULL)
	// 	printf("%s\n", aux->key);

	startSlaves(q);

	return 0;
}