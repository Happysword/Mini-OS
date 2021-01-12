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


int createSharedMemory(int bufferSize, char identifier);

void* attachSharedMemory(int sharedMemoryId);

int createSemaphore(int value, char identifier);

void consumeItem(const int* buffer, int* numberOfElements,int* consumeLocAddress);

void up(int semaphore);

void down(int semaphore);

int IsSharedMemoryNotCreated(int bufferSize, char identifier);


void signalHandler(int signum);

int bufferSharedId;
int numberOfElementsSharedId;
int sleepWakeupMessageQueueId;
int bufferSizeMessageQueueId;
int semaphore;
int prodLocSharedId;
int produceLocMessageQueueId;
int prodItemSharedId;
int initSemConsumer;
int bufferSize = 10;

int consumeLocSharedId;

int createSemaphore(int value, char identifier);
int mutex,empty,full;


int main()
{
    signal(SIGINT, signalHandler);

    mutex = createSemaphore(1, 'm');
    empty = createSemaphore(bufferSize, 'e');
    full = createSemaphore(0, 'f');
    initSemConsumer = createSemaphore(1, 'x');

    down(initSemConsumer);

    int isNotCreated = IsSharedMemoryNotCreated(sizeof(int), 'c');

    consumeLocSharedId = createSharedMemory(sizeof(int), 'c');
    int* sharedConsumeLoc = attachSharedMemory(consumeLocSharedId);

    bufferSharedId = createSharedMemory(bufferSize, 'b');
    int* sharedBuffer = attachSharedMemory(bufferSharedId);

    numberOfElementsSharedId = createSharedMemory(sizeof(int), 'n');
    int* numberOfElements = attachSharedMemory(numberOfElementsSharedId);

    if (isNotCreated == 1)
    {
        *sharedConsumeLoc = 0;
        *numberOfElements = 0;
    }

    up(initSemConsumer);

    while(1)
    {
        down(full);
        down(mutex);
        consumeItem(sharedBuffer,numberOfElements,sharedConsumeLoc);
        up(mutex);
        up(empty);
    }

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

void consumeItem(const int* buffer, int* numberOfElements,int* consumeLocAddress)
{
    int consumeLocation = *consumeLocAddress;

    int item = buffer[consumeLocation];

    consumeLocation = (consumeLocation + 1) % bufferSize;
    *consumeLocAddress = consumeLocation;

    (*numberOfElements)--;

    printf("[Consumer] Consumed Item %d\n", item);
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

int IsSharedMemoryNotCreated(int bufferSize, char identifier)
{
    // Create shared memory with size = bufferSize
    key_t key_id_shared = ftok("keyfile", identifier);
    int sharedMemoryId = shmget(key_id_shared, bufferSize * sizeof(int), IPC_CREAT | 0666 | IPC_EXCL);

    if (sharedMemoryId == -1)
    {
        return 0;
    }

    return 1;
}

void signalHandler(int signum)
{
    printf("\nCaptured Ctrl+C , Only the producer free the resources\n");

    exit(0);
}
