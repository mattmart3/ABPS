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


/*int shared_descriptor;*/

/*int is_shared_instance_instantiated;*/

/*int is_shared_instance_ipv6;*/



struct sockaddr_in ipv4_dest_addr;

struct sockaddr_in6 ipv6_dest_addr;


int net_create_socket(int ip_vers, char *address, int port, char *ifname, int ifbind)
{
	int sd, error, enabled; 

	if (ip_vers == IPPROTO_IPV6) {
		if (ifname == NULL) {
			exit_err("%s: Can't set ipv6 address without"
				 "source network interface name\n", __func__);
		}
		/* prepare destination sockaddr_in */
		ipv6_dest_addr.sin6_family = AF_INET6;
		inet_pton(AF_INET6, address, &(ipv6_dest_addr.sin6_addr));
		ipv6_dest_addr.sin6_port = htons(port);

		/* Probably this is not the proper way. */
		ipv6_dest_addr.sin6_scope_id = if_nametoindex(ifname);

		sd = socket(AF_INET6, SOCK_DGRAM, 0);
	} else {
		/* prepare destination sockaddr_in */
		ipv4_dest_addr.sin_family = AF_INET;
		inet_pton(AF_INET, address, &(ipv4_dest_addr.sin_addr));
		ipv4_dest_addr.sin_port = htons(port);

		sd = socket(AF_INET, SOCK_DGRAM, 0);
	}

	if (sd == SOCKET_ERROR) {
		print_err("%s: failed getting socket.\n%s",
		                  __func__, strerror(errno));
		return -errno;
	}

	/* configuring the socket */
	enabled = 1;
	error = setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, 
			   (char *)&enabled, sizeof(enabled));
	if (error == SOCKET_ERROR) {
		print_err("%s: setsockopt for reuse address failed\n%s",
				  __func__, strerror(errno));
		return -errno;
	}

	error = setsockopt(sd, IPPROTO_IP, IP_RECVERR, 
			(char *)&enabled, sizeof(enabled));
	if (error == SOCKET_ERROR) {
		print_err("%s: setsockopt for IP_RECVERR failed\n%s",
				  __func__, strerror(errno));
		return -errno;
	}

	/* TODO */
	if (ifbind) {
		;/*...*/
	}
	return sd;
}

// DEPRECATED
/*[> create IPv6 socket <]*/
/*int __create_ipv6_socket(char *ifname, char *address, */
                       /*int port, int *sd)*/
/*{*/
	/*int error, enabled;*/

	/*[> get datagram socket <]*/
	/**sd = socket(AF_INET6, SOCK_DGRAM, 0);*/
	
	/*if (*sd == SOCKET_ERROR) {*/
		/*print_err("%s: failed getting socket.\n%s",*/
				  /*__func__, strerror(errno));*/
		/*return errno;*/
	/*}*/

	/*[> configuring the socket <]*/
	/*enabled = 1;*/
	/*error = setsockopt(*sd, SOL_SOCKET, SO_REUSEADDR,*/
			   /*(char *)&enabled, sizeof(enabled));*/
	/*if (error == SOCKET_ERROR) {*/
		/*print_err("%s: setsockopt for reuse address failed\n%s",*/
				  /*__func__, strerror(errno));*/
		/*return errno;*/
	/*}*/

	/*error = setsockopt(*sd, IPPROTO_IP, IP_RECVERR,*/
			   /*(char *)&enabled, sizeof(enabled));*/
	/*if (error == SOCKET_ERROR) {*/
		/*print_err("%s: setsockopt for IP_RECVERR failed\n%s",*/
				  /*__func__, strerror(errno));*/
		/*return errno;*/
	/*}*/

	/*return 1;*/
/*}*/

/*int net_init_ipv4_shared_instance(char *address, int port)*/
/*{*/
	/*int error;*/

	/*is_shared_instance_ipv6 = 0;*/
	/*is_shared_instance_instantiated = 1;*/

	/*error = __create_ipv4_socket(address, port, &shared_descriptor,*/
				   /*&ipv4_dest_addr);*/
	/*return error;*/
/*}*/


/*int net_init_ipv6_shared_instance(char *ifname, char *address, int port)*/
/*{*/
	/*int error;*/

	/*is_shared_instance_ipv6 = 1;*/
	/*is_shared_instance_instantiated = 1;*/

	/*error = __create_ipv6_socket(ifname, address, port, &shared_descriptor, */
				   /*&ipv6_dest_addr);*/
	/*return error;*/
/*}*/


/*void net_release_shared_instance(void)*/
/*{*/
	/*shared_descriptor = -1;*/
	/*is_shared_instance_instantiated = 0;*/
	/*is_shared_instance_ipv6 = 0;*/
/*}*/

int net_get_shared_descriptor(void)
{
	return shared_descriptor;
}

/* It returns -1 if the shared_instance is not instantiated.
 * Otherwise it returns the ip version of the shared_instance. */
int __get_ipvers(int sd)
{
	int family_opt;
	socklen_t family_optlen = sizeof(family_opt);
	if (getsockopt(sd, SOL_SOCKET, SO_DOMAIN, &family_opt, &family_optlen) < 0)
		exit_err("%s: Error in getsockopt\n", __func__);
	
	return family_opt;
}

int net_send_msg(char *buffer, int length, int sd)
{
	struct sockaddr *dest_addr;
	socklen_t dest_addr_len;
	switch (__get_ipvers(sd)) {
	case AF_INET6:
		dest_addr = (struct sockaddr *)&ipv6_dest_addr;
		dest_addr_len = sizeof(ipv6_dest_addr);
		break;
	case AF_INET:
		dest_addr = (struct sockaddr *)&ipv4_dest_addr;
		dest_addr_len = sizeof(ipv4_dest_addr);
		break;
	default:
		print_err("%s: Unexpected ip version\n", __func__);
		return -1;
	}
	return 
		sendto(sd, buffer, strlen(buf),
		       MSG_NOSIGNAL | MSG_DONTWAIT, dest_addr, dest_addr_len); 
}

/* Send a message asking for TED notifications. Available for the socket related
 * to the wifi interface only. */
int net_send_ted_msg(char *buffer, int length, uint32_t *id_pointer, int sd)
{
	int result_value, ip_vers;
	struct iovec iov[1];
	struct msghdr msg_header;
	struct cmsghdr *cmsg;
	char cbuf[CMSG_SPACE(sizeof(id_pointer))];
	
	memset(cbuf, '\0', sizeof(cbuf));
	
	iov[0].iov_base = (void *) buffer;
	iov[0].iov_len = length;

	switch (__get_ipvers(sd)) {
	case AF_INET6:
		msg_header.msg_name = (void *)&ipv6_dest_addr;
		msg_header.msg_namelen = sizeof(ipv6_dest_addr);
		break;
	case AF_INET:
		msg_header.msg_name = (void *)&ipv4_dest_addr;
		msg_header.msg_namelen = sizeof(ipv4_dest_addr);
		break;
	default:
		print_err("%s: Unexpected ip version\n", __func__);
		return -1;
	}

	msg_header.msg_iov = iov;
	msg_header.msg_iovlen = 1;
	msg_header.msg_control = cbuf;
	msg_header.msg_controllen = sizeof(cbuf);

	cmsg = CMSG_FIRSTHDR(&msg_header);

	cmsg->cmsg_level = SOL_UDP;
	cmsg->cmsg_type = TED_CMSG_TYPE;
	cmsg->cmsg_len = CMSG_LEN(sizeof(id_pointer));

	/* Copy the address of our user space pointer (id_pointer)
	 * into the cmsg data. Later the kernel will put a new id 
	 * directly in our user space pointer accessing the cmsg data. */
	memcpy((uint32_t *)CMSG_DATA(cmsg), &id_pointer, sizeof(id_pointer));
	
	msg_header.msg_controllen = cmsg->cmsg_len;

	return 
		sendmsg(sd, &msg_header, MSG_NOSIGNAL | MSG_DONTWAIT);

}

struct err_msg_s *alloc_init_ErrMsg(void)
{
	struct err_msg_s *em;
	em = (struct err_msg_s *) malloc(sizeof(struct err_msg_s));

	if(em == NULL) {
		return(NULL);
	} else {
		memset(&(em->msg),0,sizeof(em->msg));
		memset(&(em->errmsg),0,sizeof(em->errmsg));
		em->lenrecv=0;
		em->myerrno=0;

		return(em);
	}
}

int net_recv_ted_err(struct err_msg_s **error_message, int sd)
{
	
	int return_value, ip_vers, shared_descriptor;

	struct sockaddr *addr;
	struct iovec iov[1];
	struct err_msg_s *em;

	addr = NULL;
	*error_message = alloc_init_ErrMsg();	
	em = *error_message;

	iov[0].iov_base = em->errmsg;
	iov[0].iov_len = sizeof(em->errmsg);

	switch (__get_ipvers(sd)) {
	case AF_INET6:
		memset(&(em->name.ipv6_name), 0, sizeof(em->name.ipv6_name));
		em->name.ipv6_name.sin6_family = AF_INET6;
		em->namelen = sizeof(em->name.ipv6_name);
		em->is_ipv6 = 1;

		addr = (struct sockaddr *)&(em->name.ipv6_name);
		break;
	case AF_INET:
		memset(&(em->name.ipv4_name),0,sizeof(em->name.ipv4_name));
		em->name.ipv4_name.sin_family = AF_INET;
		em->namelen = sizeof(em->name.ipv4_name);
		em->is_ipv6 = 0;

		addr = (struct sockaddr *)&(em->name.ipv4_name);

	default:
		print_err("%s: Unexpected ip version\n", __func__);
		return -1;
	}

	
	return_value = getsockname(sd, addr, (socklen_t *)&(em->namelen));
	
	if(return_value != 0) {
		utils_print_error("%s: getsockname failed (%s).\n",
		                  __func__, strerror(errno));
		return 0;
	}
	do {
		memset(&(em->msg),0,sizeof(em->msg));
		em->msg->msg_name = &(em->name);
		em->msg->msg_namelen = sizeof(em->name);
		em->msg->msg_iov = iov;
		em->msg->msg_iovlen = 1;
		em->msg->msg_control = em->control;
		em->msg->msg_controllen = sizeof(em->control);
		return_value = recvmsg(sd, em->msg, MSG_ERRQUEUE | MSG_DONTWAIT); 
		em->myerrno=errno;
	} while ((return_value < 0) && ((em->myerrno == EINTR)));

	int ret;
	ret = 0;

	if (return_value < 0) {
		if (em->myerrno == EAGAIN) {
			/* No message available on error queue. */
			ret = 0;
		} else {
			errno = em->myerrno;
			utils_print_error("%s: recvmsg failed with error (%s).\n",
					__func__, strerror(errno));
		 	ret = -1;
		}
	} else if ((em->msg->msg_flags & MSG_ERRQUEUE) != MSG_ERRQUEUE) {

		printf("recvmsg: no errqueue\n");
		ret =  0;

	} else if (em->msg->msg_flags & MSG_CTRUNC) {

		printf("recvmsg: extended error was truncated\n");
		ret = 0;

	} else {
		ret = 1; // read, ok
	}

	if (ret <= 0) 
		free(em);

	return ret;
}
