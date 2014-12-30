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
#include "list.h"

#define DEVID_LEN 	(33)
struct client_t {
	char cliclass[DEVID_LEN];
	struct sockaddr_in cliaddr;
	int 	sock;

	struct list_head totlist;
	struct list_head classlist;
};

struct cliclass_t {
	char cliclass[DEVID_LEN];
	struct list_head classlist;
	struct list_head clilist;
};

#endif /* __SER_H__ */
