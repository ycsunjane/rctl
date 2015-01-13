/*
 * =====================================================================================
 *
 *       Filename:  common.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2014年12月30日 13时51分24秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  jianxi sun (jianxi), ycsunjane@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef __LNET_H__
#define __LNET_H__
#include <sys/socket.h>
#include <pthread.h>

int Socket(int domain, int type, int protocol);
int Bind(int socket, const struct sockaddr *address,
	socklen_t address_len);
int Listen(int socket, int backlog);
int Select(int nfds, fd_set * readfds,
	fd_set * writefds, fd_set * errorfds,
	struct timeval * timeout);
int Accept(int socket, struct sockaddr * address,
	socklen_t * address_len);
ssize_t Send(int socket, const void *buffer, 
	size_t length, int flags);
void *Malloc(size_t size);
int Pthread_create(void *(*start_routine) (void *), void *arg);

int Setsockopt(int socket, int level, int option_name,
	const void *option_value, socklen_t option_len);
ssize_t Recv(int socket, void *buffer, size_t length, int flags);
#endif /* __LNET_H__ */
