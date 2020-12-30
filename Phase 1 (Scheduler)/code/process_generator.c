#include "headers.h"
int pid[2];
void clearResources(int);

typedef struct processData
{
    int arrivaltime;
    int priority;
    int runningtime;
    int id;
    struct processData* next;
}processData;

int main(int argc, char * argv[])
{
    signal(SIGINT, clearResources);
    // TODO Initialization

    // 1. Read the input files.
    char mode[5];
    char fileName[256];
    printf("Enter File path: ");
    scanf("%s",fileName);

    FILE * pFile;
    pFile = fopen(fileName, "r");
    processData* head;
    processData* prev;

    char line[256];
    long unsigned int size = 256;
    int flag = 1;

    while(fgets(line, size, pFile)){
        if(line[0] == '#') continue;

        processData* process = (processData*) malloc(sizeof(processData));

        sscanf(line,"%d%d%d%d", &(process->id), &(process->arrivaltime), &(process->runningtime), &(process->priority));
        if(flag == 1){
            head = process;
            prev = process;
        }
        else{
            prev->next = process;
            prev = process;
        }
        flag = 0;
        //printf("%d %d %d %d\n", (process->id), (process->arrivaltime), (process->runningtime), (process->priority));
    }
    prev->next = NULL;
    fclose(pFile);

    prev = head;
    while(prev){
        printf("%d ",prev->id);
        prev = prev->next;
    }

    // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.
    printf("\n1- HPF\n2- SRTN\n3-RR\n");
    printf("Enter Scheduling mode(1/2/3): ");
    scanf("%s",mode);

    char quantum[10];
    if(mode == "3"){
        printf("Enter Quantum: ");
        scanf("%s",quantum);
    }

    // 3. Initiate and create the scheduler and clock processes.
    pid[0]  = fork();
    if(pid[0] == 0){
        char* path[] = {"./clk.out", NULL};
        execv(path[0], path);
    }
    pid[1] = fork();
    if(pid[1] == 0){
        char* path[] = {"./scheduler.out", mode, quantum, NULL};
        execv(path[0], path);
    }
    // 4. Use this function after creating the clock process to initialize clock
    initClk();
    // To get time use this
    
    // TODO Generation Main Loop
    // 5. Create a data structure for processes and provide it with its parameters.
    // 6. Send the information to the scheduler at the appropriate time.
    // 7. Clear clock resources
    while(1){
        int x = getClk();
        printf("current time is %d\n", x);
    };
    destroyClk(true);
}

void clearResources(int signum)
{
    //TODO Clears all resources in case of interruption
    kill(pid[0],SIGINT);
    kill(pid[1],SIGINT);
    exit(0);
}
