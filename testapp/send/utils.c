/*****************************************************************************/
/* Transmission Error Detector test application                              */
/* This program is a part of the ABPS software suite.                        */
/*                                                                           */
/* Copyright (C) 2015                                                        */
/* Gabriele Di Bernardo, Matteo Martelli                                     */
/*                                                                           */
/* This program is free software; you can redistribute it and/or             */
/* modify it under the terms of the GNU General Public License               */
/* as published by the Free Software Foundation; either version 2            */
/* of the License, or any later version.                                     */
/*                                                                           */
/* This program is distributed in the hope that it will be useful,           */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of            */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             */
/* GNU General Public License for more details.                              */
/*                                                                           */
/* You should have received a copy of the GNU General Public License         */
/* along with this program; if not, write to the Free Software               */
/* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,*/
/* USA.                                                                      */
/*****************************************************************************/

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


/* Init default configuration */
void utils_default_conf(void)
{
	conf.ip_vers = IPPROTO_IP;
	conf.n_packets = DEFAULT_N_PACKETS; 
	conf.msg_length = DEFAULT_MSG_LENGTH;
	conf.iface_name = NULL;
	conf.iface_name_length = 0;
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
			{ 0, 0, 0, 0 },
		};

		c = getopt_long(argc, argv, "h6i:n:s:",
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
			case 'h':
			default:
				return -1;
		}
	}
	return 0;
}

