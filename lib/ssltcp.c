/*
 * ============================================================================
 *
 *       Filename:  ssltcp.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2015年01月09日 10时48分49秒
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
#include <assert.h>
#include <openssl/err.h>

#include "ssltcp.h"
#include "config.h"
#include "log.h"

SSL_CTX *ctx = NULL;

static void 
ssltcp_cert(SSL_CTX *ctx, const char *file, int type)
{
	int ret;
	ret = SSL_CTX_use_certificate_file(ctx, file, type);
	if(ret != 1) {
		ret = ERR_get_error();
		sys_err("SSL_CTX_use_cert failed: %s(%d)\n", 
			ERR_error_string(ret, NULL), ret);
		exit(-1);
	}
}

static void 
ssltcp_priv(SSL_CTX *ctx, const char *file, int type)
{
	int ret;
	ret = SSL_CTX_use_PrivateKey_file(ctx, file, type);
	if(ret != 1) {
		ret = ERR_get_error();
		sys_err("SSL_CTX_use_PrivateKey failed: %s(%d)\n", 
			ERR_error_string(ret, NULL), ret);
		exit(-1);
	}
}

static void
ssltcp_ca(SSL_CTX *ctx, const char *CAfile, const char *CApath)
{
	int ret;
	ret = SSL_CTX_load_verify_locations(ctx, CAfile, CApath);
	if(ret != 1) {
		ret = ERR_get_error();
		sys_err("SSL_ca failed: %s(%d)\n", 
			ERR_error_string(ret, NULL), ret);
		exit(-1);
	}
}

static void ssltcp_ctx(int isserver)
{
	assert(ctx == NULL);
	int ret;
	const SSL_METHOD *method;
	if(isserver)
		method = SSLv23_server_method();
	else
		method = SSLv23_client_method();

	ctx = SSL_CTX_new(method);
	if(!ctx) {
		ret = ERR_get_error();
		sys_err("SSL_CTX_new failed: %s(%d)\n", 
			ERR_error_string(ret, NULL), ret);
		exit(-1);
	}

	if(isserver) {
		ssltcp_cert(ctx, CERT_FILE, SSL_FILETYPE_PEM);
		ssltcp_priv(ctx, PRIV_FILE, SSL_FILETYPE_PEM);
	} else {
		ssltcp_ca(ctx, CA_FILE, NULL);
	}
}

void ssltcp_init(int isserver)
{
	SSL_load_error_strings();
	SSL_library_init();
	ssltcp_ctx(isserver);
}

SSL *ssltcp_ssl(int fd)
{
	SSL *ssl = SSL_new(ctx);
	if(!ssl) {
		sys_err("SSL new failed");
		return NULL;
	}

	int ret;
	ret = SSL_set_fd(ssl, fd);
	if(ret != 1) {
		SSL_free(ssl);
		ret = ERR_get_error();
		sys_err("SSL_set_fd failed: %s(%d)\n", 
			ERR_error_string(ret, NULL), ret);
		return NULL;
	}
	return ssl;
}

int ssltcp_accept(SSL *ssl)
{
	int ret = SSL_accept(ssl);
	if(ret < 0) {
		ret = SSL_get_error(ssl, ret);
		sys_err("SSL_accept failed: %d\n", ret);
		return 0;
	} else if(ret == 0) {
		ret = ERR_get_error();
		sys_err("SSL_accept failed: %s(%d)\n", 
			ERR_error_string(ret, NULL), ret);
		return 0;
	}
	return 1;
}

int ssltcp_connect(SSL *ssl)
{
	int ret = SSL_connect(ssl);
	if(ret < 0) {
		ret = SSL_get_error(ssl, ret);
		sys_err("SSL_connect failed: %d\n", ret);
		return -1;
	} else if(ret == 0) {
		ret = ERR_get_error();
		sys_err("SSL_connect failed: %s(%d)\n", 
			ERR_error_string(ret, NULL), ret);
		return -1;
	}
	return 0;
}

int ssltcp_read(SSL *ssl, char *buf, int num)
{
	int ret;
repeat:
	ret = SSL_read(ssl, buf, num);
	if(ret < 0) {
		ret = SSL_get_error(ssl, ret);
		if(ret == SSL_ERROR_WANT_READ ||
			ret == SSL_ERROR_WANT_WRITE)
			goto repeat;
		if(ret == SSL_ERROR_ZERO_RETURN) {
			sys_debug("remote ssl closed\n");
			return -1;
		}
		sys_err("SSL_read failed: %d\n", ret);
		return -1;
	} else if(ret == 0) {
		ret = ERR_get_error();
		sys_err("SSL_connect failed: %s(%d)\n", 
			ERR_error_string(ret, NULL), ret);
		return -1;
	}
	return ret;
}

int ssltcp_write(SSL *ssl, char *buf, int num)
{
	int ret;
repeat:
	ret = SSL_write(ssl, buf, num);
	if(ret < 0) {
		ret = SSL_get_error(ssl, ret);
		if(ret == SSL_ERROR_WANT_READ ||
			ret == SSL_ERROR_WANT_WRITE)
			goto repeat;
		if(ret == SSL_ERROR_ZERO_RETURN) {
			sys_debug("remote ssl closed\n");
			return -1;
		}
		sys_err("SSL_write failed: %d\n", ret);
		return -1;
	} else if(ret == 0) {
		ret = ERR_get_error();
		sys_err("SSL_connect failed: %s(%d)\n", 
			ERR_error_string(ret, NULL), ret);
		return -1;
	}
	return ret;
}

int ssltcp_shutdown(SSL *ssl)
{
	int ret;
repeat:
	ret = SSL_shutdown(ssl);
	if(ret == 0) {
		goto repeat;
	} else if(ret < 0) {
		ret = SSL_get_error(ssl, ret);
		sys_warn("SSL_shutdown failed: %d\n", ret);
		return -1;
	}
	return 0;
}

void ssltcp_free(SSL *ssl)
{
	SSL_free(ssl);
}
