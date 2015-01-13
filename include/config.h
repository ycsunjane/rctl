/*
 * =====================================================================================
 *
 *       Filename:  config.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2015年01月13日 14时33分50秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  jianxi sun (jianxi), ycsunjane@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef __CONFIG_H__
#define __CONFIG_H__

#define CERT_FILE 	"/etc/rctl/rctl_cert.pem"
#define PRIV_FILE 	"/etc/rctl/rctl_priv.pem"
#define CA_FILE 	"/etc/ssl/certs/wirelesser_ca.crt"

static char serverip[][50] = {
	"127.0.0.1",
	"rctl.shanliren.net",
	"shanliren.net",
};
#define TOTSER 	(sizeof(serverip) / sizeof(serverip[0]))

static uint16_t port[] = {
	7000,
};
#define TOTPRT (sizeof(port) / sizeof(port[0]))

#define BASHPORT 	(6001)
#define RCTLBASH 	"rctlbash"
#define CMDLEN (2048)
#define BUFLEN (2048)

#define DEVID_LEN 	(33)
#define ETH_ALEN 	(6)
#define MACSTR 		(18)

struct reg_t {
	char  class[DEVID_LEN];
	unsigned char  mac[ETH_ALEN];
}__attribute__((packed));

#endif /* __CONFIG_H__ */
