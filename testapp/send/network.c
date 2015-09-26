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

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "utils.h"
#include "consts.h"

#define HAVE_IP_RECVERR 1

#if HAVE_IP_RECVERR

	#include <sys/uio.h>
#else
	#warning "HAVE_IP_RECVERR not defined"
	printf ("setsockopt() IP_PROTO IP_RECVERR not defined\n");
#endif


int shared_descriptor;

int is_shared_instance_instantiated;

int is_shared_instance_ipv6;



struct sockaddr_in ipv4_dest_addr;

struct sockaddr_in6 ipv6_dest_addr;

/* create IPv4 socket */

int __create_ipv4_socket(char *address, int port, int *file_descriptor, 
                       struct sockaddr_in *destination_address)
{
	int error, option_value;

	/* get datagram socket */
	*file_descriptor = socket(AF_INET, SOCK_DGRAM, 0);

	if (*file_descriptor == SOCKET_ERROR) {
		utils_print_error("%s: failed getting socket.\n%s",
		                  __func__, strerror(errno));
		return errno;
	}

	/* prepare destination sockaddr_in */
	destination_address->sin_family = AF_INET;
	inet_pton(AF_INET, address, &(destination_address->sin_addr));
	destination_address->sin_port = htons(port);


	/* configuring the socket */
	option_value = 1;
	error = setsockopt(*file_descriptor, SOL_SOCKET, SO_REUSEADDR, 
	                   (char *)&option_value, sizeof(option_value));

	if (error == SOCKET_ERROR) {
		utils_print_error("%s: setsockopt for reuse address failed\n%s",
		                  __func__, strerror(errno));
		return errno;
	}


	error = setsockopt(*file_descriptor, IPPROTO_IP, IP_RECVERR, 
	                   (char *)&option_value, sizeof(option_value));

	if (error == SOCKET_ERROR) {
		utils_print_error("%s: setsockopt for IP_RECVERR failed\n%s",
		                  __func__, strerror(errno));
		return errno;
	}

	return 1;
}



/* create IPv6 socket */
int __create_ipv6_socket(char *ifname, char *address, 
                       int port, int *file_descriptor, 
                       struct sockaddr_in6 *destination_address)
{
	int error, option_value;

	/* get datagram socket */
	*file_descriptor = socket(AF_INET6, SOCK_DGRAM, 0);
	
	if (*file_descriptor == SOCKET_ERROR) {
		utils_print_error("%s: failed getting socket.\n%s",
		                  __func__, strerror(errno));
		return errno;
	}

	/* prepare destination sockaddr_in */
	destination_address->sin6_family = AF_INET6;
	inet_pton(AF_INET6, address, &(destination_address->sin6_addr));
	destination_address->sin6_port = htons(port);


	/* Probably this is not the proper way. */
	destination_address->sin6_scope_id = if_nametoindex(ifname);

	/* configuring the socket */

	option_value = 1;

	error = setsockopt(*file_descriptor, SOL_SOCKET, SO_REUSEADDR,
	                   (char *)&option_value, sizeof(option_value));
	
	if (error == SOCKET_ERROR) {
		utils_print_error("%s: setsockopt for reuse address failed\n%s",
		                  __func__, strerror(errno));
		return errno;
	}


	error = setsockopt(*file_descriptor, IPPROTO_IP, IP_RECVERR,
	                   (char *)&option_value, sizeof(option_value));

	if (error == SOCKET_ERROR) {
		utils_print_error("%s: setsockopt for IP_RECVERR failed\n%s",
		                  __func__, strerror(errno));
		return errno;
	}

	return 1;
}

int net_init_ipv4_shared_instance(char *address, int port)
{
	int error;

	is_shared_instance_ipv6 = 0;
	is_shared_instance_instantiated = 1;

	error = __create_ipv4_socket(address, port, &shared_descriptor,
	                           &ipv4_dest_addr);
	return error;
}


int net_init_ipv6_shared_instance(char *ifname, char *address, int port)
{
	int error;

	is_shared_instance_ipv6 = 1;
	is_shared_instance_instantiated = 1;

	error = __create_ipv6_socket(ifname, address, port, &shared_descriptor, 
	                           &ipv6_dest_addr);
	return error;
}


void net_release_shared_instance(void)
{
	shared_descriptor = -1;
	is_shared_instance_instantiated = 0;
	is_shared_instance_ipv6 = 0;
}

int net_get_shared_descriptor(void)
{
	return shared_descriptor;
}

/* It returns -1 if the shared_instance is not instantiated.
 * Otherwise it returns the ip version of the shared_instance. */
int net_check_ip_instance_and_version(void)
{
	if(!is_shared_instance_instantiated)
		return -1;
	
	if(is_shared_instance_ipv6)
		return IPPROTO_IPV6;
	else
		return IPPROTO_IP;
}

int net_sendmsg(const char *buffer, int length, uint32_t *id_pointer)
{
	int result_value, ip_vers;

	char *pointer;

	char ancillary_buffer[CMSG_SPACE(sizeof(id_pointer))];

	struct iovec iov[1];
	
	struct msghdr msg_header;
		
	struct cmsghdr *cmsg;

	iov[0].iov_base = (void *) buffer;
	iov[0].iov_len = length;


	if ((ip_vers = net_check_ip_instance_and_version()) == -1)
		return -1;
	
	if (ip_vers == IPPROTO_IPV6) {

		msg_header.msg_name = (void *)&ipv6_dest_addr;
		msg_header.msg_namelen = sizeof(ipv6_dest_addr);

	} else if (ip_vers == IPPROTO_IP) {

		msg_header.msg_name = (void *)&ipv4_dest_addr;
		msg_header.msg_namelen = sizeof(ipv4_dest_addr);

	} else {
		utils_print_error("%s: wrong ip_vers.\n", __func__);	
	}

	msg_header.msg_iov = iov;
	msg_header.msg_iovlen = 1;

	msg_header.msg_control = ancillary_buffer;
	msg_header.msg_controllen = sizeof(ancillary_buffer);

	cmsg = CMSG_FIRSTHDR(&msg_header);

	cmsg->cmsg_level = SOL_UDP;
	cmsg->cmsg_type = ABPS_CMSG_TYPE;

	cmsg->cmsg_len = CMSG_LEN(sizeof(id_pointer));

	pointer = (char *)CMSG_DATA(cmsg);

	memcpy(pointer, &id_pointer, sizeof(id_pointer));

	msg_header.msg_controllen = cmsg->cmsg_len;

	/* Prepare for sending. */
	result_value = sendmsg(shared_descriptor, &msg_header, MSG_NOSIGNAL | MSG_DONTWAIT);
	if(result_value < 0)
		utils_print_error("%s: sendmsg error (%s).\n",
		                  __func__, strerror(errno));

	return result_value;
}

