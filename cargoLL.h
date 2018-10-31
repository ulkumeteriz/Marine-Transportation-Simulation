#ifndef __CARGOLL__
#define __CARGOLL__
typedef struct cargo {
        int id;
        int from;
        int to;
} cargo;

typedef struct node {
    int cargo_id;
    int cargo_from;
    int cargo_to;
    struct node* prev;
    struct node* next; 
} node;




int doesExist(int dockId, node* head);
node* removeFromLL(node* head, cargo * cargo);
node* insertToLL(node* head, cargo* cargo);
int doesExistLoop(int* route,int length,node* head);
void printLL(node* head);
void clearLL(node* head);

#endif
