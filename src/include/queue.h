struct QNode {
    char* key;
    struct QNode *next;
};

struct Queue {
    struct QNode *front, *rear;
};

struct QNode * newNode(char* k);

struct Queue * createQueue();

void enQueue(struct Queue *q, char* k);

struct QNode * deQueue(struct Queue *q);