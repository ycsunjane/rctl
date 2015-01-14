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
#include <stdlib.h>

/*inet_ntoa*/
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "epoll.h"
#include "common.h"
#include "log.h"
#include "serd.h"

static char recvbuf[BUFLEN];

pthread_mutex_t classlock = PTHREAD_MUTEX_INITIALIZER;
LIST_HEAD(classhead);
pthread_mutex_t totlock = PTHREAD_MUTEX_INITIALIZER;
LIST_HEAD(tothead);

void close_outfd(struct client_t *cli)
{
	close(cli->outfd);
	fclose(cli->outfile);
	cli->outfd = -1;
	cli->outfile = NULL;
}

int open_outfd(struct client_t *cli)
{
	char path[PATH_MAX];
	char *macstr = getmacstr(cli->mac);
	snprintf(path, PATH_MAX, "/tmp/%s_%s_%s",
		cli->class->cliclass,
		inet_ntoa(cli->cliaddr.sin_addr),
		macstr);
	if(macstr)
		free(macstr);

	cli->outfile = fopen(path, "a+");
	fflush(cli->outfile);
	if(!cli->outfile) {
		sys_err("Open %s failed: %s(%d)\n", 
			path, strerror(errno), errno);
		cli->outfd = -1;
		return 0;
	}
	cli->outfd = fileno(cli->outfile);
	return 1;
}

static void getclass(struct cliclass_t *class)
{
	class->count++;
}

static void putclass(struct cliclass_t *class)
{
	if(!(--class->count)) {
		list_del(&class->classlist);
		pthread_mutex_destroy(&class->lock);
		free(class);
	}
}

static struct cliclass_t *newclass(char *classname)
{
	struct cliclass_t *ptr;
	pthread_mutex_lock(&classlock);
	list_for_each_entry(ptr, &classhead, classlist) {
		if(!strcmp(classname, ptr->cliclass)) {
			pthread_mutex_lock(&ptr->lock);
			getclass(ptr);
			pthread_mutex_unlock(&ptr->lock);
			pthread_mutex_unlock(&classlock);
			return ptr;
		}
	}

	struct cliclass_t *new =
		Malloc(sizeof(struct cliclass_t));
	if(!new) {
		pthread_mutex_unlock(&classlock);
		return NULL;
	}

	memset(new, 0, sizeof(struct cliclass_t));
	strncpy(new->cliclass, classname, DEVID_LEN);
	INIT_LIST_HEAD(&new->classlist);
	INIT_LIST_HEAD(&new->clilist);
	pthread_mutex_init(&new->lock, NULL);
	pthread_mutex_lock(&new->lock);
	getclass(new);
	pthread_mutex_unlock(&new->lock);
	list_add_tail(&new->classlist, &classhead);

	pthread_mutex_unlock(&classlock);
	return new;
}

static void accept_newcli(int sock)
{
	struct client_t *new;
	new = Malloc(sizeof(struct client_t));
	if(!new) return;

	socklen_t socklen = sizeof(struct sockaddr_in);
	int fd = Accept(sock, (struct sockaddr *)&new->cliaddr, &socklen);
	if(fd < 0) goto clean1;

	new->sock = fd;
	if( !(new->ssl = ssltcp_ssl(fd)) )
		goto clean2;
	if( !ssltcp_accept(new->ssl) )
		goto clean3;

	struct reg_t reg;
	ssize_t nread = ssltcp_read(new->ssl, 
		(char *)&reg, sizeof(reg));
	if(nread < 0) goto clean4;

	struct cliclass_t *class;
	char *classname = reg.class;
	classname[DEVID_LEN - 1] = 0;
	if(!(class = newclass(classname))) 
		goto clean4;
	new->class = class;
	memcpy(new->mac, reg.mac, ETH_ALEN);
	pthread_mutex_init(&new->lock, NULL);

	pthread_mutex_lock(&class->lock);
	pthread_mutex_lock(&totlock);
	list_add_tail(&new->totlist, &tothead);
	list_add_tail(&new->classlist, &class->clilist);
	epoll_insert(new);
	pthread_mutex_unlock(&totlock);
	pthread_mutex_unlock(&class->lock);
	return;

clean4:
	ssltcp_shutdown(new->ssl);
clean3:
	ssltcp_free(new->ssl);
clean2:
	close(fd);
clean1:
	free(new);
}

static void *rctlreg(void * arg)
{
	static int fd[TOTPRT];
	struct sockaddr_in seraddr;
	memset(&seraddr, 0, sizeof(seraddr));

	int i, j, count, flag = 1, len = sizeof(flag);
	/* listen all support port */
	for(i = 0, j = 0, count = 0; i < TOTPRT; i++) {
		/* enable socket reuse */
		fd[j] = Socket(AF_INET, SOCK_STREAM, 0);
		if(fd[j] < 0) continue;

		if( Setsockopt(fd[j], SOL_SOCKET, SO_REUSEADDR, &flag, len))
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
	return NULL;
}

void cli_free(struct client_t *cli)
{
	pthread_mutex_lock(&classlock);
	pthread_mutex_lock(&cli->class->lock);
	pthread_mutex_lock(&totlock);
	list_del(&cli->totlist);
	list_del(&cli->classlist);
	epoll_delete(cli);
	putclass(cli->class);
	pthread_mutex_unlock(&totlock);
	pthread_mutex_unlock(&cli->class->lock);
	pthread_mutex_unlock(&classlock);

	ssltcp_shutdown(cli->ssl);
	ssltcp_free(cli->ssl);
	close(cli->sock);
	pthread_mutex_destroy(&cli->lock);
	free(cli);
}

void serd_init()
{
	if(Pthread_create(rctlreg, NULL))
		exit(-1);
}
