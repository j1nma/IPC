#include <stdio.h>
#include <ctype.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <macros.h>
#include <hashfunc.h>

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

void listDir(char* path) {
	DIR* dir;
	struct dirent *ent;

	char md5[MD5_LEN + 1];

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
					if (calculateMD5(current, md5)) {
						printf("%s : %s\n", current, md5);
					} else {
						puts("Error occured with MD5.");
					}
				}

				listDir(current);
			}

		}

		closedir(dir);
	}
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

int main(int argc, char **argv) {

	struct Queue * q = createQueue();
	loadFiles(argv[1], q);

	struct QNode * aux;

	while ( (aux = deQueue(q)) != NULL)
			printf("%s\n", aux->key);

	return 0;
}