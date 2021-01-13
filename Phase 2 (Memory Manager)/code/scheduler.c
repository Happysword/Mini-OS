#include "headers.h"
#include <math.h>

// Global Variables
FILE *logFile;
int timePassed = 0, idleTime = 0;
int numberOfTotalProc, numberOfFinishedProc = 0;
int totalWT = 0;
int msgq_id, msgq_id2, msgq_id3;
int mode;
bool RunningFlag = false;
Node *currentProcess;
float* WTAnums;

// Ready Queue
Node *head = NULL;
Node *tail = NULL;

// Waiting queue
Node* waitingHead = NULL;
Node* waitingTail = NULL;

// Head and tail of the linked list of pairs (Buddy system)
pair *freeList[11];
pair* newPair(int start, int end);
void pushPair(pair **listHead, pair * newPair);
pair* popPair(pair **listHead);
void clearFreeList();
pair* allocate(int memsize);
void deallocate(pair* freePair);
void tryToAllocate();

// functions prototype
void TerminateSched();
void clearResources(int);


/*****************************
 *****************************
 *      Priority Queue
 *          Queue
 *****************************
 *****************************
*/

// Constructor for a New Node
Node *newNode(struct processData data, int pid, pair* allocatedMem, Node* next)
{
    Node *temp = (Node *)malloc(sizeof(Node));
    temp->data = data;
    temp->next = next;
    temp->pid = pid;
    temp->allocatedMem = allocatedMem;
    return temp;
}

// Function to check is list is empty
int isEmpty(Node **head)
{
    return (*head) == NULL;
}

struct Node pop(Node **head)
{
    Node *temp = *head;
    *head = (*head)->next;
    Node tempnode = *temp;
    free(temp);
    return tempnode;
}

// Function to push according to priority
void insertSorted(Node **head, Node *data, bool option)
{
    if(isEmpty(head)){
        *head = data;
        return;
    }
    Node *start = *head;

    // Create new Node
    Node *temp = data;

    // if the current Head has higher priority than inserted
    // Then we make the new element the head
    if (option == 1)
    {
        if ((*head)->data.priority > temp->data.priority)
        {
            temp->next = *head;
            *head = temp;
        }
        // Else we traverse the Linked list until we find the suitable place for the node
        else
        {
            while (start->next != NULL && start->next->data.priority <= temp->data.priority)
            {
                start = start->next;
            }

            // Found the Position
            temp->next = start->next;
            start->next = temp;
        }
    }
    else
    {
        if ((*head)->data.remainingtime > temp->data.remainingtime)
        {
            temp->next = *head;
            *head = temp;
        }
        // Else we traverse the Linked list until we find the suitable place for the node
        else
        {
            while (start->next != NULL && start->next->data.remainingtime <= temp->data.remainingtime)
            {
                start = start->next;
            }

            // Found the Position
            temp->next = start->next;
            start->next = temp;
        }
    }
}

void push(Node **head, Node **tail, Node *data)
{
    if (isEmpty(head))
    {
        *head = data;
        *tail = *head;
    }else
    {
        (*tail)->next = data;
        *tail = data;
    }
}


// when a process finishes its job it sends signal to the schedular
void handlerChildTermination(int sigNum)
{
    // Calculate the wait time from what we have
    int waittime = getClk() - currentProcess->data.arrivaltime - ( currentProcess->data.runningtime - currentProcess->data.remainingtime ) ;
    int tatime = getClk() - currentProcess->data.arrivaltime;
    
    float wtatime = (float)tatime / currentProcess->data.runningtime;
    if (currentProcess->data.runningtime == 0) wtatime = 0;
    
    logFile = fopen("scheduler.log", "a+");
    fprintf(logFile, "At time %d process %d finished arr %d total %d remain %d wait %d TA %d WTA %.2f\n",getClk(),currentProcess->data.id,currentProcess->data.arrivaltime,currentProcess->data.runningtime,currentProcess->data.remainingtime,waittime,tatime,wtatime);
    fclose(logFile);

    RunningFlag = false;
    timePassed = 0;

    //Deallocate the Memory
    deallocate(currentProcess->allocatedMem);
    tryToAllocate();

    //Calculate Total Stats
    numberOfFinishedProc++;
    totalWT += waittime;
    WTAnums[currentProcess->data.id - 1] = wtatime;
    if(numberOfFinishedProc == numberOfTotalProc) TerminateSched();

    //Free the process
    free(currentProcess);
    currentProcess = NULL;

}

/**********************************************************MAIN************************************************************************************/

int main(int argc, char *argv[])
{
    // Create the log file
    logFile = fopen("scheduler.log", "w+");
    fprintf(logFile, "#At time x process y state arr w toal z remain y wait k\n");
    fclose(logFile);
    
    logFile = fopen("memory.log", "w+");
    fprintf(logFile, "#At time x allocated y bytes for process z from i to j\n");
    fclose(logFile);
    

    initClk();
    clearFreeList();
    
    signal(SIGUSR1, handlerChildTermination);
    signal(SIGINT, clearResources);

    //Number of processes and WTA array to calculate values
    numberOfTotalProc = atoi(argv[3]);
    WTAnums = (float*) malloc( sizeof(float)*numberOfTotalProc );

    // Create the Message queue that we recieve processes on
    key_t key_id, key_id2, key_id3;

    key_id = ftok("keyfile", 1);                //create unique key for Sending the processes
    msgq_id = msgget(key_id, 0666 | IPC_CREAT); //create message queue and return id

    key_id2 = ftok("keyfile", 10);                //create unique key for Sending the processes
    msgq_id2 = msgget(key_id2, 0666 | IPC_CREAT); //create message queue and return id

    key_id3 = ftok("keyfile", 11);                //create unique key for Sending the processes
    msgq_id3 = msgget(key_id3, 0666 | IPC_CREAT); //create message queue and return id


    if (msgq_id == -1 || msgq_id2 == -1 || msgq_id3 == -1)
    {
        perror("Error in create");
        exit(-1);
    }

    /*
    *
    * 
    * Main Scheduler loop
    * 
    * 
    */

    mode = atoi(argv[1]);
    int quantum = atoi(argv[2]);
    struct msgbuff message;
    msgrembuff msgrem,msgrem2;
    int rec_val, rem_time;
    int clk = getClk();
    int clk2 = getClk();

    while (1)
    {
        // Calculate the current clock
        int now = getClk();
        if ((now - clk2) == 1)
        {
            if (currentProcess != NULL)
            {
                // Send the Remaining Time to the process 
                msgrem.mtype = 2;
                currentProcess->data.remainingtime -= 1;
                msgrem.data = currentProcess->data.remainingtime;
                int lastremtime = currentProcess->data.remainingtime;
                if (msgsnd(msgq_id2, &msgrem, sizeof(msgrem.data), !IPC_NOWAIT) == -1)
                {
                    perror("Errror in send");
                }

                // Wait to recieve confirmation from process and clear all unrecieved messages from before

                msgrcv(msgq_id3, &msgrem2, sizeof(msgrem2.data), 1, !IPC_NOWAIT);
                while (msgrem2.data != lastremtime)
                {
                    msgrcv(msgq_id3, &msgrem2, sizeof(msgrem2.data), 1, !IPC_NOWAIT);
                }

            }else
            {
                idleTime ++;
            }
            clk2 = now;
            timePassed++;
        }

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // 1 - Highest Priority first
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        if (mode == 1)
        {
            /* receive the messages from the process_generator */
            rec_val = msgrcv(msgq_id, &message, sizeof(message.data), processMessagetype, IPC_NOWAIT);

            // if we recieve a message
            if (rec_val != -1)
            { 
                Node *temp = newNode(message.data, -1, NULL, NULL);
                insertSorted(&head, temp, 1);
                
            }

            if (!RunningFlag && !isEmpty(&head))
            {    
                struct Node tempnode = pop(&head);
                currentProcess = newNode(tempnode.data,tempnode.pid, NULL, NULL);
                int waittime = getClk() - currentProcess->data.arrivaltime - ( currentProcess->data.runningtime - currentProcess->data.remainingtime ) ;

                //Memory Allocation
                pair* allocatedMem = allocate(currentProcess->data.memsize);
                currentProcess->allocatedMem = allocatedMem;
                //printf("At time %d allocated %d bytes for process %d from %d to %d\n", getClk(), message.data.memsize, message.data.id, allocatedMem->start, allocatedMem->end);
                logFile = fopen("memory.log", "a+");
                fprintf(logFile,"At time %d allocated %d bytes for process %d from %d to %d\n", getClk(), message.data.memsize, message.data.id, allocatedMem->start, allocatedMem->end);
                fclose(logFile);
                
                logFile = fopen("scheduler.log", "a+");
                fprintf(logFile, "At time %d process %d started arr %d total %d remain %d wait %d\n",getClk(),currentProcess->data.id,currentProcess->data.arrivaltime,currentProcess->data.runningtime,currentProcess->data.remainingtime,waittime);
                fclose(logFile);

                int pid = fork();
                if (pid == 0)
                {
                    char remchar[5];
                    sprintf(remchar, "%d", currentProcess->data.remainingtime);
                    char *path[] = {"./process.out", remchar, NULL};
                    execv(path[0], path);
                }
                else
                {
                    currentProcess->pid = pid;
                }
                RunningFlag = true;
            }
        }

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // 2 - SRTN
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        else if (mode == 2)
        {
            /* receive the messages from the process_generator */
            rec_val = msgrcv(msgq_id, &message, sizeof(message.data), processMessagetype, IPC_NOWAIT);

            // if we recieve a message
            if (rec_val != -1)
            {
                Node *temp = newNode(message.data, -1, NULL, NULL);
                

                // We compare the head to the current process if the head has lower time we switch
                if (currentProcess!=NULL && temp->data.remainingtime < currentProcess->data.remainingtime)
                {
                    pair* allocatedMem = allocate(temp->data.memsize);
                    if(allocatedMem != NULL)
                    {   temp->allocatedMem = allocatedMem;
                        //printf("At time %d allocated %d bytes for process %d from %d to %d\n", getClk(), head->data.memsize, head->data.id, allocatedMem->start, allocatedMem->end);
                        logFile = fopen("memory.log", "a+");
                        fprintf(logFile,"At time %d allocated %d bytes for process %d from %d to %d\n", getClk(), temp->data.memsize, temp->data.id, allocatedMem->start, allocatedMem->end);
                        fclose(logFile);

                        // Send to the current to sleep
                        kill(currentProcess->pid, SIGUSR1);
                        int waittime = getClk() - currentProcess->data.arrivaltime - ( currentProcess->data.runningtime - currentProcess->data.remainingtime ) ;
                        
                        logFile = fopen("scheduler.log", "a+");
                        fprintf(logFile, "At time %d process %d stopped arr %d total %d remain %d wait %d\n",getClk(),currentProcess->data.id,currentProcess->data.arrivaltime,currentProcess->data.runningtime,currentProcess->data.remainingtime,waittime);
                        fclose(logFile);
                        
                        Node *temp2 = newNode(currentProcess->data,currentProcess->pid, currentProcess->allocatedMem, NULL);
                        
                        insertSorted(&head, temp2, 0);

                        // Start the new process
                        
                        currentProcess = temp;
                        currentProcess->data.is_running = true;
                        waittime = getClk() - currentProcess->data.arrivaltime;
                        
                        logFile = fopen("scheduler.log", "a+");
                        fprintf(logFile, "At time %d process %d started arr %d total %d remain %d wait %d\n",getClk(),currentProcess->data.id,currentProcess->data.arrivaltime,currentProcess->data.runningtime,currentProcess->data.remainingtime,waittime);
                        fclose(logFile);

                        int pid = fork();
                        if (pid == 0)
                        {
                            char remchar[5];
                            sprintf(remchar, "%d", currentProcess->data.remainingtime);
                            char *path[] = {"./process.out", remchar, NULL};
                            execv(path[0], path);
                        }
                        else
                        {
                            currentProcess->pid = pid;
                        }
                    }else{
                        insertSorted(&head, temp, 0);
                    }
                }else
                {
                    insertSorted(&head, temp, 0);
                }
            }

            if (!RunningFlag && !isEmpty(&head))
            {
                struct Node tempnode = pop(&head);
                currentProcess = newNode(tempnode.data, tempnode.pid, tempnode.allocatedMem, NULL);
                
                if (currentProcess->data.is_running == true)
                {   
                    int waittime = getClk() - currentProcess->data.arrivaltime - ( currentProcess->data.runningtime - currentProcess->data.remainingtime ) ;
                    
                    logFile = fopen("scheduler.log", "a+");
                    fprintf(logFile, "At time %d process %d resumed arr %d total %d remain %d wait %d\n",getClk(),currentProcess->data.id,currentProcess->data.arrivaltime,currentProcess->data.runningtime,currentProcess->data.remainingtime,waittime);
                    fclose(logFile);
                    
                    kill(currentProcess->pid, SIGCONT);
                    RunningFlag = true;
                }
                else
                {

                    pair* allocatedMem = currentProcess->allocatedMem;
                     if (allocatedMem == NULL) allocatedMem = allocate(currentProcess->data.memsize);
                    if(allocatedMem != NULL)
                    {
                        //printf("At time %d allocated %d bytes for process %d from %d to %d\n", getClk(), currentProcess->data.memsize, currentProcess->data.id, allocatedMem->start, allocatedMem->end);
                        logFile = fopen("memory.log", "a+");
                        fprintf(logFile,"At time %d allocated %d bytes for process %d from %d to %d\n", getClk(), currentProcess->data.memsize, currentProcess->data.id, allocatedMem->start, allocatedMem->end);
                        fclose(logFile);
                        currentProcess->data.is_running = true;
                        currentProcess->allocatedMem = allocatedMem;
                        int waittime = getClk() - currentProcess->data.arrivaltime;
                        
                        logFile = fopen("scheduler.log", "a+");
                        fprintf(logFile, "At time %d process %d started arr %d total %d remain %d wait %d\n",getClk(),currentProcess->data.id,currentProcess->data.arrivaltime,currentProcess->data.runningtime,currentProcess->data.remainingtime,waittime);
                        fclose(logFile);
                        
                        int pid = fork();
                        if (pid == 0)
                        {
                            char remchar[5];
                            sprintf(remchar, "%d", currentProcess->data.remainingtime);
                            char *path[] = {"./process.out", remchar, NULL};
                            execv(path[0], path);
                        }
                        else
                        {
                            currentProcess->pid = pid;
                        }
                        RunningFlag = true;
                    }else{
                        insertSorted(&waitingHead,currentProcess,0);
                    }
                }

            }
        }

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // 3- Round Robin
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        else
        {
            /* receive the messages from the process_generator */
            rec_val = msgrcv(msgq_id, &message, sizeof(message.data), processMessagetype, IPC_NOWAIT);

            // if we recieve a message
            if (rec_val != -1)
            {
                
                Node *temp = newNode(message.data, -1, NULL, NULL);
                push(&head, &tail, temp);
                
            }

            if (!RunningFlag || timePassed == quantum)
            {

                if (!isEmpty(&head))
                {
                    usleep(10); // Because when the remaining time in the process reaches zero the running flag is set to false
                            // and the current process is set to null But the scheduler is faster so it enters the if condition first
                            // with running flag set to true and after it enters the handler then is called and sets the current process
                            // to Null and the program crashes so we have to wait here until the process finishes first
                    if (RunningFlag)
                    {
                        kill(currentProcess->pid, SIGUSR1);
                        int waittime = getClk() - currentProcess->data.arrivaltime - ( currentProcess->data.runningtime - currentProcess->data.remainingtime ) ;
                        
                        logFile = fopen("scheduler.log", "a+");
                        fprintf(logFile, "At time %d process %d stopped arr %d total %d remain %d wait %d\n",getClk(),currentProcess->data.id,currentProcess->data.arrivaltime,currentProcess->data.runningtime,currentProcess->data.remainingtime,waittime);
                        fclose(logFile);

                        Node *temp = newNode(currentProcess->data, currentProcess->pid, currentProcess->allocatedMem, NULL);
                        push(&head, &tail, temp);

                        RunningFlag = false;
                    }

                    while (1)
                    {
                        struct Node tempnode = pop(&head);
                        if (isEmpty(&head))
                            tail = NULL;
                        currentProcess = newNode(tempnode.data, tempnode.pid, tempnode.allocatedMem, NULL);
                        
                        if (currentProcess->data.is_running == true)
                        {
                            int waittime = getClk() - currentProcess->data.arrivaltime - ( currentProcess->data.runningtime - currentProcess->data.remainingtime ) ;
                            
                            logFile = fopen("scheduler.log", "a+");
                            fprintf(logFile, "At time %d process %d resumed arr %d total %d remain %d wait %d\n",getClk(),currentProcess->data.id,currentProcess->data.arrivaltime,currentProcess->data.runningtime,currentProcess->data.remainingtime,waittime);
                            fclose(logFile);
                            
                            kill(currentProcess->pid, SIGCONT);

                            RunningFlag = true;
                            break;
                        }
                        else
                        {
                            pair* allocatedMem;
                            // Check if it's already allocated when another process finished its work and then we checked the 
                            if (currentProcess->allocatedMem == NULL)
                            {
                                allocatedMem = allocate(currentProcess->data.memsize);
                            }
                            else
                            {
                                allocatedMem = currentProcess->allocatedMem;
                            }
                            
                            if(allocatedMem != NULL)
                            {
                                currentProcess->data.is_running = true;
                                currentProcess->allocatedMem = allocatedMem;
                                int waittime = getClk() - currentProcess->data.arrivaltime - ( currentProcess->data.runningtime - currentProcess->data.remainingtime ) ;
                                
                                logFile = fopen("scheduler.log", "a+");
                                fprintf(logFile, "At time %d process %d started arr %d total %d remain %d wait %d\n",getClk(),currentProcess->data.id,currentProcess->data.arrivaltime,currentProcess->data.runningtime,currentProcess->data.remainingtime,waittime);
                                fclose(logFile);

                                //printf("At time %d allocated %d bytes for process %d from %d to %d\n", getClk(), currentProcess->data.memsize, currentProcess->data.id, allocatedMem->start, allocatedMem->end);
                                logFile = fopen("memory.log", "a+");
                                fprintf(logFile,"At time %d allocated %d bytes for process %d from %d to %d\n", getClk(), currentProcess->data.memsize, currentProcess->data.id, allocatedMem->start, allocatedMem->end);
                                fclose(logFile);
                        
                                
                                int pid = fork();
                                if (pid == 0)
                                {
                                    char remchar[5];
                                    sprintf(remchar, "%d", currentProcess->data.remainingtime);
                                    char *path[] = {"./process.out", remchar, NULL};
                                    execv(path[0], path);
                                }
                                else
                                {
                                    currentProcess->pid = pid;
                                    RunningFlag = true;
                                    break;
                                }
                            }
                            else
                            {
                                printf("Sorry there is no enough space to allocate\n");
                                
                                Node *temp = newNode(currentProcess->data, -1, NULL, NULL);
                                push(&waitingHead, &waitingTail, temp);
                            }
                        }
                    }
                    timePassed = 0;
                }
            }
        }
    }

    //upon termination release the clock resources.
    destroyClk(true);
}


void TerminateSched () {
    float avgWT = (float)totalWT / numberOfTotalProc;
    
    //Calc Standard deviation
    float sum = 0.0, mean, SD = 0.0;
    int i;
    for (i = 0; i < numberOfTotalProc; ++i) {
        sum += WTAnums[i];
    }
    mean = sum / numberOfTotalProc;

    for (i = 0; i < numberOfTotalProc; ++i)
        SD += (WTAnums[i] - mean) * (WTAnums[i] - mean);
    SD = sqrt(SD/numberOfTotalProc);

    int totalTime = getClk();
    float cpuUtilization = 100.0 * (totalTime - idleTime) / totalTime;

    FILE* prefFile;
    prefFile = fopen("scheduler.perf", "w+"); 
    fprintf(prefFile, "CPU utilization = %.2f%% \nAvg WTA = %.2f \nAvg Waiting = %.2f \nStd WTA = %.2f \n",cpuUtilization,mean,avgWT,SD);
    fclose(prefFile);

    kill(getppid(),SIGINT);
    exit(0);
}

// Clear all resources in case of interruption
void clearResources(int signum)
{
    pair *temp = popPair(&freeList[10]);
    free(temp);
    temp = NULL;
    msgctl(msgq_id2 , IPC_RMID, (struct msqid_ds *) 0);
    msgctl(msgq_id3 , IPC_RMID, (struct msqid_ds *) 0);
    exit(0);
}

///////////////////////////////////////////////////////////////////
//////////////// Buddy system allocation Functions ////////////////
///////////////////////////////////////////////////////////////////

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