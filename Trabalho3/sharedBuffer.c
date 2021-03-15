#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "semaphore.h"
#include "pthread.h"
#include "sharedBuffer.h"



void sharedBuffer_init (SharedBuffer *sb, int capacity)
{
    sb->buffer      = (void **)malloc(capacity * sizeof(void *));
    sb->iGet        = 0;
    sb->iPut        = 0;
    sb->nelems      = 0;
    sb->maxCapacity = capacity;

    sem_init(&sb->sEspacoLivre,   0,   capacity);
    sem_init(&sb->sEspacoOcupado, 0,   0       );

    pthread_mutex_init(&sb->mutex, NULL);
}

void sharedBuffer_destroy (SharedBuffer *sb)
{
    free(sb->buffer);

    sem_destroy(&sb->sEspacoLivre);
    sem_destroy(&sb->sEspacoOcupado);

    pthread_mutex_destroy(&sb->mutex);
}

void sharedBuffer_Put (SharedBuffer *sb, void *data)
{
    sem_wait(&sb->sEspacoLivre);

    pthread_mutex_lock(&sb->mutex);
        sb->buffer[sb->iPut] = data;
        //sb->iPut = ++sb->iPut % sb->maxCapacity;
        sb->iPut++;
        if (sb->iPut == sb->maxCapacity) sb->iPut =0;

        ++sb->nelems;
    pthread_mutex_unlock(&sb->mutex);

    sem_post(&sb->sEspacoOcupado);
}

void * sharedBuffer_Get (SharedBuffer *sb)
{
    void *ret;
    sem_wait(&sb->sEspacoOcupado);

    pthread_mutex_lock(&sb->mutex);

        ret=sb->buffer[sb->iGet];
        sb->iGet++;
        if(sb->iGet == sb->maxCapacity) sb->iGet=0;

        --sb->nelems;
    pthread_mutex_unlock(&sb->mutex);

    sem_post(&sb->sEspacoLivre);

    return ret;
}
