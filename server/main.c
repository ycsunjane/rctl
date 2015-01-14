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
#include <time.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <pthread.h>
#include <readline/readline.h>
#include <readline/history.h>

/* inet_ntoa */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "common.h"
#include "list.h"
#include "serd.h"
#include "epoll.h"
#include "bash.h"
#include "log.h"

void cmd_listclass()
{
	struct cliclass_t *ptr;
	printf("Class list:\n");

	pthread_mutex_lock(&classlock);
	list_for_each_entry(ptr, &classhead, classlist) {
		printf("%s\n", ptr->cliclass);
	}
	pthread_mutex_unlock(&classlock);
}

void cmd_listcli()
{
	char *macstr;
	struct cliclass_t *class;
	struct client_t *cli;
	pthread_mutex_lock(&classlock);
	list_for_each_entry(class, &classhead, classlist) {
		printf("%s\n", class->cliclass);

		pthread_mutex_lock(&class->lock);
		list_for_each_entry(cli, &class->clilist, 
			classlist) {
			macstr = getmacstr(cli->mac);
			printf("\t%d : %s %s\n", 
				cli->sock, macstr,
				inet_ntoa(cli->cliaddr.sin_addr));
			if(macstr)
				free(macstr);
		}
		pthread_mutex_unlock(&class->lock);
	}
	pthread_mutex_unlock(&classlock);
}

static void fllush_stdin()
{
	int c;
	while ( (c = getchar()) != '\n' && c != EOF ) { }
}

static char cmd[BUFLEN];
void cmd_sendcmd()
{
	printf("Input class:\n");
	char classname[DEVID_LEN];
	scanf("%s", classname);
	fllush_stdin();

	printf("Input cmd:\n");
	fgets(cmd, BUFLEN, stdin);
	cmd[strlen(cmd) - 1] = 0;

	struct cliclass_t *class;
	struct client_t *cli;
	pthread_mutex_lock(&classlock);
	list_for_each_entry(class, &classhead, classlist) {
		if(strcmp(classname, class->cliclass))
			continue;

		pthread_mutex_lock(&class->lock);
		list_for_each_entry(cli, &class->clilist, classlist) {
			pthread_mutex_lock(&cli->lock);
			if(open_outfd(cli)) {
				time_t now = time(NULL);
				char *nowstr = ctime(&now);
				nowstr[strlen(nowstr) - 1] = 0;
				fprintf(cli->outfile, "[%s] '%s'\n", 
					nowstr, cmd);
				fflush(cli->outfile);
				close_outfd(cli);
			}
			pthread_mutex_unlock(&cli->lock);

			if(ssltcp_write(cli->ssl, cmd, 
					strlen(cmd)) <= 0)
				continue;
		}
		pthread_mutex_unlock(&class->lock);
	}
	pthread_mutex_unlock(&classlock);
	return;
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

int debug = 0;
int main()
{
	epoll_init();
	ssltcp_init(1);
	serd_init();

	command();
	return 0;
}

