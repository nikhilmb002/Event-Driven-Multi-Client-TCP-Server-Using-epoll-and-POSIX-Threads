#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>

#include "shared_memory.h"

#define SHM_NAME "/server_stats_shm"

static int shm_fd;
static server_stats_t *stats = NULL;

int shared_memory_init() {

	shm_fd = shm_open(SHM_NAME,
			  O_CREAT | O_RDWR,
			  0666);

	if (shm_fd == -1) {

		perror("shm_open");
		return -1;
	}

	if (ftruncate(shm_fd,
		      sizeof(server_stats_t)) == -1) {

		perror("ftruncate");
		return -1;
	}

	stats = mmap(NULL,
		     sizeof(server_stats_t),
		     PROT_READ | PROT_WRITE,
		     MAP_SHARED,
		     shm_fd,
		     0);

	if (stats == MAP_FAILED) {

		perror("mmap");
		return -1;
	}

	memset(stats, 0, sizeof(server_stats_t));

	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_setpshared(&attr,
				     PTHREAD_PROCESS_SHARED);

	pthread_mutex_init(&stats->mutex, &attr);

	return 0;
}

void shared_memory_cleanup() {

	munmap(stats, sizeof(server_stats_t));
	close(shm_fd);
	shm_unlink(SHM_NAME);
}

server_stats_t *get_server_stats() {

	return stats;
}
