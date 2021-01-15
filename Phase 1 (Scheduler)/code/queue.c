#include "headers.h"


/*****************************
 *****************************
 *      Priority Queue
 *          Queue
 *****************************
 *****************************
*/


/*
* Constructor for a New Node
*/
Node *newNode(struct processData data, int pid, Node* next)
{
    Node *temp = (Node *)malloc(sizeof(Node));
    temp->data = data;
    temp->next = next;
    temp->pid = pid;
    return temp;
}

/*
* Function to check is list is empty
*/
int isEmpty(Node **head)
{
    return (*head) == NULL;
}

/*
* Function to pop from the top of the queue
*/
struct Node pop(Node **head)
{
    Node *temp = *head;
    *head = (*head)->next;
    Node tempnode = *temp;
    free(temp);
    return tempnode;
}

/*
* Function to push according to priority or remaining time
*/ 
void insertSorted(Node **head, Node *data, bool option)
{
    if(isEmpty(head)){
        *head = data;
        return;
    }
    Node *start = *head;

    // Create new Node
    Node *temp = data;

    // if the current Head has higher priority than inserted
    // Then we make the new element the head
    if (option == 1)
    {
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
    }
    else
    {
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

/*
* Push to the end of the queue
*/
void push(Node **head, Node **tail, Node *data)
{
    if (isEmpty(head))
    {
        *head = data;
        *tail = *head;
    }else
    {
        (*tail)->next = data;
        *tail = data;
    }
}