#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define MAX_BOTTLES 10
#define PRODUCER_DELAY 2
#define CONSUMER_DELAY 3

typedef struct
{
        int bottles;
        pthread_mutex_t mutex;
        pthread_cond_t can_produce;
        pthread_cond_t can_consume;
} BottleQueue;

/* Queue */
BottleQueue queue = {
    .bottles = 0,
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .can_produce = PTHREAD_COND_INITIALIZER,
    .can_consume = PTHREAD_COND_INITIALIZER};

char *get_timestamp()
{
        static char buffer[26];
        time_t timer;
        struct tm *tm_info;

        timer = time(NULL);
        tm_info = localtime(&timer);
        strftime(buffer, 26, "%H:%M:%S", tm_info);

        return buffer;
}

void *producer(void *arg)
{
        while (1)
        {
                pthread_mutex_lock(&queue.mutex);

                while (queue.bottles >= MAX_BOTTLES)
                {
                        printf("[%s] Producer waiting: Queue full (bottles: %d)\n",
                               get_timestamp(), queue.bottles);
                        pthread_cond_wait(&queue.can_produce, &queue.mutex);
                }

                queue.bottles++;
                printf("[%s] Producer added bottle: Now %d bottle(s) in queue\n",
                       get_timestamp(), queue.bottles);

                pthread_cond_signal(&queue.can_consume);

                pthread_mutex_unlock(&queue.mutex);

                sleep(PRODUCER_DELAY);
        }
        return NULL;
}

void *consumer(void *arg)
{
        while (1)
        {
                pthread_mutex_lock(&queue.mutex);

                while (queue.bottles <= 0)
                {
                        printf("\n[%s] Consumer waiting: Queue empty\n\n", get_timestamp());
                        pthread_cond_wait(&queue.can_consume, &queue.mutex);
                }

                queue.bottles--;
                printf("\n[%s] Consumer removed bottle: Now %d bottles in queue\n\n",
                       get_timestamp(), queue.bottles);

                pthread_cond_signal(&queue.can_produce);

                pthread_mutex_unlock(&queue.mutex);

                sleep(CONSUMER_DELAY);
        }
        return NULL;
}

int main()
{
        pthread_t producer_thread, consumer_thread;

        printf("Starting Bottle Production Simulation\n");
        printf("Maximum queue size: %d bottles\n", MAX_BOTTLES);
        printf("Producer delay: %d seconds\n", PRODUCER_DELAY);
        printf("Consumer delay: %d seconds\n\n", CONSUMER_DELAY);

        if (pthread_create(&producer_thread, NULL, producer, NULL) != 0)
        {
                perror("Failed to create producer thread");
                return 1;
        }

        if (pthread_create(&consumer_thread, NULL, consumer, NULL) != 0)
        {
                perror("Failed to create consumer thread");
                return 1;
        }

        pthread_join(producer_thread, NULL);
        pthread_join(consumer_thread, NULL);

        pthread_mutex_destroy(&queue.mutex);
        pthread_cond_destroy(&queue.can_produce);
        pthread_cond_destroy(&queue.can_consume);

        return 0;
}