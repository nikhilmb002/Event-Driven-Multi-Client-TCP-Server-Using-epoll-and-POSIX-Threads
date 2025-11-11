#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>

typedef struct task {

	int client_fd;
    	struct task *next;
} task_t;

typedef struct {
    
	pthread_t *threads;
    	int thread_count;

    	task_t *task_queue_head;
   	task_t *task_queue_tail;

    	pthread_mutex_t mutex;
    	pthread_cond_t cond;

    	int stop;
} thread_pool_t;

int thread_pool_init(thread_pool_t *pool, int num_threads);
void thread_pool_add_task(thread_pool_t *pool, int client_fd);
void thread_pool_destroy(thread_pool_t *pool);

#endif
