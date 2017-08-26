#include <stdio.h>
#include <openssl/md5.h>

#include <hashmake.h>

// en una mac:
// $ gcc getFiles.c -o crsce -I /usr/local/opt/openssl/include -L /usr/local/opt/openssl/lib
// $ ./getFiles testies/*
// (tener instalado openssl)

int main(int argc, char **argv) {

	readAndHash(argc, argv);

	return 0;
}