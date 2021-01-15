#include "headers.h"
int pid[2];
int msgq_id;
void clearResources(int);
processWrapper *head;

int main(int argc, char *argv[])
{
    signal(SIGINT, clearResources);

    // 1. Read the input files.
    char fileName[256];
    printf("Enter File path: ");
    scanf("%s", fileName);

    FILE *pFile;
    pFile = fopen(fileName, "r");
    
    processWrapper *prev = NULL;

    char line[256];
    long unsigned int size = 256;
    bool flag = 1;
    int numberOfProcesses = 0;

    while (fgets(line, size, pFile))
    {
        if (line[0] == '#')
            continue;

        processWrapper *process = (processWrapper *)malloc(sizeof(processWrapper));

        sscanf(line, "%d%d%d%d%d", &(process->data.id), &(process->data.arrivaltime), &(process->data.runningtime), &(process->data.priority), &(process->data.memsize));
        process->data.remainingtime = process->data.runningtime;
        process->data.is_running = false;
        if (flag == 1)
        {
            head = process;
            prev = process;
        }
        else
        {
            prev->next = process;
            prev = process;
        }
        flag = 0;
        numberOfProcesses++;
    }
    if(prev == NULL){
        printf("Number of processes equal 0\nEnter valid Number of Processes\n");
        return 0;
    }
    prev->next = NULL;
    fclose(pFile);

    // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.
    int quantum = 0, mode;

    printf("\n1- HPF\n2- SRTN\n3- RR\n");
    printf("Enter Scheduling mode(1/2/3): ");
    scanf("%d", &mode);

    if (mode == 3)
    {
        printf("Enter Quantum: ");
        scanf("%d", &quantum);
    }

    // 3. Initiate and create the scheduler and clock processes.

    // Clock
    pid[0] = fork();
    if (pid[0] == 0)
    {
        char *path[] = {"./clk.out", NULL};
        execv(path[0], path);
    }

    // Scheduler
    key_t key_id;
    
    key_id = ftok("keyfile", 1);                //create unique key for Sending the processes
    msgq_id = msgget(key_id, 0666 | IPC_CREAT); //create message queue and return id

    if (msgq_id == -1)
    {
        perror("Error in create");
        exit(-1);
    }

    pid[1] = fork();
    if (pid[1] == 0)
    {
        //Change the mode and quantum to strings so we can send them
        char modeStr[5], quantumStr[5],processNum[5];
        sprintf(modeStr, "%d", mode);
        sprintf(quantumStr, "%d", quantum);
        sprintf(processNum, "%d", numberOfProcesses);
        char *path[] = {"./scheduler.out", modeStr, quantumStr,processNum, NULL};
        execv(path[0], path);
    }

    // 4. Use this function after creating the clock process to initialize clock
    initClk();

    // 5. Create a data structure for processes and provide it with its parameters.
    processWrapper *currentProcess = head;
    struct msgbuff message;
    int send_val;

    // 6. Send the information to the scheduler at the appropriate time.
    while (1)
    {
        int x = getClk();

        // Send All the processes that arrived at the current timestamp
        while (currentProcess && x == currentProcess->data.arrivaltime)
        {
            message.mtype = processMessagetype; /* User Defined Message Type */
            message.data = currentProcess->data;

            send_val = msgsnd(msgq_id, &message, sizeof(message.data), !IPC_NOWAIT);

            // if there is an error in send 
            if (send_val == -1)
                perror("Errror in send");
            
            // Get next process
            currentProcess = currentProcess->next;
        }
    };

    // 7. Clear clock resources
    destroyClk(true);
}

//Clear all resources in case of interruption
void clearResources(int signum)
{
    msgctl(msgq_id , IPC_RMID, (struct msqid_ds *) 0);
    kill(pid[0], SIGINT);
    kill(pid[1], SIGINT);
    processWrapper *temp;
    while(head->next != NULL){
        temp = head;
        head = head->next;
        free(temp);
    }
    exit(0);
}
