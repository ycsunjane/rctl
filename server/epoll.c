/*
 * ============================================================================
 *
 *       Filename:  epoll.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2015年01月13日 14时35分31秒
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
#include <assert.h>
#include <sys/epoll.h>

#include "epoll.h"
#include "common.h"
#include "log.h"

#define PARA_NUM 	(10)
static int ep = -1;

void epoll_insert(struct client_t *client)
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

void epoll_delete(struct client_t *client)
{
	int fd = client->sock;
	if(epoll_ctl(ep, EPOLL_CTL_DEL, fd, NULL) < -1) {
		sys_err("epoll ctl failed: %s(%d)\n",
			strerror(errno), errno);
		exit(-1);
	}
}

void epoll_recv(struct client_t *cli)
{
	assert(cli->ssl);

	ssize_t num;
	num = ssltcp_read(cli->ssl, cli->recvbuf, BUFLEN);
	if(num < 0) {
		cli_free(cli);
		return;
	}

	if(cli->outfd >= 0)
		write(cli->outfd, cli->recvbuf, num);
}

int Epoll_wait(struct epoll_event *ev)
{
	int num;
	do {
		num = epoll_wait(ep, ev, PARA_NUM, -1);
	} while(num < 0 && (errno == EINTR));
	if(num < 0) {
		sys_err("epoll wait: %s(%d)\n", 
			strerror(errno), errno);
		exit(-1);
	}
	return num;
}

void *epoll_loop(void *arg)
{
	struct epoll_event *ev;
	ev = Malloc(sizeof(struct epoll_event) * PARA_NUM);
	if(!ev) exit(-1);

	int i, num;
	while(1) {
		num = Epoll_wait(ev);

		for(i = 0; i < num; i++) {
			/* socket colse will set 
			 * EPOLLRDHUP and EPOLLIN */
			if(ev[i].events & EPOLLRDHUP ||
				!(ev[i].events & EPOLLIN)) {
				cli_free(ev[i].data.ptr);
				break;
			} else {
				epoll_recv(ev[i].data.ptr);
			}
		}
	}
}

void epoll_init()
{
	assert(ep < 0);
	ep = epoll_create(1);
	if(ep < 0) {
		sys_err("epoll create failed: %s(%d)\n",
			strerror(errno), errno);
		exit(-1);
	}
	if(Pthread_create(epoll_loop, NULL))
		exit(-1);
}

