#include <stdlib.h>
#include <stdio.h>

#include "include/queue.h"

/*
 *  Simple queue implementation from
 *  http://www.geeksforgeeks.org/queue-set-2-linked-list-implementation/
 *  accesed by j1nma, 1 Sep. 2017 
 */

struct QNode * newNode(char * k) {
    struct QNode *temp = (struct QNode*)malloc(sizeof(struct QNode));
    temp->key = k;
    temp->next = NULL;
    return temp;
}

struct Queue * createQueue() {
    struct Queue *q = (struct Queue*)malloc(sizeof(struct Queue));
    q->front = q->rear = NULL;
    q->size = 0;
    return q;
}

void enQueue(struct Queue * q, char * k) {
    struct QNode *temp = newNode(k);

    if (q->rear == NULL) {
        q->front = q->rear = temp;
        q->size++;
        return;
    }

    q->rear->next = temp;
    q->rear = temp;
    q->size++;
}

struct QNode * deQueue(struct Queue * q) {
    if (q->front == NULL)
        return NULL;

    struct QNode *temp = q->front;
    q->front = q->front->next;

    if (q->front == NULL)
        q->rear = NULL;

    q->size--;
    return temp;
}
