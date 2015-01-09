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
#include <unistd.h>
#include <sys/stat.h>
#include <linux/limits.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>

#include "common.h"
#include "log.h"
#include "rctl.h"
#include "serd.h"
#include "ssltcp.h"

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

void cli_free(struct client_t *cli)
{
	ssltcp_shutdown(cli->ssl);
	ssltcp_free(cli->ssl);
	dequeue_cli(cli);
	epoll_delete(cli);
}

static int epoll_recv(struct client_t *cli)
{
	assert(cli->ssl);

	ssize_t num;
	sys_debug("ssl: %p\n", cli->ssl);
	/* BUFLEN-1, ensure recvbuf[num] = 0 */
	num = ssltcp_read(cli->ssl, cli->recvbuf, BUFLEN - 1);
	if(num <= 0) {
		cli_free(cli);
		return -1;
	}
	cli->recvbuf[num] = 0;

	if(cli->outfd >= 0)
		write(cli->outfd, cli->recvbuf, num);

	return 0;
}

static void *epoll_loop(void *arg)
{
	sys_debug("Pthread epoll_loop init\n");
	struct epoll_event *ev;
	ev = Malloc(sizeof(struct epoll_event) * PARA_NUM);
	if(!ev) exit(-1);

	int num;
	while(1) {
		do {
			num = epoll_wait(ep, ev, PARA_NUM, -1);
		} while(num < 0 && (errno == EINTR));
		if(num < 0) {
			sys_err("epoll loop down\n");
			exit(-1);
		}

		int i;
		for(i = 0; i < num; i++) {
			if(ev[i].events & EPOLLRDHUP) {
				sys_debug("epoll close\n");
				cli_free(ev[i].data.ptr);
				/* socket colse will set 
				 * EPOLLRDHUP and EPOLLIN */
				break;
			} else if(ev[i].events & EPOLLIN) {
				sys_debug("epoll recv\n");
				epoll_recv(ev[i].data.ptr);
			} else {
				sys_debug("epoll error\n");
				cli_free(ev[i].data.ptr);
			}
		}
	}
}

static void epoll_init()
{
	sys_debug("Epoll init\n");
	ep = epoll_create(1);
	if(ep < 0) {
		sys_err("epoll create failed: %s(%d)\n",
			strerror(errno), errno);
		exit(-1);
	}

	int ret;
	pthread_t thread;
	ret = Pthread_create(&thread, NULL, epoll_loop, NULL);
	if(ret) exit(-1);
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
	memset(new, 0, sizeof(struct cliclass_t));
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

static void open_outfd(struct client_t *cli)
{
	char path[PATH_MAX];
	snprintf(path, PATH_MAX, "/tmp/%s_%s",
		cli->mclass->cliclass,
		inet_ntoa(cli->cliaddr.sin_addr));

	cli->outfile = fopen(path, "a+");
	fflush(cli->outfile);
	if(!cli->outfile) {
		sys_err("Open %s failed: %s(%d)\n", 
			path, strerror(errno), errno);
		cli->outfd = -1;
		return;
	}
	cli->outfd = fileno(cli->outfile);
}

static void accept_newcli(int sock)
{
	struct client_t *new = Malloc(sizeof(struct client_t));
	if(!new) goto end;

	socklen_t socklen = sizeof(struct sockaddr_in);
	int fd = Accept(sock, 
		(struct sockaddr *)&new->cliaddr, &socklen);
	if(fd < 0) goto clean1;

	new->sock = fd;

	if( !(new->ssl = ssltcp_ssl(fd)) )
		goto clean2;

	if( !ssltcp_accept(new->ssl) )
		goto clean2;
	sys_debug("ssl accept success: %p\n", new->ssl);

	ssize_t nread = ssltcp_read(new->ssl, 
		recvbuf, BUFLEN);
	if(nread <= 0) goto clean3;

	strncpy(new->cliclass, recvbuf, DEVID_LEN);

	struct cliclass_t *class;
	if(!(class = newclass(new->cliclass))) 
		goto clean3;
	new->mclass = class;
	class->total++;
	open_outfd(new);

	list_add_tail(&new->totlist, &tothead);
	list_add_tail(&new->classlist, &class->clilist);
	epoll_insert(new);
	goto end;

clean3:
	ssltcp_shutdown(new->ssl);
	ssltcp_free(new->ssl);
clean2:
	close(fd);
clean1:
	free(new);
end:
	return;
}

void *rctlreg(void * arg)
{
	sys_debug("Pthread rctlreg init\n");
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

void serd_init()
{
	epoll_init();
	ssltcp_init(1);

	int ret;
	pthread_t thread;
	ret = Pthread_create(&thread, NULL, rctlreg, NULL);
	if(ret) exit(-1);
}
