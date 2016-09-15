/*****************************************************************************/
/* Transmission Error Detector test application                              */
/* This program is a part of the ABPS software suite.                        */
/*                                                                           */
/* Copyright (C) 2015                                                        */
/* Gabriele Di Bernardo, Alessandro Mengoli, Matteo Martelli                 */
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


void __print_dbg_dummy(const char *fmt, ...)
{
}

void __print_dbg(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

void (*print_dbg)(const char * fmt, ...) = __print_dbg_dummy;

void print_err(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

void exit_err(const char *fmt, ...)
{
	print_err(fmt);
	exit(EXIT_FAILURE);
}


/* Init default configuration */
void utils_default_conf(void)
{
	int i;
	conf.ip_vers = IPPROTO_IP;
	conf.bind_iface = 0;
	conf.n_packets = DEFAULT_N_PACKETS; 
	conf.msg_length = DEFAULT_MSG_LENGTH;
	conf.nifaces = 0;
	conf.test = 0;

	for (i = 0; i < MAX_IFACES; i++) {
		conf.ifaces[i].name = NULL;
		conf.ifaces[i].name_length = 0;
		conf.ifaces[i].type = IFACE_TYPE_UNSPEC;
	}
}

int utils_get_opt(int argc, char **argv) 
{
	
	for (;;) {

		int option_index = 0;
		int c, len;

		static struct option long_options[] = {
			{ "help", no_argument, 0, 'h' },
			{ "ipv6", no_argument, 0, '6'},
			{ "bind_iface", no_argument, 0, 'b'},
			{ "iface", required_argument, 0, 'i'},
			{ "npkts", required_argument, 0, 'n'},
			{ "size", required_argument, 0, 's'},
			{ "test", no_argument, 0, 't'},
			{ "debug", no_argument, 0, 'd'},
			{ 0, 0, 0, 0 },
		};

		c = getopt_long(argc, argv, "h6bi:n:s:dt",
				long_options, &option_index);
		if (c == -1)
			break;
		switch(c) {
			case '6':
				conf.ip_vers = IPPROTO_IPV6;
				break;
			case 'b':
				conf.bind_iface = 1;
				break;
			case 'i':
				if (conf.nifaces + 1 > MAX_IFACES) {
					exit_err("Too many network interfaces,"
							  "limit is set to %d", MAX_IFACES);
				}

				if (optarg[0] == 't' && optarg[1] == ':') {
					conf.ifaces[conf.nifaces].type = IFACE_TYPE_TED;
					optarg += 2;
				}
				len = asprintf(&(conf.ifaces[conf.nifaces].name), "%s", optarg);
				if (len == -1)
					exit_err("%s: error in asprinf", __func__);

				conf.ifaces[conf.nifaces].name_length = len;
				conf.nifaces++;
				break;
			case 'n':
				conf.n_packets = atoi(optarg);
				break;
			case 's':
				conf.msg_length = atoi(optarg);
				break;
			case 't':
				conf.test = 1;
			case 'd':
				print_dbg = __print_dbg;
				break;
			case 'h':
			default:
				return -1;
		}
	}
	return 0;
}

