#include "queue.h"


void loadFiles(char* path, struct Queue *q);

/*
Calculates the MD5 file passed as parameter.
*/
int calculateMD5(char *file_name, char *md5_sum);