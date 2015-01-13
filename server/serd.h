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
#include <netinet/in.h>
#include <pthread.h>
#include "ssltcp.h"
#include "list.h"
#include "config.h"

struct cliclass_t {
	char 	cliclass[DEVID_LEN];

	int 	count;
	pthread_mutex_t 	lock;

	struct list_head 	classlist;
	struct list_head 	clilist;
};

struct client_t {
	struct cliclass_t *class;

	struct 	sockaddr_in cliaddr;
	int 	sock;
	SSL * 	ssl;

	unsigned char 	mac[ETH_ALEN];
	char 	recvbuf[BUFLEN];
	int 	outfd;
	FILE 	*outfile;

	struct list_head totlist;
	struct list_head classlist;
};

extern pthread_mutex_t classlock;
extern struct list_head classhead;
extern pthread_mutex_t totlock;
extern struct list_head tothead;

void serd_init();
void cli_free(struct client_t *cli);
#endif /* __SERD_H__ */
