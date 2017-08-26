#include <stdio.h>
#include <openssl/md5.h>

#include <hashmake.h>

// en una mac:
// $ gcc getFiles.c -o crsce -I /usr/local/opt/openssl/include -L /usr/local/opt/openssl/lib
// $ ./getFiles testies/*
// (tener instalado openssl)

void readAndHash(int argc, char **argv) {

	if (argc >= 2) {

		printf("%d %s %s\n", argc - 1, (argc - 1 == 1) ? "file" : "files", "to process.");

		// https://stackoverflow.com/questions/10324611/how-to-calculate-the-md5-hash-of-a-large-file-in-c

		unsigned char c[MD5_DIGEST_LENGTH];
		char const* const fileName = argv[1];
		FILE* file = fopen(fileName, "r");
		MD5_CTX mdContext;
		int bytes;
		unsigned char data[1024];

		if ( file == NULL ) {
			printf ("%s can't be opened.\n", fileName);
			return;
		}

		MD5_Init (&mdContext);
		while ((bytes = fread (data, 1, 1024, file)) != 0)
			MD5_Update (&mdContext, data, bytes);

		MD5_Final (c, &mdContext);

		for (int i = 0; i < MD5_DIGEST_LENGTH; i++) printf("%02x", c[i]);
		printf (" %s\n", fileName);

		fclose (file);

		// while (fgets(line, sizeof(line), file)) {
		// 	 note that fgets don't strip the terminating \n, checking its
		// 	   presence would allow to handle lines longer that sizeof(line)
		// 	printf("%s", line);
		// }
		// /* may check feof here to make a difference between eof and io failure -- network
		//    timeout for instance */

		// fclose(file);


	} else {

		printf("%s\n", "No files provided.");

	}

}