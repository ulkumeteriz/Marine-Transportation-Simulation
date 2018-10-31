#include "cargoLL.h"
#include <stdio.h>
#include <stdlib.h>


/*---------------------------linked list start-------------------------------------------------*/


node* insertToLL(node* head,cargo* data) {
    if(head==NULL) {
        // there is no element in the list
        // make one and insert 
        node* new_node = (node*)malloc(sizeof(node));
        new_node->cargo_to = data->to;
        new_node->cargo_id = data->id;
        new_node->cargo_from = data->from;
        new_node->next = NULL;
        new_node->prev = NULL;
        return new_node;
    }
    else {
        node* temp = head;
        while(temp->next!=NULL) {
            temp=temp->next;
        }
        // when while end at this point temp points to the last element of the queue
        node* new_node = (node*)malloc(sizeof(node));
        new_node->cargo_to = data->to;
        new_node->cargo_id = data->id;
        new_node->cargo_from = data->from;
        new_node->next = NULL;
        new_node->prev = temp; 
        temp->next = new_node;
        return head;
    }
}


node* removeFromLL(node* head,cargo* data) {

     if(head==NULL) return NULL;
     
     node* temp = head;
     
     if(head->cargo_to==data->to&&head->cargo_from==data->from&&head->cargo_id==data->id) {
        temp=head->next;
        if(temp!=NULL)
            temp->prev = NULL;
        free(head);
        return temp;
     }
     
     while(temp->next!=NULL) {
        if(temp->cargo_to==data->to&&temp->cargo_from==data->from&&temp->cargo_id==data->id) {
            if(temp->prev!=NULL)
                    temp->prev->next = temp->next;
                temp->next->prev = temp->prev;
                free(temp);
                return head;
            }
            
            temp=temp->next;
     }
        if(temp->cargo_to==data->to&&temp->cargo_from==data->from&&temp->cargo_id==data->id) {
            if(temp->prev!=NULL)
            temp->prev->next=NULL;
            free(temp);
        }
        return head;
    
}


int doesExist(int dockId, node* head) {

	if(head==NULL) return 0;
	node* current = head; // first element
	while(current!=NULL) {
		if (current->cargo_to == dockId) {
			return 1;
		}
		current=current->next;
	}
	return 0;
}

int doesExistLoop(int* route,int length,node* head) {

	/* !! THIS FUNCTION IS NOT TESTED YET !! */

	int i,ret=0;
	for(i=0;i<length;i++) {
		ret = doesExist(route[i],head);
		if(ret==1) break;
	}
	return ret;
}

void printLL(node* head) {
    
    node* current = head;
    while(current != NULL) {
        printf("cargo id: %d , from: %d , to: %d \n",current->cargo_id,current->cargo_from,current->cargo_to);
        current = current->next;
    } 

}

void clearLL(node* head) {
    node* temp = head;
    while(head!=NULL) {
        temp=head;
        head=head->next;
        free(temp);
    }
}

/*-------------------------------linked list end------------------------------------------------------------*/

