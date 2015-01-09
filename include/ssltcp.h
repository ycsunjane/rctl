/*
 * =====================================================================================
 *
 *       Filename:  ssltcp.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2015年01月09日 10时49分23秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  jianxi sun (jianxi), ycsunjane@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef __SSLTCP_H__
#define __SSLTCP_H__
#include <openssl/ssl.h>

void ssltcp_init(int isserver);
SSL *ssltcp_ssl(int fd);
int ssltcp_accept(SSL *ssl);
int ssltcp_connect(SSL *ssl);
int ssltcp_read(SSL *ssl, char *buf, int num);
int ssltcp_write(SSL *ssl, char *buf, int num);
int ssltcp_shutdown(SSL *ssl);
void ssltcp_free(SSL *ssl);
#endif /* __SSLTCP_H__ */
