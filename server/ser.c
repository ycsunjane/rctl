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
#include <unistd.h>
#include <sys/wait.h>
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
#include <readline/readline.h>
#include <readline/history.h>

#include "rctl.h"
#include "common.h"
#include "list.h"
#include "ser.h"
#include "log.h"

static int fd[TOTPRT];
LIST_HEAD(tothead);
LIST_HEAD(classhead);

static char recvbuf[BUFLEN];

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

	socklen_t socklen = sizeof(struct sockaddr_in);
	int fd = Accept(sock, 
		(struct sockaddr *)&new->cliaddr, &socklen);
	if(fd < 0) goto clean;
	sys_debug("Accept: %d, %d new client: %s\n", 
		sock, fd, inet_ntoa(new->cliaddr.sin_addr));

	new->sock = fd;
	ssize_t nread = Recv(fd, recvbuf, BUFLEN, 0);
	if(nread <= 0) goto clean;
	strncpy(new->cliclass, recvbuf, DEVID_LEN);
	sys_debug("Recv class: %s, nread\n", new->cliclass);

	struct cliclass_t *class;
	if(!(class = newclass(new->cliclass))) 
		goto clean;

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
			printf("\t%d : %s\n", 
				cli->sock, inet_ntoa(cli->cliaddr.sin_addr));
		}
	}
}

void cmd_prcmd(struct client_t *cli, char *cmd)
{
	printf("\n------------- %s -------------\n > %s\n", 
		inet_ntoa(cli->cliaddr.sin_addr),
		cmd);
}

void cmd_prret(struct client_t *cli, char *ret)
{
	printf("%s", ret);
	printf("\n-----------------------------------\n");
}

static char cmd[BUFLEN];
static char buf[BUFLEN];
void cmd_sendcmd()
{
	printf("Input class:\n");
	char classname[DEVID_LEN];
	scanf("%s", classname);

	printf("Input cmd:\n");
	scanf("%s", cmd);

	struct cliclass_t *class;
	struct client_t *cli;
	list_for_each_entry(class, &classhead, classlist) {
		if(strcmp(classname, class->cliclass))
			continue;

		list_for_each_entry(cli, &class->clilist, classlist) {
			Send(cli->sock, cmd, strlen(cmd), 0);
			cmd_prcmd(cli, cmd);
			Recv(cli->sock, buf, BUFLEN, 0);
			cmd_prret(cli, buf);
		}
	}
}

void bash(int fd)
{
	Send(fd, "bash", strlen("bash"), 0);
	fd_set rset, bset;
	FD_ZERO(&rset);
	FD_SET(0, &rset);
	FD_SET(fd, &rset);
	int maxfd = fd + 1;
	bset = rset;

	int i, ret;
	char *rbuf;
	while(1) {
		rset = bset;
		ret = Select(maxfd, &rset, NULL, NULL, NULL);
		if(ret < 0) continue;

		if(FD_ISSET(0, &rset)) {
			rbuf = readline("jianxi >");
			if(Send(fd, rbuf, strlen(rbuf), 0) < 0) {
				free(rbuf);
				return;
			}
			free(rbuf);
		}

		if(FD_ISSET(fd, &rset)) {
			if(Recv(fd, recvbuf, BUFLEN, 0) <= 0) 
				return;
			printf("%s", recvbuf);
		}
	}
}

void cmd_bashto()
{
	printf("Input destip:\n");
	char destip[16];
	scanf("%s", destip);

	in_addr_t addr = inet_addr(destip);
	struct client_t *cli;
	list_for_each_entry(cli, &tothead, totlist) {
		if(cli->cliaddr.sin_addr.s_addr != addr)
			continue;
		bash(cli->sock);		
	}
}

int command()
{
	while(1) {
		printf("1: list all class\n");
		printf("2: list all client\n");
		printf("3: send command to all client\n");
		printf("4: connect bash to client\n");

		int num;
		scanf("%d", &num);
		switch(num) {
		case 1:
			cmd_listclass();
			break;
		case 2:
			cmd_listcli();
			break;
		case 3:
			cmd_sendcmd();
			break;
		case 4:
			cmd_bashto();
			break;
		}
	}
}

int debug = 1;
int main()
{
	int ret;
	pthread_t thread;
	ret = Pthread_create(&thread, NULL, netloop, NULL);
	if(ret) exit(-1);

	command();
	return 0;
}

