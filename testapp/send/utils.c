#define _GNU_SOURCE
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <stdarg.h>
#include <netinet/in.h>

#include "consts.h"
#include "structs.h"


struct conf_s conf;

void utils_print_error(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

void utils_exit_error(const char *fmt, ...)
{
	utils_print_error(fmt);
	exit(EXIT_FAILURE);
}

/* TODO: move to logger file */
FILE *open_logfile(char *path)
{
	FILE *f;

	f = fopen(path, "r+");

	if (f == NULL && errno == ENOENT) 
		f = fopen(path, "w+");

	return f;

}

/* Init default configuration */
void utils_default_conf(void)
{
	conf.ip_vers = IPPROTO_IP;
	conf.n_packets = DEFAULT_N_PACKETS; 
	conf.msg_length = DEFAULT_MSG_LENGTH;
	conf.test_type = DEFAULT_TEST_TYPE;
	conf.iface_name = NULL;
	conf.iface_name_length = 0;
	conf.logfile = NULL;
}

int utils_get_opt(int argc, char **argv) 
{
	
	for (;;) {

		int option_index = 0;
		int c;

		static struct option long_options[] = {
			{ "help", no_argument, 0, 'h' },
			{ "ipv6", no_argument, 0, '6'},
			{ "iface", required_argument, 0, 'i'},
			{ "npkts", required_argument, 0, 'n'},
			{ "size", required_argument, 0, 's'},
			{ "path", required_argument, 0, 'p' },
			{ "type", required_argument, 0, 't'},
			{ 0, 0, 0, 0 },
		};

		c = getopt_long(argc, argv, "h6i:n:s:p:t:",
				long_options, &option_index);
		if (c == -1)
			break;
		switch(c) {
			case '6':
				conf.ip_vers = IPPROTO_IPV6;
				break;
			case 'i':
				conf.iface_name_length = asprintf(&(conf.iface_name), "%s", optarg);
				if (conf.iface_name_length == -1) { 
					utils_print_error("%s: error in asprinf", __func__);
					return -1;
				}
				break;
			case 'n':
				conf.n_packets = atoi(optarg);
				break;
			case 's':
				conf.msg_length = atoi(optarg);
				break;
			case 'p':
				conf.logfile = open_logfile(optarg);
	
				if (conf.logfile == NULL) {
					utils_print_error("%s: can't open log file", __func__);
					return -1;
				}
				break;
			case 't':
				conf.test_type = atoi(optarg);
				break;
			case 'h':
			default:
				return -1;
		}
	}
	return 0;
}

