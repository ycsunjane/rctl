/*
 * ============================================================================
 *
 *       Filename:  ser.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2014年12月30日 11时26分37秒
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
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>



#include "rctl.h"
#include "common.h"
#include "list.h"
#include "ser.h"

static int fd[TOTPRT];
LIST_HEAD(tothead);
LIST_HEAD(classhead);

#define RCVBUF_LEN 	(2048) 
static char recvbuf[RCVBUF_LEN];

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
		list_add_tail(&new->classlist, 
			&classhead);
	}
	return new;
}

static void accept_newcli(int sock)
{
	struct client_t *new = Malloc(sizeof(struct client_t));
	if(!new) goto end;

	socklen_t socklen;
	int fd = Accept(sock, 
		(struct sockaddr *)&new->cliaddr, &socklen);
	if(fd < 0) goto clean;
	new->sock = fd;

	ssize_t nread = Recv(fd, recvbuf, RCVBUF_LEN, 0);
	if(nread < 0) goto clean;
	strncpy(new->cliclass, recvbuf, DEVID_LEN);

	struct cliclass_t *class;
	if(!(class = newclass(new->cliclass))) goto clean;

	list_add_tail(&new->totlist, &tothead);
	list_add_tail(&new->classlist, &class->clilist);

	goto end;
clean:
	free(new);
end:
	return;
}

void *netloop(void * arg)
{
	struct sockaddr_in seraddr;

	memset(&seraddr, 0, sizeof(seraddr));

	int i, j, count;
	for(i = 0, j = 0, count = 0; i < TOTPRT; i++) {
		fd[j] = Socket(AF_INET, SOCK_STREAM, 0);
		if(fd[j] < 0) continue;
		seraddr.sin_family = AF_INET;
		seraddr.sin_addr.s_addr = htonl(INADDR_ANY);
		seraddr.sin_port = htons(port[i]);
		if(Bind(fd[j], (struct sockaddr *)&seraddr, sizeof(seraddr)))
			continue;
		if(Listen(fd[j], 0))
			continue;
		j++; count++;
	}

	int maxfd = 0;
	fd_set rset, rsetbak;
	FD_ZERO(&rset);
	for(i = 0; i < count; i++) {
		FD_SET(fd[i], &rset);
		maxfd = (maxfd > fd[i]) ? maxfd : fd[i];
	}
	maxfd++;
	rsetbak = rset;

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

void cmd_listclass()
{
	struct cliclass_t *ptr;
	printf("Class list:\n");
	list_for_each_entry(ptr, &classhead, classlist) {
		printf("%s\n", ptr->cliclass);
	}
}

void cmd_listcli()
{
	struct cliclass_t *class;
	struct client_t *cli;
	list_for_each_entry(class, &classhead, classlist) {
		printf("%s\n", class->cliclass);
		list_for_each_entry(cli, &class->clilist, classlist) {
			printf("\t%s\n", inet_ntoa(cli->cliaddr.sin_addr));
		}
	}
}

int command()
{
	while(1) {
		printf("1: list all class\n");
		printf("2: list all client\n");

		int num;
		scanf("%d", &num);
		switch(num) {
		case 1:
			cmd_listclass();
			break;
		case 2:
			cmd_listcli();
			break;
		}
	}
}

int main()
{
	int ret;
	pthread_t thread;
	ret = Pthread_create(&thread, NULL, netloop, NULL);
	if(ret) exit(-1);

	command();
	return 0;
}

