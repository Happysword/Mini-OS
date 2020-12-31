#include "headers.h"

bool RunningFlag = false;

void handlerChildTermination(int sigNum){
    RunningFlag = false;
    printf("Process ended at time: %d\n", getClk());
}
/*****************************
 *****************************
 *      Priority Queue
 *          Queue
 *****************************
 *****************************
*/

// Node
typedef struct Node
{

    struct processData data;
    struct Node *next;
    int pid;

} Node;

// Constructor for a New Node
Node *newNode(struct processData data)
{
    Node *temp = (Node *)malloc(sizeof(Node));
    temp->data = data;
    temp->next = NULL;
    temp->pid = -1;
    return temp;
}

// Return the value at head
// And removes it from Queue
Node* pop(Node **head)
{
    Node *temp = *head;
    *head = (*head)->next;
    return temp;
}

// Function to push according to priority
void insertSorted(Node **head, struct processData data, bool option)
{
    Node *start = *head;

    // Create new Node
    Node *temp = newNode(data);

    // if the current Head has higher priority than inserted
    // Then we make the new element the head
    if(option == 1){
        if ((*head)->data.priority > temp->data.priority)
        {
            temp->next = *head;
            *head = temp;
        }
        // Else we traverse the Linked list until we find the suitable place for the node
        else
        {
            while (start->next != NULL && start->next->data.priority <= temp->data.priority)
            {
                start = start->next;
            }

            // Found the Position
            temp->next = start->next;
            start->next = temp;
        }
    }else{
        if ((*head)->data.remainingtime > temp->data.remainingtime)
        {
            temp->next = *head;
            *head = temp;
        }
        // Else we traverse the Linked list until we find the suitable place for the node
        else
        {
            while (start->next != NULL && start->next->data.remainingtime <= temp->data.remainingtime)
            {
                start = start->next;
            }

            // Found the Position
            temp->next = start->next;
            start->next = temp;
        }
    }
}


void push(Node **tail, struct processData data)
{
    // Create new Node
    Node *temp = newNode(data);

    (*tail)->next = temp;
    *tail = temp;

}

// Function to check is list is empty
int isEmpty(Node **head)
{
    return (*head) == NULL;
}

/**********************************************************MAIN************************************************************************************/

int main(int argc, char *argv[])
{
    initClk();

    signal(SIGUSR1, handlerChildTermination);

    // Create the Message queue that we recieve processes on
    key_t key_id, key_id2;
    int msgq_id, msgq_id2;

    key_id = ftok("keyfile", 1);                //create unique key for Sending the processes
    msgq_id = msgget(key_id, 0666 | IPC_CREAT); //create message queue and return id

    key_id2 = ftok("keyfile", 10);                //create unique key for Sending the processes
    msgq_id2 = msgget(key_id2, 0666 | IPC_CREAT); //create message queue and return id

    if (msgq_id == -1 || msgq_id2 == -1)
    {
        perror("Error in create");
        exit(-1);
    }

    printf("The mode is %s and the quantum is %s\n", argv[1], argv[2]);

    /*
    *
    * 
    * Main Scheduler loop
    * 
    */
    Node *head = NULL;
    Node *tail = NULL;
    Node *current;
    int mode = atoi(argv[1]);
    int quantum = atoi(argv[2]);
    struct msgbuff message;
    msgrembuff msgrem;
    int rec_val, rem_time;
    int clk = getClk();

    while (1)
    {

        // 1 - Highest Priority first
        if (mode == 1)
        {
            /* receive the messages from the process_generator */
            rec_val = msgrcv(msgq_id, &message, sizeof(message.data), processMessagetype, IPC_NOWAIT);

            // if we recieve a message
            if (rec_val != -1)
            {
                if (isEmpty(&head))
                {
                    head = newNode(message.data);
                }
                else
                {

                    insertSorted(&head, message.data, 1);
                }
            }

            if(!RunningFlag && !isEmpty(&head)){
                printf("before POP\n");
                Node* toRun = pop(&head);
                printf("Process %d started at time: %d\n", toRun->data.id, getClk());
                int pid = fork();
                if(pid == 0){
                    char remchar [5];
                    sprintf(remchar, "%d", toRun->data.remainingtime);
                    char *path[] = {"./process.out", remchar, NULL};
                    execv(path[0], path);
                } else{
                    toRun->pid = pid;
                }
                RunningFlag = true;
            }

            /*Node *temp = head;
            while (temp != NULL)
            {
                printf("%d ", temp->data.id);
                temp = temp->next;
            }
            printf("\n");
            */
        }

        // 2 - SRTN
        else if (mode == 2)
        {
            /* receive the messages from the process_generator */
            rec_val = msgrcv(msgq_id, &message, sizeof(message.data), processMessagetype, IPC_NOWAIT);

            // if we recieve a message
            if (rec_val != -1)
            {
                if (isEmpty(&head))
                {
                    head = newNode(message.data);
                }
                else
                {
                    insertSorted(&head, message.data, 0);
                }

                Node *temp = head;
                while (temp != NULL)
                {
                    printf("%d ", temp->data.id);
                    temp = temp->next;
                }
                printf("\n");
            }
        }

        // 3- Round Robin
        else
        {
            /* receive the messages from the process_generator */
            rec_val = msgrcv(msgq_id, &message, sizeof(message.data), processMessagetype, IPC_NOWAIT);

            // if we recieve a message
            if (rec_val != -1)
            {
                if (isEmpty(&head))
                {
                    head = newNode(message.data);
                    tail = head;
                }
                else
                {
                    push(&tail, message.data);
                }

                /*Node *temp = head;
                while (temp != NULL)
                {
                    printf("%d ", temp->data.id);
                    temp = temp->next;
                }
                printf("\n");*/
            }
            int now = getClk();
            if(!RunningFlag || (now - clk) == quantum){
                if(RunningFlag){
                    kill(current->pid, SIGSTOP);
                    rem_time = msgrcv(msgq_id2, &msgrem, sizeof(msgrem.data), 0, !IPC_NOWAIT);
                    current->data.remainingtime = msgrem.data;
                    push(&tail, current->data);
                }else{

                }
            }
        }
    }

    //upon termination release the clock resources.
    destroyClk(true);
}

