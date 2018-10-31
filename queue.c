#include <stdio.h>
#include <stdlib.h>
#include "queue.h"


queueNode* insertToQueue(queueNode* head,int data) {
    if(head==NULL) {
        // there is no element in the list
        // make one and insert 
        queueNode* new_node = (queueNode*)malloc(sizeof(queueNode));
        new_node->data = data;
        new_node->next = NULL;
        new_node->prev = NULL;
        return new_node;
    }
    else {
        queueNode* temp = head;
        while(temp->next!=NULL) {
            temp=temp->next;
        }
        // when while end at this point temp points to the last element of the queue
        queueNode* new_node = (queueNode*)malloc(sizeof(queueNode));
        new_node->data = data;
        new_node->next = NULL;
        new_node->prev = temp; 
        temp->next = new_node;
        return head;
    
    }
     
}


queueNode* removeHead(queueNode* head) {

    if(head==NULL) return NULL; // there is no element at the list

    queueNode* temp_head = head->next;
    if(temp_head != NULL)
        temp_head -> prev = NULL;
    free(head);
    return temp_head;

}

queueNode* deleteFromQueue(queueNode* head,int data) {

     if(head==NULL) return NULL;
     
     queueNode* temp = head;
     
     if(head->data==data) {
        temp=head->next;
        if(temp!=NULL)
            temp->prev = NULL;
        free(head);
        return temp;
     }
     
     while(temp->next!=NULL) {
        if(temp->data==data) {
            if(temp->prev!=NULL)
                    temp->prev->next = temp->next;
                temp->next->prev = temp->prev;
                free(temp);
                return head;
            }
            
            temp=temp->next;
     }
        if(temp->data==data) {
            if(temp->prev!=NULL)
            temp->prev->next=NULL;
            free(temp);
        }
        return head;
    
}


int getHeadData(queueNode* head){
    if(head==NULL) return -1;
    else return head->data;
}

void print(queueNode* head) {
        if(head==NULL) {
        return;
    }
    else {
        queueNode* temp = head;
        while(temp->next!=NULL) {
            fprintf(stderr,"%d -> ",temp->data);
            temp=temp->next;
        }
        fprintf(stderr,"%d \n",temp->data);
            
    }

}

queueNode* clearQueue(queueNode* head) {
    if(head==NULL) return NULL;
    queueNode* temp = head;
    while(temp->next!=NULL) {
        temp=temp->next;
        free(temp->prev);
    }
    free(temp);
    return NULL; //?
}


