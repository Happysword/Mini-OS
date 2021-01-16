#include "headers.h"
#include "queue.c"
#include <math.h>

// Global Variables
int timePassed = 0, runTime = 0;
int numberOfTotalProc, numberOfFinishedProc = 0;
int totalWT = 0;
int msgq_id, msgq_id2, msgq_id3;
int mode;
bool RunningFlag = false;
Node *currentProcess;
float* WTAnums;
int mode, quantum;
struct msgbuff message;
msgrembuff msgrem,msgrem2;
int rec_val, rem_time;
int current_Clk;

// Ready Queue
Node *head = NULL;
Node *tail = NULL;

// Waiting queue
Node* waitingHead = NULL;
Node* waitingTail = NULL;



///////////////////////////////////////////////////////////////////
//////////////// Buddy system allocation Functions ////////////////
///////////////////////////////////////////////////////////////////



// Head and tail of the linked list of pairs (Buddy system)
pair *freeList[11];
pair* newPair(int start, int end);
void pushPair(pair **listHead, pair * newPair);
pair* popPair(pair **listHead);
void clearFreeList();
pair* allocate(int memsize);
void deallocate(pair* freePair);
void tryToAllocate();
FILE *logFile;


pair* newPair(int start, int end){
    pair *newP = (pair *)malloc(sizeof(pair));
    newP->start = start;
    newP->end = end;
    newP->next = NULL;
    return newP;
}

void pushPair(pair **listHead, pair * newPair){

    newPair->next = NULL;

    if(*listHead == NULL){
        *listHead = newPair;
    }else{
        pair* temp = *listHead;
        while (temp->next != NULL)
        {
            temp = temp->next;
        }
        
        temp->next = newPair;
    }
}

pair* popPair(pair **listHead){
    pair *popped;
    if(listHead != NULL){
        popped = *listHead;
        *listHead = (*listHead)->next;
    }else
    {
        popped = NULL;
    }
    return popped;
}

void clearFreeList(){
    for (int i = 0; i<11; i++){
        freeList[i] = NULL;
    }
    pair *newP = newPair(0,1023);
    
    pushPair(&freeList[10],newP);
}

pair* allocate(int memsize){
    int power = (int) ceil(log2(memsize));

    if(freeList[power] != NULL)
    {
        return popPair(&freeList[power]);
    }
    int i;
    for (i=power; i<11; i++)
    {
        if(freeList[i] != NULL)
            break;
    }

    if(i == 11){
        return NULL;
    }
    pair *temp = popPair(&freeList[i]);
    i--;
    for(; i>=power; i--)
    {
        pair *firstPair = newPair(temp->start, temp->start + (temp->end - temp->start)/2);
        pair *secondPair = newPair(temp->start + (temp->end - temp->start + 1)/2, temp->end);
        free(temp);
        pushPair(&freeList[i], secondPair);
        temp = firstPair;
    }
    return temp;
}


void deallocate(pair* freePair) 
{
    // Size of block to be searched 
    int n = (int) ceil(log2(freePair->end-freePair->start));
    int i, buddyNumber, buddyAddress; 

    //printf("At time %d freed %d bytes for process %d from %d to %d\n",getClk(),currentProcess->data.memsize,currentProcess->data.id,freePair->start,freePair->end);
    logFile = fopen("memory.log", "a+");
    fprintf(logFile,"At time %d freed %d bytes for process %d from %d to %d\n",getClk(),currentProcess->data.memsize,currentProcess->data.id,freePair->start,freePair->end);
    fclose(logFile);
    
    pushPair(&freeList[n], freePair);

    bool flag;
    pair* newValue = freePair;
    
    for (int i = n; i < 11; i++)
    {
        flag = true;
        freePair = newValue;
        // Calculate buddy number 
        buddyNumber = freePair->start / (freePair->end-freePair->start+1) ;  
        if (buddyNumber % 2 != 0) 
            buddyAddress = freePair->start - pow(2, i); 
        else
            buddyAddress = freePair->start + pow(2, i); 

        
        pair* temp = freeList[i];
        while( temp!=NULL )  
        { 
            // If buddy found and is also free 
            if (temp->start == buddyAddress)  
            { 
                flag = false;
                
                // Merge the buddies 
                if (buddyNumber % 2 == 0) 
                { 
                    pair* nPair = newPair(freePair->start, freePair->start + (pow(2, i+1) - 1 ) );
                    pushPair(&freeList[i + 1], nPair); 
                    // printf("Merged Memory from %d to %d\n",nPair->start,nPair->end);
                    newValue = nPair;
                } 
                else
                { 
                    pair* nPair = newPair(buddyAddress, buddyAddress + (pow(2, i+1) - 1)); 
                    pushPair(&freeList[i + 1], nPair); 
                    // printf("Merged Memory from %d to %d\n",nPair->start,nPair->end);
                    newValue = nPair;
                } 
                
                //Erase the Allocated memory Find the parent to set next with NULL
                pair* parentOfTemp = freeList[i];
                while (parentOfTemp->next != freePair)
                {
                    parentOfTemp = parentOfTemp->next;
                }
                parentOfTemp->next = freePair->next;
                free(freePair);
                freePair = NULL;

                // Find the parent of the buddy to remove correctly by merging it

                parentOfTemp = freeList[i];
                if (parentOfTemp != temp)
                {
                    while (parentOfTemp->next != temp)
                    {
                        parentOfTemp = parentOfTemp->next;
                    }
                    
                    // Merge the parent of temp with the next of temp and Erase temp
                    parentOfTemp->next = temp->next;
                    free(temp);
                    temp = NULL;
                    break; 
                }
                else
                {
                    freeList[i] = temp->next;
                    free(temp);
                    temp = NULL;
                    break;
                }
                
                
            }
            temp = temp->next;
        }
        if (flag == true) break;
    }    
}

void tryToAllocate(){
    
    // SRTN
    if(mode == 2) 
    {
        if (!isEmpty(&waitingHead))
        {
            if(isEmpty(&head) || waitingHead->data.remainingtime < head->data.remainingtime)
            {
                pair* allocatedMem = allocate(waitingHead->data.memsize);
                if (allocatedMem != NULL)
                {
                    //pop from waiting list
                    struct Node tempnode = pop(&waitingHead);
                    if (isEmpty(&waitingHead))
                        waitingTail = NULL;

                    //push to regular Queue
                    Node *temp = newNode(tempnode.data, tempnode.pid, allocatedMem, NULL);
                    insertSorted(&head, temp, 0);
                }  
            } 
        }
    }

    //Round robin
    else if(mode == 3) 
    {
        while (!isEmpty(&waitingHead))
        {
            pair* allocatedMem = allocate(waitingHead->data.memsize);
            if (allocatedMem != NULL)
            {
                //pop from waiting list
                struct Node tempnode = pop(&waitingHead);
                if (isEmpty(&waitingHead))
                    waitingTail = NULL;

                //push to regular list
                Node *temp = newNode(tempnode.data, tempnode.pid, allocatedMem, NULL);
                push(&head, &tail, temp);
            }else{
                break;
            } 
        }
    }  
}