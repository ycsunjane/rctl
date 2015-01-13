/*
 * =====================================================================================
 *
 *       Filename:  log.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2014年07月25日 10时13分27秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  jianxi sun (jianxi), ycsunjane@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef __LOG_H__
#define __LOG_H__
#include <stdio.h>
#include <syslog.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

extern int debug;

#define SYSLOG_ERR 	(LOG_ERR|LOG_USER)
#define SYSLOG_WARN 	(LOG_WARNING|LOG_USER)
#define SYSLOG_DEBUG 	(LOG_DEBUG|LOG_USER)
#define SYSLOG_INFO 	(LOG_INFO|LOG_USER)
#define SYSLOG_LOCK 	(LOG_NOTICE|LOG_USER)

#define __sys_log2(fmt, ...) 						\
	do { 								\
		syslog(SYSLOG_INFO, fmt, ##__VA_ARGS__); 		\
		if(debug) fprintf(stderr, fmt,  ##__VA_ARGS__); 	\
	} while(0)

#define __sys_log(LEVEL, fmt, ...) 										\
	do { 													\
		syslog(LEVEL, "%s +%d %s(): "fmt,  								\
			__FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__); 					\
		if(debug || LEVEL == SYSLOG_ERR) 								\
			fprintf(stderr, "(%lu) %s +%d %s(): "fmt, 							\
				(unsigned long)getpid(), __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__); 				\
	} while(0)

#define sys_err(fmt, ...) 	__sys_log(SYSLOG_ERR, "ERR: "fmt, ##__VA_ARGS__)

#ifdef DEBUG
#define sys_warn(fmt, ...) 	__sys_log(SYSLOG_WARN, "WARNING: "fmt, ##__VA_ARGS__)
#define sys_debug(fmt, ...) 	__sys_log(SYSLOG_DEBUG, "DEBUG: "fmt, ##__VA_ARGS__)
#define sys_lock(fmt, ...) 	__sys_log(SYSLOG_LOCK, "LOCK: "fmt, ##__VA_ARGS__)
#define pure_info(fmt, ...) 	__sys_log2(fmt, ##__VA_ARGS__)
#else
#define sys_warn(fmt, ...) 	NULL
#define sys_debug(fmt, ...)  	NULL
#define sys_lock(fmt, ...) 	NULL
#define pure_info(fmt, ...) 	NULL
#endif

#define panic() 		\
do { 				\
	sys_err("Panic\n"); 	\
	exit(-1); 		\
} while(0)

#endif /* __LOG_H__ */
