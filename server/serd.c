/*
 * ============================================================================
 *
 *       Filename:  serd.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2015年01月07日 14时50分16秒
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
#include <sys/epoll.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>

#include "ser.h"
#include "log.h"
#include "rctl.h"

static char recvbuf[BUFLEN];
static int ep;

pthread_mutex_t totlock = PTHREAD_MUTEX_INITIALIZER;
LIST_HEAD(tothead);
pthread_mutex_t classlock = PTHREAD_MUTEX_INITIALIZER;
LIST_HEAD(classhead);

static void epoll_insert(struct client_t *client)
{
	int fd = client->sock;
	struct epoll_event ev;
	memset(&ev, 0, sizeof(ev));
	ev.events |= EPOLLIN | EPOLLRDHUP;
	ev.data.ptr = client;
	if(epoll_ctl(ep, EPOLL_CTL_ADD, fd, &ev) < -1) {
		sys_err("epoll ctl failed: %s(%d)\n",
			strerror(errno), errno);
		exit(-1);
	}
}

static void epoll_delete(struct client_t *client)
{
	int fd = client->sock;
	if(epoll_ctl(ep, EPOLL_CTL_DEL, fd, NULL) < -1) {
		sys_err("epoll ctl failed: %s(%d)\n",
			strerror(errno), errno);
		exit(-1);
	}
}


#define PARA_NUM 	(10)
static void epoll_init()
{
	ep = epoll_create(1);
	if(ep < 0) {
		sys_err("epoll create failed: %s(%d)\n",
			strerror(errno), errno);
		exit(-1);
	}
}

static void dequeue_class(struct cliclass_t *cliclass)
{
	pthread_mutex_lock(&classlock);
	list_del(&cliclass->classlist);
	pthread_mutex_unlock(&classlock);
	free(cliclass);
}

static void dequeue_cli(struct client_t *cli)
{
	close(cli->sock);
	pthread_mutex_lock(&totlock);
	list_del(&cli->totlist);
	pthread_mutex_unlock(&totlock);

	if(cli->mclass->total == 1) {
		pthread_mutex_lock(&cli->mclass->clilock);
		list_del(&cli->classlist);
		pthread_mutex_unlock(&cli->mclass->clilock);

		dequeue_class(cli->mclass);
		goto end;
	} else {
		cli->mclass->total--;
		pthread_mutex_lock(&cli->mclass->clilock);
		list_del(&cli->classlist);
		pthread_mutex_unlock(&cli->mclass->clilock);
		goto end;
	}
end:
	free(cli);
}

static int epoll_recv(struct client_t *cli)
{
	ssize_t num;
	num = Recv(cli->sock, cli->recvbuf, BUFLEN - 1, 0);
	if(num <= 0) {
		dequeue_cli(cli);
		return -1;
	}
	return 0;
}

static void *epoll_loop(void *arg)
{
	struct epoll_event *ev;
	ev = Malloc(sizeof(struct epoll_event) * PARA_NUM);
	if(!ev) exit(-1);

	int num;
	while(1) {
		do {
			num = epoll_wait(ep, ev, PARA_NUM, -1);
		} while(num < 0 && (errno == EINTR));

		int i;
		for(i = 0; i < num; i++) {
			if(ev[i].events & EPOLLIN) {
				if(epoll_recv(ev[i].data.ptr) < 0)
					epoll_delete(ev[i].data.ptr);
			} else {
				sys_debug("events : %u, isclose: %d\n",
					ev[i].events,
					ev[i].events & EPOLLRDHUP);
				dequeue_cli(ev[i].data.ptr);
				epoll_delete(ev[i].data.ptr);
			}
		}
	}
}

struct cliclass_t *newclass(char *class)
{
	struct cliclass_t *ptr;
	list_for_each_entry(ptr, &classhead, classlist) {
		if(!strcmp(class, ptr->cliclass))
			return ptr;
	}

	struct cliclass_t *new =
		Malloc(sizeof(struct cliclass_t));
	if(new) {
		strncpy(new->cliclass, class, DEVID_LEN);
		INIT_LIST_HEAD(&new->classlist);
		INIT_LIST_HEAD(&new->clilist);
		pthread_mutex_init(&new->clilock, NULL);

		pthread_mutex_lock(&classlock);
		list_add_tail(&new->classlist, 
			&classhead);
		pthread_mutex_unlock(&classlock);
	}
	return new;
}

static void accept_newcli(int sock)
{
	struct client_t *new = Malloc(sizeof(struct client_t));
	if(!new) goto end;

	socklen_t socklen = sizeof(struct sockaddr_in);
	int fd = Accept(sock, 
		(struct sockaddr *)&new->cliaddr, &socklen);
	if(fd < 0) goto clean;

	new->sock = fd;
	ssize_t nread = Recv(fd, recvbuf, BUFLEN, 0);
	if(nread <= 0) goto clean;
	strncpy(new->cliclass, recvbuf, DEVID_LEN);
	sys_debug("Registe new class: %s\n", new->cliclass);

	struct cliclass_t *class;
	if(!(class = newclass(new->cliclass))) 
		goto clean;

	list_add_tail(&new->totlist, &tothead);
	list_add_tail(&new->classlist, &class->clilist);
	epoll_insert(new);
	goto end;

clean:
	free(new);
end:
	return;
}

void *rctlreg(void * arg)
{
	epoll_init();

	struct sockaddr_in seraddr;
	static int fd[TOTPRT];

	memset(&seraddr, 0, sizeof(seraddr));

	int i, j, count, flag = 1, len = sizeof(flag);
	/* listen all support port */
	for(i = 0, j = 0, count = 0; i < TOTPRT; i++) {
		/* enable socket reuse */
		fd[j] = Socket(AF_INET, SOCK_STREAM, 0);
		if(fd[j] < 0) continue;
		if( Setsockopt(fd[j], SOL_SOCKET, 
				SO_REUSEADDR, &flag, len))
			continue;

		/* Bind INADDR_ANY */
		seraddr.sin_family = AF_INET;
		seraddr.sin_addr.s_addr = htonl(INADDR_ANY);
		seraddr.sin_port = htons(port[i]);
		if(Bind(fd[j], (struct sockaddr *)&seraddr, sizeof(seraddr)))
			continue;

		/* Listen */
		if(Listen(fd[j], 0))
			continue;
		j++; count++;
	}

	/* select all listen fd */
	int maxfd = 0;
	fd_set rset, rsetbak;
	FD_ZERO(&rset);
	for(i = 0; i < count; i++) {
		FD_SET(fd[i], &rset);
		maxfd = (maxfd > fd[i]) ? maxfd : fd[i];
	}
	maxfd++;
	rsetbak = rset;

	/* accept client connect */
	int num;
	while(1) {
		rset = rsetbak;
		num = Select(maxfd, &rset, NULL, NULL, NULL);
		if(num == -1) continue;
		for(i = 0, j = 0; i < count && j < num; i++) {
			if(FD_ISSET(fd[i], &rset)) {
				accept_newcli(fd[i]);
				j++;
			}
		}
	}
}
