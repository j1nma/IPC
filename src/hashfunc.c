#include <stdio.h>
#include <openssl/md5.h>
#include <hashmake.h>

void hashFile(int argc, const char* fileName) {

	unsigned char c[MD5_DIGEST_LENGTH];
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

	for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
		printf("%02x", c[i]);

	printf (" %s\n", fileName);

	fclose (file);

}

void readAndHash(int argc, char** files) {

	if (argc >= 2) {

		printf("%d %s %s\n\n", argc - 1, (argc - 1 == 1) ? "file" : "files", "to process:");

		// https://stackoverflow.com/questions/10324611/how-to-calculate-the-md5-hash-of-a-large-file-in-c

		for (int i = 1; i < argc ; i++)
			hashFile(argc, files[i]);

	} else {

		printf("%s\n", "No files provided.");

	}

}