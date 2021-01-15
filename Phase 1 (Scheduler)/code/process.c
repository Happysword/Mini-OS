#include "headers.h"


/* Modify this file as needed*/
int remainingtime;
int msgq_id, msgq_id2;
bool sleep_flag = false;



int main(int agrc, char * argv[])
{
    initClk();
    
    key_t key_id, key_id2;

    key_id = ftok("keyfile", 10);                //create unique key for Sending the processes
    msgq_id = msgget(key_id, 0666 | IPC_CREAT); //create message queue and return id

    key_id2 = ftok("keyfile", 11);                //create unique key for Sending the processes
    msgq_id2 = msgget(key_id2, 0666 | IPC_CREAT); //create message queue and return id


    if (msgq_id == -1)
    {
        perror("Error in create");
        exit(-1);
    }


    //TODO it needs to get the remaining time from somewhere
    msgrembuff msg2;
    msgrembuff confirmation;
    

    remainingtime = atoi(argv[1]);
    
    while (remainingtime > 0)
    {
        // Wait to recieve the Remaining time
        msgrcv(msgq_id, &msg2, sizeof(msg2.data), 2, !IPC_NOWAIT);
        remainingtime = msg2.data;

        // Send a message back to confirm that the remaining time is set
        confirmation.mtype = 1;
        confirmation.data = remainingtime;
        if (msgsnd(msgq_id2, &confirmation, sizeof(confirmation.data), !IPC_NOWAIT) == -1)
        {
            perror("Error in Confirmation");
        }
        
    }
    //printf("Inside Process: Process Finished\n");
    kill(getppid(),SIGUSR1);
    destroyClk(false);
    
    return 0;
}

