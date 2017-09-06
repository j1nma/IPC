#include "queue.h"

// typedef struct node_t {
// 	pqueue_pri_t pri;
// 	char*    val;
// 	size_t pos;
// } node_t;

/*
Gets each directory until it cannot find another.
*/
void listDir(char *path);

// void loadFiles(char* path, pqueue_t *pq, node_t  *ns, int index);

void loadFiles(char* path, struct Queue *q);

/*
Calculates the MD5 file passed as parameter.
*/
int calculateMD5(char *file_name, char *md5_sum);