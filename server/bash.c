/*
 * ============================================================================
 *
 *       Filename:  bash.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2015年01月13日 14时09分27秒
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

/* inet_addr */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <termios.h>
#include <unistd.h>

#include "serd.h"
#include "common.h"
#include "config.h"
#include "log.h"
 
static struct termios tin;
 
static void terminal_set(void) {
    tcgetattr(STDIN_FILENO, &tin);
     
    static struct termios tlocal;
    memcpy(&tlocal, &tin, sizeof(tin));
    cfmakeraw(&tlocal);
    tcsetattr(STDIN_FILENO,TCSANOW,&tlocal);
}
 
static void terminal_reset(void) {
    tcsetattr(STDIN_FILENO,TCSANOW,&tin);
}

int bashfd(in_addr_t addr)
{
	struct sockaddr_in bashaddr;
	int sock = Socket(AF_INET, SOCK_STREAM, 0);
	if(sock < 0) return -1;

	int flag = 1, len = sizeof(flag);
	if( Setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &flag, len))
		return -1;

	bashaddr.sin_family = AF_INET;
	bashaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	bashaddr.sin_port = htons(BASHPORT);
	if(Bind(sock, (struct sockaddr *)&bashaddr, sizeof(bashaddr)))
		return -1;

	if(Listen(sock, 0))
		return -1;

	struct sockaddr_in cliaddr;
	socklen_t socklen = sizeof(struct sockaddr_in);
	int fd = Accept(sock, (struct sockaddr *)&cliaddr, &socklen);
	if(fd < 0) {
		sys_debug("Accept addr failed: %s(%d)\n", 
			strerror(errno), errno);
		close(sock);
		return -1;
	}

	if(cliaddr.sin_addr.s_addr != addr) {
		sys_debug("Accept addr is error\n");
		close(sock);
		close(fd);
		return -1;
	}

	close(sock);
	return fd;
}

char recvbuf[BUFLEN];
void bashto(in_addr_t addr, SSL *oldssl)
{
	/* send rctlbash command */
	if(ssltcp_write(oldssl, RCTLBASH, 
			strlen(RCTLBASH)) < 0)
		return;

	int fd = bashfd(addr);
	if(fd < 0) return;

	SSL *ssl;
	if( !(ssl = ssltcp_ssl(fd))) {
		sys_err("Create ssl failed\n");
		close(fd);
		return;
	}

	if( !ssltcp_accept(ssl)) {
		sys_err("Accept ssl failed\n");
		ssltcp_free(ssl);
		close(fd);
		return;
	}
	terminal_set();

	fd_set rset, bset;
	FD_ZERO(&rset);
	FD_SET(0, &rset);
	FD_SET(fd, &rset);
	int maxfd = fd + 1;
	bset = rset;

	int i, ret;
	ssize_t nread;
	while(1) {
		rset = bset;
		ret = Select(maxfd, &rset, NULL, NULL, NULL);
		if(ret < 0) break;

		if(FD_ISSET(0, &rset)) {
			nread = read(0, recvbuf, BUFLEN);
			if(ssltcp_write(ssl, recvbuf, nread) < 0)
				break;
		}

		if(FD_ISSET(fd, &rset)) {
			if( (nread = ssltcp_read(ssl, recvbuf, BUFLEN)) < 0)
				break;
			write(1, recvbuf, nread);
		}
	}
}

void macstr_to_mac(char *macstr, char *mac)
{
	int i;
	char *next;
	for(i = 0; i < ETH_ALEN; i++) {
		mac[i] = (unsigned char)
			strtol(macstr, &next, 16);
		if(*next == '\0')
			break;
		macstr = next + 1;
	}
}

void cmd_bashto()
{
	printf("Input mac:\n");
	char macstr[MACSTR];
	scanf("%s", macstr);
	
	char mac[ETH_ALEN];
	macstr_to_mac(macstr, mac);

	struct client_t *cli;
	pthread_mutex_lock(&totlock);
	list_for_each_entry(cli, &tothead, totlist) {
		if(memcmp(mac, cli->mac, ETH_ALEN))
			continue;
		pthread_mutex_unlock(&totlock);
		bashto(cli->cliaddr.sin_addr.s_addr, cli->ssl);
		terminal_reset();
		return;
	}

	/* if no find unlock */
	pthread_mutex_unlock(&totlock);
}

