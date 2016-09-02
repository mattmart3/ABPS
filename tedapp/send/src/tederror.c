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

/* TED */
/* TED convenient wrapper APIs for First-hop Transmission Notification. */

/* TED identifier of the datagram whose notification refers to. */
#define ted_msg_id(notification) \
			((struct sock_extended_err *) notification)->ee_info

/* Message status to the first hop.
   Return 1 if the message was successfully delivered to the AP, 0 otherwise. */
#define ted_status(notification) \
			((struct sock_extended_err *) notification)->ee_type

/* Returns the number of times that the packet, 
   associated to the notification provided, was retrasmitted to the AP.  */
#define ted_retry_count(notification) \
			((struct sock_extended_err *) notification)->ee_retry_count

/* Returns the fragment length */
#define ted_fragment_length(notification) \
			(((struct sock_extended_err *) notification)->ee_data >> 16)

/* Returns the offset of the current message 
   associated with the notification from the original message. */
#define ted_fragment_offset(notification) \
			((((struct sock_extended_err *) notification)->ee_data << 16) >> 16)

/* Indicates if there is more fragment with the same TED identifier */
#define ted_more_fragment(notification) \
			((struct sock_extended_err *) notification)->ee_code
/* end TED */


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



int tederror_check_ted_info(struct err_msg_s *emsg, struct ted_info_s *ted_info)
{
	int there_is_ted = 0;

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
					there_is_ted = 1;
				} else {
					//TODO: handle errno for local notify
				}
				break;
			default:
				if(emsg->ee->ee_errno != 0)
					printf("%s\n", strerror(emsg->ee->ee_errno));

				break;
		}
	}

	return there_is_ted;
}


