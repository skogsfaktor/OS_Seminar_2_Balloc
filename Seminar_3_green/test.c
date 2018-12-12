#include <stdio.h>
#include "green.h"
#include <assert.h>
#include <pthread.h>
#include <time.h>

int flag = 0;
green_cond_t cond;
green_mutex_t mutex;

int buffer;
int count = 0;

void *test(void *arg)
{
    //Casting, will be 0 and 1
    int i = *(int *)arg;
    //Loops 4 times
    int loop = 4;
    //Will go sick-sack until the main thread wakes up
    while (loop > 0)
    {
        printf("thread %d: %d\n", i, loop);
        loop--;
        green_yield();
    }
}

void *test_join(struct green_t *thread)
{
    //printf("TEST_JOIN: \n");
    green_join(thread);
    printf("Im done waiting!\n");
}

void *test_cond(void *arg)
{
    int id = *(int *)arg;
    int loop = 4;
    green_cond_init(&cond);
    green_mutex_init(&mutex);
    while (loop > 0)
    {
        if (flag == id)
        {
            printf("thread %d: %d\n", id, loop);
            loop--;
            //flips the flag to 0 or 1, depending on the current value
            flag = (id + 1) % 2;
            green_cond_signal(&cond);
        }
        else
        {
            green_cond_wait(&cond, &mutex);
        }
    }
}

void *test_timer(void *arg)
{
    int id = *(int *)arg;
    int loop = 500;
    while (loop > 0)
    {
        if (flag == id)
        {
            printf("thread %d: %d\n", id, loop);
            loop--;
            //flips the flag to 0 or 1, depending on the current value
            flag = (id + 1) % 2;
        }
    }
}

long mutex_count = 0;

void *test_timer_error(void *arg)
{
    int id = *(int *)arg;
    for (long i = 0; i < 2000000000; i++)
    {
        mutex_count++;
    }
}

int mutex_count_2 = 0;

void *test_mutex(void *arg)
{
    int id = *(int *)arg;
    int loop = 1000;
    while (loop > 0)
    {
        green_mutex_lock(&mutex);
        //printf("thread %d: %d\n", id, mutex_count_2);
        mutex_count_2++;
        loop--;
        green_mutex_unlock(&mutex);
    }
    //printf("Mutex Count %d\n", mutex_count_2);
}

int count_p = 0;
pthread_mutex_t p_lock;

void *test_mutex_p(void *arg)
{
    int loop = 1000;
    while (loop > 0)
    {
        pthread_mutex_lock(&p_lock);
        //printf("thread %d: \n", count_p);
        count_p++;
        loop--;
        pthread_mutex_unlock(&p_lock);
    }
    //printf("Pthread count %d\n", count_p);
    return 0;
}

#define MAX 20
int buffers[MAX];
int *getp = 0;
int *putp = 0;
int cond_count = 0;

void put(int value)
{
    assert(count < MAX);
    buffers[(int)putp] = value;
    putp = putp + 1 % MAX;
    count++;
}

int get()
{
    assert(count > 0);
    int val = buffers[(int)getp];
    getp = getp + 1 % MAX;
    count--;
    return val;
}

void *produce(void *arg)
{
    int val = *(int *)arg;
    green_mutex_lock(&mutex);
    if (count == MAX)
    {
        green_cond_wait(&cond, &mutex);
    }
    put(val);
    green_cond_signal(&cond);
    green_mutex_unlock(&mutex);
}

int *consume(void *arg)
{
    green_mutex_lock(&mutex);
    if (count == 0)
    {
        green_cond_wait(&cond, &mutex);
    }
    int val = get();
    green_cond_signal(&cond);
    green_mutex_unlock(&mutex);
    return &val;
}

// void* last_test(void* args) {
//     int id = ((struct args *)arg)->id;
//     int loop = ((struct args *)arg)->loop;

//     while(loop > 0) {
//         green_mutex_lock(&lock);
//         while(1) {
//             if(flag == id) {
//                 flag = (id+1) % 2;
//                 green_cond_signal(&cond);
//                 green_mutex_unlock(&mutex);
//                 break;
//             } else {
//                 green_cond_wait(&cond, &mutex);
//             }
//         }
//         loop--
//     }
//     return NULL;
// }

int main()
{
    //Two threads
    green_t g0, g1, g2;
    //Two arguments, 0, 1, for the threads
    int a0 = 0;
    int a1 = 1;
    //Pointers to the threads, should call test
    // green_create(&g2, test, &a0);
    // green_create(&g0, test_join, &g2);
    // green_create(&g1, test_join, &g2);

    //test condition variables
    // green_create(&g0, test_cond, &a0);
    // green_create(&g1, test_cond, &a1);

    //Test timer
    // green_create(&g0, test_timer, &a0);
    // green_create(&g1, test_timer, &a1);

    //Test mutex
    // green_mutex_init(&mutex);
    // green_create(&g0, test_mutex, &a0);
    // green_create(&g1, test_mutex, &a1);
    // green_create(&g2, test_mutex, &a0);

    //Test that shows race conditions, timer errors

    //Test that shows without race conditions

    //Test mutex and condition variables
    // green_cond_init(&cond);
    // green_mutex_init(&mutex);
    // green_create(&g0, consume, &a0);
    // green_create(&g1, produce, &a1);

    //green_join(&g2);
    // green_join(&g1);
    // green_join(&g0);
    // printf("done\n");

    printf("BENCHMARK: \n");
    green_t green_array[100];
    pthread_t pthread_array[100];
    pthread_mutex_init(&p_lock, NULL);

    clock_t start, end;
    double cpu_time_used;

    start = clock();

    for (int i = 0; i < 100; i++)
    {
        pthread_create(&pthread_array[i], NULL, test_mutex_p, NULL);
    }

    for (int i = 0; i < 100; i++)
    {
        pthread_join(pthread_array[i], NULL);
    }

    end = clock();
    cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("Pthread mutex count: %d\n", count_p);
    printf("CPU Time pthread %f\n", cpu_time_used);


    green_mutex_init(&mutex);
    start = clock();

    for (int i = 0; i < 100; i++)
    {
        green_create(&green_array[i], test_mutex, &a0);
    }

    for (int i = 19; i >= 1; i--)
    {
        green_join(&green_array[i]);
    }

    end = clock();
    cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("Green mutex count: %d\n", mutex_count_2);
    printf("CPU Time green %f\n", cpu_time_used);

    return 0;
}