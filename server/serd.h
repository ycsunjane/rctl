/*
 * =====================================================================================
 *
 *       Filename:  serd.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2015年01月07日 20时43分50秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  jianxi sun (jianxi), ycsunjane@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef __SERD_H__
#define __SERD_H__
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "common.h"
#include "list.h"
#include "ssltcp.h"

/* epoll parallel number */
#define PARA_NUM 	(10)
#define DEVID_LEN 	(33)
struct cliclass_t {
	char 	cliclass[DEVID_LEN];
	int 	total;
	struct list_head 	classlist;
	pthread_mutex_t 	clilock;
	struct list_head 	clilist;
};

struct client_t {
	char 	cliclass[DEVID_LEN];
	struct 	sockaddr_in cliaddr;
	int 	sock;
	SSL * 	ssl;


	char 	recvbuf[BUFLEN];
	int 	outfd;
	FILE 	*outfile;

	struct list_head totlist;
	struct list_head classlist;
	struct cliclass_t *mclass;
};

extern pthread_mutex_t totlock;
extern struct list_head tothead;
extern pthread_mutex_t classlock;
extern struct list_head classhead;

void serd_init();
void cli_free(struct client_t *cli);
#endif /* __SERD_H__ */
