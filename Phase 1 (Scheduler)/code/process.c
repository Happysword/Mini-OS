#include "headers.h"


/* Modify this file as needed*/
int remainingtime;
int msgq_id;

//void handlerTermination(int sigNum);
void handlerSleep(int sigNum);

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

    signal(SIGSTOP,handlerSleep);
    
    //TODO it needs to get the remaining time from somewhere
    printf("In process\n");
    remainingtime = atoi(argv[1]);
    int clk = getClk(); 
    int now;
    while (remainingtime > 0)
    {
        now = getClk();
        if(now - clk == 1){
            remainingtime -= 1;
            clk = now; 
            //printf("Clk: %d remainingTime: %d\n",clk,remainingtime);
        }
    }
    kill(getppid(),SIGUSR1);
    destroyClk(false);
    
    return 0;
}

void handlerSleep(int sigNum){
    msgrembuff msg;
    msg.mtype = 1;
    msg.data = remainingtime;
    if(msgsnd(msgq_id, &msg, sizeof(msg.data), !IPC_NOWAIT) == -1){
        perror("Errror in send");
    }
    pause();
}
