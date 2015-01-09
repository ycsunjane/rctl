/*
 * =====================================================================================
 *
 *       Filename:  rctl.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2014年12月30日 09时48分58秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  jianxi sun (jianxi), ycsunjane@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef __RCTL_H__
#define __RCTL_H__
#include <unistd.h>
#include <stdint.h>

static char serverip[][50] = {
	"127.0.0.1",
	"shanliren.net",
};
#define TOTSER 	(sizeof(serverip) / sizeof(serverip[0]))

static uint16_t port[] = {
	6000,
	7000,
	8000,
};
#define TOTPRT (sizeof(port) / sizeof(port[0]))

void rctl(char *devid);
#endif /* __RCTL_H__ */
