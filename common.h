/*
 * =====================================================================================
 *
 *       Filename:  lnet.h
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

int Socket(int domain, int type, int protocol);
int Bind(int socket, const struct sockaddr *address,
	socklen_t address_len);
int Listen(int socket, int backlog);
int Select(int nfds, fd_set * readfds,
	fd_set * writefds, fd_set * errorfds,
	struct timeval * timeout);
int Accept(int socket, struct sockaddr * address,
	socklen_t * address_len);
void *Malloc(size_t size);
#ifdef SERVER
int Pthread_create(pthread_t *thread, const pthread_attr_t *attr,
	void *(*start_routine) (void *), void *arg);
#endif
ssize_t Recv(int socket, void *buffer, size_t length, int flags);
#endif /* __LNET_H__ */
