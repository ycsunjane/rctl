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
#include <stdint.h>

#ifdef CLIENT
static char serverip[][50] = {
	"shanliren.net",
};
#define TOTSER 	(sizeof(serverip) / sizeof(serverip[0]))
#endif

static uint16_t port[] = {
	6000,
	7000,
	8000,
};

void rctl(char *devid);
#define TOTPRT (sizeof(port) / sizeof(port[0]))

#endif /* __RCTL_H__ */
