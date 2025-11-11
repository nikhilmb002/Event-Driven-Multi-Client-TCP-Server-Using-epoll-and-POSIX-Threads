#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "thread_pool.h"

#define BUFFER_SIZE 1024

void handle_client(int client_fd) {

	 char buffer[BUFFER_SIZE];
    	 ssize_t bytes_read;

	 bytes_read = read(client_fd, buffer, BUFFER_SIZE - 1);

	 if (bytes_read <= 0) {
		
		close(client_fd);
		return;
	 }

	 buffer[bytes_read] = '\0';
	 printf("Worker handled FD %d: %s", client_fd, buffer);

	 write(client_fd, buffer, bytes_read);
}

void *worker_thread(void *arg) {

	thread_pool_t *pool = (thread_pool_t *)arg;

    	while (1) {
       
		 pthread_mutex_lock(&pool->mutex);

        	while (pool->task_queue_head == NULL && !pool->stop) {
           
			 pthread_cond_wait(&pool->cond, &pool->mutex);
        	}

        	if (pool->stop){
        
	    		pthread_mutex_unlock(&pool->mutex);
            		break;
        	}

        	task_t *task = pool->task_queue_head;
       		pool->task_queue_head = task->next;

		if (pool->task_queue_head == NULL)
	
			    pool->task_queue_tail = NULL;

		pthread_mutex_unlock(&pool->mutex);

		handle_client(task->client_fd);
		free(task);
	}

    return NULL;
}

int thread_pool_init(thread_pool_t *pool, int num_threads) {
   
    	pool->thread_count = num_threads;
    	pool->stop = 0;
	pool->task_queue_head = NULL;
	pool->task_queue_tail = NULL;

	pthread_mutex_init(&pool->mutex, NULL);
	pthread_cond_init(&pool->cond, NULL);

	pool->threads = malloc(sizeof(pthread_t) * num_threads);

	for (int i = 0; i < num_threads; i++) {
	       
		 if (pthread_create(&pool->threads[i], NULL, worker_thread, pool) != 0) {
		    
			perror("pthread_create");
		    	return -1;
		}
	}

   	 return 0;
}

void thread_pool_add_task(thread_pool_t *pool, int client_fd) {
   	
	task_t *task = malloc(sizeof(task_t));
    	task->client_fd = client_fd;
    	task->next = NULL;

    	pthread_mutex_lock(&pool->mutex);

  	if (pool->task_queue_tail == NULL) {
       
		 pool->task_queue_head = task;
      		 pool->task_queue_tail = task;
    	}
    
	else {
        
		pool->task_queue_tail->next = task;
       		pool->task_queue_tail = task;
    	}

    	pthread_cond_signal(&pool->cond);
    	pthread_mutex_unlock(&pool->mutex);
}

void thread_pool_destroy(thread_pool_t *pool) {
    
	pthread_mutex_lock(&pool->mutex);
    	pool->stop = 1;
    	pthread_cond_broadcast(&pool->cond);
   	pthread_mutex_unlock(&pool->mutex);

    	for (int i = 0; i < pool->thread_count; i++) {
        	
		pthread_join(pool->threads[i], NULL);
    	}

    	free(pool->threads);
 	pthread_mutex_destroy(&pool->mutex);
   	pthread_cond_destroy(&pool->cond);
}
