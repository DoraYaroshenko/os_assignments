#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <stdatomic.h>

// Node for item queue
typedef struct node
{
    void *item;
    struct node *next;
} node;

// Node for the waiting threads (FIFO sleep requirement)
typedef struct requestsQueue
{
    cnd_t cond;
    struct requestsQueue *next;
} requestsQueue;

//Queue data structure with head, tail, mutex and properties for its requests queque
typedef struct queue
{
    node *head;
    node *tail;
    size_t size;

    requestsQueue *requests_head;
    requestsQueue *requests_tail;

    mtx_t mutex;
} queue;

queue *myQueue;
atomic_int visited_count = 0;

void initQueue(void)
{
    myQueue = (queue *)malloc(sizeof(queue));

    myQueue->head = NULL;
    myQueue->tail = NULL;
    myQueue->size = 0;

    myQueue->requests_head = NULL;
    myQueue->requests_tail = NULL;

    mtx_init(&myQueue->mutex, mtx_plain);
    visited_count=0;
}

/*Frees queues memory. No need to free the nodes of requests queue,
because they are allocated on stack*/
void destroyQueue(void)
{
    node *current = myQueue->head;
    while (current != NULL)
    {
        node *next = current->next;
        free(current);
        current = next;
    }
    mtx_destroy(&myQueue->mutex);
    free(myQueue);
}

void enqueue(void *item)
{
    node *new_node = (node *)malloc(sizeof(node));
    new_node->item = item;
    new_node->next = NULL;

    mtx_lock(&myQueue->mutex);
    if (myQueue->size == 0)
    {
        myQueue->head = new_node;
        myQueue->tail = new_node;
    }
    else
    {
        myQueue->tail->next = new_node;
        myQueue->tail = new_node;
    }
    myQueue->size++;
    if (myQueue->requests_head != NULL)
    {
        //sends signal to the oldest dequeue request
        cnd_signal(&myQueue->requests_head->cond);
    }
    mtx_unlock(&myQueue->mutex);
}

void *dequeue(void)
{
    mtx_lock(&myQueue->mutex);
    if (myQueue->size == 0 || myQueue->requests_head != NULL)
    {
        //creates a node for this dequeue in requests queue
        requestsQueue my_waiter;
        cnd_init(&my_waiter.cond);
        my_waiter.next = NULL;
        if (myQueue->requests_tail == NULL)
        {
            myQueue->requests_head = &my_waiter;
            myQueue->requests_tail = &my_waiter;
        }
        else
        {
            myQueue->requests_tail->next = &my_waiter;
            myQueue->requests_tail = &my_waiter;
        }
        //waits for enqueue or another dequeue request to wake it
        cnd_wait(&my_waiter.cond, &myQueue->mutex);
        myQueue->requests_head = my_waiter.next;
        if (myQueue->requests_head == NULL)
        {
            myQueue->requests_tail = NULL;
        }
        cnd_destroy(&my_waiter.cond);
    }
    node *removed_node = myQueue->head;
    void *item = removed_node->item;

    myQueue->head = removed_node->next;
    if (myQueue->head == NULL)
    {
        myQueue->tail = NULL;
    }
    myQueue->size--;
    //we count the nodes that entered the queue and are subsequantly removed
    ++visited_count;
    //wakes the next dequeue request in line if myQueue is not empty
    if (myQueue->size > 0 && myQueue->requests_head != NULL)
    {
        cnd_signal(&myQueue->requests_head->cond);
    }
    mtx_unlock(&myQueue->mutex);
    free(removed_node);
    return item;
}

size_t visited(void)
{
    return visited_count;
}