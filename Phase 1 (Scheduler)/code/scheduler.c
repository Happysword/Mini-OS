#include "headers.h"

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

} Node;

// Constructor for a New Node
Node *newNode(struct processData data)
{
    Node *temp = (Node *)malloc(sizeof(Node));
    temp->data = data;
    temp->next = NULL;

    return temp;
}

// Return the value at head
// And removes it from Queue
struct processData pop(Node **head)
{
    Node *temp = *head;
    *head = (*head)->next;
    processData tempdata = temp->data;
    free(temp);
    return tempdata;
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

    // Create the Message queue that we recieve processes on
    key_t key_id;
    int msgq_id;

    key_id = ftok("keyfile", 1);                //create unique key for Sending the processes
    msgq_id = msgget(key_id, 0666 | IPC_CREAT); //create message queue and return id

    if (msgq_id == -1)
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
    int mode = atoi(argv[1]);
    int quantum = atoi(argv[2]);
    struct msgbuff message;
    int rec_val;
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

                Node *temp = head;
                while (temp != NULL)
                {
                    printf("%d ", temp->data.id);
                    temp = temp->next;
                }
                printf("\n");
            }
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

                Node *temp = head;
                while (temp != NULL)
                {
                    printf("%d ", temp->data.id);
                    temp = temp->next;
                }
                printf("\n");
            }
        }
    }

    //upon termination release the clock resources.
    destroyClk(true);
}
