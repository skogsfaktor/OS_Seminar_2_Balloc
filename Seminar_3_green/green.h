#include <ucontext.h>
typedef struct green_t {
    //Pointer to the context of the thread. From the OS.  
    ucontext_t *context;
    //The actual function name. Pointer to the function. Type declaration for a function 
    //Void pointer to the pointer of a function that takes a void pointer as an argument 
    void* (*fun)(void*);
    //Pointer to the arguments for the functions
    void* arg;
    //Suspended threads are stored in a single linked list 
    //Used for ready queue only 
    struct green_t *next;
    //Want to keep track of a thread waiting to join the thread
    //Save the actual thread here.
    //Does not need to kept globally like the ready thread. 
    //When this thread dies, iterate through the  
    struct green_t *join;
    //Indicates if the thread has terminated or not. 
    int zombie;
} green_t;

typedef struct green_cond_t {
    struct green_t* suspended; 
} green_cond_t;

typedef struct green_mutex_t {
    volatile int taken;
    struct green_t* susp;
} green_mutex_t;

int green_create(green_t *thread, void*(*fun)(void*), void* arg);
int green_yield();
int green_join(green_t *thread);
void green_cond_init(green_cond_t*);
void green_cond_wait(green_cond_t*, green_mutex_t*);
void green_cond_signal(green_cond_t*); 
int green_mutex_init(green_mutex_t* mutex);
int green_mutex_lock(green_mutex_t* mutex);
int green_mutex_unlock(green_mutex_t* mutex);