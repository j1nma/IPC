#include <stdio.h>
#include <ctype.h>

#include <macros.h>
#include <hashmake.h>

int calculateMD5(const char *file_name, char *md5_sum) {

#define MD5SUM_CMD_FMT "md5sum %." STR(PATH_LEN) "s 2>/dev/null"
	char cmd[PATH_LEN + sizeof (MD5SUM_CMD_FMT)];
	sprintf(cmd, MD5SUM_CMD_FMT, file_name);
#undef MD5SUM_CMD_FMT

	puts(cmd);

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

void recieveAndCalculate(int fileQuantity, char **files) {

	// https://stackoverflow.com/questions/3395690/md5sum-of-file-in-linux-c

	if (fileQuantity > 1) {

		printf("%d %s %s\n\n", fileQuantity - 1, (fileQuantity - 1 == 1) ? "file" : "files", "to process:");

		// https://stackoverflow.com/questions/10324611/how-to-calculate-the-md5-hash-of-a-large-file-in-c

		char md5[MD5_LEN + 1];

		for (int i = 1; i < fileQuantity ; i++) {
			// hashFile(fileQuantity, files[i]);

			if (!calculateMD5(files[i], md5)) {
				puts("Error occured!");
			} else {
				printf("Success! MD5 sum of %s is: %s\n", files[i], md5);
			}

		}

	} else {

		printf("%s\n", "No files provided.");

	}

}