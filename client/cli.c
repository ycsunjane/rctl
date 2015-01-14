/*
 * ============================================================================
 *
 *       Filename:  cli.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2014年12月30日 11时12分59秒
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
#include <unistd.h>
#include <stdlib.h>

#include "rctl.h"
#include "cmdline.h"

int debug = 0;
struct  gengetopt_args_info args_info;

static int file_exist(char *filename)
{
	/*  try to open file to read */
	FILE *file;
	if ((file = fopen(filename, "r"))){
		fclose(file);
		return 1;
	}
	return 0;
}

void proc_args(int argc, char *argv[])
{
	int ret;
	struct cmdline_parser_params *params;
	params = cmdline_parser_params_create();

	/* get config file first */
	params->check_required = 0;
	ret = cmdline_parser_ext(argc, argv, &args_info, params);
	if(ret != 0) exit(ret);

	if(args_info.config_arg && 
		file_exist(args_info.config_arg)) {
		ret = cmdline_parser_config_file(
			args_info.config_arg,
			&args_info, params);
		if(ret != 0) exit(ret);
	}

	/* get all config and check */
	params->initialize = 0;
	params->check_required = 1;
	ret = cmdline_parser_ext(argc, argv, &args_info, params);
	if(ret != 0) exit(ret);

	debug = args_info.debug_flag;
}

int main(int argc, char *argv[])
{
	proc_args(argc, argv);
	if(args_info.daemon_flag)
		daemon(0, 0);

	rctl(args_info.class_arg, args_info.wan_arg);
	pause();
	return 0;
}
