#include "queue.h"


void loadFiles(char* path, struct Queue *q);

/*  Calculates the MD5 file passed as parameter.
*/
int calculateMD5(char *file_name, char *md5_sum);

/*  Calculates the size of the file at the specified
	path by verifying how long is the data stream.
*/
long int getFileSize(char* fpath);

/*  Function to swap nodes x and y in linked list by
	changing links.
*/
void swapNodes(struct QNode **head_ref, int x, int y);

/*  Separates the queue acording to the chosen pivot.
*/
int partition (Queue q, int low, int high);

/*  Sorts the queue using the quicksort method.
*/
void quickSort(Queue q, int low, int high);