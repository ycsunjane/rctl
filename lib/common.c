/*
 * ============================================================================
 *
 *       Filename:  lnet.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2014年12月30日 13时50分57秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  jianxi sun (jianxi), ycsunjane@gmail.com
 *   Organization:  
 *
 * ============================================================================
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <sys/select.h>
#include <errno.h>
#include <sys/socket.h>
#include <pthread.h>

int Socket(int domain, int type, int protocol)
{
	int fd = socket(domain, type, protocol);
	if(fd < 0) {
		fprintf(stderr, "Create socket failed: %s\n", 
			strerror(errno));
		return -1;
	}
	return fd;
}

int Bind(int socket, const struct sockaddr *address,
	socklen_t address_len) 
{
	int ret = bind(socket, address, address_len);
	if(ret < 0) {
		fprintf(stderr, "Bind socket failed: %s\n", 
			strerror(errno));
		return -1;
	}
	return 0;
}

int Listen(int socket, int backlog)
{
	int ret = listen(socket, backlog);
	if(ret < 0) {
		fprintf(stderr, "Listen socket failed: %s\n", 
			strerror(errno));
		return -1;
	}
	return 0;
}

int Select(int nfds, fd_set * readfds,
	fd_set * writefds, fd_set * errorfds,
	struct timeval * timeout)
{
	int ret;
	do {
		ret = select(nfds, readfds, writefds, errorfds, 
			timeout);
	} while(ret < 0 && errno == EINTR);

	return ret;
}

int Accept(int socket, struct sockaddr * address,
	socklen_t * address_len)
{
	int fd;
	fd = accept(socket, address, address_len);
	if(fd < 0) {
		fprintf(stderr, "Accept socket failed: %s\n", 
			strerror(errno));
		return -1;
	}
	return fd;
}

ssize_t Recv(int socket, void *buffer, size_t length, int flags)
{
	ssize_t ret;
	do {
		ret = recv(socket, buffer, length, flags);
	} while(ret < 0 && errno == EINTR);

	return ret;
}

ssize_t Send(int socket, const void *buffer, 
	size_t length, int flags)
{
	int ret;
	do {
		ret = send(socket, buffer, length, flags);
	} while(ret < 0 && errno == EINTR);

	return ret;
}

void *Malloc(size_t size)
{
	void *ptr = malloc(size);
	if(!ptr) {
		fprintf(stderr, "Malloc failed: %s\n", 
			strerror(errno));
		return NULL;
	}
	return ptr;
}

int Pthread_create(void *(*start_routine) (void *), void *arg)
{
	int ret;
	pthread_t thread;
	ret = pthread_create(&thread, NULL, start_routine, arg);
	if(ret)
		fprintf(stderr, "Pthread_create failed: %s\n", 
			strerror(errno));
	return ret;
}

int Setsockopt(int socket, int level, int option_name,
	const void *option_value, socklen_t option_len)
{
	int ret = setsockopt(socket, level, 
		option_name, option_value, option_len);
	if(ret < 0)
		fprintf(stderr, "Setsocket failed: %s\n", 
			strerror(errno));
	return ret;
}
