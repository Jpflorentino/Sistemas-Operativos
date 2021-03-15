#include <fcntl.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "semaphore.h"

typedef struct {
	void **buffer;
	int iPut;
	int iGet;
	int nelems;
	int maxCapacity;
	sem_t sEspacoLivre;
	sem_t sEspacoOcupado;
	pthread_mutex_t mutex;

}SharedBuffer;

void sharedBuffer_init (SharedBuffer *sb, int capacity);
void sharedBuffer_destroy (SharedBuffer *sb);
void sharedBuffer_Put (SharedBuffer *sb, void *data);
void * sharedBuffer_Get (SharedBuffer *sb);
