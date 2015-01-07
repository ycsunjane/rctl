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
#include <sys/select.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "rctl.h"
#include "common.h"
#include "list.h"
#include "ser.h"
#include "serd.h"
#include "log.h"

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
			//if(Recv(fd, recvbuf, BUFLEN, 0) <= 0) 
			//	return;
			//printf("%s", recvbuf);
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
	ret = Pthread_create(&thread, NULL, rctlreg, NULL);
	if(ret) exit(-1);

	command();
	return 0;
}

