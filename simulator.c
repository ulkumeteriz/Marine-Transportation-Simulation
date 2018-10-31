#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include "writeOutput.h"
#include "queue.h"
#include "cargoLL.h"
#include <unistd.h>

int count;
sem_t mutex1,barrier;

typedef struct dock {
        int id;
        int capacity;
        int remaining_capacity;
        int loadCount;
        int unloadCount;
        int unloadWaitCount;
        int loadWaitCount;
        node* cargo_list;
        queueNode* enterenceQueue;
        queueNode* loadQueue;
        queueNode* unloadQueue;
        sem_t enterenceMutex;
        sem_t loadMutex;
        sem_t unloadMutex;
        sem_t cargoMutex;

} dock;

typedef struct ship {
        int id;
        int capacity;
        int used_capacity;
        int travel_time;
        int arrival_time;
        int current_route;
        int route_length;
        int* route;
        node* cargoes;
        sem_t ePermission;
        sem_t lPermission;
        sem_t uPermission;
} ship;

typedef struct threadParameter {
	ship* ships;
	ship* theShip;
	dock* docks;
	int nos;
} threadParameter;

/*-------------------HELPERS--------------------------------*/

void notifyAllUnloader(dock* theDock,ship* ships, int shipId2) {

	queueNode* uQ = theDock->unloadQueue;
	int shipId = 0 ;
    int i = 0;
    // You have to protect unloadQueue with unloadMutex

    sem_wait(&(theDock->unloadMutex));
    int limit = (theDock->unloadWaitCount);
    for(i=0;i<limit;i++) {
         shipId = getHeadData(uQ);
         if(shipId!=-1) {
		    theDock->unloadCount++;
		    theDock->unloadWaitCount--;
		    uQ = removeHead(uQ);
		    sem_post(&(theDock->unloadMutex));
			sem_post(&(ships[shipId].uPermission));
		}
		else sem_post(&(theDock->unloadMutex));
    
    }

}

void notifyAllLoader(dock* theDock,ship* ships , int shipId2) {


	queueNode* lQ = theDock->loadQueue;
	int shipId = 0 ;
    int i = 0;
    // You have to protect unloadQueue with unloadMutex

    sem_wait(&(theDock->loadMutex));
    int limit = (theDock->loadWaitCount);
    for(i=0;i<limit;i++) {
         shipId = getHeadData(lQ);
         if(shipId!=-1) {
		    theDock->loadCount++;
		    theDock->loadWaitCount--;
		    lQ = removeHead(lQ);
		    sem_post(&(theDock->loadMutex));
			sem_post(&(ships[shipId].lPermission));
		}
		else sem_post(&(theDock->loadMutex));
    
    }

}





int isOnMyWay(int to, int* route, int route_length) {
	int i;
	for(i=0;i<route_length;i++) {
		if(route[i]==to) return 1;
	}
	return 0;
}

cargo loadOneToShip(node** cargo_list,int* route,int route_length,int current_route) {
    // cargo_list of dock.
	node* current = *cargo_list;
	cargo cargo_temp;
	cargo_temp.id=-1;
	while(current!=NULL) {
	    // 1. to variable of current cargo, 2. starting from following route , 3. remaining length of route
		if (isOnMyWay( current->cargo_to, &(route[current_route+1]), (route_length-current_route-1) ) )
		{
			cargo_temp.id = current->cargo_id;
			cargo_temp.to = current->cargo_to;
			cargo_temp.from = current->cargo_from;
			// remove the cargo
			*cargo_list = removeFromLL(*cargo_list, &cargo_temp);
			return cargo_temp;
		}
		else {
			current=current->next;
		}
	}
	return cargo_temp;
}


void unloadHelper(int shipId, int dockId, node** cargoes, dock* dock, ship* ships) {

		node* current = *cargoes;
		cargo cargo_temp;

		while(current!=NULL) { // for each cargo of mine
		
		// if there exist a cargo to unload, unload
		if(current->cargo_to == dockId) {
			cargo_temp.id=current->cargo_id;
			cargo_temp.to=current->cargo_to;
			cargo_temp.from=current->cargo_from;
					
			WriteOutput(shipId,dockId,cargo_temp.id,UNLOAD_CARGO);
			usleep(2000); // sleep for 2 ms
				
				// TODO : put the cargoes in an array at the end of the function remove them in a loop
				// TODO : because this way may cause a problem
				
				// remove the unloaded one from cargoes in the ship and decrement the used_capacity
			ships[shipId].used_capacity--;
			current = current->next;
			*cargoes = removeFromLL(*cargoes,&cargo_temp);
		}
				//take next one before removing the current one
		else current = current->next;
	}
}


void loadHelper(int shipId, int dockId, int* route, int route_length, int current_route, node** cargoes , int capacity, int* used_capacity, dock* theDock) {

	cargo cargoToLoad;

    while(*used_capacity<=capacity) {
		//	for each cargo in dock->cargo_array if the destination of a cargo is included in the route of ship 
		
		// 	take the cargo info and remove it and protect cargo_list of theDock
		sem_wait(&(theDock->cargoMutex));
		cargoToLoad = loadOneToShip(&(theDock->cargo_list),route,route_length,current_route);
		sem_post(&(theDock->cargoMutex));
		
		// there is one cargo to load to the ship whose information in cargoToLoad.
		if(cargoToLoad.id != -1) { 
			WriteOutput(shipId,dockId,cargoToLoad.id,LOAD_CARGO);
			usleep(3000); // sleep 3 ms
			// insert this cargo to the ship's list
			*cargoes = insertToLL(*cargoes,&cargoToLoad);
			// increment the used_capacity of the ship
			(*used_capacity)=(*used_capacity)+1;
		}
		else break;	// there is no cargo to load left, break the loop. 
	}
}


/*-------------------------HELPERS END--------------------------------*/

void travel(int shipId,int dockId,int travelTime){
    usleep(1000*travelTime);    
}

void enterDock(int shipId,int dockId,dock* theDock,ship* ships) {
    WriteOutput(shipId,dockId,0,REQUEST_ENTRY);
    
    // you have to protect enterenceQueue and the remaining_capacity
    // there should be only one thread which change them
    
    sem_wait(&(theDock->enterenceMutex)); // wait for permission of modify
    
    if(theDock->remaining_capacity <= 0) {
        // there is no capacity in the dock, wait in the enterenceQueue
        theDock->enterenceQueue=insertToQueue(theDock->enterenceQueue,shipId);
        theDock->remaining_capacity--;
        sem_post(&(theDock->enterenceMutex)); // say that you're done with modification job

        // wait for your permission signalled
       
        sem_wait(&(ships[shipId].ePermission)); 
      
    }
    else {
        // there is capacity, enter directly
        theDock->remaining_capacity--;
        sem_post(&(theDock->enterenceMutex)); // say that you're done with modification job
    
    }
    
    WriteOutput(shipId,dockId,0,ENTER_DOCK);
}

void unload(int shipId,int dockId,node** cargoes,dock* theDock,ship* ships) {
    
     WriteOutput(shipId,dockId,0,REQUEST_UNLOAD);
     
     // while controlling theDock's variable protect them from modification
     sem_wait(&(theDock->loadMutex));
     sem_wait(&(theDock->unloadMutex));
     // there is loading process going on in the dock, wait in the queue.
     if(theDock->unloadCount==0 && theDock->loadCount>0) {
         // I have done with controlling the load variables for this case
         sem_post(&(theDock->loadMutex));
         
         // you have already protected dock's unloadQueue and waitCount
             //insert yourself the unloadQueue
             theDock->unloadQueue = insertToQueue(theDock->unloadQueue,shipId);
             // increment the number of ships waiting for unload.
             theDock->unloadWaitCount++;
             
         // say that you are done with modification
         sem_post(&(theDock->unloadMutex));
         
         
         // look whether you have a permission to unload, the last load process will notify me
         sem_wait(&(ships[shipId].uPermission));
         
                // OK , I'm in, let me erase myself from the queue and decrement the waiting count.
       

         // now I can start the unload process , for each cargo of mine look the ones whose "to" value is the current dockId
         unloadHelper(shipId,dockId,cargoes,theDock,ships);
         
         // I'm done with unload process
         sem_wait(&(theDock->unloadMutex)); 
              theDock->unloadCount--;
         sem_post(&(theDock->unloadMutex));
     
     }
     // there are unloading processes in dock, I can also unload
     else if (theDock->unloadCount > 0) {
         // I have done with controlling for this case
         sem_post(&(theDock->loadMutex));
         
         // increment the unloadCount while protecting it, leave my uPermission 0.
             theDock->unloadCount++;
         // say that you are done with modification
         sem_post(&(theDock->unloadMutex));
         
          // now I can start the unload process , for each cargo of mine look the ones whose "to" value is the current dockId
         unloadHelper(shipId,dockId,cargoes,theDock,ships);
          // I'm done with unload process
         sem_wait(&(theDock->unloadMutex)); 
              theDock->unloadCount--;
         sem_post(&(theDock->unloadMutex));
         
     }
     // there is neither loading nor unloading process currently in theDock 
     else if (theDock->unloadCount==0 && theDock->loadCount==0) {
        // I have done with controlling for this case
         sem_post(&(theDock->loadMutex));
         
         // increment the unloadCount while protecting it
             theDock->unloadCount++;
         sem_post(&(theDock->unloadMutex));
         
          // now I can start the unload process , for each cargo of mine look the ones whose "to" value is the current dockId
         unloadHelper(shipId,dockId,cargoes,theDock,ships);
          // I'm done with unload process
         sem_wait(&(theDock->unloadMutex)); 
              theDock->unloadCount--;
         sem_post(&(theDock->unloadMutex));
     
     }
     else {
        //printf("WTF?! There are loading and unloading ships at the same time!\n WAKE UP!!\n");
     }
     
     // control the unloaderCount and loaderWaitCount
     sem_wait(&(theDock->loadMutex));
     sem_wait(&(theDock->unloadMutex));
     if(theDock->unloadCount == 0 && theDock->loadWaitCount > 0) {
        sem_post(&(theDock->loadMutex));
        sem_post(&(theDock->unloadMutex));
        notifyAllLoader(theDock,ships,shipId);
     } 
     else {
        sem_post(&(theDock->loadMutex));
        sem_post(&(theDock->unloadMutex));
     }
     
}  

void load(int shipId,int dockId,int* route,int route_length,int current_route,node** cargoes,int capacity,int* used_capacity,dock* theDock,ship* ships) {

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_nsec += 20000000;
     
     // if used_capacity of mine is equal to my capacity I cannot load anyway go away and away on the open sea <3
     // else load until my used_capacity becomes equal to my capacity
     if((*used_capacity) != capacity) { // since these variables are owned by ship itself, there is no need to protect them.
        
        WriteOutput(shipId,dockId,0,REQUEST_LOAD);
     
        sem_wait(&(theDock->cargoMutex));
        if(doesExistLoop(&route[current_route+1],(route_length-current_route),theDock->cargo_list)) { 
            sem_post(&(theDock->cargoMutex));
        // protect the loadCount and unloadCount from modification
        sem_wait(&(theDock->loadMutex));
        sem_wait(&(theDock->unloadMutex));
        
        // there is unloading processes, wait 
        if(theDock->loadCount==0 && theDock->unloadCount > 0) {
            // I am done with controlling the unloadCount for this case
             sem_post(&(theDock->unloadMutex));
            
            // loadWaitCount and loadQueue are being protected already
                //insert yourself to the loadQueue 
                theDock->loadWaitCount++;
                theDock->loadQueue = insertToQueue(theDock->loadQueue,shipId);
            sem_post(&(theDock->loadMutex));
            
            // wait for lPermission of yours just for 20ms
                
            int s = sem_timedwait(&(ships[shipId].lPermission),&ts);
            if(s==-1) { // time is up, go go go!
                  
                sem_wait(&(theDock->loadMutex));
                  
                    // delete yourself from the queue 
                    theDock->loadWaitCount--;
                    theDock->loadQueue = deleteFromQueue(theDock->loadQueue,shipId);
                sem_post(&(theDock->loadMutex));
                return;
            }
            
                loadHelper(shipId, dockId, route,route_length,current_route, cargoes , capacity,used_capacity,theDock);
                
                // I'm done with load process
                sem_wait(&(theDock->loadMutex)); 
                    theDock->loadCount--;
                sem_post(&(theDock->loadMutex));
        }
        // there is no loading or unloading process you can load.
        else if(theDock->loadCount==0 && theDock->unloadCount == 0) {
            sem_post(&(theDock->unloadMutex));
            // loadMutex is on your hand
                theDock->loadCount++;
            sem_post(&(theDock->loadMutex));
            
            loadHelper(shipId, dockId, route,route_length,current_route, cargoes , capacity,used_capacity,theDock);
   
            // I'm done with load process
            sem_wait(&(theDock->loadMutex)); 
                theDock->loadCount--;
            sem_post(&(theDock->loadMutex));
        }
        //there are loading ones in the dock I can also load!
        else if (theDock->loadCount > 0 && theDock->unloadCount==0) {
            sem_post(&(theDock->unloadMutex));
                theDock->loadCount++;
            sem_post(&(theDock->loadMutex));
            loadHelper(shipId, dockId, route,route_length,current_route, cargoes , capacity,used_capacity,theDock);
            
            // I'm done with load process
            sem_wait(&(theDock->loadMutex)); 
                theDock->loadCount--;
            sem_post(&(theDock->loadMutex));       
            
        }
        else {
           // printf("WTF?! There are loading and unloading ships at the same time!\n WAKE UP!!\n");
        }
     }
     else sem_post(&(theDock->cargoMutex));
    }
    
    sem_wait(&(theDock->loadMutex));
    sem_wait(&(theDock->unloadMutex));
    if(theDock->loadCount==0 && theDock->unloadWaitCount > 0){
        sem_post(&(theDock->loadMutex));
        sem_post(&(theDock->unloadMutex));
        notifyAllUnloader(theDock,ships,shipId);
     }
     else {
        sem_post(&(theDock->loadMutex));
        sem_post(&(theDock->unloadMutex));
     }
    
    
}
       		
void exitDock(int shipId,int dockId,dock* theDock,ship* ships) {
        
     sem_wait(&(theDock->enterenceMutex));
       
        if(theDock->remaining_capacity < 0) {
 
            // there is someone waiting for enterence, notify him
            int theNextShip = getHeadData(theDock->enterenceQueue);
            if(theNextShip>=0) {
              
                sem_post(&(ships[theNextShip].ePermission));
             
               
                theDock->enterenceQueue=deleteFromQueue(theDock->enterenceQueue,shipId);

            }
            else {
               
            }
        }
        else {
            theDock->remaining_capacity++;
         
        }
         
     sem_post(&(theDock->enterenceMutex));
     
     WriteOutput(shipId,dockId,0,LEAVE_DOCK);
     
}

void* shipRoutine(void * threadParameterPointer) {
    
    pthread_detach(pthread_self()); // detaching thread
    threadParameter* myParameter = threadParameterPointer;

    ship* ships = myParameter->ships;
    int nos = myParameter->nos;
    ship* theShip = myParameter->theShip;
    dock* docks = myParameter->docks;
        	
    int travelTime = theShip->arrival_time;
    int dockId,i,isLastOne=0;
        	
    WriteOutput(theShip->id,0,0,CREATE_SHIP);
        	
    for(i=0;i<theShip->route_length;i++) {
        dockId = theShip->route[i];
        
        travel(theShip->id,dockId,travelTime);
        travelTime = theShip->travel_time;
        
        enterDock(theShip->id,dockId,&docks[dockId],ships);
        
        if(doesExist(dockId,theShip->cargoes)) {
        	unload(theShip->id,dockId,&(theShip->cargoes),&docks[dockId],ships);        
        }
        
        if(i!=(theShip->route_length)-1) {
       		// the current dock is not the last entry on the Route
       		load(theShip->id,dockId,theShip->route,theShip->route_length,theShip->current_route,&(theShip->cargoes),theShip->capacity,&(theShip-> used_capacity),&(docks[dockId]),ships); 
       		exitDock(theShip->id,dockId,&docks[dockId],ships); 
        	theShip->current_route++;
       	}
    }

	WriteOutput(theShip->id,0,0,DESTROY_SHIP);
			// these are for communication with the main function. 
	sem_wait(&mutex1);
	count++;
	if (count == nos) {
		sem_post(&barrier);
	}
	sem_post(&mutex1);
}


int main(){
        int i,j,nod, nos, noc;
        
        count=0;
        sem_init(&mutex1,0,1);
		sem_init(&barrier,0,0);

        // first line
        scanf("%d %d %d",&nod,&nos,&noc);

        //allocate structs
        dock * docks = (dock *) malloc(nod*sizeof(dock));
        ship * ships = (ship *) malloc(nos*sizeof(ship));
        
        // second line , assignment of dock capacities.
        int c;
        for(i=0;i<nod;i++) {
                scanf("%d",&c);
                docks[i].capacity = c;
                docks[i].remaining_capacity = c;
                docks[i].id = i ;
                docks[i].loadCount = 0;
                docks[i].unloadCount = 0;
                docks[i].loadWaitCount = 0;
                docks[i].unloadWaitCount = 0;
                docks[i].enterenceQueue = NULL;
                docks[i].loadQueue = NULL;
                docks[i].unloadQueue = NULL;
                
                // initialize the semaphore values of docks.
                sem_init(&(docks[i].unloadMutex),0,1);
                sem_init(&(docks[i].loadMutex),0,1);
                sem_init(&(docks[i].enterenceMutex),0,1);
                sem_init(&(docks[i].cargoMutex),0,1);
                
                // initialize the head of cargo_list 
                docks[i].cargo_list = NULL;
        }
        
        // getting ship information
        int travel_time,arrival_time,length,capacity;
        for(i=0;i<nos;i++) {
                scanf("%d %d %d %d", &travel_time,&capacity,&arrival_time,&length);
                ships[i].id = i;
                ships[i].travel_time = travel_time;
                ships[i].capacity = capacity;
                ships[i].used_capacity = 0;
                ships[i].arrival_time = arrival_time;
                ships[i].route_length = length;
                ships[i].route = (int*)malloc(sizeof(int)*length);
                for (j=0;j<length;j++) { // get the route information
                        scanf("%d",&ships[i].route[j]);
                }
                ships[i].current_route = 0 ; // initialize the current route index
                
                sem_init(&(ships[i].ePermission),0,0); 
                sem_init(&(ships[i].lPermission),0,0); 
                sem_init(&(ships[i].uPermission),0,0); 
                
                // initialize the head of cargoes LL for each ship
                ships[i].cargoes = NULL;
        }
        
        // getting cargo information 

        int from,to;
        cargo cargo_temp;
        for (i=0;i<noc;i++) {
        	scanf("%d %d",&from,&to);
        	cargo_temp.id = i;
        	cargo_temp.to = to;
        	cargo_temp.from = from;
        	insertToLL(docks[from].cargo_list,&cargo_temp);
        }
	
        /* initialize threads */

		pthread_t myThread;

		InitWriteOutput();

		for(i=0;i<nos;i++) {
			threadParameter* myParameter = (threadParameter*) malloc(sizeof(threadParameter));
			myParameter->docks = docks;
			myParameter->ships = ships;
			myParameter->theShip = &ships[i];
			myParameter->nos = nos;
			pthread_create(&myThread,NULL,shipRoutine,(void *)myParameter);
		}


		sem_wait(&barrier); // waiting for all threads finish their jobs.

	// free the cargoes list in the docks and ships before deleting them

   	for(i=0;i<nod;i++){
    	clearLL(docks[i].cargo_list);
    	//docks[i].enterenceQueue = clearQueue(docks[i].enterenceQueue);
    	//docks[i].loadQueue = clearQueue(docks[i].loadQueue);
    	//docks[i].unloadQueue = clearQueue(docks[i].unloadQueue);
    }

    for(i=0;i<nos;i++) {
    	clearLL(ships[i].cargoes);
    	free(ships[i].route);
    }

	free(docks);
	free(ships);
return 0;
}
