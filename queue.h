#ifndef __QUEUE__
#define __QUEUE__

typedef struct queueNode {

    int data;
    struct queueNode* next;
    struct queueNode* prev;

} queueNode;

queueNode* insertToQueue(queueNode* head,int data);
queueNode* removeHead(queueNode* head);
queueNode* deleteFromQueue(queueNode* head,int data);
queueNode* clearQueue(queueNode* head);
int getHeadData(queueNode* head);

#endif
