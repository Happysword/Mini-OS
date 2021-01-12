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

void produceItem(int* buffer, int* numberOfElements, int* addressProduceLocation, int* addressProduceItem);

void up(int semaphore);

void down(int semaphore);

void signalHandler(int signum);

int IsSharedMemoryNotCreated(int bufferSize, char identifier);


int bufferSharedId;
int numberOfElementsSharedId;
int sleepWakeupMessageQueueId;
int bufferSizeMessageQueueId;
int prodLocSharedId;
int produceLocMessageQueueId;
int prodItemSharedId;
int initSem;

int bufferSize = 10;

int createSemaphore(int value, char identifier);
int mutex,empty,full;


int main()
{
    signal(SIGINT, signalHandler);

    mutex = createSemaphore(1, 'm');
    empty = createSemaphore(bufferSize, 'e');
    full = createSemaphore(0, 'f');
    initSem = createSemaphore(1, 'z');

    down(initSem);

    int isNotCreated = IsSharedMemoryNotCreated(sizeof(int), 'p');

    prodLocSharedId = createSharedMemory(sizeof(int), 'p');
    int* sharedProdLoc = attachSharedMemory(prodLocSharedId);

    prodItemSharedId = createSharedMemory(sizeof(int), 'i');
    int* sharedItemLoc = attachSharedMemory(prodItemSharedId);

    bufferSharedId = createSharedMemory(bufferSize, 'b');
    int* sharedBuffer = attachSharedMemory(bufferSharedId);

    numberOfElementsSharedId = createSharedMemory(sizeof(int), 'n');
    int* numberOfElements = attachSharedMemory(numberOfElementsSharedId);

    if (isNotCreated == 1)
    {
        *sharedProdLoc = 0;
        *sharedItemLoc = 0;
        *numberOfElements = 0;
    }

    up(initSem);

    while(1)
    {
        down(empty);
        down(mutex);
        produceItem(sharedBuffer,numberOfElements,sharedProdLoc,sharedItemLoc);
        up(mutex);
        up(full);

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

void produceItem(int* buffer, int* numberOfElements, int* addressProduceLocation, int* addressProduceItem)
{
    int item = *addressProduceItem;
    int produceLocation = *addressProduceLocation;

    buffer[produceLocation] = item;
    produceLocation = (produceLocation + 1) % bufferSize;
    *addressProduceLocation = produceLocation;

    (*numberOfElements)++;

    printf("[Producer] Inserted %d\n", item);

    (*addressProduceItem)++;
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
    printf("\nCaptured Ctrl+C , Freeing Resources\n");
    int consumeLocSharedId = createSharedMemory(sizeof(int), 'c');
    int initSemConsumer = createSemaphore(1, 'x');
    shmctl(bufferSharedId, IPC_RMID, NULL);
    shmctl(prodLocSharedId, IPC_RMID, NULL);
    shmctl(prodItemSharedId, IPC_RMID, NULL);
    shmctl(numberOfElementsSharedId, IPC_RMID, NULL);
    semctl(initSem, 0, IPC_RMID, NULL);
    semctl(empty, 0, IPC_RMID, NULL);
    semctl(full, 0, IPC_RMID, NULL);
    semctl(mutex, 0, IPC_RMID, NULL);
    shmctl(consumeLocSharedId, IPC_RMID, NULL);
    semctl(initSemConsumer, 0, IPC_RMID, NULL);

    exit(0);
}
