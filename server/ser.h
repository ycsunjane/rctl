/*
 * =====================================================================================
 *
 *       Filename:  ser.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2014年12月30日 13时53分22秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  jianxi sun (jianxi), ycsunjane@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef __SER_H__
#define __SER_H__
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "common.h"
#include "list.h"

#define DEVID_LEN 	(33)
struct cliclass_t {
	char cliclass[DEVID_LEN];
	int total;
	struct list_head classlist;
	pthread_mutex_t clilock;
	struct list_head clilist;
};

struct client_t {
	char cliclass[DEVID_LEN];
	struct sockaddr_in cliaddr;
	int 	sock;
	char recvbuf[BUFLEN];

	struct list_head totlist;
	struct list_head classlist;
	struct cliclass_t *mclass;
};


extern pthread_mutex_t totlock;
extern struct list_head tothead;
extern pthread_mutex_t classlock;
extern struct list_head classhead;

#endif /* __SER_H__ */
