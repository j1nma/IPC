#include <stdio.h>
#include <openssl/md5.h>
#include <hashmake.h>

// (tener instalado openssl)

int main(int argc, char ** argv) {

	readAndHash(argc, argv);
	
	return 0;
}