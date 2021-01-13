#include "headers.h"
#include <math.h>

// Global Variables
FILE *logFile;
int timePassed = 0, idleTime = 0;
int numberOfTotalProc, numberOfFinishedProc = 0;
int totalWT = 0;
int msgq_id, msgq_id2, msgq_id3;
bool RunningFlag = false;
Node *currentProcess;
float* WTAnums;

// Functions Prototypes
void TerminateSched();
void clearResources(int);


/*****************************
 *****************************
 *      Priority Queue
 *          Queue
 *****************************
 *****************************
*/


/*
* Constructor for a New Node
*/
Node *newNode(struct processData data, int pid, Node* next)
{
    Node *temp = (Node *)malloc(sizeof(Node));
    temp->data = data;
    temp->next = next;
    temp->pid = pid;
    return temp;
}

/*
* Function to check is list is empty
*/
int isEmpty(Node **head)
{
    return (*head) == NULL;
}

/*
* Function to pop from the top of the queue
*/
struct Node pop(Node **head)
{
    Node *temp = *head;
    *head = (*head)->next;
    Node tempnode = *temp;
    free(temp);
    return tempnode;
}

/*
* Function to push according to priority or remaining time
*/ 
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

/*
* Push to the end of the queue
*/
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

    //Calculate Total Stats
    numberOfFinishedProc++;
    totalWT += waittime;
    WTAnums[currentProcess->data.id - 1] = wtatime;
    if(numberOfFinishedProc == numberOfTotalProc) TerminateSched();

    //Free the process
    free(currentProcess);
    currentProcess = NULL;

}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**********************************************************MAIN************************************************************************************/
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
    // Create the log file
    logFile = fopen("scheduler.log", "w+");
    fprintf(logFile, "#At time x process y state arr w toal z remain y wait k\n");
    fclose(logFile);
    
    initClk();
    
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
    Node *head = NULL;
    Node *tail = NULL;

    int mode = atoi(argv[1]);
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

            }else{
                idleTime ++;
            }
            clk2 = now;
            timePassed++;
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // 1 - Highest Priority first
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        if (mode == 1)
        {
            /* receive the messages from the process_generator */
            rec_val = msgrcv(msgq_id, &message, sizeof(message.data), processMessagetype, IPC_NOWAIT);

            // if we recieve a message
            if (rec_val != -1)
            {
                
                Node *temp = newNode(message.data, -1, NULL);
                insertSorted(&head, temp, 1);
            }

            // Run another process if there is no running one
            if (!RunningFlag && !isEmpty(&head))
            {
                struct Node tempnode = pop(&head);
                currentProcess = newNode(tempnode.data, tempnode.pid, NULL);

                int waittime = getClk() - currentProcess->data.arrivaltime - ( currentProcess->data.runningtime - currentProcess->data.remainingtime ) ;
                
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

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // 2 - SRTN
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        else if (mode == 2)
        {
            /* receive the messages from the process_generator */
            rec_val = msgrcv(msgq_id, &message, sizeof(message.data), processMessagetype, IPC_NOWAIT);

            // if we recieve a message
            if (rec_val != -1)
            {
                
                Node *temp = newNode(message.data, -1, NULL);
                insertSorted(&head, temp, 0);

                // We compare the head to the current process if the head has lower time we switch
                if (currentProcess!=NULL && head->data.remainingtime < currentProcess->data.remainingtime)
                {
                    // Send to the current to sleep
                    kill(currentProcess->pid, SIGUSR1);
                    int waittime = getClk() - currentProcess->data.arrivaltime - ( currentProcess->data.runningtime - currentProcess->data.remainingtime ) ;
                    
                    logFile = fopen("scheduler.log", "a+");
                    fprintf(logFile, "At time %d process %d stopped arr %d total %d remain %d wait %d\n",getClk(),currentProcess->data.id,currentProcess->data.arrivaltime,currentProcess->data.runningtime,currentProcess->data.remainingtime,waittime);
                    fclose(logFile);
                    
                    Node *temp = newNode(currentProcess->data, currentProcess->pid, NULL);
                    insertSorted(&head, temp, 0);

                    // Start the new process
                    struct Node tempnode = pop(&head);
                    currentProcess = newNode(tempnode.data, tempnode.pid, NULL);
                    currentProcess->data.is_running = true;

                    waittime = getClk() - currentProcess->data.arrivaltime - ( currentProcess->data.runningtime - currentProcess->data.remainingtime ) ;
                    
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
                }
            }

            // Run another process if there is no running process
            if (!RunningFlag && !isEmpty(&head))
            {
                struct Node tempnode = pop(&head);
                currentProcess = newNode(tempnode.data, tempnode.pid, NULL);

                // Check if it was paused before
                if (currentProcess->data.is_running == true)
                {
                    int waittime = getClk() - currentProcess->data.arrivaltime - ( currentProcess->data.runningtime - currentProcess->data.remainingtime ) ;
                    
                    logFile = fopen("scheduler.log", "a+");
                    fprintf(logFile, "At time %d process %d resumed arr %d total %d remain %d wait %d\n",getClk(),currentProcess->data.id,currentProcess->data.arrivaltime,currentProcess->data.runningtime,currentProcess->data.remainingtime,waittime);
                    fclose(logFile);
                    
                    kill(currentProcess->pid, SIGCONT);
                }
                else
                {
                    currentProcess->data.is_running = true;
                    int waittime = getClk() - currentProcess->data.arrivaltime - ( currentProcess->data.runningtime - currentProcess->data.remainingtime ) ;
                    
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
                }

                RunningFlag = true;
            }
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // 3- Round Robin
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        else
        {
            /* receive the messages from the process_generator */
            rec_val = msgrcv(msgq_id, &message, sizeof(message.data), processMessagetype, IPC_NOWAIT);

            // if we recieve a message
            if (rec_val != -1)
            {
                
                Node *temp = newNode(message.data, -1, NULL);
                push(&head, &tail, temp);
            }

            // If there is no running process or the quantem is over
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

                        Node *temp = newNode(currentProcess->data, currentProcess->pid, NULL);
                        push(&head, &tail, temp);
                        
                        RunningFlag = false;
                    }

                    struct Node tempnode = pop(&head);
                    if (isEmpty(&head))
                        tail = NULL;
                    currentProcess = newNode(tempnode.data, tempnode.pid, NULL);

                    if (currentProcess->data.is_running == true)
                    {
                        int waittime = getClk() - currentProcess->data.arrivaltime - ( currentProcess->data.runningtime - currentProcess->data.remainingtime ) ;
                        
                        logFile = fopen("scheduler.log", "a+");
                        fprintf(logFile, "At time %d process %d resumed arr %d total %d remain %d wait %d\n",getClk(),currentProcess->data.id,currentProcess->data.arrivaltime,currentProcess->data.runningtime,currentProcess->data.remainingtime,waittime);
                        fclose(logFile);
                        
                        kill(currentProcess->pid, SIGCONT);
                    }
                    else
                    {
                        currentProcess->data.is_running = true;
                        int waittime = getClk() - currentProcess->data.arrivaltime - ( currentProcess->data.runningtime - currentProcess->data.remainingtime ) ;
                        
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
                    }
                    RunningFlag = true;
                }
                timePassed = 0;
            }
        }
    }

    //upon termination release the clock resources.
    destroyClk(true);
}

// Calculate statistics for scheduler.pref file
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

// Clears all resources in case of interruption
void clearResources(int signum)
{
    msgctl(msgq_id2 , IPC_RMID, (struct msqid_ds *) 0);
    msgctl(msgq_id3 , IPC_RMID, (struct msqid_ds *) 0);
    exit(0);
}