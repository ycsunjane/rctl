/*
 * =====================================================================================
 *
 *       Filename:  epoll.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2015年01月13日 14时37分26秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  jianxi sun (jianxi), ycsunjane@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef __EPOLL_H__
#define __EPOLL_H__
#include "serd.h"

void epoll_init();
void *epoll_loop(void *arg);
void epoll_recv(struct client_t *cli);
void epoll_delete(struct client_t *client);
void epoll_insert(struct client_t *client);
#endif /* EPOLL_H */
