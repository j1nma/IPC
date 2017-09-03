#include <stdio.h>
#include <ctype.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <macros.h>
#include <hashmake.h>

// https://github.com/espressif/esp-idf/issues/823

char* concat(const char *s1, const char *s2) {
	char *result = malloc(strlen(s1) + strlen(s2) + 1);

	if (result == NULL) {
		printf("Memory allocation failed");
		return;
	}

	strcpy(result, s1);
	strcat(result, s2);
	return result;
}

// https://stackoverflow.com/questions/4553012/checking-if-a-file-is-a-directory-or-just-a-file

int is_regular_file(const char *path) {
	struct stat path_stat;
	stat(path, &path_stat);

	return S_ISREG(path_stat.st_mode);
}

int calculateMD5(char *file_name, char *md5_sum) {

#define MD5SUM_CMD_FMT "md5sum %." STR(PATH_LEN) "s 2>/dev/null"
	char cmd[PATH_LEN + sizeof (MD5SUM_CMD_FMT)];
	sprintf(cmd, MD5SUM_CMD_FMT, file_name);
#undef MD5SUM_CMD_FMT

	FILE *p = popen(cmd, "r");
	if (p == NULL) return 0;

	int i, ch;
	for (i = 0; i < MD5_LEN && isxdigit(ch = fgetc(p)); i++) {
		*md5_sum++ = ch;
	}

	*md5_sum = '\0';
	pclose(p);
	return i == MD5_LEN;
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

				if (is_regular_file(current)) {
					if (calculateMD5(current, md5)) {
						printf("%s : %s\n", current, md5);
					} else {
						puts("Error occured with MD5!");
					}
				}

				listDir(current);
			}

		}

		closedir(dir);
	}
}