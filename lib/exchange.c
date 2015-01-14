/*
 * ============================================================================
 *
 *       Filename:  exchange.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2015年01月14日 17时13分38秒
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
#include <signal.h>
#include <sys/wait.h>
#include <sys/select.h>

#include "log.h"
#include "ssltcp.h"
#include "common.h"
#include "config.h"

static void term(int signum)
{
	sys_debug("SIGCHLD received\n");
	int status;
	pid_t pid = waitpid((pid_t)-1, &status, WNOHANG);
	sys_debug("child process %ld exited: %d\n",
		(long) pid, WEXITSTATUS(status));
	exit(0);
}

static char buf[BUFLEN];
void exchange(int ptm, int fd, SSL *ssl)
{
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
			return;
		}

		if(FD_ISSET(fd, &fset)) {
			nrcv = ssltcp_read(ssl, buf, BUFLEN);
			if(nrcv < 0) {
				sys_debug("Connection closed: %s(%d)",
					strerror(errno), errno);
				return;
			}

			if(write(ptm, buf, nrcv) < 0) {
				sys_debug("pty closed: %s(%d)",
					strerror(errno), errno);
				return;
			}
		}

		if(FD_ISSET(ptm, &fset)) {
			nrcv = read(ptm, buf, BUFLEN);
			if(nrcv <= 0) {
				sys_debug("pty closed: %s(%d)",
					strerror(errno), errno);
				return;
			}

			if(ssltcp_write(ssl, buf, nrcv) < 0) {
				sys_debug("Connection closed: %s(%d)",
					strerror(errno), errno);
				return;
			}
		}
	}
}
