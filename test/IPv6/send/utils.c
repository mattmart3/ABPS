
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <getopt.h>

#include "consts.h"
#include "structs.h"

struct conf_s conf;

void utils_exit_error(char *emsg)
{
	fprintf(stderr, "%s", emsg);
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
	conf.ip_vers = 4;
	conf.n_packets = DEFAULT_N_PACKETS; 
	conf.msg_length = DEFAULT_MSG_LENGTH;
	conf.test_type = DEFAULT_TEST_TYPE;
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
			{ "path", required_argument, 0, 'p' },
			{ "type", required_argument, 0, 't'},
			{ 0, 0, 0, 0 },
		};

		c = getopt_long(argc, argv, "h6p:t:",
				long_options, &option_index);
		if (c == -1)
			break;
		switch(c) {
			case '6':
				conf.ip_vers = 6;
				break;
			case 'p':
				conf.logfile = open_logfile(optarg);
	
				if (conf.logfile == NULL) 
					fprintf(stderr, "Can't open log file");

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

