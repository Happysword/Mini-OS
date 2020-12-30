#include "headers.h"
int pid[2];
void clearResources(int);

int main(int argc, char *argv[])
{
    signal(SIGINT, clearResources);
    // TODO Initialization

    // 1. Read the input files.
    char fileName[256];
    printf("Enter File path: ");
    scanf("%s", fileName);

    FILE *pFile;
    pFile = fopen(fileName, "r");
    processWrapper *head;
    processWrapper *prev;

    char line[256];
    long unsigned int size = 256;
    bool flag = 1;

    while (fgets(line, size, pFile))
    {
        if (line[0] == '#')
            continue;

        processWrapper *process = (processWrapper *)malloc(sizeof(processWrapper));

        sscanf(line, "%d%d%d%d", &(process->data.id), &(process->data.arrivaltime), &(process->data.runningtime), &(process->data.priority));
        process->data.remainingtime = process->data.runningtime;
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
    }
    prev->next = NULL;
    fclose(pFile);

    // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.
    int quantum, mode;

    printf("\n1- HPF\n2- SRTN\n3- RR\n");
    printf("Enter Scheduling mode(1/2/3): ");
    scanf("%d", &mode);

    if (mode == 3)
    {
        printf("Enter Quantum: \n");
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
    int msgq_id;
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
        char modeStr[5], quantumStr[5];
        sprintf(modeStr, "%d", mode);
        sprintf(quantumStr, "%d", quantum);
        char *path[] = {"./scheduler.out", modeStr, quantumStr, NULL};
        execv(path[0], path);
    }

    // 4. Use this function after creating the clock process to initialize clock
    initClk();

    // TODO Generation Main Loop
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

void clearResources(int signum)
{
    //TODO Clears all resources in case of interruption
    kill(pid[0], SIGINT);
    kill(pid[1], SIGINT);
    exit(0);
}
