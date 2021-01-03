#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <ctype.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <signal.h>

/* arg for semctl system calls. */
union Semun
{
    int value;                  /* value for SETVAL */
    struct semid_ds* buffer;    /* buffer for IPC_STAT & IPC_SET */
    ushort* array;              /* array for GETALL & SETALL */
    struct seminfo* __buf;      /* buffer for IPC_INFO */
    void* __pad;
};

struct message_
{
    long type;
    int message;
};

typedef struct message_ Message;

int createSharedMemory(int bufferSize, char identifier);

void* attachSharedMemory(int sharedMemoryId);

int createSemaphore(int value, char identifier);

int createMessageQueue(char identifier);

void produceItem(int* buffer, int bufferSize, int* numberOfElements);

void up(int semaphore);

void down(int semaphore);

void signalHandler(int signum);

int getBufferSize(int bufferSizeMessageQueueId);

int bufferSharedId;
int numberOfElementsSharedId;
int sleepWakeupMessageQueueId;
int bufferSizeMessageQueueId;
int semaphore;

int main()
{
    signal(SIGINT, signalHandler);

    bufferSizeMessageQueueId = createMessageQueue('b');
    int bufferSize = getBufferSize(bufferSizeMessageQueueId);
    printf("[Info] Buffer Size = %d\n", bufferSize);

    bufferSharedId = createSharedMemory(bufferSize, 'b');
    int* sharedBuffer = attachSharedMemory(bufferSharedId);

    numberOfElementsSharedId = createSharedMemory(sizeof(int), 'n');
    int* numberOfElements = attachSharedMemory(numberOfElementsSharedId);

    semaphore = createSemaphore(1, 's');

    sleepWakeupMessageQueueId = createMessageQueue('m');

    while (1)
    {
        down(semaphore);
        // If buffer empty
        if (*numberOfElements == 0)
        {
            produceItem(sharedBuffer, bufferSize, numberOfElements);

            Message message;
            message.type = 'p';

            int sendingStatus = msgsnd(sleepWakeupMessageQueueId, &message, sizeof(message.message), !IPC_NOWAIT);

            if (sendingStatus == -1)
                perror("Error in send");

            if (bufferSize == 1)
            {
                message.type = 'c';

                up(semaphore);

                int receivingStatus = msgrcv(sleepWakeupMessageQueueId, &message, sizeof(message.message), message.type, !IPC_NOWAIT);
                if (receivingStatus == -1)
                    perror("Error in receive");
            }

        }
        if (bufferSize != 1)
        {
            up(semaphore);
        }


        down(semaphore);
        if (*numberOfElements == bufferSize)
        {
            Message message;
            message.type = 'c'; /* arbitrary value */

            up(semaphore);
            int receivingStatus = msgrcv(sleepWakeupMessageQueueId, &message, sizeof(message.message), message.type, !IPC_NOWAIT);

            if (receivingStatus == -1)
                perror("Error in receive");
        }
        else
        {
            up(semaphore);
        }

        down(semaphore);
        if (*numberOfElements != 0 && *numberOfElements != bufferSize)
        {
            produceItem(sharedBuffer, bufferSize, numberOfElements);
        }
        up(semaphore);

        // sleep(1);
    }

    return 0;
}

int getBufferSize(int bufferSizeMessageQueueId)
{
    int bufferSize = -1;
    Message message;
    message.type = 'c';
    // Check if we received the buffer size but do not wait for it
    int receivingStatus = msgrcv(bufferSizeMessageQueueId, &message, sizeof(message.message), message.type, IPC_NOWAIT);
    if (receivingStatus == -1) // if we didn't receive anything
    {
        printf("Please Enter Buffer Size: ");
        scanf("%d", &bufferSize);

        message.type = 'p';
        message.message = bufferSize;
        int sendingStatus = msgsnd(bufferSizeMessageQueueId, &message, sizeof(message.message), !IPC_NOWAIT);
        if (sendingStatus == -1)
        {
            perror("Error in sending buffer size");
            exit(-1);
        }
    }
    else
    {
        bufferSize = message.message;
    }
    return bufferSize;
}

void produceItem(int* buffer, int bufferSize, int* numberOfElements)
{
    static int item = 0;
    static int produceLocation = 0;

    buffer[produceLocation] = item;
    produceLocation = (produceLocation + 1) % bufferSize;

    (*numberOfElements)++;

    printf("[Producer] Inserted %d\n", item);

    item++;
}

int createSharedMemory(int bufferSize, char identifier)
{
    // Create shared memory with size = bufferSize
    key_t key_id_shared = ftok("keyfile", identifier);
    int sharedMemoryId = shmget(key_id_shared, bufferSize * sizeof(int), IPC_CREAT | 0666);

    if (sharedMemoryId == -1)
    {
        perror("Error in create Shared Memory");
        exit(-1);
    }

    return sharedMemoryId;
}

void* attachSharedMemory(int sharedMemoryId)
{
    void* sharedMemoryAddress = shmat(sharedMemoryId, NULL, 0);
    if (sharedMemoryAddress == -1)
    {
        perror("Error in attaching shared memory");
        exit(-1);
    }
    return sharedMemoryAddress;
}

int createSemaphore(int value, char identifier)
{
    union Semun semun;
    key_t keyToken = ftok("keyfile", identifier);

    int semaphore = semget(keyToken, 1, 0666 | IPC_CREAT | IPC_EXCL);

    if (semaphore == -1)
    {
        return semget(keyToken, 1, 0666 | IPC_CREAT);;
    }

    semun.value = value;
    if (semctl(semaphore, 0, SETVAL, semun) == -1)
    {
        perror("Error in creating semaphore");
        exit(-1);
    }

    return semaphore;
}

int createMessageQueue(char identifier)
{

    key_t keyToken = ftok("keyfile", identifier);
    int messageQueueId = msgget(keyToken, 0666 | IPC_CREAT);

    if (messageQueueId == -1)
    {
        perror("Error in creating Message Queue");
        exit(-1);
    }

    return messageQueueId;
}

void down(int semaphore)
{
    struct sembuf operation;

    operation.sem_num = 0;
    operation.sem_op = -1;
    operation.sem_flg = !IPC_NOWAIT;

    if (semop(semaphore, &operation, 1) == -1)
    {
        perror("Error in semaphore down");
        exit(-1);
    }
}

void up(int semaphore)
{
    struct sembuf operation;

    operation.sem_num = 0;
    operation.sem_op = 1;
    operation.sem_flg = !IPC_NOWAIT;

    if (semop(semaphore, &operation, 1) == -1)
    {
        perror("Error in semaphore up");
        exit(-1);
    }
}

void signalHandler(int signum)
{
    msgctl(sleepWakeupMessageQueueId, IPC_RMID, NULL);
    msgctl(bufferSizeMessageQueueId, IPC_RMID, NULL);
    shmctl(bufferSharedId, IPC_RMID, NULL);
    shmctl(numberOfElementsSharedId, IPC_RMID, NULL);
    semctl(semaphore, 0, IPC_RMID, NULL);

    exit(0);
}
