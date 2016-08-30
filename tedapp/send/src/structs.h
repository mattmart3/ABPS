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

#ifndef STRUCTS_H
#define STRUCTS_H

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

#include "consts.h"

/* Configuration data */
struct conf_s {
	int ip_vers;
	int bind_iface;
	int n_packets;
	int msg_length;
	int nifaces;
	struct iface_s {
		char *iface_name;
		int iface_name_length;
	} ifaces[MAX_IFACES];

};


struct socket_s {
	int sd;
	char *iface_name;
	int iface_name_length;
};

struct ted_info_s {
	uint32_t msg_id;
	uint8_t retry_count;
	uint8_t status;
	uint16_t frag_length;
	uint16_t frag_offset;
	uint8_t more_frag;
	uint8_t ip_vers;
	char *msg_pload;
};

struct msg_info_s {
	int size;
	uint32_t id;
	/* TODO time */
	struct ted_info_s *frags[MAX_FRAGS];
	int n_frags;
	short int last_frag_received;
};

struct err_msg_s {
	union
	{
		struct sockaddr_in6 ipv6_name;
		struct sockaddr_in  ipv4_name;
	} name;

	unsigned int namelen;

	struct cmsghdr *c;

	struct sock_extended_err *ee;

	char control[512];

	int len_control;

	char errmsg[MSGERR_BUFF_LEN];//[64 + 768];

	int len_errmsg;

	struct msghdr msg[1];

	int lenrecv;
	int myerrno;

	int is_ipv6;

};


#endif
