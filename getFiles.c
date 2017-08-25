#include <stdio.h>
#include <openssl/md5.h>


int main(int argc, char **argv) {

	if (argc >= 2) {

		printf("%d %s\n", argc - 1, "files to process.");

		char const* const fileName = argv[1];
		FILE* file = fopen(fileName, "r");
		char line[256];

		if ( file != NULL ) {

			while (fgets(line, sizeof(line), file)) {
				/* note that fgets don't strip the terminating \n, checking its
				   presence would allow to handle lines longer that sizeof(line) */
				printf("%s", line);
			}
			/* may check feof here to make a difference between eof and io failure -- network
			   timeout for instance */

			fclose(file);

		} else {

			printf("%s\n", "Could not open files.");
			return 1;
		}

	} else {

		printf("%s\n", "No files provided.");

	}



	return 0;
}