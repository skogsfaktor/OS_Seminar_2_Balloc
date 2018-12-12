#include <stdlib.h>
#include <ucontext.h>
#include <assert.h>
#include "green.h"
#include <stdio.h>
#include <signal.h>
#include <sys/time.h>

//C does not have any boolean values
#define FALSE 0
#define TRUE 1

//Arbitrary
#define STACK_SIZE 4096

#define PERIOD 100

//For timer interrupts
static sigset_t block;
void timer_handler(int);

struct green_t *queue = NULL;

//The main context should be there when the program runs.
//Gets zeroed all over.
//main_cntx needs to be initialized at runtime.
static ucontext_t main_cntx = {0};
//Holds a pointer to the main context. The struct is NULL initialized.
//Used for the main thread. Needs to be prepared here to make everything work
static green_t main_green = {&main_cntx, NULL, NULL, NULL, NULL, FALSE};

//Always points to the currently running thread.
//Starts with main running.
static green_t *running = &main_green;

//This is called for the main_cntx to be initialized.
//We give the init procedure a constructor attribute.
//This results in init() being called before anything else
static void init() __attribute__((constructor));

//Initializes the main context
void init()
{
    sigemptyset(&block);
    sigaddset(&block, SIGVTALRM);

    struct sigaction act = {0};
    struct timeval interval;
    struct itimerval period;

    act.sa_handler = timer_handler;
    //Remove this to disable timer
    assert(sigaction(SIGVTALRM, &act, NULL) == 0);

    interval.tv_sec = 0;
    interval.tv_usec = PERIOD;
    period.it_interval = interval;
    period.it_value = interval;
    //Remove this to disable timer
    setitimer(ITIMER_VIRTUAL, &period, NULL);

    getcontext(&main_cntx);
}

int try(int *lock)
    {
        return __sync_val_compare_and_swap(lock, FALSE, TRUE);
    }

void release(int *lock)
{
    *lock = 0;
}

void add(struct green_t *thread)
{
    // printf("ADD: queue %p\n", queue);
    // printf("ADD: adding %p to queue\n", thread);
    //printf("ADD: Before adding to queue\n");
    if (queue == NULL)
    {
        // printf("ADD: queue is null, adding to queue\n");
        queue = thread;
        return;
    }
    //Iterate to last element in the ready queue
    struct green_t *element = queue;
    while (element != NULL)
    { //Needed to avoid segmentation fault
    
        while (element->next != NULL)
        {
            element = element->next;
            //This was added 
            if(element->next == element) {
               element->next = NULL;
            }
        }
        if (element->next == NULL)
        {
            break;
            //printf("At last element %p \n", element);
        }
    }
    
    //Add thread last in ready queue
    element->next = thread;
    //printf("ADD: After adding to existing ready queue\n");
}

void add_to(struct green_t *q, struct green_t *thread)
{
    //printf("ADD: Before adding to queue\n");
    if (q == NULL)
    {
        //printf("ADD: queue is null, adding to queue\n");
        q = thread;
        return;
    }
    //Iterate to last element in the ready queue
    struct green_t *element = q;
    while (element != NULL)
    { //Needed to avoid segmentation fault
        while (element->next != NULL)
        {
            element = element->next;
        }
        if (element->next == NULL)
        {
            break;
            //printf("At last element %p \n", element);
        }
    }
    //Add thread last in queue
    element->next = thread;
}

struct green_t *next_thread()
{
    struct green_t *next = queue;
    if (queue != NULL)
    {
        queue = queue->next;
    }
    next->next = NULL;
}

//Handles the timer interrupt
void timer_handler(int sig)
{
    green_t *susp = running;
    // printf("HANDLE IT!\n");
    //Add running to ready queue
    add(susp);
    //Schedule next thread
    
    struct green_t *next = queue;
    if (queue != NULL)
    {
        queue = queue->next;
    }
    next->next = NULL;

    running = next;
    swapcontext(susp->context, next->context);
}

int green_mutex_init(green_mutex_t *mutex)
{
    mutex->taken = FALSE;
    mutex->susp = NULL;
}

int green_mutex_lock(green_mutex_t *mutex)
{
    sigprocmask(SIG_BLOCK, &block, NULL);
    // printf("LOCK: 1\n");
    green_t *susp = running;

    //Suspend the running thread, add to the mutex queue
    //Add running thread susp to the end of the mutex suspended threads

    while (mutex->taken)
    {
        // printf("LOCK: 2\n");
        if (mutex->susp == NULL)
        {
            //printf("ADD: queue is null, adding to queue\n");
            // printf("LOCK: 3\n");
            mutex->susp = susp;
            return;
        }
        //Iterate to last element in the ready queue
        struct green_t *element = mutex->susp;
        while (element != NULL)
        { //Needed to avoid segmentation fault
            while (element->next != NULL)
            {
                element = element->next;
            }
            if (element->next == NULL)
            {
                break;
                //printf("At last element %p \n", element);
            }
        }
        //Add thread last in ready queue
        // printf("LOCK: 3.5\n");
        element->next = susp;
        //printf("ADD: After adding to existing ready queue\n");
        // printf("LOCK: mutex->susp %p\n", mutex->susp);
        //Find the next thread and run it
        struct green_t *next = next_thread();
        // printf("LOCK: 3.8\n");
        // printf("LOCK: green_t next %p\n", next);

        running = next;
        swapcontext(susp->context, next->context);
        // printf("LOCK: 3.9999\n");
    }
    // printf("LOCK: 4\n");
    //Lock not taken, Take the lock
    while (try(&mutex->taken) != 0)
    {
        // printf("DEADLOCK \n");
    }
    // printf("LOCK: 5\n");
    //printf("LOCK: Taken by %p\n", susp);
    //Unblock
    sigprocmask(SIG_UNBLOCK, &block, NULL);
    // printf("LOCK: 6\n");
    return 0;
}

int green_mutex_unlock(green_mutex_t *mutex)
{
    // printf("UNLOCK: 1\n");
    //Block timer interrupt
    sigprocmask(SIG_BLOCK, &block, NULL);
    //Move suspended threads to ready queue
    // printf("UNLOCK: 2\n");
    if(mutex->susp != NULL) {
        add(mutex->susp);
    }
    // printf("UNLOCK: 3\n");
    mutex->susp = NULL;
    //Release lock
    release(&mutex->taken);
    // printf("UNLOCK: 4\n");
    //Unblock
    sigprocmask(SIG_UNBLOCK, &block, NULL);
    // printf("UNLOCK: 5\n");
    return 0;
}

void green_cond_init(green_cond_t *cond)
{
    cond->suspended = NULL;
}

void green_cond_wait(green_cond_t *cond, green_mutex_t *mutex)
{
    //Block timer interrupt
    sigprocmask(SIG_BLOCK, &block, NULL);
    
    //Get running thread
    struct green_t *susp = running;

    //Suspend running thread on condition
    //Add running thread susp to the end of the condition variable list
    
    if (cond->suspended == NULL)
    {
        
        cond->suspended = susp;
    }
    else
    {
        //Iterate to last element in the waiting queue of the thread
        struct green_t *element = cond->suspended;
        while (element != NULL)
        { //Needed to avoid segmentation fault
            while (element->next != NULL)
            {
                element = element->next;
            }
            if (element->next == NULL)
            {
                break;
            }
        }
        element->next = susp;
         
    }
    if (mutex != NULL)
    {
        //release the lock if we have a mutex
        //green_mutex_unlock(mutex);
        
        release(&mutex->taken);
        //schedule suspended threads
        if (mutex->susp != NULL)
        {
            add(mutex->susp);
        }
        
        mutex->susp = NULL;
    }
    //Get next thread in ready queue and run it
    //THIS IS NULL, WTF
    struct green_t *next = queue;
    if (queue != NULL)
    {
        queue = queue->next;
    }
    next->next = NULL;
    

    running = next;
    //printf("Done adding to join queue\n");
    swapcontext(susp->context, next->context);

    if (mutex != NULL)
    {
        //try to take the lock
        while (mutex->taken)
        {
            //bad luck, suspend
            add_to(&mutex->susp, susp);
        }
        //take the lock
        mutex->taken = TRUE;
    }
    //unblock
    sigprocmask(SIG_UNBLOCK, &block, NULL);
    return;
}

void green_cond_signal(green_cond_t *cond)
{
    //If there are no threads to wake up
    if (cond->suspended == NULL)
    {
        return;
    }

    //Wake up all threads by adding them to the ready queue
    if (cond->suspended != NULL)
    {
        add(cond->suspended);
    }

    //Reset condition variable queue
    cond->suspended = NULL;
}

//Puts running thread last in the ready queue
//Selects the first thread from the queue as the
//next to run
int green_yield()
{
    //Who is the yielding process, probably the running one.
    green_t *susp = running;
    //printf("YIELD: Thread %p is yielding \n", susp);

    //Add susp last in the ready queue
    //sanity();
    add(susp);

    //Select the next thread in the ready queue for execution
    //Delink the selected one
    struct green_t *next = queue;
    if (queue != NULL)
    {
        queue = queue->next;
    }
    next->next = NULL;

    running = next;
    //Only swap with another, need to keep the other one around for later
    //Unlike the end of green_thread()
    swapcontext(susp->context, next->context);
    return 0;
}

//Allow several threads to wait for the same thread.
//
int green_join(green_t *thread)
{
    //If thread already is half dead
    if (thread->zombie)
    {
        return 0;
    }

    //Who is running? Me
    green_t *susp = running;
    //Add to waiting threads i.e. join (queue) in the thread to be joined
    //Add susp to join field of the "thread" thread
    //printf("Adding to join queue\n");
    if (thread->join == NULL)
    {
        thread->join = susp;
    }
    else
    {
        //Iterate to last element in the waiting queue of the thread
        struct green_t *element = thread->join;
        while (element != NULL)
        { //Needed to avoid segmentation fault
            while (element->next != NULL)
            {
                element = element->next;
            }
            if (element->next == NULL)
            {
                break;
            }
        }
        element->next = susp;
    }
    //Select next thread for execution
    struct green_t *next = queue;
    if (queue != NULL)
    {
        queue = queue->next;
    }
    next->next = NULL;

    running = next;
    //printf("Done adding to join queue\n");
    swapcontext(susp->context, next->context);
    return 0;
}

//1. Starts execution of the real function
//2. After returning from the function call, terminate the thread.
//Will run once per thread
green_thread()
{
    green_t *this = running;
    //printf("GREEN_THREAD: Running thread \p\n", this);

    //Runs the function
    //Execution will return here after the function is done
    (*this->fun)(this->arg);

    //Place waiting(joining) thread in ready queue
    //There can be multiple threads waiting i.e. the join is a queue
    //printf("GREEN_THREAD: Wake up waiting threads\n");
    if (this->join != NULL)
    {
        add(this->join);
    }

    //Free allocated memory structures
    //Especially the stack. Free the stack!
    free(this->context->uc_stack.ss_sp);
    free(this->context);

    //We're a zombie
    //If anyone does join they will see this zombie stuff
    this->zombie = 1;

    //Find the next thread to run
    struct green_t *next = queue;
    if (queue != NULL)
    {
        queue = queue->next;
    }

    //Run next thread
    running = next;
    //Actually jumps to the next thread to run. OS handles this
    setcontext(next->context);
}

//Populates the thread struct
int green_create(green_t *new, void *(*fun)(void *), void *arg)
{
    ucontext_t *cntx = (ucontext_t *)malloc(sizeof(ucontext_t));
    //Initializes the context
    getcontext(cntx);

    //Allocate the stack
    void *stack = malloc(STACK_SIZE);

    //Needs to set pointers to the stack and the stack size.
    //The pointers are set to other things initially by getcontext().
    cntx->uc_stack.ss_sp = stack;
    cntx->uc_stack.ss_size = STACK_SIZE;

    //OS call. Sets the function pointer
    //The thread will start by calling green_thread() with 0 arguments
    //green_thread() will only be called once for each creation
    makecontext(cntx, green_thread, 0);
    //Sets context of the thread
    new->context = cntx;
    //Sets the function of the thread
    new->fun = fun;
    new->arg = arg;
    new->next = NULL;
    new->join = NULL;
    new->zombie = FALSE;

    //Add new to ready queue
    //3-4 rows of code
    add(new);
    //sanity();

    return 0;
}

void sanity()
{
    printf("SANITY: \n");
    printf("Printing ready queue\n");
    struct green_t *element = queue;
    printf("First thread %p\n", element);
    while (element != NULL)
    { //Needed to avoid segmentation fault
        while (element->next != NULL)
        {
            element = element->next;
            printf("Thread %p\n", element);
        }
        break;
    }
    printf("\n");
}

sanity_of(struct green_t **list)
{
    printf("SANITY_OF: \n");
    printf("Printing queue\n");
    struct green_t *element = list;
    printf("First thread %p\n", element);
    while (element != NULL)
    { //Needed to avoid segmentation fault
        while (element->next != NULL)
        {
            element = element->next;
            printf("Thread %p\n", element);
        }
        if (element->next == NULL)
        {
            break;
        }
    }
    printf("\n");
}