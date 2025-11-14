#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

#include <pthread.h>

typedef struct {

	int total_connections;
	int active_connections;
	long total_requests;

	pthread_mutex_t mutex;

} server_stats_t;

int shared_memory_init();
void shared_memory_cleanup();

server_stats_t *get_server_stats();

#endif
