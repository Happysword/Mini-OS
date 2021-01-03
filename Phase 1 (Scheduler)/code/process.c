#include "headers.h"


/* Modify this file as needed*/
int remainingtime;
int msgq_id;
bool sleep_flag = false;

//void handlerTermination(int sigNum);
void handlerSleep(int sigNum);
void handlerWake(int signum);

//Clock variables
int clk, now;

int main(int agrc, char * argv[])
{
    initClk();
    
    key_t key_id;

    key_id = ftok("keyfile", 10);                //create unique key for Sending the processes
    msgq_id = msgget(key_id, 0666 | IPC_CREAT); //create message queue and return id

    if (msgq_id == -1)
    {
        perror("Error in create");
        exit(-1);
    }

    signal(SIGUSR1,handlerSleep);
    signal(SIGUSR2,handlerWake);
    //TODO it needs to get the remaining time from somewhere
    msgrembuff msg2;

    remainingtime = atoi(argv[1]);
    clk = getClk(); 
    
    while (remainingtime > 0)
    {
        msgrcv(msgq_id, &msg2, sizeof(msg2.data), 2, !IPC_NOWAIT);
        remainingtime = msg2.data;
        // printf("Process: %d remainingTime: %d\n",getpid(),remainingtime);
        /*while(sleep_flag)
            sleep(1);*/
        /*now = getClk();
        if(now - clk == 1){
            remainingtime -= 1;
            clk = now; 
            printf("Process: %d remainingTime: %d\n",getpid(),remainingtime);
        }*/
    }
    printf("Inside Process: Process Finished\n");
    kill(getppid(),SIGUSR1);
    destroyClk(false);
    
    return 0;
}

void handlerSleep(int sigNum){
    msgrembuff msg;
    msg.mtype = 1;
    
    /*if(getClk() - clk == 1 && remainingtime != 0){
        remainingtime -= 1;
        printf("Process: %d remainingTime: %d\n",getpid(),remainingtime);
    }*/
    msg.data = remainingtime;
    printf("Process: %d remainingTime: %d\n",getpid(),remainingtime);
    // printf("Inside Sleep handler\n");
    if(msgsnd(msgq_id, &msg, sizeof(msg.data), !IPC_NOWAIT) == -1){
        perror("Errror in send");
    }
    //sleep_flag= true;
    /*if(remainingtime == 0){
        printf("Inside Process: Process Finished\n");
        kill(getppid(),SIGUSR1);
        destroyClk(false);
        exit(0);
    }*/
    raise(SIGSTOP);
    //printf("Awaked\n");
    clk = getClk();
}

void handlerWake(int signum)
{
    sleep_flag = false;
    clk = getClk();
    // printf("inside wake hanlder\n");
}
