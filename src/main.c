#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#include "thread_pool.h"

#define PORT 8080
#define MAX_EVENTS 1024
#define THREAD_COUNT 4

int server_fd;
int epoll_fd;
volatile sig_atomic_t stop_server = 0;

thread_pool_t pool;

void handle_sigint(int sig) {

	(void)sig;
	stop_server = 1;
}

int set_nonblocking(int fd) {

	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1)
		return -1;

	return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int create_server_socket() {

	int fd;
	struct sockaddr_in addr;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1) {

		perror("socket");
		exit(EXIT_FAILURE);
	}

	int opt = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(PORT);

	if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {

		perror("bind");
		close(fd);
		exit(EXIT_FAILURE);
	}

	if (listen(fd, SOMAXCONN) == -1) {

		perror("listen");
		close(fd);
		exit(EXIT_FAILURE);
	}

	if (set_nonblocking(fd) == -1) {

		perror("fcntl");
		close(fd);
		exit(EXIT_FAILURE);
	}

	return fd;
}

void accept_new_connection() {

	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);

	while (1) {

		int client_fd = accept(server_fd,
				(struct sockaddr *)&client_addr,
				&client_len);

		if (client_fd == -1) {

			if (errno == EAGAIN || errno == EWOULDBLOCK)
				break;
			else {

				perror("accept");
				break;
			}
		}

		if (set_nonblocking(client_fd) == -1) {

			perror("fcntl client");
			close(client_fd);
			continue;
		}

		struct epoll_event ev;
		ev.events = EPOLLIN;
		ev.data.fd = client_fd;

		if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) == -1) {

			perror("epoll_ctl: client add");
			close(client_fd);
			continue;
		}

		printf("New client connected: FD=%d\n", client_fd);
	}
}

int main() {

	signal(SIGINT, handle_sigint);

	server_fd = create_server_socket();

	epoll_fd = epoll_create1(0);
	if (epoll_fd == -1) {

		perror("epoll_create1");
		close(server_fd);
		exit(EXIT_FAILURE);
	}

	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = server_fd;

	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) == -1) {

		perror("epoll_ctl: server add");
		close(server_fd);
		close(epoll_fd);
		exit(EXIT_FAILURE);
	}

	if (thread_pool_init(&pool, THREAD_COUNT) != 0) {

		fprintf(stderr, "Failed to initialize thread pool\n");
		close(server_fd);
		close(epoll_fd);
		exit(EXIT_FAILURE);
	}

	struct epoll_event events[MAX_EVENTS];

	printf("Server started on port %d...\n", PORT);

	while (!stop_server) {

		int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

		if (n == -1) {

			if (errno == EINTR)
				continue;

			perror("epoll_wait");
			break;
		}

		for (int i = 0; i < n; i++) {

			if (events[i].data.fd == server_fd) {

				accept_new_connection();
			}
			else {

				thread_pool_add_task(&pool,
						events[i].data.fd);
			}
		}
	}

	printf("\nShutting down server...\n");

	thread_pool_destroy(&pool);
	close(server_fd);
	close(epoll_fd);

	return 0;
}
