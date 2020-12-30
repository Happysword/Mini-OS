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

/* arg for semctl system calls. */
union Semun
{
    int val;               /* value for SETVAL */
    struct semid_ds *buf;  /* buffer for IPC_STAT & IPC_SET */
    ushort *array;         /* array for GETALL & SETALL */
    struct seminfo *__buf; /* buffer for IPC_INFO */
    void *__pad;
};

struct msgbuff
{
    long mtype;
    char mtext[70];
};

int bufferSize = 5; // Should be read from the user

int CreateSharedMemory(int bufferSize,char identifier);
void* AttachSharedMemory(int shmid);
int CreateSemaphore(int value,char identifier);
int CreateMessageQueue(char identifier);
void ProduceItem(int* shmaddr_buffer,int* shmaddr_number_of_elements);
void up(int sem);
void down(int sem);

// Assume Producer starts first for initializtion ?????
void main()
{
    // Create(get) shared memory for buffer
    int shmid_buffer = CreateSharedMemory(bufferSize,'b');
    int* shmaddr_buffer = AttachSharedMemory(shmid_buffer);

    // Create(get) shared memory for number of elements
    int shmid_number_of_elements = CreateSharedMemory(sizeof(int),'n');
    int* shmaddr_number_of_elements = AttachSharedMemory(shmid_number_of_elements);

    int sem1 = CreateSemaphore(1,'1');

    int msgq_id = CreateMessageQueue('m');    

    while (1)
    {
        // If buffer empty
        if (*shmaddr_number_of_elements == 0)
        {
            printf("\nfirst condition1\n");


            down(sem1);
            ProduceItem(shmaddr_buffer,shmaddr_number_of_elements);
            up(sem1);

            struct msgbuff message;

            message.mtype = 'p'; /* arbitrary value */
            strcpy(message.mtext, "produced");

            int send_val = msgsnd(msgq_id, &message, sizeof(message.mtext), !IPC_NOWAIT);

            if (send_val == -1)
                perror("Errror in send");

            printf("\nfirst condition2\n");
        }

        if (*shmaddr_number_of_elements == bufferSize)
        {
            printf("\nsecond condition1\n");

            struct msgbuff message;

            message.mtype = 'c'; /* arbitrary value */

            int rec_val = msgrcv(msgq_id, &message, sizeof(message.mtext), message.mtype, !IPC_NOWAIT);

            if (rec_val == -1)
                perror("Error in receive");

            printf("\nsecond condition2\n");

        }

        if(*shmaddr_number_of_elements != 0 && *shmaddr_number_of_elements != bufferSize)
        {
            printf("\nthird condition 1\n");

            down(sem1);
            ProduceItem(shmaddr_buffer,shmaddr_number_of_elements);
            up(sem1);

            printf("\nthird condition 2\n");
        }
        
    }
    
}

void ProduceItem(int* shmaddr_buffer,int* shmaddr_number_of_elements)
{
    static int item = 0;
    static int produceLocation = 0;

    shmaddr_buffer[produceLocation] = item;
    produceLocation = (produceLocation+1) % bufferSize;

    shmaddr_number_of_elements[0]++;

	printf ("producer: inserted %d\n", item);

    item++;
}

int CreateSharedMemory(int bufferSize,char identifier)
{
    key_t key_id_shared;
    int shmid;

    // Create shared memory with bufferSize
    key_id_shared = ftok("keyfile", identifier);
    shmid = shmget(key_id_shared, bufferSize * sizeof(int), IPC_CREAT | 0666);

    if (shmid == -1)
    {
        perror("Error in create Shared Memory");
        exit(-1);
    }

    return shmid;
}

void* AttachSharedMemory(int shmid)
{
    // Client writing on the shared memory
    void *shmaddr = shmat(shmid, (void *)0, 0);
    if (shmaddr == -1)
    {
        perror("Error in attach in client");
        exit(-1);
    }
}

int CreateSemaphore(int value,char identifier)
{
    key_t  key_id_sem;

    union Semun semun;
    key_id_sem = ftok("keyfile", identifier);

    int sem = semget(key_id_sem, 1, 0666 | IPC_CREAT);

    if (sem == -1)
    {
        perror("Error in create sem");
        exit(-1);
    }

    semun.val = value;
    if (semctl(sem, 0, SETVAL, semun) == -1)
    {
        perror("Error in semctl");
        exit(-1);
    }

    return sem;
}

int CreateMessageQueue(char identifier)
{
    key_t key_id;
    int msgq_id;

    key_id = ftok("keyfile", identifier);
    msgq_id = msgget(key_id, 0666 | IPC_CREAT);

    if (msgq_id == -1)
    {
        perror("Error in create");
        exit(-1);
    }

    return msgq_id;

}

void down(int sem)
{
    struct sembuf p_op;

    p_op.sem_num = 0;
    p_op.sem_op = -1;
    p_op.sem_flg = !IPC_NOWAIT;

    if (semop(sem, &p_op, 1) == -1)
    {
        perror("Error in down()");
        exit(-1);
    }
}

void up(int sem)
{
    struct sembuf v_op;

    v_op.sem_num = 0;
    v_op.sem_op = 1;
    v_op.sem_flg = !IPC_NOWAIT;

    if (semop(sem, &v_op, 1) == -1)
    {
        perror("Error in up()");
        exit(-1);
    }
}
