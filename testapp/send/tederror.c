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

#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include <linux/errqueue.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "utils.h"
#include "network.h"
#include "structs.h"

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


/* Get TED information of an error message */
void __get_ted_info(struct err_msg_s *emsg, struct ted_info_s *ted_info) 
{
	ted_info->ip_vers = emsg->c->cmsg_level;
	ted_info->msg_id = ted_msg_id(emsg->ee);
	ted_info->retry_count = ted_retry_count(emsg->ee);
	ted_info->status  = ted_status(emsg->ee);
	ted_info->more_frag = ted_more_fragment(emsg->ee);
	ted_info->frag_length = ted_fragment_length(emsg->ee);
	ted_info->frag_offset = ted_fragment_offset(emsg->ee);	
	ted_info->msg_pload = emsg->errmsg;
}

int tederror_recv_nowait(struct err_msg_s **error_message)
{
	
	int return_value, ip_vers, shared_descriptor;

	struct sockaddr *addr;
	struct iovec iov[1];
	struct err_msg_s *em;


	*error_message = alloc_init_ErrMsg();	
	em = *error_message;

	iov[0].iov_base = em->errmsg;
	iov[0].iov_len = sizeof(em->errmsg);



	if ((ip_vers = net_check_ip_instance_and_version()) == -1)
		return -1;

	if (ip_vers == IPPROTO_IPV6) {

		memset(&(em->name.ipv6_name), 0, sizeof(em->name.ipv6_name));
		em->name.ipv6_name.sin6_family = AF_INET6;
		em->namelen = sizeof(em->name.ipv6_name);
		em->is_ipv6 = 1;
		
		addr = (struct sockaddr *)&(em->name.ipv6_name);
		
	} else if (ip_vers == IPPROTO_IP) {

		memset(&(em->name.ipv4_name),0,sizeof(em->name.ipv4_name));
		em->name.ipv4_name.sin_family = AF_INET;
		em->namelen = sizeof(em->name.ipv4_name);
		em->is_ipv6 = 0;

		addr = (struct sockaddr *)&(em->name.ipv4_name);

	} else {
		utils_print_error("%s: wrong ip_vers.\n", __func__);	
	}
	
	shared_descriptor = net_get_shared_descriptor();

	return_value = getsockname(shared_descriptor, addr, &(em->namelen));
	
	if(return_value != 0) {
		utils_print_error("%s: getsockname failed (%s).\n",
		                  __func__, strerror(errno));
		return 0;
	}

	do
	{
		/*XXX: name can be omitted for local error msg (see man) */
		memset(&(em->msg),0,sizeof(em->msg));
		em->msg->msg_name = &(em->name);
		em->msg->msg_namelen = sizeof(em->name);
		em->msg->msg_iov = iov;
		em->msg->msg_iovlen = 1;
		em->msg->msg_control = em->control;
		em->msg->msg_controllen = sizeof(em->control);
		return_value = recvmsg(shared_descriptor, em->msg, MSG_ERRQUEUE | MSG_DONTWAIT); 
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
		/* XXX: empty payload comes from here */
		printf("%d ,buffer: %s\n", return_value, em->errmsg);
		ret = 1; // read, ok
	}

	if (ret <= 0) 
		free(em);

	return ret;
}

int tederror_check_ted_info(struct err_msg_s *emsg, struct ted_info_s *ted_info)
{
	/* For each error header of the previuosly sent message, 
	 * look for TED notifications. */
	for (emsg->c = CMSG_FIRSTHDR(emsg->msg); 
	     emsg->c; 
	     emsg->c = CMSG_NXTHDR(emsg->msg, emsg->c)) {
			
		if ((emsg->c->cmsg_level == IPPROTO_IPV6) && 
		    (emsg->c->cmsg_type == IPV6_RECVERR)) {
			
			/*struct sockaddrin_6 *from;
			from = (struct sockaddrin_6 *)SO_EE_OFFENDER(emsg->ee);*/
			emsg->ee = (struct sock_extended_err *)CMSG_DATA(emsg->c);

		} else if((emsg->c->cmsg_level == IPPROTO_IP) && 
		          (emsg->c->cmsg_type == IP_RECVERR)) {

			/*struct sockaddrin *from;
			from = (struct sockaddrin *) SO_EE_OFFENDER(emsg->ee);*/
			emsg->ee = (struct sock_extended_err *) CMSG_DATA(emsg->c);
		} else {
			/* Skip non interesting error messages */
			continue;
		}

		switch (emsg->ee->ee_origin) {
			
			/* TED notification */
			case SO_EE_ORIGIN_LOCAL_NOTIFY: 
				if(emsg->ee->ee_errno == 0) {
					__get_ted_info(emsg, ted_info);
				} else {
					//TODO: handle errno for local notify
				}
				break;
			default:
				if(emsg->ee->ee_errno != 0)
					printf(strerror(emsg->ee->ee_errno));

				break;
		}
	}

	return 0;
}


